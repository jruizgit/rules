import json
import copy
import durable_rules_engine
import threading
import inspect
import random
import time
import datetime
import os
import sys
import traceback

from . import logger

def _unix_now():
    dt = datetime.datetime.now()
    epoch = datetime.datetime.utcfromtimestamp(0)
    delta = dt - epoch
    return delta.total_seconds()

class MessageNotHandledException(Exception):

    def __init__(self, message):
        self.message = 'Could not handle message: {0}'.format(json.dumps(message, ensure_ascii=False))

class MessageObservedException(Exception):

    def __init__(self, message):
        self.message = 'Message has already been observed: {0}'.format(json.dumps(message, ensure_ascii=False))

class Closure(object):

    def __init__(self, host, ruleset, state, message, handle):
        self.host = host
        self.s = Content(state)
        self._handle = handle
        self._ruleset = ruleset
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

    def post(self, ruleset_name, message = None):
        if message: 
            if not 'sid' in message:
                message['sid'] = self.s.sid

            if isinstance(message, Content):
                message = message._d

            self.host.assert_event(ruleset_name, message) 
        else:
            message = ruleset_name 
            if not 'sid' in message:
                message['sid'] = self.s.sid

            if isinstance(message, Content):
                message = message._d

            self._ruleset.assert_event(message)


    def assert_fact(self, ruleset_name, fact = None):
        if fact: 
            if not 'sid' in fact:
                fact['sid'] = self.s.sid

            if isinstance(fact, Content):
                fact = fact._d

            self.host.assert_fact(ruleset_name, fact) 
        else:
            fact = ruleset_name 
            if not 'sid' in fact:
                fact['sid'] = self.s.sid

            if isinstance(fact, Content):
                fact = fact._d

            self._ruleset.assert_fact(fact)



    def retract_fact(self, ruleset_name, fact = None):
        if fact: 
            if not 'sid' in fact:
                fact['sid'] = self.s.sid

            if isinstance(fact, Content):
                fact = fact._d

            self.host.retract_fact(ruleset_name, fact) 
        else:
            fact = ruleset_name 
            if not 'sid' in fact:
                fact['sid'] = self.s.sid

            if isinstance(fact, Content):
                fact = fact._d

            self._ruleset.retract_fact(fact)

    def start_timer(self, timer_name, duration, manual_reset = False):
        self._ruleset.start_timer(self.s.sid, timer_name, duration, manual_reset)
        
    def cancel_timer(self, timer_name):
        self._ruleset.cancel_timer(self.s.sid, timer_name)
        
    def renew_action_lease(self):
        if _unix_now() - self._start_time < 10:
            self._start_time = _unix_now()
            self._ruleset.renew_action_lease(self.s.sid) 

    def delete_state(self):
        self._deleted = True

    def get_facts(self):
        return self._ruleset.get_facts(self.s.sid)

    def get_pending_events(self):
        return self._ruleset.get_pending_events(self.s.sid)

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
                c.s.exception = 'exception caught {0}, traceback {1}'.format(str(error), traceback.format_tb(tb))
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
                c.s.exception = 'exception caught {0}, traceback {1}'.format(str(error), traceback.format_tb(tb))
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
            try:
                if self._from_state:
                    if c.m and isinstance(c.m, list):
                        c.retract_fact(c.m[0].chart_context)
                    else:
                        c.retract_fact(c.chart_context)

                if self._assert_state:
                    c.assert_fact({ 'label': self._to_state, 'chart': 1 })
                else:
                    c.post({ 'label': self._to_state, 'chart': 1 })
            except MessageNotHandledException:
                pass
            

class Ruleset(object):

    def __init__(self, name, host, ruleset_definition):
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

        self._handle = durable_rules_engine.create_ruleset(name, json.dumps(ruleset_definition, ensure_ascii=False))
        self._definition = ruleset_definition

    def _handle_result(self, result, message):
        if result[0] == 1:
            raise MessageNotHandledException(message)
        elif result[0] == 2:
            raise MessageObservedException(message)
        elif result[0] == 3:
            return 0

        return result[1] 

    def assert_event(self, message):
        return self._handle_result(durable_rules_engine.assert_event(self._handle, json.dumps(message, ensure_ascii=False)), message)

    def assert_events(self, messages):
        return self._handle_result(durable_rules_engine.assert_events(self._handle, json.dumps(messages, ensure_ascii=False)), messages)

    def assert_fact(self, fact):
        return self._handle_result(durable_rules_engine.assert_fact(self._handle, json.dumps(fact, ensure_ascii=False)), fact)

    def assert_facts(self, facts):
        return self._handle_result(durable_rules_engine.assert_facts(self._handle, json.dumps(facts, ensure_ascii=False)), facts)

    def retract_fact(self, fact):
        return self._handle_result(durable_rules_engine.retract_fact(self._handle, json.dumps(fact, ensure_ascii=False)), fact)

    def retract_facts(self, facts):
        return self._handle_result(durable_rules_engine.retract_facts(self._handle, json.dumps(facts, ensure_ascii=False)), facts)

    def start_timer(self, sid, timer, timer_duration, manual_reset):
        if sid != None: 
            sid = str(sid)

        durable_rules_engine.start_timer(self._handle, timer_duration, manual_reset, timer, sid)

    def cancel_timer(self, sid, timer_name):
        if sid != None: 
            sid = str(sid)

        durable_rules_engine.cancel_timer(self._handle, sid, timer_name)

    def update_state(self, state):
        state['$s'] = 1
        return durable_rules_engine.update_state(self._handle, json.dumps(state, ensure_ascii=False))

    def get_state(self, sid):
        if sid != None: 
            sid = str(sid)

        return json.loads(durable_rules_engine.get_state(self._handle, sid))

    def delete_state(self, sid):
        if sid != None: 
            sid = str(sid)

        durable_rules_engine.delete_state(self._handle, sid)
    
    def renew_action_lease(self, sid):
        if sid != None: 
            sid = str(sid)

        durable_rules_engine.renew_action_lease(self._handle, sid)

    def get_facts(self, sid):
        if sid != None: 
            sid = str(sid)

        return json.loads(durable_rules_engine.get_facts(self._handle, sid))

    def get_pending_events(self, sid):
        if sid != None: 
            sid = str(sid)

        return json.loads(durable_rules_engine.get_events(self._handle, sid))

    def set_store_message_callback(self, func):
        durable_rules_engine.set_store_message_callback(self._handle, func)

    def set_delete_message_callback(self, func):
        durable_rules_engine.set_delete_message_callback(self._handle, func)

    def set_queue_message_callback(self, func):
        durable_rules_engine.set_queue_message_callback(self._handle, func)
   
    def set_get_stored_messages_callback(self, func):
        durable_rules_engine.set_get_stored_messages_callback(self._handle, func)
      
    def set_get_queued_messages_callback(self, func):
        durable_rules_engine.set_get_queued_messages_callback(self._handle, func)
   
    def complete_get_queued_messages(self, sid, queued_messages):
        if sid != None: 
            sid = str(sid)

        durable_rules_engine.complete_get_queued_messages(self._handle, sid, queued_messages)
   
    def set_get_idle_state_callback(self, func):
        durable_rules_engine.set_get_idle_state_callback(self._handle, func)
               
    def complete_get_idle_state(self, sid, stored_messages):
        if sid != None: 
            sid = str(sid)

        durable_rules_engine.complete_get_idle_state(self._handle, sid, stored_messages)

    def get_definition(self):
        return self._definition

    @staticmethod
    def create_rulesets(host, ruleset_definitions):
        branches = {}
        for name, definition in ruleset_definitions.items():  
            if name.rfind('$state') != -1:
                name = name[:name.rfind('$state')]
                branches[name] = Statechart(name, host, definition)
            elif name.rfind('$flow') != -1:
                name = name[:name.rfind('$flow')]
                branches[name] = Flowchart(name, host, definition)
            else:
                branches[name] = Ruleset(name, host, definition)

        return branches

    def dispatch_timers(self):
        return durable_rules_engine.assert_timers(self._handle)
        
    def _flush_actions(self, state, result_container, state_offset, complete):
        while 'message' in result_container:
            action_name = None
            for action_name, message in result_container['message'].items():
                break

            del(result_container['message'])
            c = Closure(self._host, self, state, message, state_offset)
            
            def action_callback(e):
                if c._has_completed():
                    return

                if e:
                    durable_rules_engine.abandon_action(self._handle, c._handle)
                    complete(e, None)
                else:
                    try:
                        durable_rules_engine.update_state(self._handle, json.dumps(c.s._d, ensure_ascii=False))
                        
                        new_result = durable_rules_engine.complete_and_start_action(self._handle, c._handle)
                        if new_result:
                            result_container['message'] = json.loads(new_result)
                        else:
                            complete(None, state)
                                    
                    except BaseException as error:
                        t, v, tb = sys.exc_info()
                        logger.exception('base exception type %s, value %s, traceback %s', t, str(v), traceback.format_tb(tb))
                        durable_rules_engine.abandon_action(self._handle, c._handle)
                        complete(error, None)
                    except:
                        logger.exception('unknown exception type %s, value %s, traceback %s', t, str(v), traceback.format_tb(tb))
                        durable_rules_engine.abandon_action(self._handle, c._handle)
                        complete('unknown error', None)

                    if c._is_deleted():
                        try:
                            self.delete_state(c.s['sid'])
                        except:
                           pass 
                
            self._actions[action_name].run(c, action_callback) 

    def do_actions(self, state_handle, complete):
        try:
            result = durable_rules_engine.start_action_for_state(self._handle, state_handle)
            if not result:
                complete(None, None)
            else:
                self._flush_actions(json.loads(result[0]), {'message': json.loads(result[1])}, state_handle, complete)
        except BaseException as error:
            complete(error, None)

    def dispatch(self):
        def callback(error, result):
            pass 

        result = durable_rules_engine.start_action(self._handle)
        if result:
            self._flush_actions(json.loads(result[0]), {'message': json.loads(result[1])}, result[2], callback)


class Statechart(Ruleset):

    def __init__(self, name, host, chart_definition):
        self._name = name
        self._host = host
        ruleset_definition = {}
        self._transform(None, None, None, chart_definition, ruleset_definition)
        super(Statechart, self).__init__(name, host, ruleset_definition)
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

    def __init__(self, name, host, chart_definition):
        self._name = name
        self._host = host
        ruleset_definition = {} 
        self._transform(chart_definition, ruleset_definition)
        super(Flowchart, self).__init__(name, host, ruleset_definition)
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

    def __init__(self, ruleset_definitions = None):    
        self._ruleset_directory = {}
        self._ruleset_list = []
        self.store_message_callback = None
        self.delete_message_callback = None
        self.queue_message_callback = None
        self.get_stored_messages_callback = None
        self.get_queued_messages_callback = None
        self.get_idle_state_callback = None

        if ruleset_definitions:
            self.register_rulesets(ruleset_definitions)

        self._run()

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
            self.register_rulesets(ruleset_definition)
            return self._ruleset_directory[ruleset_name]

    def set_rulesets(self, ruleset_definitions):
        self.register_rulesets(ruleset_definitions)
        for ruleset_name, ruleset_definition in ruleset_definitions.items():
            self.save_ruleset(ruleset_name, ruleset_definition)

    def _handle_function(self, rules, func, args, complete):
        error = [0]
        result = [0]
        def callback(e, state):
            error[0] = e
            result[0] = state
            
        if not complete:
            rules.do_actions(func(args), callback)
            if error[0]:
                raise error[0]

            return result[0]
        else:
            try:
                rules.do_actions(func(args), complete)
            except BaseException as e:
                complete(e, None)

    def post(self, ruleset_name, message, complete = None):
        if isinstance(message, list):
            return self.post_batch(ruleset_name, message)

        rules = self.get_ruleset(ruleset_name)
        return self._handle_function(rules, rules.assert_event, message, complete)

    def post_batch(self, ruleset_name, messages, complete = None):
        rules = self.get_ruleset(ruleset_name)
        return self._handle_function(rules, rules.assert_events, messages, complete)

    def assert_fact(self, ruleset_name, fact, complete = None):
        if isinstance(fact, list):
            return self.assert_facts(ruleset_name, fact)

        rules = self.get_ruleset(ruleset_name)
        return self._handle_function(rules, rules.assert_fact, fact, complete)

    def assert_facts(self, ruleset_name, facts, complete = None):
        rules = self.get_ruleset(ruleset_name)
        return self._handle_function(rules, rules.assert_facts, facts, complete)

    def retract_fact(self, ruleset_name, fact, complete = None):
        rules = self.get_ruleset(ruleset_name)
        return self._handle_function(rules, rules.retract_fact, fact, complete)

    def retract_facts(self, ruleset_name, facts, complete = None):
        rules = self.get_ruleset(ruleset_name)
        return self._handle_function(rules, rules.retract_facts, facts, complete)

    def update_state(self, ruleset_name, state, complete = None):
        rules = self.get_ruleset(ruleset_name)
        self._handle_function(rules, rules.update_state, state, complete)

    def get_state(self, ruleset_name, sid = None):
        return self.get_ruleset(ruleset_name).get_state(sid)

    def delete_state(self, ruleset_name, sid = None):
        self.get_ruleset(ruleset_name).delete_state(sid)

    def renew_action_lease(self, ruleset_name, sid = None):
        self.get_ruleset(ruleset_name).renew_action_lease(sid)

    def get_facts(self, ruleset_name, sid = None):
        return self.get_ruleset(ruleset_name).get_facts(sid)

    def get_pending_events(self, ruleset_name, sid = None):
        return self.get_ruleset(ruleset_name).get_pending_events(sid)

    def set_store_message_callback(self, func):
        self.store_message_callback = func

        for ruleset in self._ruleset_list:
            ruleset.set_store_message_callback(func)

    def set_delete_message_callback(self, func):
        self.delete_message_callback = func

        for ruleset in self._ruleset_list:
            ruleset.set_delete_message_callback(func)

    def set_queue_message_callback(self, func):
        self.queue_message_callback = func

        for ruleset in self._ruleset_list:
            ruleset.set_queue_message_callback(func)

    def set_get_queued_messages_callback(self, func):
        self.get_queued_messages_callback = func

        for ruleset in self._ruleset_list:
            ruleset.set_get_queued_messages_callback(func)

    def complete_get_queued_messages(self, ruleset_name, sid, queued_messages):
        self.get_ruleset(ruleset_name).complete_get_queued_messages(sid, queued_messages)

    def set_get_idle_state_callback(self, func):
        self.get_idle_state_callback = func

        for ruleset in self._ruleset_list:
            ruleset.set_get_idle_state_callback(func)

    def complete_get_idle_state(self, ruleset_name, sid, stored_messages):
        self.get_ruleset(ruleset_name).complete_get_idle_state(sid, stored_messages)
        
    def register_rulesets(self, ruleset_definitions):
        rulesets = Ruleset.create_rulesets(self, ruleset_definitions)
        for ruleset_name, ruleset in rulesets.items():
            if ruleset_name in self._ruleset_directory:
                raise Exception('Ruleset with name {0} already registered'.format(ruleset_name))
            else:    
                self._ruleset_directory[ruleset_name] = ruleset
                self._ruleset_list.append(ruleset)

                if self.store_message_callback:
                    ruleset.set_store_message_callback(self.store_message_callback)
                
                if self.delete_message_callback:
                    ruleset.set_delete_message_callback(self.delete_message_callback)

                if self.queue_message_callback:
                    ruleset.set_queue_message_callback(self.queue_message_callback)

                if self.get_stored_messages_callback:
                    ruleset.set_get_stored_messages_callback(self.get_stored_messages_callback)
                    
                if self.get_queued_messages_callback:
                    ruleset.set_get_queued_messages_callback(self.get_queued_messages_callback)
                    
                if self.get_idle_state_callback:
                    ruleset.set_get_idle_state_callback(self.get_idle_state_callback)
                    
        return list(rulesets.keys())

    def _run(self):

        def dispatch_ruleset(index):
            if not len(self._ruleset_list):
                self._d_timer = threading.Timer(0.5, dispatch_ruleset, (0, ))
                self._d_timer.daemon = True
                self._d_timer.start()
            else: 
                ruleset = self._ruleset_list[index]
                try: 
                    ruleset.dispatch()
                except BaseException as e:
                    logger.exception('Error dispatching ruleset')

                timeout = 0
                if (index == (len(self._ruleset_list) -1)):
                    timeout = 0.2

                self._d_timer = threading.Timer(timeout, dispatch_ruleset, ((index + 1) % len(self._ruleset_list), ))
                self._d_timer.daemon = True
                self._d_timer.start()

        def dispatch_timers(index):
            if not len(self._ruleset_list):
                self._t_timer = threading.Timer(0.5, dispatch_timers, (0, ))
                self._t_timer.daemon = True
                self._t_timer.start()
            else: 
                ruleset = self._ruleset_list[index]
                try: 
                    ruleset.dispatch_timers()
                except BaseException as e:
                    logger.exception('Error dispatching timers')

                timeout = 0
                if (index == (len(self._ruleset_list) -1)):
                    timeout = 0.2

                self._t_timer = threading.Timer(timeout, dispatch_timers, ((index + 1) % len(self._ruleset_list), ))
                self._t_timer.daemon = True
                self._t_timer.start()


        self._d_timer = threading.Timer(0.1, dispatch_ruleset, (0, ))
        self._d_timer.daemon = True
        self._d_timer.start()
        self._t_timer = threading.Timer(0.1, dispatch_timers, (0, ))
        self._t_timer.daemon = True
        self._t_timer.start()


