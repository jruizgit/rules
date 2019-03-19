import json
import copy
import rules
import threading
import inspect
import random
import time
import datetime
import os
import sys
import traceback

def _unix_now():
    dt = datetime.datetime.now()
    epoch = datetime.datetime.utcfromtimestamp(0)
    delta = dt - epoch
    return delta.total_seconds()


class Closure_Queue(object):

    def __init__(self):
        self._queued_posts = []
        self._queued_asserts = []
        self._queued_retracts = []

    def get_queued_posts(self):
        return self._queued_posts

    def get_queued_asserts(self):
        return self._queued_posts

    def get_queued_retracts(self):
        return self._queued_posts      

    def post(self, message):
        if isinstance(message, Content):
            message = message._d

        self._queued_posts.append(message)

    def assert_fact(self, message):
        if isinstance(message, Content):
            message = message._d

        self._queued_asserts.append(message)

    def retract_fact(self, message):
        if isinstance(message, Content):
            message = message._d

        self._queued_retracts.append(message)

class Closure(object):

    def __init__(self, host, state, message, handle, ruleset_name):
        self.ruleset_name = ruleset_name
        self.host = host
        self.s = Content(state)
        self._handle = handle
        self._timer_directory = {}
        self._cancelled_timer_directory = {}
        self._message_directory = {}
        self._queue_directory = {}
        self._branch_directory = {}
        self._fact_directory = {}
        self._delete_directory = {}
        self._retract_directory = {}
        self._completed = False
        self._deleted = False
        self._start_time = _unix_now()
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

    def get_cancelled_timers(self):
        return self._cancelled_timer_directory

    def get_branches(self):
        return self._branch_directory

    def get_messages(self):
        return self._message_directory

    def get_queues(self):
        return self._queue_directory

    def get_deletes(self):
        return self._delete_directory

    def get_facts(self):
        return self._fact_directory

    def get_retract_facts(self):
        return self._retract_directory

    def get_queue(self, ruleset_name):
        if not ruleset_name in self._queue_directory:
            self._queue_directory[ruleset_name] = Closure_Queue()
        
        return self._queue_directory[ruleset_name]

    def post(self, ruleset_name, message = None):
        if not message: 
            message = ruleset_name
            ruleset_name = self.ruleset_name

        if not 'sid' in message:
            message['sid'] = self.s.sid

        if isinstance(message, Content):
            message = message._d

        message_list = []
        if  ruleset_name in self._message_directory:
            message_list = self._message_directory[ruleset_name]
        else:
            self._message_directory[ruleset_name] = message_list

        message_list.append(message)

    def delete(self, ruleset_name = None, sid = None):
        if not ruleset_name: 
            ruleset_name = self.ruleset_name
            
        if not sid:
            sid = self.s.sid

        if (ruleset_name == self.ruleset_name) and (sid == self.s.sid):
            self._deleted = True

        sid_list = []
        if  ruleset_name in self._delete_directory:
            sid_list = self._delete_directory[ruleset_name]
        else:
            self._delete_directory[ruleset_name] = sid_list

        sid_list.append(sid)

    def start_timer(self, timer_name, duration, manual_reset = False):
        if timer_name in self._timer_directory:
            raise Exception('Timer with name {0} already added'.format(timer_name))
        else:
            timer = {'sid': self.s.sid, '$t': timer_name}
            self._timer_directory[timer_name] = (timer, duration, manual_reset)
        
    def cancel_timer(self, timer_name):
        if timer_name in self._cancelled_timer_directory:
            raise Exception('Timer with name {0} already cancelled'.format(timer_name))
        else:
            self._cancelled_timer_directory[timer_name] = True

    def _retract_timer(self, timer_name, message):
        if '$t' in message and message['$t'] == timer_name:
            self.retract_fact(message)
            return True

        for property_name, property_value in message.items():
            if isinstance(property_value, dict) and self._retract_timer(timer_name, property_value):
                return True

        return False

    def reset_timer(self, timer_name):
        if self._m:
            return self._retract_timer(timer_name, self._m)
        else:
            for message in self.m:
                if self._retract_timer(timer_name, message):
                    return True

            return False

    def assert_fact(self, ruleset_name, fact = None):
        if not fact: 
            fact = ruleset_name
            ruleset_name = self.ruleset_name

        if not 'sid' in fact:
            fact['sid'] = self.s.sid

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
            fact['sid'] = self.s.sid

        if isinstance(fact, Content):
            fact = copy.deepcopy(fact._d)

        retract_list = []
        if  ruleset_name in self._retract_directory:
            retract_list = self._retract_directory[ruleset_name]
        else:
            self._retract_directory[ruleset_name] = retract_list

        retract_list.append(fact)

    def renew_action_lease(self):
        if _unix_now() - self._start_time < 10:
            self._start_time = _unix_now()
            self.host.renew_action_lease(self.ruleset_name, self.s.sid) 

    def _has_completed(self):
        if _unix_now() - self._start_time > 10:
            self._completed = True

        value = self._completed
        self._completed = True
        return value

    def _is_deleted(self):
        return self._deleted

    def __getattr__(self, name):
        if name == '_m':
            return None

        if name in self._m:
            return Content(self._m[name])
        else:
            return None

class Content(object):

    def items(self):
        return self._d.items()

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
        if value == None:
            del self._d[key]
        elif isinstance(value, Content):
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
        self._timer = None
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
        def timeout(max_time):
            if _unix_now() > max_time:
                c.s.exception = 'timeout expired'
                complete(None)
            else:
                c.renew_action_lease()
                self._timer = threading.Timer(5, timeout, (max_time, ))
                self._timer.daemon = True
                self._timer.start()

        if self._sync:
            try:
                self._func(c) 
            except BaseException as error:
                t, v, tb = sys.exc_info()
                c.s.exception = 'exception caught {0}, traceback {1}'.format(
                    str(error), traceback.format_tb(tb))
            except:
                c.s.exception = 'unknown exception'
               
            if self._next:
                self._next.run(c, complete)
            else:
                complete(None)
        else:
            try:
                def callback(e):
                    if self._timer:
                        self._timer.cancel()
                        self._timer = None

                    if e:
                        c.s.exception = str(e) 
                         
                    if self._next: 
                        self._next.run(c, complete) 
                    else: 
                        complete(None)

                time_left = self._func(c, callback)     
                if time_left:
                    self._timer = threading.Timer(5, timeout, (_unix_now() + time_left, ))      
                    self._timer.daemon = True     
                    self._timer.start()
            except BaseException as error:
                t, v, tb = sys.exc_info()
                c.s.exception = 'exception caught {0}, traceback {1}'.format(
                    str(error), traceback.format_tb(tb))
                complete(None)
            except:
                c.s.exception = 'unknown exception'
                complete(None)
        

class To(Promise):

    def __init__(self, from_state, to_state, assert_state):
        super(To, self).__init__(self._execute)
        self._from_state = from_state
        self._to_state = to_state
        self._assert_state = assert_state
        
    def _execute(self, c):
        c.s.running = True
        if self._from_state != self._to_state:
            if self._from_state:
                if c.m and isinstance(c.m, list):
                    c.retract_fact(c.m[0].chart_context)
                else:
                    c.retract_fact(c.chart_context)

            if self._assert_state:
                c.assert_fact({ 'label': self._to_state, 'chart': 1 })
            else:
                c.post({ 'label': self._to_state, 'chart': 1 })
        

class Ruleset(object):

    def __init__(self, name, host, ruleset_definition, state_cache_size):
        self._actions = {}
        self._name = name
        self._host = host
        for rule_name, rule in ruleset_definition.items():
            action = rule['run']
            del rule['run']
            if isinstance(action, str):
                self._actions[rule_name] = Promise(host.get_action(action))
            elif isinstance(action, Promise):
                self._actions[rule_name] = action.root
            elif (hasattr(action, '__call__')):
                self._actions[rule_name] = Promise(action)

        self._handle = rules.create_ruleset(state_cache_size, name, json.dumps(ruleset_definition, ensure_ascii=False))
        self._definition = ruleset_definition
        
    def bind(self, databases):
        for db in databases:
            if isinstance(db, str):
                rules.bind_ruleset(0, 0, db, None, self._handle)
            else:
                if not 'password' in db:
                    db['password'] = None

                if not 'db' in db:
                    db['db'] = 0

                rules.bind_ruleset(db['port'], db['db'], db['host'], db['password'], self._handle)

    def assert_event(self, message):
        return rules.assert_event(self._handle, json.dumps(message, ensure_ascii=False))

    def queue_assert_event(self, sid, ruleset_name, message):
        if sid != None: 
            sid = str(sid)

        rules.queue_assert_event(self._handle, sid, ruleset_name, json.dumps(message, ensure_ascii=False))

    def start_assert_event(self, message):
        return rules.start_assert_event(self._handle, json.dumps(message, ensure_ascii=False))

    def assert_events(self, messages):
        return rules.assert_events(self._handle, json.dumps(messages, ensure_ascii=False))
    
    def start_assert_events(self, messages):
        return rules.start_assert_events(self._handle, json.dumps(messages, ensure_ascii=False))

    def assert_fact(self, fact):
        return rules.assert_fact(self._handle, json.dumps(fact, ensure_ascii=False))

    def queue_assert_fact(self, sid, ruleset_name, message):
        if sid != None: 
            sid = str(sid)

        rules.queue_assert_fact(self._handle, sid, ruleset_name, json.dumps(message, ensure_ascii=False))

    def start_assert_fact(self, fact):
        return rules.start_assert_fact(self._handle, json.dumps(fact, ensure_ascii=False))

    def assert_facts(self, facts):
        return rules.assert_facts(self._handle, json.dumps(facts, ensure_ascii=False))

    def start_assert_facts(self, facts):
        return rules.start_assert_facts(self._handle, json.dumps(facts, ensure_ascii=False))

    def retract_fact(self, fact):
        return rules.retract_fact(self._handle, json.dumps(fact, ensure_ascii=False))
        
    def queue_retract_fact(self, sid, ruleset_name, message):
        if sid != None: 
            sid = str(sid)

        rules.queue_retract_fact(self._handle, sid, ruleset_name, json.dumps(message, ensure_ascii=False))

    def start_retract_fact(self, fact):
        return rules.start_retract_fact(self._handle, json.dumps(fact, ensure_ascii=False))

    def retract_facts(self, facts):
        return rules.retract_facts(self._handle, json.dumps(facts, ensure_ascii=False))

    def start_retract_facts(self, facts):
        return rules.start_retract_facts(self._handle, json.dumps(facts, ensure_ascii=False))

    def start_timer(self, sid, timer, timer_duration, manual_reset):
        if sid != None: 
            sid = str(sid)

        rules.start_timer(self._handle, timer_duration, manual_reset, json.dumps(timer, ensure_ascii=False), sid)

    def cancel_timer(self, sid, timer_name):
        if sid != None: 
            sid = str(sid)

        rules.cancel_timer(self._handle, sid, timer_name)

    def assert_state(self, state):
        if 'sid' in state:
            return rules.assert_state(self._handle, str(state['sid']), json.dumps(state, ensure_ascii=False))
        else:
            return rules.assert_state(self._handle, None, json.dumps(state, ensure_ascii=False))

    def get_state(self, sid):
        if sid != None: 
            sid = str(sid)

        return json.loads(rules.get_state(self._handle, sid))

    def delete_state(self, sid):
        if sid != None: 
            sid = str(sid)

        rules.delete_state(self._handle, sid)
    
    def renew_action_lease(self, sid):
        if sid != None: 
            sid = str(sid)

        rules.renew_action_lease(self._handle, sid)

    def get_definition(self):
        return self._definition

    @staticmethod
    def create_rulesets(parent_name, host, ruleset_definitions, state_cache_size):
        branches = {}
        for name, definition in ruleset_definitions.items():  
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
            if not rules.assert_timers(self._handle):
               complete(None, True)
            else:
               complete(None, False) 
        except Exception as error:
            complete(error, True)
            return

    def dispatch(self, complete, async_result = None):
        state = None
        action_handle = None
        action_binding = None
        result_container = {}
        if async_result:
            state = async_result[0]
            result_container = {'message': json.loads(async_result[1])}
            action_handle = async_result[2]
            action_binding = async_result[3]
        else:
            try:
                result = rules.start_action(self._handle)
                if not result:
                    complete(None, True)
                    return
                else: 
                    state = json.loads(result[0])
                    result_container = {'message': json.loads(result[1])}
                    action_handle = result[2]
                    action_binding = result[3]
            except BaseException as error:
                t, v, tb = sys.exc_info()
                print('start action base exception type {0}, value {1}, traceback {2}'.format(t, str(v), traceback.format_tb(tb)))
                complete(error, True)
                return
            except:
                t, v, tb = sys.exc_info()
                print('start action unknown exception type {0}, value {1}, traceback {2}'.format(t, str(v), traceback.format_tb(tb)))
                complete('unknown error', True)
                return
        
        while 'message' in result_container:
            action_name = None
            for action_name, message in result_container['message'].items():
                break

            del(result_container['message'])
            c = Closure(self._host, state, message, action_handle, self._name)
            
            def action_callback(e):
                if c._has_completed():
                    return

                if e:
                    rules.abandon_action(self._handle, c._handle)
                    complete(e, True)
                else:
                    try:
                        for timer_name, timer in c.get_cancelled_timers().items():
                            self.cancel_timer(c.s['sid'], timer_name)

                        for timer_id, timer_tuple in c.get_timers().items():
                            self.start_timer(c.s['sid'], timer_tuple[0], timer_tuple[1], timer_tuple[2])

                        for ruleset_name, q in c.get_queues().items():
                            for message in q.get_queued_posts():
                                self.queue_assert_event(message['sid'], ruleset_name, message)

                            for message in q.get_queued_asserts():
                                self.queue_assert_fact(message['sid'], ruleset_name, message)

                            for message in q.get_queued_retracts():
                                self.queue_retract_fact(message['sid'], ruleset_name, message)

  
                        for ruleset_name, sid in c.get_deletes().items():
                            self._host.delete_state(ruleset_name, sid)

                        binding  = 0
                        replies = 0
                        pending = {action_binding: 0}
        
                        for ruleset_name, facts in c.get_retract_facts().items():
                            if len(facts) == 1:
                                binding, replies = self._host.start_retract_fact(ruleset_name, facts[0])
                            else:
                                binding, replies = self._host.start_retract_facts(ruleset_name, facts)
                           
                            if binding in pending:
                                pending[binding] = pending[binding] + replies
                            else:
                                pending[binding] = replies
                        
                        for ruleset_name, facts in c.get_facts().items():
                            if len(facts) == 1:
                                binding, replies = self._host.start_assert_fact(ruleset_name, facts[0])
                            else:
                                binding, replies = self._host.start_assert_facts(ruleset_name, facts)
                            
                            if binding in pending:
                                pending[binding] = pending[binding] + replies
                            else:
                                pending[binding] = replies

                        for ruleset_name, messages in c.get_messages().items():
                            if len(messages) == 1:
                                binding, replies = self._host.start_post(ruleset_name, messages[0])
                            else:
                                binding, replies = self._host.start_post_batch(ruleset_name, messages)
                            
                            if binding in pending:
                                pending[binding] = pending[binding] + replies
                            else:
                                pending[binding] = replies

                        binding, replies = rules.start_update_state(self._handle, c._handle, json.dumps(c.s._d, ensure_ascii=False))
                        if binding in pending:
                            pending[binding] = pending[binding] + replies
                        else:
                            pending[binding] = replies
                        
                        for binding, replies in pending.items():
                            if binding != 0:
                                if binding != action_binding:
                                    rules.complete(binding, replies)
                                else:
                                    new_result = rules.complete_and_start_action(self._handle, replies, c._handle)
                                    if new_result:
                                        if 'async' in result_container:
                                            def terminal(e, wait):
                                                return

                                            self.dispatch(terminal, [state, new_result, action_handle, action_binding])
                                        else:
                                            result_container['message'] = json.loads(new_result)

                    except BaseException as error:
                        t, v, tb = sys.exc_info()
                        print('base exception type {0}, value {1}, traceback {2}'.format(t, str(v), traceback.format_tb(tb)))
                        rules.abandon_action(self._handle, c._handle)
                        complete(error, True)
                    except:
                        print('unknown exception type {0}, value {1}, traceback {2}'.format(t, str(v), traceback.format_tb(tb)))
                        rules.abandon_action(self._handle, c._handle)
                        complete('unknown error', True)

                    if c._is_deleted():
                        try:
                            self.delete_state(c.s.sid)
                        except BaseException as error:
                            complete(error, True)

            if 'async' in result_container:
                del result_container['async']
                
            self._actions[action_name].run(c, action_callback) 
            result_container['async'] = True 
           
        complete(None, False)

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
        start_state = {}
        reflexive_states = {}

        for state_name, state in chart_definition.items():
            qualified_name = state_name
            if parent_name:
                qualified_name = '{0}.{1}'.format(parent_name, state_name)

            start_state[qualified_name] = True

            for trigger_name, trigger in state.items():
                if ('to' in trigger and trigger['to'] == state_name) or 'count' in trigger or 'cap' in trigger:
                    reflexive_states[qualified_name] = True

        for state_name, state in chart_definition.items():
            qualified_name = state_name
            if parent_name:
                qualified_name = '{0}.{1}'.format(parent_name, state_name)

            triggers = {}
            if parent_triggers:
                for parent_trigger_name, trigger in parent_triggers.items():
                    triggers['{0}.{1}'.format(qualified_name, parent_trigger_name)] = trigger 

            for trigger_name, trigger in state.items():
                if trigger_name != '$chart':
                    if ('to' in trigger) and parent_name:
                        trigger['to'] = '{0}.{1}'.format(parent_name, trigger['to'])

                    triggers['{0}.{1}'.format(qualified_name, trigger_name)] = trigger

            if '$chart' in state:
                self._transform(qualified_name, triggers, start_state, state['$chart'], rules)
            else:
                for trigger_name, trigger in triggers.items():
                    rule = {}
                    state_test = {'chart_context': {'$and':[{'label': qualified_name}, {'chart': 1}]}}
                    if 'pri' in trigger:
                        rule['pri'] = trigger['pri']

                    if 'count' in trigger:
                        rule['count'] = trigger['count']

                    if 'cap' in trigger:
                        rule['cap'] = trigger['cap']

                    if 'all' in trigger:
                        rule['all'] = list(trigger['all'])
                        rule['all'].append(state_test)
                    elif 'any' in trigger:
                        rule['all'] = [state_test, {'m$any': trigger['any']}]
                    else:
                        rule['all'] = [state_test]

                    if 'run' in trigger:
                        if isinstance(trigger['run'], str):
                            rule['run'] = Promise(self._host.get_action(trigger['run']))
                        elif isinstance(trigger['run'], Promise):
                            rule['run'] = trigger['run']
                        elif hasattr(trigger['run'], '__call__'):
                            rule['run'] = Promise(trigger['run'])

                    if 'to' in trigger:
                        from_state = None
                        if qualified_name in reflexive_states:
                            from_state = qualified_name

                        to_state = trigger['to']
                        assert_state = False
                        if to_state in reflexive_states:
                            assert_state = True

                        if 'run' in rule:
                            rule['run'].continue_with(To(from_state, to_state, assert_state))
                        else:
                            rule['run'] = To(from_state, to_state, assert_state)

                        if to_state in start_state: 
                            del start_state[to_state]

                        if parent_start_state and to_state in parent_start_state:
                            del parent_start_state[to_state]
                    else:
                        raise Exception('Trigger {0} destination not defined'.format(trigger_name))

                    rules[trigger_name] = rule;
                    
        started = False 
        for state_name in start_state.keys():
            if started:
                raise Exception('Chart {0} has more than one start state {1}'.format(self._name, state_name))

            started = True
            if parent_name:
                rules[parent_name + '$start'] = {'all':[{'chart_context': {'$and': [{'label': parent_name}, {'chart':1}]}}], 'run': To(None, state_name, False)};
            else:
                rules['$start'] = {'all': [{'chart_context': {'$and': [{'$nex': {'running': 1}}, {'$s': 1}]}}], 'run': To(None, state_name, False)};

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
        visited = {}
        reflexive_stages = {}

        for stage_name, stage in chart_definition.items():
            if 'to' in stage:
                if isinstance(stage['to'], str):
                    if stage['to'] == stage_name:
                        reflexive_stages[stage_name] = True
                else:
                    for transition_name, transition in stage['to'].items():
                        if transition_name == stage_name or 'count' in transition or 'cap' in transition:
                            reflexive_stages[stage_name] = True

        for stage_name, stage in chart_definition.items():
            stage_test = {'chart_context': {'$and':[{'label': stage_name}, {'chart':1}]}}
            from_stage = None
            if stage_name in reflexive_stages:
                from_stage = stage_name

            if 'to' in stage:
                if isinstance(stage['to'], str):
                    next_stage = None
                    rule = {'all': [stage_test]}
                    if stage['to'] in chart_definition:
                        next_stage = chart_definition[stage['to']]
                    else:
                        raise Exception('Stage {0} not found'.format(stage['to']))

                    assert_stage = False
                    if stage['to'] in reflexive_stages:
                        assert_stage = True

                    if not 'run' in next_stage:
                        rule['run'] = To(from_stage, stage['to'], assert_stage)
                    else:
                        if isinstance(next_stage['run'], str):
                            rule['run'] = To(from_stage, stage['to'], assert_stage).continue_with(Promise(self._host.get_action(next_stage['run'])))
                        elif isinstance(next_stage['run'], Promise) or hasattr(next_stage['run'], '__call__'):
                            rule['run'] = To(from_stage, stage['to'], assert_stage).continue_with(next_stage['run'])

                    rules['{0}.{1}'.format(stage_name, stage['to'])] = rule
                    visited[stage['to']] = True
                else:
                    for transition_name, transition in stage['to'].items():
                        rule = {}
                        next_stage = None

                        if 'pri' in transition:
                            rule['pri'] = transition['pri']

                        if 'count' in transition:
                            rule['count'] = transition['count']

                        if 'cap' in transition:
                            rule['cap'] = transition['cap']

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

                        assert_stage = False
                        if transition_name in reflexive_stages:
                            assert_stage = True

                        if not 'run' in next_stage:
                            rule['run'] = To(from_stage, transition_name, assert_stage)
                        else:
                            if isinstance(next_stage['run'], str):
                                rule['run'] = To(from_stage, transition_name, assert_stage).continue_with(Promise(self._host.get_action(next_stage['run'])))
                            elif isinstance(next_stage['run'], Promise) or hasattr(next_stage['run'], '__call__'):
                                rule['run'] = To(from_stage, transition_name, assert_stage).continue_with(next_stage['run'])

                        rules['{0}.{1}'.format(stage_name, transition_name)] = rule
                        visited[transition_name] = True

        started = False
        for stage_name, stage in chart_definition.items():
            if not stage_name in visited:
                if started:
                    raise Exception('Chart {0} has more than one start state'.format(self._name))

                rule = {'all': [{'chart_context': {'$and': [{'$nex': {'running': 1}}, {'$s': 1}]}}]}
                if not 'run' in stage:
                    rule['run'] = To(None, stage_name, False)
                else:
                    if isinstance(stage['run'], str):
                        rule['run'] = To(None, stage_name, False).continue_with(Promise(self._host.get_action(stage['run'])))
                    elif isinstance(stage['run'], Promise) or hasattr(stage['run'], '__call__'):
                        rule['run'] = To(None, stage_name, False).continue_with(stage['run'])

                rules['$start.{0}'.format(stage_name)] = rule
                started = True


class Host(object):

    def __init__(self, ruleset_definitions = None, databases = None, state_cache_size = 1024):
        if not databases:
            databases = [{'host': 'localhost', 'port': 6379, 'password': None, 'db': 0}]
            
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

    def delete_state(self, ruleset_name, sid):
        self.get_ruleset(ruleset_name).delete_state(sid)

    def post_batch(self, ruleset_name, messages):
        return self.get_ruleset(ruleset_name).assert_events(messages)

    def start_post_batch(self, ruleset_name, messages):
        return self.get_ruleset(ruleset_name).start_assert_events(messages)

    def post(self, ruleset_name, message):
        if isinstance(message, list):
            return self.post_batch(ruleset_name, message)

        return self.get_ruleset(ruleset_name).assert_event(message)

    def start_post(self, ruleset_name, message):
        if isinstance(message, list):
            return self.start_post_batch(ruleset_name, message)

        return self.get_ruleset(ruleset_name).start_assert_event(message)

    def assert_fact(self, ruleset_name, fact):
        if isinstance(fact, list):
            return self.assert_facts(ruleset_name, fact)

        return self.get_ruleset(ruleset_name).assert_fact(fact)

    def start_assert_fact(self, ruleset_name, fact):
        if isinstance(fact, list):
            return self.start_assert_facts(ruleset_name, fact)

        return self.get_ruleset(ruleset_name).start_assert_fact(fact)

    def assert_facts(self, ruleset_name, facts):
        return self.get_ruleset(ruleset_name).assert_facts(facts)

    def start_assert_facts(self, ruleset_name, facts):
        return self.get_ruleset(ruleset_name).start_assert_facts(facts)

    def retract_fact(self, ruleset_name, fact):
        return self.get_ruleset(ruleset_name).retract_fact(fact)

    def start_retract_fact(self, ruleset_name, fact):
        return self.get_ruleset(ruleset_name).start_retract_fact(fact)

    def retract_facts(self, ruleset_name, facts):
        return self.get_ruleset(ruleset_name).retract_facts(facts)

    def start_retract_facts(self, ruleset_name, facts):
        return self.get_ruleset(ruleset_name).start_retract_facts(facts)

    def patch_state(self, ruleset_name, state):
        return self.get_ruleset(ruleset_name).assert_state(state)

    def renew_action_lease(self, ruleset_name, sid):
        self.get_ruleset(ruleset_name).renew_action_lease(sid)

    def register_rulesets(self, parent_name, ruleset_definitions):
        rulesets = Ruleset.create_rulesets(parent_name, self, ruleset_definitions, self._state_cache_size)
        for ruleset_name, ruleset in rulesets.items():
            if ruleset_name in self._ruleset_directory:
                raise Exception('Ruleset with name {0} already registered'.format(ruleset_name))
            else:    
                self._ruleset_directory[ruleset_name] = ruleset
                self._ruleset_list.append(ruleset)
                ruleset.bind(self._databases)

        return list(rulesets.keys())

    def run(self):
        def dispatch_ruleset(index, wait):
            def callback(e, w):
                inner_wait = wait
                if e:
                    if str(e).find('306') == -1:
                        print('Exiting {0}'.format(str(e)))
                        os._exit(1)
                elif not w:
                    inner_wait = False

                if (index == (len(self._ruleset_list) -1)) and inner_wait:
                    self._d_timer = threading.Timer(0.25, dispatch_ruleset, ((index + 1) % len(self._ruleset_list), inner_wait, ))
                    self._d_timer.daemon = True
                    self._d_timer.start()
                else:
                    self._d_timer = threading.Thread(target = dispatch_ruleset, args = ((index + 1) % len(self._ruleset_list), inner_wait, ))
                    self._d_timer.daemon = True
                    self._d_timer.start()

            if not len(self._ruleset_list):
                self._d_timer = threading.Timer(0.5, dispatch_ruleset, (0, False, ))
                self._d_timer.daemon = True
                self._d_timer.start()
            else: 
                ruleset = self._ruleset_list[index]
                if not index:
                    wait = True

                ruleset.dispatch(callback)

        def dispatch_timers(index, wait):
            def callback(e, w):
                inner_wait = wait
                if e:
                    print('Error {0}'.format(str(e)))
                elif not w:
                    inner_wait = False

                if (index == (len(self._ruleset_list) -1)) and inner_wait:
                    self._t_timer = threading.Timer(0.25, dispatch_timers, ((index + 1) % len(self._ruleset_list), inner_wait, ))
                    self._t_timer.daemon = True
                    self._t_timer.start()
                else:
                    self._t_timer = threading.Thread(target = dispatch_timers, args = ((index + 1) % len(self._ruleset_list), inner_wait, ))
                    self._t_timer.daemon = True
                    self._t_timer.start()



            if not len(self._ruleset_list):
                self._t_timer = threading.Timer(0.5, dispatch_timers, (0, False, ))
                self._t_timer.daemon = True
                self._t_timer.start()
            else: 
                ruleset = self._ruleset_list[index]
                if not index:
                    wait = True

                ruleset.dispatch_timers(callback)


        self._d_timer = threading.Timer(0.1, dispatch_ruleset, (0, False, ))
        self._d_timer.daemon = True
        self._d_timer.start()
        self._t_timer = threading.Timer(0.1, dispatch_timers, (0, False, ))
        self._t_timer.daemon = True
        self._t_timer.start()


class Queue(object):

    def __init__(self, ruleset_name, database = None, state_cache_size = 1024):
        if not database:
            database = {'host': 'localhost', 'port': 6379, 'password':None, 'db': 0}

        self._ruleset_name = ruleset_name
        self._handle = rules.create_client(state_cache_size, ruleset_name)
        if isinstance(database, str):
            rules.bind_ruleset(0, 0, database, None, self._handle)
        else:
            if not 'password' in database:
                database['password'] = None

            if not 'db' in database:
                database['db'] = 0

            rules.bind_ruleset(database['port'], database['db'], database['host'], database['password'], self._handle)
        
    def isClosed(self):
        return self._handle == 0

    def post(self, message):
        if self._handle == 0:
            raise Exception('Queue has already been closed')

        if 'sid' in message:
            rules.queue_assert_event(self._handle, str(message['sid']), self._ruleset_name, json.dumps(message, ensure_ascii=False))
        else:
            rules.queue_assert_event(self._handle, None, self._ruleset_name, json.dumps(message, ensure_ascii=False))

    def assert_fact(self, message):
        if self._handle == 0:
            raise Exception('Queue has already been closed')

        if 'sid' in message:
            rules.queue_assert_fact(self._handle, str(message['sid']), self._ruleset_name, json.dumps(message, ensure_ascii=False))
        else: 
            rules.queue_assert_fact(self._handle, None, self._ruleset_name, json.dumps(message, ensure_ascii=False))

    def retract_fact(self, message):
        if self._handle == 0:
            raise Exception('Queue has already been closed')

        if 'sid' in message:
            rules.queue_retract_fact(self._handle, str(message['sid']), self._ruleset_name, json.dumps(message, ensure_ascii=False))
        else:
            rules.queue_retract_fact(self._handle, None, self._ruleset_name, json.dumps(message, ensure_ascii=False))

    def close(self):
        if self._handle != 0:
            rules.delete_client(self._handle)
            self._handle = 0

