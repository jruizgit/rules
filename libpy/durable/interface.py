import os
import json
import engine
from werkzeug.wrappers import Request, Response
from werkzeug.routing import Map, Rule
from werkzeug.exceptions import HTTPException, NotFound
from werkzeug.wsgi import SharedDataMiddleware
from werkzeug.serving import run_simple


class Application(object):

    def __init__(self, host, routing_rules = []):
        self._host = host
        routing_rules.append(Rule('/<ruleset_name>', endpoint=self._ruleset_definition_request))
        routing_rules.append(Rule('/durableVisual.js', endpoint=self._visual_request))
        routing_rules.append(Rule('/<ruleset_name>/<sid>', endpoint=self._state_request))
        routing_rules.append(Rule('/<ruleset_name>/<sid>/admin.html', endpoint=self._admin_request))
        self._url_map = Map(routing_rules)

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
            ruleset_definition = json.loads(request.stream.read())
            self._host.set_ruleset(ruleset_name, ruleset_definition)

        return Response()(environ, start_response)

    def _state_request(self, environ, start_response, ruleset_name, sid):
        request = Request(environ)
        if request.method == 'GET':
            result = self._host.get_state(ruleset_name, sid)
            return Response(json.dumps(result))(environ, start_response)
        elif request.method == 'POST':
            message = json.loads(request.stream.read())
            message['sid'] = sid
            self._host.post(ruleset_name, message)
        elif request.method == 'PATCH':
            document = json.loads(request.stream.read())
            document['id'] = sid
            self._host.patch_state(ruleset_name, document)
        
        return Response()(environ, start_response)

    def _not_found(self, environ, start_response):
        return Exception('File not found')

    def _visual_request(self, environ, start_response):
        middleware = SharedDataMiddleware(self._not_found, {
            '/': os.path.join(os.path.dirname(__file__), 'ux')
        })
        return middleware(environ, start_response)

    def _admin_request(self, environ, start_response, ruleset_name, sid):
        middleware = SharedDataMiddleware(self._not_found, {
            '/{0}/{1}/'.format(ruleset_name, sid): os.path.join(os.path.dirname(__file__), 'ux')
        })
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
        self._host.run()
        run_simple('127.0.0.1', 5000, self, use_debugger=True, use_reloader=False)

