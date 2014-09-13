import threading
import json
import copy
import rules

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
        elif (hasattr(action, '__call__')):
            self._next = Promise(next)
        else:
            raise Exception('Unexpected Promise Type')

        self._next.root = self.root

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
            if (isinstance(action, Promise)):
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

    def assert_event(self, message, complete):
        try:
            complete(None, rules.assert_event(self._handle, json.dumps(message)))
        except Exception as error:
            complete(error, None)

    def assert_events(self, messages, complete):
        try:
            complete(None, rules.assert_events(self._handle, json.dumps(messages)))
        except Exception as error:
            complete(error, None)

    def assert_state(self, state, complete):
        try:
            complete(None, rules.assert_state(self._handle, json.dumps(state)))
        except Exception as error:
            complete(error, None)

    def get_state(self, complete):
        try:
            complete(None, json.loads(rules.get_state(self._handle)))
        except Exception as error:
            complete(error, None)

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

    def _start_branches(self, branches, index, s, complete):
        def callback(e, result):
            if e:
                complete(e)
            else:
                self._start_branches(branches, index + 1, s, complete)

        if index == len(branches):
            complete(None)
        else:
            try:
                branch_data = branches[index]
                self._host.start(branch_data[0], branch_data[1], callback)  
            except Exception as error:
                complete(error)  

    def _start_timers(self, timers, index, s, complete):
        if index == len(timers):
            complete(None)
        else:
            try:
                timer_data = timers[index]
                timer = {'sid':s.id, 'id':timer_data[0], '$t':timer_data[0]}
                rules.start_timer(self._handle, str(s.state['id']), timer_data[1], json.dumps(timer))
                self._start_timers(timers, index + 1, s, complete)
            except Exception as error:
                complete(error)

    def _post_messages(self, messages, index, s, complete):
        def callback(e, result):
            if e:
                complete(e)
            else:
                self._post_messages(messages, index + 1, s, complete)

        if index == len(messages):
            complete(None)
        else:
            try:
                message_data = messages[index]
                if len(message_data[1]) == 1:
                    self._host.post(message_data[0], message_data[1][0], callback)
                else:
                    self._host.post_batch(message_data[0], message_data[1], callback)
            except Exception as error:
                complete(error)            

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
            action = self._actions[actionName]
                        
            def timers_callback(e):
                if e:
                    try:
                        rules.abandon_action(self._handle, s._handle)
                        complete(e)
                    except Exception as error:
                        complete(error)
                else:
                    try:
                        rules.complete_action(self._handle, s._handle, json.dumps(s.state))
                        complete(None)
                    except Exception as error:
                        complete(error)

            def messages_callback(e):
                if e:
                    try:
                        rules.abandon_action(self._handle, s._handle)
                        complete(e)
                    except Exception as error:
                        complete(error)
                else:
                    timers = list(s.get_timers().items())
                    self._start_timers(timers, 0, s, timers_callback)

            def branches_callback(e):
                if e:
                    try:
                        rules.abandon_action(self._handle, s._handle)
                        complete(e)
                    except Exception as error:
                        complete(error)
                else:
                    messages = list(s.get_messages().items())
                    self._post_messages(messages, 0, s, messages_callback)

            def action_callback(e):
                if e:
                    try:
                        rules.abandon_action(self._handle, s._handle)
                        complete(e)
                    except Exception as error:
                        complete(error)
                else:
                    branches = list(s.get_branches().items())
                    self._start_branches(branches, 0, s, branches_callback)

            action.run(s, action_callback)


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
                        if (isinstance(trigger['run'], Promise)):
                            rule['run'] = trigger['run']
                        elif (hasattr(trigger['run'], '__call__')):
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


class Host(object):

    def __init__(self):
        self._ruleset_directory = {}
        self._ruleset_list = []
        self._exit = threading.Event()

    def bind(self, databases):
        for ruleset in self._ruleset_list:
            ruleset.bind(databases)

    def post_batch(self, ruleset_name, messages, complete):
        if ruleset_name in self._ruleset_directory:
            self._ruleset_directory[ruleset_name].assert_events(messages, complete)
        else:
            raise Exception('Ruleset with name {0} not found'.format(ruleset_name))

    def post(self, ruleset_name, message, complete):
        if ruleset_name in self._ruleset_directory:
            self._ruleset_directory[ruleset_name].assert_event(message, complete)
        else:
            raise Exception('Ruleset with name {0} not found'.format(ruleset_name))

    def start(self, ruleset_name, state, complete):
        if ruleset_name in self._ruleset_directory:
            self._ruleset_directory[ruleset_name].assert_state(state, complete)
        else:
            raise Exception('Ruleset with name {0} not found'.format(ruleset_name))

    def register_rulesets(self, parent_name, ruleset_definitions):
        rulesets = Ruleset.create_rulesets(parent_name, self, ruleset_definitions)
        for ruleset_name, ruleset_definition in rulesets.iteritems():
            if ruleset_name in self._ruleset_directory:
                raise Exception('Ruleset with name {0} already registered'.format(ruleset_name))
            else:    
                self._ruleset_directory[ruleset_name] = ruleset_definition
                self._ruleset_list.append(ruleset_definition)

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

                if index % 10:
                    ruleset = self._ruleset_list[(index / 10) % len(self._ruleset_list)]
                    ruleset.dispatch_timers(callback)
                else:
                    callback(e)

            ruleset = self._ruleset_list[index % len(self._ruleset_list)]
            ruleset.dispatch(timers_callback)

        self._timer = threading.Timer(0.01, dispatch_ruleset, (0,))
        self._timer.start()
        self._exit.wait()

def run(ruleset_definitions, databases, start):
    main_host = Host()
    main_host.register_rulesets(None, ruleset_definitions)
    main_host.bind(databases)
    if start:
        start(main_host)

    main_host.run()








