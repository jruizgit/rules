import threading
import json
import copy
import rules
from werkzeug.wrappers import Request, Response
from werkzeug.routing import Map, Rule
from werkzeug.exceptions import HTTPException, NotFound
from werkzeug.wsgi import SharedDataMiddleware
from werkzeug.serving import run_simple

class Session(object):

    def __init__(self, state, event, handle, ruleset_name):
        self.state = state
        self.ruleset_name = ruleset_name
        self.id = state['id']
        self._handle = handle
        self._timer_directory = {}
        self._message_directory = {}
        self._branch_directory = {}
        self._message_directory = {}
        if '$m' in event:
            self.event = event['$m']
        else:
            self.event = event

    def get_timers(self):
        return self._timer_directory

    def get_branches(self):
        return self._branch_directory

    def get_messages(self):
        return self._message_directory

    def signal(self, message):
        name_index = self.ruleset_name.rindex('.')
        parent_ruleset_name = self.ruleset_name[:name_index]
        name = self.ruleset_name[name_index + 1:]
        message['sid'] = self.id
        message['id'] = '{0}.{1}'.format(name, message['id'])
        self.post(parent_ruleset_name, message)

    def post(self, ruleset_name, message):
        message_list = []
        if  ruleset_name in self._message_directory:
            message_list = self._message_directory[ruleset_name]
        else:
            self._message_directory[ruleset_name] = message_list

        message_list.append(message)

    def start_timer(self, timer_name, duration):
        if timer_name in self._timer_directory:
            raise Exception('Timer with name {0} already added'.format(timer_name))
        else:
            self._timer_directory[timer_name] = duration

    def fork(self, branch_name, branch_state):
        if branch_name in self._branch_directory:
            raise Exception('Branch with name {0} already forked'.format(branch_name))
        else:
            self._branch_directory[branch_name] = branch_state

            
class Promise(object):

    def __init__(self, func):
        self._func = func
        self._next = None
        self._sync = True
        self.root = self

    def continue_with(self, next):
        if (isinstance(next, Promise)):
            self._next = next
        elif (hasattr(next, '__call__')):
            self._next = Promise(next)
        else:
            raise Exception('Unexpected Promise Type')

        self._next.root = self.root
        return self._next

    def run(self, s, complete):
        if self._sync:
            try:
                self._func(s)
            except Exception as error:
                complete(error)
                return

            if self._next:
                self._next.run(s, complete)
            else:
                complete(None)
        else:
            try:
                def callback(e):
                    if e: 
                        complete(e) 
                    elif self._next: 
                        self._next.run(s, complete) 
                    else: 
                        complete(None)

                self._func(s, callback)
            except Exception as error:
                complete(error)


class Fork(Promise):

    def __init__(self, branch_names, state_filter = None):
        super(Fork, self).__init__(self._execute)
        self.branch_names = branch_names
        self.state_filter = state_filter
        
    def _execute(self, s):
        for branch_name in self.branch_names:
            state = copy.deepcopy(s.state)
            if self.state_filter:
                self.state_filter(state)

            s.fork(branch_name, state)
        

class To(Promise):

    def __init__(self, state):
        super(To, self).__init__(self._execute)
        self._state = state
        
    def _execute(self, s):
        s.state['label'] = self._state


class Ruleset(object):

    def __init__(self, name, host, ruleset_definition):
        self._actions = {}
        self._name = name
        self._host = host
        for rule_name, rule in ruleset_definition.iteritems():
            action = rule['run']
            del rule['run']
            if isinstance(action, basestring):
                self._actions[rule_name] = Promise(host.get_action(action))
            elif isinstance(action, Promise):
                self._actions[rule_name] = action.root
            elif (hasattr(action, '__call__')):
                self._actions[rule_name] = Promise(action)
            else:
                self._actions[rule_name] = Fork(host.register_rulesets(name, action))

        self._handle = rules.create_ruleset(name, json.dumps(ruleset_definition))
        self._definition = ruleset_definition
        
    def bind(self, databases):
        for db in databases:
            if isinstance(db, basestring):
                rules.bind_ruleset(self._handle, None, 0, db)
            else: 
                rules.bind_ruleset(self._handle, db['password'], db['port'], db['host'])

    def assert_event(self, message):
        rules.assert_event(self._handle, json.dumps(message))
        
    def assert_events(self, messages):
        rules.assert_events(self._handle, json.dumps(messages))
        
    def assert_state(self, state):
        rules.assert_state(self._handle, json.dumps(state))
        
    def get_state(self, sid):
        return json.loads(rules.get_state(self._handle, sid))
        
    def get_definition(self):
        return self._definition

    @staticmethod
    def create_rulesets(parent_name, host, ruleset_definitions):
        branches = {}
        for name, definition in ruleset_definitions.iteritems():  
            if name.rfind('$state') != -1:
                name = name[:name.rfind('$state')]
                if parent_name:
                    name = '{0}.{1}'.format(parent_name, name) 

                branches[name] = Statechart(name, host, definition)
            elif name.rfind('$flow') != -1:
                name = name[:name.rfind('$flow')]
                if parent_name:
                    name = '{0}.{1}'.format(parent_name, name) 

                branches[name] = Flowchart(name, host, definition)
            else:
                if parent_name:
                    name = '{0}.{1}'.format(parent_name, name)

                branches[name] = Ruleset(name, host, definition)

        return branches

    def dispatch_timers(self, complete):
        try:
            rules.assert_timers(self._handle)
        except Exception as error:
            complete(error)
            return

        complete(None)

    def dispatch(self, complete):
        result = None
        try:
            result = rules.start_action(self._handle)
        except Exception as error:
            complete(error)
            return

        if not result:
            complete(None)
        else:
            state = json.loads(result[0])
            event = json.loads(result[1])[self._name]
            actionName = None
            for actionName, event in event.iteritems():
                break

            s = Session(state, event, result[2], self._name)
            
            def action_callback(e):
                if e:
                    rules.abandon_action(self._handle, s._handle)
                    complete(e)
                else:
                    try:
                        branches = list(s.get_branches().items())
                        for branch_data in branches:
                            self._host.patch_state(branch_data[0], branch_data[1])  

                        messages = list(s.get_messages().items())
                        for message_data in messages:
                            if len(message_data[1]) == 1:
                                self._host.post(message_data[0], message_data[1][0])
                            else:
                                self._host.post_batch(message_data[0], message_data[1])

                        timers = list(s.get_timers().items())
                        for timer_data in timers:
                            timer = {'sid':s.id, 'id':timer_data[0], '$t':timer_data[0]}
                            rules.start_timer(self._handle, str(s.state['id']), timer_data[1], json.dumps(timer))

                        rules.complete_action(self._handle, s._handle, json.dumps(s.state))
                        complete(None)
                    except Exception as error:
                        rules.abandon_action(self._handle, s._handle)
                        complete(error)
            
            self._actions[actionName].run(s, action_callback)         


class Statechart(Ruleset):

    def __init__(self, name, host, chart_definition):
        self._name = name
        self._host = host
        ruleset_definition = {}
        self._transform(None, None, None, chart_definition, ruleset_definition)
        super(Statechart, self).__init__(name, host, ruleset_definition)
        self._definition = chart_definition

    def _transform(self, parent_name, parent_triggers, parent_start_state, chart_definition, rules):
        def state_filter(s):
            if 'label' in s:
                del s['label']

        start_state = {}

        for state_name, state in chart_definition.iteritems():
            qualified_name = state_name
            if parent_name:
                qualified_name = '{0}.{1}'.format(parent_name, state_name)

            start_state[qualified_name] = True

        for state_name, state in chart_definition.iteritems():
            qualified_name = state_name
            if parent_name:
                qualified_name = '{0}.{1}'.format(parent_name, state_name)

            triggers = {}
            if parent_triggers:
                for parent_trigger_name, trigger in parent_triggers.iteritems():
                    trigger_name = parent_trigger_name[parent_trigger_name.rindex('.') + 1:]
                    triggers['{0}.{1}'.format(qualified_name, trigger_name)] = trigger 

            for trigger_name, trigger in state.iteritems():
                if trigger_name != '$state':
                    if ('to' in trigger) and parent_name:
                        trigger['to'] = '{0}.{1}'.format(parent_name, trigger['to'])

                triggers['{0}.{1}'.format(qualified_name, trigger_name)] = trigger

            if '$chart' in state:
                self._transform(qualified_name, triggers, start_state, state['$chart'], rules)
            else:
                for trigger_name, trigger in triggers.iteritems():
                    rule = {}
                    state_test = {'label': qualified_name} 
                    if 'when' in trigger:
                        if '$s' in trigger['when']:
                            rule['when'] = {'$s': {'$and': [state_test, trigger['when']['$s']]}}
                        else:
                            rule['whenAll'] = {'$s': state_test, '$m': trigger['when']}

                    elif 'whenAll' in trigger:
                        test = {'$s': state_test}
                        for test_name, current_test in trigger['whenAll']:
                            if test_name != '$s':
                                test[test_name] = current_test
                            else:
                                test['$s'] = {'$and': [state_test, current_test]}
                        rule['whenAll'] = test
                    elif 'whenAny' in trigger:
                        rule['whenAll'] = {'$s': state_test, 'm$any': trigger['whenAny']}
                    elif 'whenSome' in trigger:
                        rule['whenAll'] = {'$s': state_test, 'm$some': trigger['whenSome']}
                    else:
                        rule['when'] = {'$s': state_test}

                    if 'run' in trigger:
                        if isinstance(trigger['run'], basestring):
                            rule['run'] = Promise(self._host.get_action(trigger['run']))
                        if isinstance(trigger['run'], Promise):
                            rule['run'] = trigger['run']
                        elif hasattr(trigger['run'], '__call__'):
                            rule['run'] = Promise(trigger['run'])
                        else:
                            rule['run'] = Fork(self._host.register_rulesets(self._name, trigger['run']), state_filter)

                    if 'to' in trigger:
                        if 'run' in rule:
                            rule['run'].continue_with(To(trigger['to']))
                        else:
                            rule['run'] = To(trigger['to'])

                        if trigger['to'] in start_state: 
                            del start_state[trigger['to']]

                        if parent_start_state and trigger['to'] in parent_start_state:
                            del parent_start_state[trigger['to']]
                    else:
                        raise Exception('Trigger {0} destination not defined'.format(trigger_name))

                    rules[trigger_name] = rule;
                    
        started = False 
        for state_name in start_state.keys():
            if started:
                raise Exception('Chart {0} has more than one start state'.format(self._name))

            started = True
            if parent_name:
                rules[parent_name + '$start'] = {'when': {'$s': {'label': parent_name}}, 'run': To(state_name)}
            else:
                rules['$start'] = {'when': {'$s': {'$nex': {'label': 1}}}, 'run': To(state_name)}

        if not started:
            raise Exception('Chart {0} has no start state'.format(self._name))


class Flowchart(Ruleset):

    def __init__(self, name, host, chart_definition):
        self._name = name
        self._host = host
        ruleset_definition = {} 
        self._transform(chart_definition, ruleset_definition)
        super(Flowchart, self).__init__(name, host, ruleset_definition)
        self._definition = chart_definition

    def _transform(self, chart_definition, rules):
        def stage_filter(s):
            if 'label' in s:
                del s['label']

        visited = {}
        for stage_name, stage in chart_definition.iteritems():
            stage_test = {'label': stage_name}
            if 'to' in stage:
                if isinstance(stage['to'], basestring):
                    next_stage = None
                    rule = {'when': {'$s': stage_test}}
                    if stage['to'] in chart_definition:
                        next_stage = chart_definition[stage['to']]
                    else:
                        raise Exception('Stage {0} not found'.format(stage['to']))

                    if not 'run' in next_stage:
                        rule['run'] = To(stage['to'])
                    else:
                        if isinstance(next_stage['run'], basestring):
                            rule['run'] = To(stage['to']).continue_with(Promise(self._host.get_action(next_stage['run'])))
                        elif isinstance(next_stage['run'], Promise) or hasattr(next_stage['run'], '__call__'):
                            rule['run'] = To(stage['to']).continue_with(next_stage['run'])
                        else:
                            fork_promise = Fork(self._host.register_rulesets(self._name, next_stage['run']), stage_filter)
                            rule['run'] = To(stage['to']).continue_with(fork_promise)

                    rules['{0}.{1}'.format(stage_name, stage['to'])] = rule
                    visited[stage['to']] = True
                else:
                    for transition_name, transition in stage['to'].iteritems():
                        rule = None
                        next_stage = None
                        if '$s' in transition:
                            rule = {'when': {'$s': {'$and': [stage_test, transition['$s']]}}}
                        elif '$all' in transition:
                            for test_name, all_test in transition['$all'].iteritems():
                                test = {'$s': stage_test}
                                if test_name != '$s':
                                    test[test_name] = all_test
                                else:
                                    test['$s'] = {'$and': [stage_test, all_test]}

                            rule = {'whenAll': test}
                        elif '$any' in transition:
                            rule = {'whenAll': {'$s': stage_test, 'm$any': transition['$any']}}
                        elif '$some' in transition:
                            rule = {'whenAll': {'$s': stage_test, 'm$some': transition['$some']}}
                        else:
                            rule = {'whenAll': {'$s': stage_test, '$m': transition}}

                        if transition_name in chart_definition:
                            next_stage = chart_definition[transition_name]
                        else:
                            raise Exception('Stage {0} not found'.format(transition_name))

                        if not 'run' in next_stage:
                            rule['run'] = To(transition_name)
                        else:
                            if isinstance(next_stage['run'], basestring):
                                rule['run'] = To(transition_name).continue_with(Promise(self._host.get_action(next_stage['run'])))
                            elif isinstance(next_stage['run'], Promise) or hasattr(next_stage['run'], '__call__'):
                                rule['run'] = To(transition_name).continue_with(next_stage['run'])
                            else:
                                fork_promise = Fork(self._host.register_rulesets(self._name, next_stage['run']), stage_filter)
                                rule['run'] = To(transition_name).continue_with(fork_promise)

                        rules['{0}.{1}'.format(stage_name, transition_name)] = rule
                        visited[transition_name] = True

        started = False
        for stage_name, stage in chart_definition.iteritems():
            if not stage_name in visited:
                if started:
                    raise Exception('Chart {0} has more than one start state'.format(self._name))

                rule = {'when': {'$s': {'$nex': {'label': 1}}}}
                if not 'run' in stage:
                    rule['run'] = To(stage_name)
                else:
                    if isinstance(stage['run'], basestring):
                        rule['run'] = To(stage_name).continue_with(Promise(self._host.get_action(stage['run'])))
                    elif isinstance(stage['run'], Promise) or hasattr(stage['run'], '__call__'):
                        rule['run'] = To(stage_name).continue_with(stage['run'])
                    else:
                        fork_promise = Fork(self._host.register_rulesets(self._name, stage['run']), stage_filter)
                        rule['run'] = To(stage_name).continue_with(fork_promise)

                rules['$start.{0}'.format(stage_name)] = rule
                started = True


class Host(object):

    def __init__(self, ruleset_definitions = None, databases = ['/tmp/redis.sock']):
        self._ruleset_directory = {}
        self._ruleset_list = []
        self._databases = databases
        if ruleset_definitions:
            self.register_rulesets(None, ruleset_definitions)

    def get_action(self, action_name):
        raise Exception('Action wiht name {0} not found'.format(action_name))

    def load_ruleset(self, ruleset_name):
        raise Exception('Ruleset with name {0} not found'.format(ruleset_name))

    def get_ruleset(self, ruleset_name):
        if ruleset_name in self._ruleset_directory:
            return self._ruleset_directory[ruleset_name]
        else:
            ruleset_definition = self.load_ruleset(ruleset_name)
            self.register_rulesets(null, ruleset_definition)
            return self._ruleset_directory[ruleset_name]

    def get_state(self, ruleset_name, sid):
        return self.get_ruleset(ruleset_name).get_state(sid)
        
    def post_batch(self, ruleset_name, messages):
        self.get_ruleset(ruleset_name).assert_events(messages)
        
    def post(self, ruleset_name, message):
        self.get_ruleset(ruleset_name).assert_event(message)
        
    def patch_state(self, ruleset_name, state):
        self.get_ruleset(ruleset_name).assert_state(state)

    def register_rulesets(self, parent_name, ruleset_definitions):
        rulesets = Ruleset.create_rulesets(parent_name, self, ruleset_definitions)
        for ruleset_name, ruleset in rulesets.iteritems():
            if ruleset_name in self._ruleset_directory:
                raise Exception('Ruleset with name {0} already registered'.format(ruleset_name))
            else:    
                self._ruleset_directory[ruleset_name] = ruleset
                self._ruleset_list.append(ruleset)
                ruleset.bind(self._databases)

        return list(rulesets.keys())

    def run(self):
        def dispatch_ruleset(index):
            def callback(e):
                if index % 10:
                    dispatch_ruleset(index + 1)
                else:
                    self._ruleset_timer = threading.Timer(0.01, dispatch_ruleset, (index + 1, ))
                    self._ruleset_timer.start()

            def timers_callback(e):
                if e:
                    print(e)

                if index % 10 and len(self._ruleset_list):
                    ruleset = self._ruleset_list[(index / 10) % len(self._ruleset_list)]
                    ruleset.dispatch_timers(callback)
                else:
                    callback(e)

            if len(self._ruleset_list):
                ruleset = self._ruleset_list[index % len(self._ruleset_list)]
                ruleset.dispatch(timers_callback)
            else:
                timers_callback(None)

        self._timer = threading.Timer(0.01, dispatch_ruleset, (0,))
        self._timer.start()
        
class Application(object):

    def __init__(self, host):
        self._host = host
        self._url_map = Map([
            Rule('/<ruleset_name>', endpoint='ruleset_definition_request'),
            Rule('/<ruleset_name>/<sid>', endpoint='ruleset_state_request')
        ])

    def ruleset_definition_request(self, request, ruleset_name):
        if request.method == 'GET':
            result = self._host.get_ruleset(ruleset_name)
            return Response(json.dumps(result.get_definition()))
        elif request.method == 'POST':
            self._host.register_rulesets(None, {ruleset_name: request.form})
        
        return Response()

    def ruleset_state_request(self, request, ruleset_name, sid):
        if request.method == 'GET':
            result = self._host.get_state(ruleset_name, sid)
            return Response(json.dumps(result))
        elif request.method == 'POST':
            self._host.post_batch(ruleset_name, sid, request.form)
        elif request.method == 'PATCH':
            self._host.patch_state(ruleset_name, sid, request.form)
        
        return Response()

    def __call__(self, environ, start_response):
        request = Request(environ)
        adapter = self._url_map.bind_to_environ(request.environ)
        try:
            endpoint, values = adapter.match()
            response = getattr(self, endpoint)(request, **values)
            return response(environ, start_response)
        except HTTPException as e:
            return e

    def run(self):
        self._host.run()
        run_simple('127.0.0.1', 5000, self, use_debugger=True, use_reloader=False)

def run(ruleset_definitions = None, databases = ['/tmp/redis.sock'], start = None):
    main_host = Host(ruleset_definitions, databases)
    if start:
        start(main_host)

    main_app = Application(main_host)
    main_app.run()







