import os
import json
import engine
from werkzeug.wrappers import Request, Response
from werkzeug.routing import Map, Rule
from werkzeug.exceptions import HTTPException, NotFound
from werkzeug.wsgi import SharedDataMiddleware
from werkzeug.serving import run_simple
from werkzeug.serving import make_ssl_devcert


class Application(object):

    def __init__(self, host, host_name, port, routing_rules = [], run = None):
        self._host = host
        self._host_name = host_name
        self._port = port
        self._run = run
        routing_rules.append(Rule('/<ruleset_name>', endpoint=self._ruleset_definition_request))
        routing_rules.append(Rule('/<ruleset_name>/<sid>', endpoint=self._state_request))
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

    def __call__(self, environ, start_response):
        request = Request(environ)
        adapter = self._url_map.bind_to_environ(environ)
        try:
            endpoint, values = adapter.match()
            return endpoint(environ, start_response, **values)
        except HTTPException as e:
            return e
    
    def run(self):
        if self._run:
            self._run(self._host, self)
        elif self._port != 443:
            run_simple(self._host_name, self._port, self, threaded=True)
        else:
            make_ssl_devcert('key', host=self._host_name)
            run_simple(self._host_name, self._port, self, threaded=True, ssl_context=('key.crt', 'key.key'))

