import os
import json
from durable.lang import *
from durable import engine
from werkzeug.wrappers import Request, Response
from werkzeug.routing import Map, Rule
from werkzeug.exceptions import HTTPException, NotFound
from werkzeug.wsgi import SharedDataMiddleware
from werkzeug.serving import run_simple

class Host(engine.Host):

    def __init__(self):
        super(Host, self).__init__()

    def get_action(self, action_name):
        def print_lamba(c):
            c.s.label = action_name
            print('Executing {0}'.format(action_name))

        return print_lamba;


class Application(object):

    def __init__(self):
        self._url_map = Map([Rule('/<ruleset_name>/definition', endpoint=self._ruleset_definition_request),
                             Rule('/<ruleset_name>/state/<sid>', endpoint=self._state_request),
                             Rule('/<ruleset_name>/events/<sid>', endpoint=self._events_request),
                             Rule('/testdynamic.html', endpoint=self._dynamic_request), 
                             Rule('/durableVisual.js', endpoint=self._dynamic_request), 
                             Rule('/<ruleset_name>/<sid>/admin.html', endpoint=self._admin_request), 
                             Rule('/favicon.ico', endpoint=self._empty)])

        self._host = Host();


    def _ruleset_definition_request(self, environ, start_response, ruleset_name):
        def encode_promise(obj):
            if isinstance(obj, engine.Promise) or hasattr(obj, '__call__'):
                return 'function'
            raise TypeError(repr(obj) + " is not JSON serializable")

        request = Request(environ)
        if request.method == 'GET':
            result = self._host.get_ruleset(ruleset_name)
            return Response(json.dumps(result.get_definition(), default=encode_promise))(environ, start_response)
        elif request.method == 'POST':
            ruleset_definition = json.loads(request.stream.read().decode('utf-8'))
            self._host.set_rulesets(ruleset_definition)

        return Response()(environ, start_response)

    def _state_request(self, environ, start_response, ruleset_name, sid):
        request = Request(environ)
        result = None
        if request.method == 'GET':
            result = self._host.get_state(ruleset_name, sid)
            return Response(json.dumps(result))(environ, start_response)
        elif request.method == 'POST':
            message = json.loads(request.stream.read().decode('utf-8'))
            message['sid'] = sid
            result = self._host.update_state(ruleset_name, message)
            return Response(json.dumps({'outcome': result}))(environ, start_response)
        
    def _events_request(self, environ, start_response, ruleset_name, sid):
        request = Request(environ)
        result = None
        if request.method == 'POST':
            message = json.loads(request.stream.read().decode('utf-8'))
            message['sid'] = sid
            result = self._host.post(ruleset_name, message)
            return Response(json.dumps({'outcome': result}))(environ, start_response)

    def _empty(self, environ, start_response):
        return Response()(environ, start_response)

    def _not_found(self, environ, start_response):
        return Exception('File not found')

    def _dynamic_request(self, environ, start_response):
        middleware = SharedDataMiddleware(self._not_found, {'/': os.path.dirname(__file__)})
        return middleware(environ, start_response)

    def _admin_request(self, environ, start_response, ruleset_name, sid):
        middleware = SharedDataMiddleware(self._not_found, {'/{0}/{1}/'.format(ruleset_name, sid): os.path.dirname(__file__)})
        return middleware(environ, start_response)

    def __call__(self, environ, start_response):
        request = Request(environ)
        adapter = self._url_map.bind_to_environ(environ)
        try:
            endpoint, values = adapter.match()
            return endpoint(environ, start_response, **values)
        except HTTPException as e:
            return e
    
    def run(self):
        run_simple('127.0.0.1', 5000, self, threaded = True)


if __name__ == '__main__':
    main_app = Application()
    main_app.run()
