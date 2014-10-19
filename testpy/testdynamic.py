import os
import redis
import json
from durable import engine
from durable import interface
from werkzeug.routing import Rule
from werkzeug.wsgi import SharedDataMiddleware


class Host(engine.Host):

    def __init__(self):
        super(Host, self).__init__()
        self._redis_server = redis.Redis(unix_socket_path='/tmp/redis.sock')

    def get_action(self, action_name):
        def print_lamba(state):
            print('Executing {0}'.format(action_name))

        return print_lamba;

    def load_ruleset(self, ruleset_name):
        ruleset_definition = self._redis_server.hget('rulesets', ruleset_name)
        if ruleset_definition:
            return json.loads(ruleset_definition)
        else:
            raise Exception('Ruleset with name {0} not found'.format(ruleset_name))

    def save_ruleset(self, ruleset_name, ruleset_definition):
        print('save ' + json.dumps(ruleset_definition))
        self._redis_server.hset('rulesets', ruleset_name, json.dumps(ruleset_definition))


class Application(interface.Application):

    def __init__(self, host):
        super(Application, self).__init__(host, [Rule('/testdynamic.html', endpoint=self._dynamic_request)])

    def _not_found(self, environ, start_response):
        return Exception('File not found')

    def _dynamic_request(self, environ, start_response):
        middleware = SharedDataMiddleware(self._not_found, {'/': os.path.dirname(__file__)})
        return middleware(environ, start_response)


if __name__ == '__main__':
    main_host = Host()
    main_app = Application(main_host)
    main_app.run()
