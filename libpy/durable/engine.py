import json
import copy
import rules
import threading
import inspect
import random
import time

class Closure(object):

    def __init__(self, state, message, handle, ruleset_name):
        self.ruleset_name = ruleset_name
        self.s = Content(state)
        self._handle = handle
        self._timer_directory = {}
        self._message_directory = {}
        self._branch_directory = {}
        self._fact_directory = {}
        self._retract_directory = {}
        if isinstance(message, dict): 
            self._m = message
        else:
            self.m = []
            for one_message in message:
                if ('m' in one_message) and len(one_message) == 1:
                    one_message = one_message['m']

                self.m.append(Content(one_message))

    def get_timers(self):
        return self._timer_directory

    def get_branches(self):
        return self._branch_directory

    def get_messages(self):
        return self._message_directory

    def get_facts(self):
        return self._fact_directory

    def get_retract_facts(self):
        return self._retract_directory

    def signal(self, message):
        if isinstance(message, Content):
            message = message._d

        name_index = self.ruleset_name.rindex('.')
        parent_ruleset_name = self.ruleset_name[:name_index]
        name = self.ruleset_name[name_index + 1:]
        message['sid'] = self.s['sid']
        message['id'] = '{0}.{1}'.format(name, message['id'])
        self.post(parent_ruleset_name, message)

    def post(self, ruleset_name, message = None):
        if not message: 
            message = ruleset_name
            ruleset_name = self.ruleset_name

        if not 'sid' in message:
            message['sid'] = self.s['sid']

        if isinstance(message, Content):
            message = message._d

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

    def assert_fact(self, ruleset_name, fact = None):
        if not fact: 
            fact = ruleset_name
            ruleset_name = self.ruleset_name

        if not 'sid' in fact:
            fact['sid'] = self.s['sid']

        if isinstance(fact, Content):
            fact = copy.deepcopy(fact._d)

        fact_list = []
        if  ruleset_name in self._fact_directory:
            fact_list = self._fact_directory[ruleset_name]
        else:
            self._fact_directory[ruleset_name] = fact_list

        fact_list.append(fact)

    def retract_fact(self, ruleset_name, fact = None):
        if not fact: 
            fact = ruleset_name
            ruleset_name = self.ruleset_name

        if not 'sid' in fact:
            fact['sid'] = self.s['sid']

        if isinstance(fact, Content):
            fact = copy.deepcopy(fact._d)

        retract_list = []
        if  ruleset_name in self._retract_directory:
            retract_list = self._retract_directory[ruleset_name]
        else:
            self._retract_directory[ruleset_name] = retract_list

        retract_list.append(fact)

    def fork(self, branch_name, branch_state):
        if branch_name in self._branch_directory:
            raise Exception('Branch with name {0} already forked'.format(branch_name))
        else:
            self._branch_directory[branch_name] = branch_state

    def __getattr__(self, name):   
        if name in self._m:
            return Content(self._m[name])
        else:
            return None

class Content(object):

    def __init__(self, data):
        self._d = data

    def __getitem__(self, key):
        if key in self._d:
            data = self._d[key]
            if isinstance(data, dict):
                data = Content(data)

            return data 
        else:
            return None

    def __setitem__(self, key, value):
        if isinstance(value, Content):
            self._d[key] = value._d
        else:    
            self._d[key] = value

    def __iter__(self):
        return self._d.__iter__

    def __contains__(self, key):
        return key in self._d

    def __getattr__(self, name):  
        return self.__getitem__(name)

    def __setattr__(self, name, value):
        if name == '_d':
            self.__dict__['_d'] = value
        else:
            self.__setitem__(name, value)

    def __repr__(self):
        return repr(self._d)

    def __str__(self):
        return str(self._d)


class Promise(object):

    def __init__(self, func):
        self._func = func
        self._next = None
        self._sync = True
        self.root = self

        arg_count = func.__code__.co_argcount
        if inspect.ismethod(func):
            arg_count -= 1

        if arg_count == 2:
            self._sync = False
        elif arg_count != 1:
            raise Exception('Invalid function signature')

    def continue_with(self, next):
        if (isinstance(next, Promise)):
            self._next = next
        elif (hasattr(next, '__call__')):
            self._next = Promise(next)
        else:
            raise Exception('Unexpected Promise Type')

        self._next.root = self.root
        return self._next

    def run(self, c, complete):
        if self._sync:
            try:
                self._func(c)

            except Exception as error:
                complete(error)
                return

            if self._next:
                self._next.run(c, complete)
            else:
                complete(None)
        else:
            try:
                def callback(e):
                    if e: 
                        complete(e) 
                    elif self._next: 
                        self._next.run(c, complete) 
                    else: 
                        complete(None)

                self._func(c, callback)
            except Exception as error:
                complete(error)


class Fork(Promise):

    def __init__(self, branch_names, state_filter = None):
        super(Fork, self).__init__(self._execute)
        self.branch_names = branch_names
        self.state_filter = state_filter
        
    def _execute(self, c):
        for branch_name in self.branch_names:
            state = copy.deepcopy(c.s._d)
            if self.state_filter:
                self.state_filter(state)

            c.fork(branch_name, state)
        

class To(Promise):

    def __init__(self, state):
        super(To, self).__init__(self._execute)
        self._state = state
        
    def _execute(self, c):
        c.s['label'] = self._state


class Ruleset(object):

    def __init__(self, name, host, ruleset_definition, state_cache_size):
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

        self._handle = rules.create_ruleset(state_cache_size, name, json.dumps(ruleset_definition))
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
        
    def start_assert_fact(self, fact):
        return rules.start_assert_fact(self._handle, json.dumps(fact))

    def assert_fact(self, fact):
        rules.assert_fact(self._handle, json.dumps(fact))

    def assert_facts(self, facts):
        rules.assert_facts(self._handle, json.dumps(facts))

    def retract_fact(self, fact):
        rules.retract_fact(self._handle, json.dumps(fact))

    def retract_facts(self, facts):
        rules.retract_facts(self._handle, json.dumps(facts))

    def start_timer(self, sid, timer_name, timer_duration):
        timer = {'sid':sid, 'id':random.randint(100000, 10000000), '$t':timer_name}
        rules.start_timer(self._handle, str(sid), timer_duration, json.dumps(timer))
        
    def assert_state(self, state):
        rules.assert_state(self._handle, json.dumps(state))
        
    def get_state(self, sid):
        return json.loads(rules.get_state(self._handle, sid))
    
    def get_definition(self):
        return self._definition

    @staticmethod
    def create_rulesets(parent_name, host, ruleset_definitions, state_cache_size):
        branches = {}
        for name, definition in ruleset_definitions.iteritems():  
            if name.rfind('$state') != -1:
                name = name[:name.rfind('$state')]
                if parent_name:
                    name = '{0}.{1}'.format(parent_name, name) 

                branches[name] = Statechart(name, host, definition, state_cache_size)
            elif name.rfind('$flow') != -1:
                name = name[:name.rfind('$flow')]
                if parent_name:
                    name = '{0}.{1}'.format(parent_name, name) 

                branches[name] = Flowchart(name, host, definition, state_cache_size)
            else:
                if parent_name:
                    name = '{0}.{1}'.format(parent_name, name)

                branches[name] = Ruleset(name, host, definition, state_cache_size)

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
            message = json.loads(result[1])
            action_name = None
            for action_name, message in message.iteritems():
                break
            c = Closure(state, message, result[2], self._name)
            
            def action_callback(e):
                if e:
                    rules.abandon_action(self._handle, c._handle)
                    complete(e)
                else:
                    try:
                        for branch_name, branch_state in c.get_branches().iteritems():
                            self._host.patch_state(branch_name, branch_state)  

                        for ruleset_name, messages in c.get_messages().iteritems():
                            if len(messages) == 1:
                                self._host.post(ruleset_name, messages[0])
                            else:
                                self._host.post_batch(ruleset_name, messages)

                        pending = []
                        for ruleset_name, facts in c.get_facts().iteritems():
                            if len(facts) == 1:
                                pending.append(self._host.start_assert_fact(ruleset_name, facts[0]))
                            else:
                                self._host.assert_facts(ruleset_name, facts)

                        for item in pending:
                            if item[0] > 0:
                                rules.complete(item[0], item[1])

                        for ruleset_name, facts in c.get_retract_facts().iteritems():
                            if len(facts) == 1:
                                self._host.retract_fact(ruleset_name, facts[0])
                            else:
                                self._host.retract_facts(ruleset_name, facts)

                        for timer_name, timer_duration in c.get_timers().iteritems():
                            self.start_timer(c.s['sid'], timer_name, timer_duration)                            
                            
                        rules.complete_action(self._handle, c._handle, json.dumps(c.s._d))
                        complete(None)
                    except Exception as error:
                        rules.abandon_action(self._handle, c._handle)
                        complete(error)
            
            self._actions[action_name].run(c, action_callback)         


class Statechart(Ruleset):

    def __init__(self, name, host, chart_definition, state_cache_size):
        self._name = name
        self._host = host
        ruleset_definition = {}
        self._transform(None, None, None, chart_definition, ruleset_definition)
        super(Statechart, self).__init__(name, host, ruleset_definition, state_cache_size)
        self._definition = chart_definition
        self._definition['$type'] = 'stateChart'

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
                if trigger_name != '$chart':
                    if ('to' in trigger) and parent_name:
                        trigger['to'] = '{0}.{1}'.format(parent_name, trigger['to'])

                    triggers['{0}.{1}'.format(qualified_name, trigger_name)] = trigger

            if '$chart' in state:
                self._transform(qualified_name, triggers, start_state, state['$chart'], rules)
            else:
                for trigger_name, trigger in triggers.iteritems():
                    rule = {}
                    state_test = {'$s': {'$and':[{'label': qualified_name}, {'$s':1}]}}
                    if 'pri' in trigger:
                        rule['pri'] = trigger['pri']

                    if 'count' in trigger:
                        rule['count'] = trigger['count']

                    if 'all' in trigger:
                        rule['all'] = list(trigger['all'])
                        rule['all'].append(state_test)
                    elif 'any' in trigger:
                        rule['all'] = [state_test, {'m$any': trigger['any']}]
                    else:
                        rule['all'] = [state_test]

                    if 'run' in trigger:
                        if isinstance(trigger['run'], basestring):
                            rule['run'] = Promise(self._host.get_action(trigger['run']))
                        elif isinstance(trigger['run'], Promise):
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
                rules[parent_name + '$start'] = {'all':[{'$s': {'$and': [{'label': parent_name}, {'$s':1}]}}], 'run': To(state_name)};
            else:
                rules['$start'] = {'all': [{'$s': {'$and': [{'$nex': {'label': 1}}, {'$s':1}]}}], 'run': To(state_name)};

        if not started:
            raise Exception('Chart {0} has no start state'.format(self._name))


class Flowchart(Ruleset):

    def __init__(self, name, host, chart_definition, state_cache_size):
        self._name = name
        self._host = host
        ruleset_definition = {} 
        self._transform(chart_definition, ruleset_definition)
        super(Flowchart, self).__init__(name, host, ruleset_definition, state_cache_size)
        self._definition = chart_definition
        self._definition['$type'] = 'flowChart'

    def _transform(self, chart_definition, rules):
        def stage_filter(s):
            if 'label' in s:
                del s['label']

        visited = {}
        for stage_name, stage in chart_definition.iteritems():
            stage_test = {'$s': {'$and':[{'label': stage_name}, {'$s':1}]}}
            if 'to' in stage:
                if isinstance(stage['to'], basestring):
                    next_stage = None
                    rule = {'all': [stage_test]}
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
                        rule = {}
                        next_stage = None

                        if 'pri' in transition:
                            rule['pri'] = transition['pri']

                        if 'count' in transition:
                            rule['count'] = transition['count']

                        if 'all' in transition:
                            rule['all'] = list(transition['all'])
                            rule['all'].append(stage_test)
                        elif 'any' in transition:
                            rule['all'] = [stage_test, {'m$any': transition['any']}]
                        else:
                            rule['all'] = [stage_test]

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

                rule = {'all': [{'$s': {'$and': [{'$nex': {'label':1}}, {'$s': 1}]}}]}
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

    def __init__(self, ruleset_definitions = None, databases = ['/tmp/redis.sock'], state_cache_size = 1024):
        self._ruleset_directory = {}
        self._ruleset_list = []
        self._databases = databases
        self._state_cache_size = state_cache_size
        if ruleset_definitions:
            self.register_rulesets(None, ruleset_definitions)

    def get_action(self, action_name):
        raise Exception('Action with name {0} not found'.format(action_name))

    def load_ruleset(self, ruleset_name):
        raise Exception('Ruleset with name {0} not found'.format(ruleset_name))

    def save_ruleset(self, ruleset_name, ruleset_definition):
        return

    def get_ruleset(self, ruleset_name):
        if ruleset_name in self._ruleset_directory:
            return self._ruleset_directory[ruleset_name]
        else:
            ruleset_definition = self.load_ruleset(ruleset_name)
            self.register_rulesets(None, ruleset_definition)
            return self._ruleset_directory[ruleset_name]

    def set_ruleset(self, ruleset_name, ruleset_definition):
        self.register_rulesets(None, ruleset_definition)
        self.save_ruleset(ruleset_name, ruleset_definition)

    def get_state(self, ruleset_name, sid):
        return self.get_ruleset(ruleset_name).get_state(sid)

    def get_ruleset_state(self, ruleset_name):
        return self.get_ruleset(ruleset_name).get_ruleset_state()
        
    def post_batch(self, ruleset_name, messages):
        self.get_ruleset(ruleset_name).assert_events(messages)
        
    def post(self, ruleset_name, message):
        self.get_ruleset(ruleset_name).assert_event(message)
        
    def start_assert_fact(self, ruleset_name, fact):
        return self.get_ruleset(ruleset_name).start_assert_fact(fact)

    def assert_fact(self, ruleset_name, fact):
        self.get_ruleset(ruleset_name).assert_fact(fact)

    def assert_facts(self, ruleset_name, facts):
        self.get_ruleset(ruleset_name).assert_facts(facts)

    def retract_fact(self, ruleset_name, fact):
        self.get_ruleset(ruleset_name).retract_fact(fact)

    def retract_facts(self, ruleset_name, facts):
        self.get_ruleset(ruleset_name).retract_facts(facts)

    def start_timer(self, ruleset_name, sid, timer_name, timer_duration):
        self.get_ruleset(ruleset_name).start_timer(sid, timer_name, timer_duration)

    def patch_state(self, ruleset_name, state):
        self.get_ruleset(ruleset_name).assert_state(state)

    def patch_ruleset_state(self, ruleset_name, state):
        self.get_ruleset(ruleset_name).set_ruleset_state(state)

    def register_rulesets(self, parent_name, ruleset_definitions):
        rulesets = Ruleset.create_rulesets(parent_name, self, ruleset_definitions, self._state_cache_size)
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
                    self._ruleset_timer = threading.Timer(0.001, dispatch_ruleset, (index + 1, ))
                    self._ruleset_timer.start()

            def timers_callback(e):
                if e:
                    print(e)

                if (index % 10 == 0) and len(self._ruleset_list):
                    ruleset = self._ruleset_list[(index / 10) % len(self._ruleset_list)]
                    ruleset.dispatch_timers(callback)
                else:
                    callback(e)

            if len(self._ruleset_list):
                ruleset = self._ruleset_list[index % len(self._ruleset_list)]
                ruleset.dispatch(timers_callback)
            else:
                timers_callback(None)

        self._timer = threading.Timer(0.001, dispatch_ruleset, (0,))
        self._timer.start()
