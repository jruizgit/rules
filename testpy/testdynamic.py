import os
import redis
import json
from durable import engine
from durable import interface
from werkzeug.routing import Rule
from werkzeug.wrappers import Request, Response
from werkzeug.wsgi import SharedDataMiddleware


class Host(engine.Host):

    def __init__(self):
        super(Host, self).__init__()
        self._redis_server = redis.Redis()

    def get_action(self, action_name):
        def print_lamba(c):
            c.s.label = action_name
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
        super(Application, self).__init__(host, '127.0.0.1', 5000, [Rule('/testdynamic.html', endpoint=self._dynamic_request), 
                                                                    Rule('/durableVisual.js', endpoint=self._dynamic_request), 
                                                                    Rule('/<ruleset_name>/<sid>/admin.html', endpoint=self._admin_request), 
                                                                    Rule('/favicon.ico', endpoint=self._empty)])

    def _empty(self, environ, start_response):
        return Response()(environ, start_response)

    def _not_found(self, environ, start_response):
        return Exception('File not found')

    def _dynamic_request(self, environ, start_response):
        middleware = SharedDataMiddleware(self._not_found, {'/': 'testjs'})
        return middleware(environ, start_response)

    def _admin_request(self, environ, start_response, ruleset_name, sid):
        middleware = SharedDataMiddleware(self._not_found, {'/{0}/{1}/'.format(ruleset_name, sid): 'testjs'})
        return middleware(environ, start_response)

if __name__ == '__main__':
    main_host = Host()
    main_app = Application(main_host)
    main_app.run()
