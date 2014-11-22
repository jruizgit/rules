import engine
import interface

class value(object):

    def __init__(self, vtype, left = None, op = None, right = None):
        self.type = vtype
        self.left = left
        self.op = op
        self.right = right

    def __lt__(self, other):
        self.op = '$lt'
        self.right = other
        return self

    def __le__(self, other):
        self.op = '$lte'
        self.right = other
        return self

    def __gt__(self, other):
        self.op = '$gt'
        self.right = other
        return self

    def __ge__(self, other):
        self.op = '$gte'
        self.right = other
        return self

    def __eq__(self, other):
        self.op = '$eq'
        self.right = other
        return self

    def __ne__(self, other):
        self.op = '$neq'
        self.right = other
        return self

    def __neg__(self):
        self.op = '$nex'
        return self

    def __pos__(self):
        self.op = '$ex'
        return self

    def __and__(self, other):
        return value(self.type, self, '$and', other)
    
    def __or__(self, other):
        return value(self.type, self, '$or', other)

    def __getattr__(self, name):
        return value(self.type, name)

    def define(self):
        new_definition = None
        if self.op == '$or' or self.op == '$and':
            definitions = [ self.left.define() ]
            definitions.append(self.right.define())
            new_definition = {self.op: definitions}
        elif self.op == '$nex' or self.op == '$ex':
            new_definition = {self.op: {self.left: 1}}
        elif self.op == '$eq':
            new_definition = {self.left: self.right}
        else:
            new_definition = {self.op: {self.left: self.right}}
        
        if self.type == '$s':
            return {'$s': new_definition}
        else:
            return new_definition


class run(object):

    def __init__(self, func):
        if len(_rule_stack) > 0:
            _rule_stack[-1].func = [func]
        else:
            raise Exception('Invalid rule context')

        self.func = func


class rule(object):

    def __init__(self, operator, multi, *args):
        self.operator = operator
        self.multi = multi
        
        if len(_ruleset_stack) and isinstance(_ruleset_stack[-1], ruleset):
            _ruleset_stack[-1].rules.append(self)
        
        if not len(args):
            raise Exception('Invalid number of arguments')

        if isinstance(args[-1], value):
            self.func = []
        else:
            self.func = args[-1:]
            args = args[:-1]

        if not multi:
            self.expression = args[0]
        else:
            self.expression = args

    def __enter__(self):
        _rule_stack.append(self)

    def __exit__(self, exc_type, exc_value, traceback):
        _rule_stack.pop()

    def __call__(self, *args):
        if (len(args) == 1 and hasattr(args[0], '__call__')):
            self.func = [args[0]]

        return self

    def define(self):
        defined_expression = None
        if not self.multi:
            defined_expression = self.expression.define()
        else:
            index = 0
            defined_expression = {}
            for current_expression in self.expression:
                if current_expression.type == '$s':
                    expression_name = '$s'
                else:    
                    expression_name = 'm_{0}'.format(index)

                defined_expression[expression_name] = current_expression.define()
              
        if len(self.func):
            if len(self.func) == 1 and not hasattr(self.func[0], 'define'):
                return {self.operator: defined_expression, 'run': self.func[0]}
            else:
                ruleset_definitions = {}
                for rset in self.func:
                    ruleset_name, ruleset_definition = rset.define()
                    ruleset_definitions[ruleset_name] = ruleset_definition

                return {self.operator: defined_expression, 'run': ruleset_definitions}   
        else:
            return {self.operator: defined_expression}


class when(rule):

    def __init__(self, *args):
        super(when, self).__init__('when', False, *args)


class when_some(rule):

    def __init__(self, *args):
        super(when_some, self).__init__('whenSome', False, *args)


class when_all(rule):

    def __init__(self, *args):
        super(when_all, self).__init__('whenAll', True, *args)


class when_any(rule):

    def __init__(self, *args):
        super(when_any, self).__init__('whenAny', True, *args)


class when_start(object):

    def __init__(self, func):
        if isinstance(_ruleset_stack[-1], ruleset):
            _ruleset_stack[-1].rules.append(self)
        elif isinstance(_ruleset_stack[-1], statechart):
            _ruleset_stack[-1].states.append(self)
        elif isinstance(_ruleset_stack[-1], flowchart):
            _ruleset_stack[-1].stages.append(self)

        self.func = func

    def __call__(self, *args):
        return self.func(*args)
    

class ruleset(object):
    
    def __init__(self, name):
        self.name = name
        self.rules = []
        if not len(_ruleset_stack):
            _rulesets.append(self)
        elif len(_rule_stack) > 0:
            _rule_stack[-1].func.append(self)
        else:
            raise Exception('Invalid rule context')

    def __enter__(self):
        _ruleset_stack.append(self)

    def __exit__(self, exc_type, exc_value, traceback):
        _ruleset_stack.pop()

    def define(self):
        index = 0
        new_definition = {}
        for rule in self.rules:
            if isinstance(rule, when_start):
                _start_functions.append(rule.func)
            else:
                new_definition['r_{0}'.format(index)] = rule.define()
                index += 1   
        
        return self.name, new_definition
        

class to(object):

    def __init__(self, state_name, func = None):
        if not len(_state_stack):
            raise Exception('Invalid state context')

        if isinstance(_state_stack[-1], state):
            _state_stack[-1].triggers.append(self)
        elif isinstance(_state_stack[-1], stage):
            _state_stack[-1].switches.append(self)
        else:
            raise Exception('Invalid state context')

        self.state_name = state_name
        self.rule = None
        self.func = func

    def when(self, *args):
        self.rule = rule('when', False, *args)
        return self.rule

    def when_some(self, *args):
        self.rule = rule('whenSome', False, *args)
        return self.rule

    def wehn_all(self, *args):
        self.rule = rule('whenAll', True, *args)
        return self.rule

    def when_any(self, *args):
        self.rule = rule('whenAny', True, *args)
        return self.rule 

    def __call__(self, *args):
        if len(args) == 1:
            if isinstance(args[0], rule):
                self.rule = args[0]
            elif hasattr(args[0], '__call__'):
                self.func = args[0]

        return self

    def define(self):
        if self.rule:
            return self.state_name, self.rule.define()
        else:
            if self.func:
                return self.state_name, {'run': self.func}
            else:
                return self.state_name, None


class state(object):
    
    def __init__(self, state_name):
        if not len(_ruleset_stack) or not isinstance(_ruleset_stack[-1], statechart):
            raise Exception('Invalid statechart context')

        _ruleset_stack[-1].states.append(self)
        self.state_name = state_name
        self.triggers = []

    def __enter__(self):
        _state_stack.append(self)

    def __exit__(self, exc_type, exc_value, traceback):
        _state_stack.pop()

    def define(self):
        index = 0
        new_definition = {}
        for trigger in self.triggers:
            trigger_to, trigger_rule = trigger.define()
            if not trigger_rule:
                trigger_rule = {}

            trigger_rule['to'] = trigger_to
            new_definition['t_{0}'.format(index)] = trigger_rule
            index += 1
        
        return self.state_name, new_definition


class statechart(object):
    
    def __init__(self, name):
        self.name = name
        self.states = []
        self.root = True
        if not len(_ruleset_stack):
            _rulesets.append(self)
        elif len(_rule_stack) > 0:
            _rule_stack[-1].func.append(self)
        else:
            raise Exception('Invalid rule context')

    def __enter__(self):
        _ruleset_stack.append(self)

    def __exit__(self, exc_type, exc_value, traceback):
        _ruleset_stack.pop()

    def define(self):
        new_definition = {}
        for state in self.states:
            if isinstance(state, when_start):
                _start_functions.append(state.func)
            else:
                state_name, state_definition = state.define()
                new_definition[state_name] = state_definition

        return '{0}$state'.format(self.name), new_definition


class stage(object):

    def __init__(self, stage_name, func = None):
        if not len(_ruleset_stack) or not isinstance(_ruleset_stack[-1], flowchart):
            raise Exception('Invalid flowchart context')

        _ruleset_stack[-1].stages.append(self)
        self.stage_name = stage_name
        if func:
            self.func = [func]
        else:
            self.func = []
        
        self.switches = []

    def __enter__(self):
        _state_stack.append(self)
        _rule_stack.append(self)

    def __exit__(self, exc_type, exc_value, traceback):
        _state_stack.pop()
        _rule_stack.pop()

    def define(self):
        new_definition = {}
        if len(self.func):
            if len(self.func) == 1 and not hasattr(self.func[0], 'define'):
                new_definition['run'] = self.func[0]
            else:
                ruleset_definitions = {}
                for rset in self.func:
                    ruleset_name, ruleset_definition = rset.define()
                    ruleset_definitions[ruleset_name] = ruleset_definition

                new_definition['run'] = ruleset_definitions

        if (len(self.switches)):
            to = {}
            for switch in self.switches:
                stage_name, switch_definition = switch.define()
                if not switch_definition:
                    to = stage_name
                    break
                elif 'when' in switch_definition:
                    switch_definition = switch_definition['when']
                elif 'whenSome' in switch_definition:
                    switch_definition = {'$some': switch_definition['whenSome']}
                elif 'whenAll' in switch_definition:
                    switch_definition = {'$all': switch_definition['whenAll']}
                elif 'whenAny' in switch_definition:
                    switch_definition = {'$any': switch_definition['whenAny']}

                to[stage_name] = switch_definition

            new_definition['to'] = to

        return self.stage_name, new_definition


class flowchart(object):
    
    def __init__(self, name):
        self.name = name
        self.stages = []
        if not len(_ruleset_stack):
            _rulesets.append(self)
        elif len(_rule_stack) > 0:
            _rule_stack[-1].func.append(self)
        else:
            raise Exception('Invalid rule context')

    def __enter__(self):
        _ruleset_stack.append(self)

    def __exit__(self, exc_type, exc_value, traceback):
        _ruleset_stack.pop()

    def define(self):
        new_definition = {}
        for stage in self.stages:
            if isinstance(stage, when_start):
                _start_functions.append(stage.func)
            else:
                stage_name, stage_definition = stage.define()
                new_definition[stage_name] = stage_definition

        return '{0}$flow'.format(self.name), new_definition

def timeout(name):
    return (value('$m', '$t') == name) 

m = value('$m')
s = value('$s')
_rule_stack = []
_state_stack = []
_ruleset_stack = []
_rulesets = []
_start_functions = []

def run_all(databases = ['/tmp/redis.sock']):
    ruleset_definitions = {}
    for rset in _rulesets:
        ruleset_name, ruleset_definition = rset.define()
        ruleset_definitions[ruleset_name] = ruleset_definition

    main_host = engine.Host(ruleset_definitions, databases)
    for start in _start_functions:
        start(main_host)

    main_app = interface.Application(main_host)
    main_app.run()

