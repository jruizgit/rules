import threading
import json
import rules

class Session(object):

    def __init__(self, document, event, handle, namespace):
        self.state = document
        self.event = event
        self._handle = handle
        self._namespace = namespace
        self._timer_directory = {}
        self._message_directory = {}
        

    def get_timers(self):
        return self._timer_directory

    def start_timer(self, timer_name, duration):
        if timer_name in self._timer_directory:
            raise Exception('Timer with name {0} already added', timer_name)
        else:
            self._timer_directory[timer_name] = duration
            
class Promise(object):

    def __init__(self, func):
        self._func = func
        self._next = None
        self._sync = True

    def continue_with(self, next):
        if (isinstance(next, Promise)):
            self._next = next
        elif (hasattr(action, '__call__')):
            self._next = Promise(next)
        else:
            raise Exception('Unexpected Promise Type')

    def run(self, s, complete):
        if self._sync:
            try:
                self._func(s)
            except Exception as error:
                complete(error, s)
                return

            if self._next:
                self._next.run(s, complete)
            else:
                complete(None, s)
        else:
            try:
                def callback(e, result):
                    if e: 
                        complete(e, s) 
                    elif self._next: 
                        self._next.run(s, complete) 
                    else: 
                        complete(None, s)

                self._func(s, callback)
            except Exception as error:
                complete(error, s)


class Ruleset(object):

    def __init__(self, name, host, ruleset_definition):
        self._actions = {}
        self._name = name
        for rule_name, rule in ruleset_definition.iteritems():
            action = rule['run']
            del rule['run']
            if (isinstance(action, Promise)):
                self._actions[rule_name] = action.get_root()
            elif (hasattr(action, '__call__')):
                self._actions[rule_name] = Promise(action)

        self._handle = rules.create_ruleset(name, json.dumps(ruleset_definition))
        
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

    def _start_timers(self, timers, index, s, complete):
        if index == len(timers):
            complete(None, s)
        else:
            try:
                timer_data = timers[index]
                timer = {'sid':s.state['id'], 'id':timer_data[0], '$t':timer_data[0]}
                rules.start_timer(self._handle, str(s.state['id']), timer_data[1], json.dumps(timer))
                self._start_timers(timers, index + 1, s, complete)
            except Exception as error:
                complete(error, s)

    def dispatch_timers(self, complete):
        try:
            rules.assert_timers(self._handle)
        except Exception as error:
            complete(error, None)
            return

        complete(None, None)

    def dispatch(self, complete):
        result = None
        try:
            result = rules.start_action(self._handle)
        except Exception as error:
            complete(error, None)
            return

        if not result:
            complete(None, None)
        else:
            document = json.loads(result[0])
            event = json.loads(result[1])[self._name]
            actionName = None
            for actionName, event in event.iteritems():
                break

            s = Session(document, event, result[2], self._name)
            action = self._actions[actionName]
                        
            def timer_callback(e, s):
                if e:
                    try:
                        rules.abandon_action(self._handle, s._handle)
                        complete(e, None)
                    except Exception as error:
                        complete(error, None)
                else:
                    try:
                        rules.complete_action(self._handle, s._handle, json.dumps(s.state))
                        complete(None, None)
                    except Exception as error:
                        complete(error, None)

            def action_callback(e, s):
                if e:
                    try:
                        rules.abandon_action(self._handle, s._handle)
                        complete(e, None)
                    except Exception as error:
                        complete(error, None)
                else:
                    timers = list(s.get_timers().items())
                    self._start_timers(timers, 0, s, timer_callback)

            action.run(s, action_callback)


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
            raise Exception('Ruleset with name {0} not found', ruleset_name)

    def post(self, ruleset_name, message, complete):
        if ruleset_name in self._ruleset_directory:
            self._ruleset_directory[ruleset_name].assert_event(message, complete)
        else:
            raise Exception('Ruleset with name {0} not found', ruleset_name)

    def register_rulesets(self, ruleset_definitions):
        for ruleset_name, ruleset_definition in ruleset_definitions.iteritems():
            self.register_ruleset(ruleset_name, ruleset_definition)

    def register_ruleset(self, ruleset_name, ruleset_definition):
        if ruleset_name in self._ruleset_directory:
            raise Exception('Ruleset with name {0} already registered', ruleset_name)
        else:
            ruleset = Ruleset(ruleset_name, self, ruleset_definition)
            self._ruleset_directory[ruleset_name] = ruleset
            self._ruleset_list.append(ruleset)

    def run(self):
        def dispatch_ruleset(index):
            def callback(e, result):
                if index % 10:
                    dispatch_ruleset(index + 1)
                else:
                    self._ruleset_timer = threading.Timer(0.01, dispatch_ruleset, (index + 1, ))
                    self._ruleset_timer.start()

            def timers_callback(e, result):
                if e:
                    print(e)

                if index % 10:
                    ruleset = self._ruleset_list[(index / 10) % len(self._ruleset_list)]
                    ruleset.dispatch_timers(callback)
                else:
                    callback(e, result)

            ruleset = self._ruleset_list[index % len(self._ruleset_list)]
            ruleset.dispatch(timers_callback)

        self._timer = threading.Timer(0.01, dispatch_ruleset, (0,))
        self._timer.start()
        self._exit.wait()

def run(ruleset_definitions, databases, start):
    main_host = Host()
    main_host.register_rulesets(ruleset_definitions)
    main_host.bind(databases)
    if start:
        start(main_host)

    main_host.run()








