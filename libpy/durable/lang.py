from . import engine
from . import interface

class avalue(object):

    def __init__(self, name, left = None, sid = None, op = None, right = None):
        self._name = name
        self._left = left
        self._sid = sid
        self._op = op
        self._right = right
        
    def __add__(self, other):
        return self._set_right('$add', other)

    def __sub__(self, other):
        return self._set_right('$sub', other)

    def __mul__(self, other):
        return self._set_right('$mul', other)

    def __div__(self, other):
        return self._set_right('$div', other)

    def __truediv__(self, other):
        return self._set_right('$div', other)

    def __lshift__(self, other):
        other.alias = self._name
        return other

    def __getattr__(self, name):
        if self._left:
            self._left = '{0}.{1}'.format(self._left, name)
        else:    
            self._left = name

        return self

    def __lt__(self, other):
        raise Exception('Only binary arithmetic operators supported')

    def __le__(self, other):
        raise Exception('Only binary arithmetic operators supported')

    def __gt__(self, other):
        raise Exception('Only binary arithmetic operators supported')

    def __ge__(self, other):
        raise Exception('Only binary arithmetic operators supported')

    def __eq__(self, other):
        raise Exception('Only binary arithmetic operators supported')

    def __ne__(self, other):
        raise Exception('Only binary arithmetic operators supported')

    def __neg__(self):
        raise Exception('Only binary arithmetic operators supported')

    def __pos__(self):
        raise Exception('Only binary arithmetic operators supported')

    def _set_right(self, op, other):
        if self._right:
            self._left = avalue(self._name, self._left, self._sid, self._op, self._right) 

        self._op = op
        self._right = other
        return self

    def define(self, parent_name = None):
        if not self._left:
            raise Exception('Property name for {0} not defined'.format(self._name))

        if self._name == '$s':
            raise Exception('s not allowed as rvalue')

        if self._name == '$sref':
            self._name = '$s'

        if not self._op:
            if self._sid:
                return {self._name: {'name': self._left, 'id': self._sid}}
            else:
                return {self._name: self._left}

        left_definition = None
        if isinstance(self._left, avalue):
            left_definition = self._left.define()
        else:
            left_definition = {self._name: self._left}
        
        righ_definition = self._right
        if isinstance(self._right, avalue):
            righ_definition = self._right.define()

        return {self._op: {'$l': left_definition, '$r': righ_definition}}
        

class closure(object):

    def __getattr__(self, name):
        return avalue(name)


class value(object):

    def __init__(self, vtype = None, left = None, op = None, right = None, alias = None):
        self.alias = alias
        self._type = vtype
        self._left = left
        self._op = op
        self._right = right
        
    def __lt__(self, other):
        return value(self._type, self._left, '$lt', other, self.alias)
        
    def __le__(self, other):
        return value(self._type, self._left, '$lte', other, self.alias)
        
    def __gt__(self, other):
        return value(self._type, self._left, '$gt', other, self.alias)
    
    def __ge__(self, other):
        return value(self._type, self._left, '$gte', other, self.alias)

    def __eq__(self, other):
        return value(self._type, self._left, '$eq', other, self.alias)

    def __ne__(self, other):
        return value(self._type, self._left, '$neq', other, self.alias)

    def __neg__(self):
        return value(self._type, self._left, '$nex', None, self.alias)

    def __pos__(self):
        return value(self._type, self._left, '$ex', None, self.alias)

    def allItems(self, other):
        return value(self._type, self._left, '$iall', other, self.alias)

    def anyItem(self, other):
        return value(self._type, self._left, '$iany', other, self.alias)

    def matches(self, other):
        return value(self._type, self._left, '$mt', other, self.alias)

    def imatches(self, other):
        return value(self._type, self._left, '$imt', other, self.alias)

    def __and__(self, other):
        return value(self._type, self, '$and', other, self.alias)
    
    def __or__(self, other):
        return value(self._type, self, '$or', other, self.alias)    

    def __add__(self, other):
        return avalue(self._type, self._left, None, '$add', other)
        
    def __sub__(self, other):
        return avalue(self._type, self._left, None, '$sub', other)

    def __mul__(self, other):
        return avalue(self._type, self._left, None, '$mul', other)

    def __div__(self, other):
        return avalue(self._type, self._left, None, '$div', other)

    def __truediv__(self, other):
        return avalue(self._type, self._left, None, '$div', other)

    def __getattr__(self, name):
        if self._type:
            if self._left:
                return value(self._type, '{0}.{1}'.format(self._left, name))
            else:
                return value(self._type, name)
        else:
            return value(self.alias, name)

    def define(self):
        if not self._left:
            raise Exception('Property name for {0} not defined'.format(self._type))

        new_definition = None
        right_definition = self._right
        if isinstance(self._right, value) or isinstance(self._right, avalue):
            right_definition = right_definition.define();

        if self._op == '$or' or self._op == '$and':
            left_definition = self._left.define()
            if not self._op in left_definition:
                left_definition = [left_definition, right_definition]
            else:
                left_definition = left_definition[self._op]
                left_definition.append(right_definition)
            
            new_definition = {self._op: left_definition}
        elif self._op == '$nex' or self._op == '$ex':
            new_definition = {self._op: {self._left: 1}}
        elif self._op == '$eq':
            new_definition = {self._left: right_definition}
        elif not self._op:
            if self._type == '$s':
                raise Exception('s not allowed as rvalue')

            print('defining {0}, {1}'.format(self._type, self._left))
            new_definition = {self._type: self._left}
        else:
            new_definition = {self._op: {self._left: right_definition}}
        
        if self._type == '$s':
            return {'$and': [new_definition, {'$s': 1}]}
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
        self.alias = None
        
        if len(_ruleset_stack) and isinstance(_ruleset_stack[-1], ruleset):
            _ruleset_stack[-1].rules.append(self)
        
        if not len(args):
            raise Exception('Invalid number of arguments')

        self.count = None
        self.pri = None
        self.cap = None
        self.dist = None
        self.func = []
        new_args = []
        for arg in args:
            if isinstance(arg, dict):
                if 'count' in arg:
                    self.count = arg['count']
                elif 'pri' in arg:
                    self.pri = arg['pri']
                elif 'cap' in arg:
                    self.cap = arg['cap']
                elif 'dist' in arg:
                    self.dist = arg['dist']
                else:
                    self.func = arg
            elif isinstance(arg, value) or isinstance(arg, rule):
                new_args.append(arg)
            else:
                self.func = arg
            
        if not multi:
            self.expression = new_args[0]
        else:
            self.expression = new_args

    def __enter__(self):
        _rule_stack.append(self)

    def __exit__(self, exc_type, exc_value, traceback):
        _rule_stack.pop()

    def __call__(self, *args):
        if (len(args) == 1 and hasattr(args[0], '__call__')):
            self.func = [args[0]]

        return self

    def define(self, parent_name = None):
        defined_expression = None
        if not self.multi:
            defined_expression = self.expression.define()
        else:
            index = 0
            defined_expression = []
            for current_expression in self.expression:
                new_expression = None
                name = None
                if current_expression.alias:
                    name = current_expression.alias
                elif len(self.expression) == 1:
                    if parent_name:
                        name = '{0}.m'.format(parent_name)
                    else:
                        name = 'm'
                else:
                    if parent_name:
                        name = '{0}.m_{1}'.format(parent_name, index)
                    else:
                        name = 'm_{0}'.format(index)

                if isinstance(current_expression, all):
                    new_expression = {'{0}$all'.format(name): current_expression.define(name)['all']}
                elif isinstance(current_expression, any):
                    new_expression = {'{0}$any'.format(name): current_expression.define(name)['any']}
                elif isinstance(current_expression, none):
                    new_expression = {'{0}$not'.format(name): current_expression.define()['none'][0]['m']}
                else:    
                    new_expression = {name: current_expression.define()}
                
                defined_expression.append(new_expression)
                index += 1
        
        if len(self.func):
            if len(self.func) == 1 and not hasattr(self.func[0], 'define'):
                defined_expression = {self.operator: defined_expression, 'run': self.func[0]}  
        elif self.operator:
                defined_expression = {self.operator: defined_expression}

        if self.count:
            defined_expression['count'] = self.count

        if self.pri:
            defined_expression['pri'] = self.pri

        if self.cap:
            defined_expression['cap'] = self.cap

        if self.dist == True:
            defined_expression['dist'] = 1

        if self.dist == False:
            defined_expression['dist'] = 0

        return defined_expression

class when_all(rule):

    def __init__(self, *args):
        super(when_all, self).__init__('all', True, *args)


class when_any(rule):

    def __init__(self, *args):
        super(when_any, self).__init__('any', True, *args)


class all(rule):

    def __init__(self, *args):
        _ruleset_stack.append(self)
        super(all, self).__init__('all', True, *args)
        _ruleset_stack.pop()


class any(rule):

    def __init__(self, *args):
        _ruleset_stack.append(self)
        super(any, self).__init__('any', True, *args)
        _ruleset_stack.pop()


class none(rule):

    def __init__(self, *args):
        _ruleset_stack.append(self)
        super(none, self).__init__('none', True, *args)
        _ruleset_stack.pop()


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
        if not len(_ruleset_stack):
            raise Exception('Invalid state context')

        if isinstance(_ruleset_stack[-1], state):
            _ruleset_stack[-1].triggers.append(self)
        elif isinstance(_ruleset_stack[-1], stage):
            _ruleset_stack[-1].switches.append(self)
        else:
            raise Exception('Invalid state context')

        self.state_name = state_name
        self.rule = None
        self.func = func

    def when_all(self, *args):
        self.rule = rule('all', True, *args)
        return self.rule

    def when_any(self, *args):
        self.rule = rule('any', True, *args)
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
        if not len(_ruleset_stack):
            raise Exception('Invalid statechart context')

        if isinstance(_ruleset_stack[-1], state):
            _ruleset_stack[-1].states.append(self)    
        elif isinstance(_ruleset_stack[-1], statechart):
            _ruleset_stack[-1].states.append(self)
        else:
            raise Exception('Invalid statechart context')

        self.state_name = state_name
        self.triggers = []
        self.states = []

    def __enter__(self):
        _ruleset_stack.append(self)

    def __exit__(self, exc_type, exc_value, traceback):
        _ruleset_stack.pop()

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
        
        if len(self.states):
            chart = {}
            for state in self.states:
                state_name, state_definition = state.define()
                chart[state_name] = state_definition

            new_definition['$chart'] = chart

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
        _ruleset_stack.append(self)
        _rule_stack.append(self)

    def __exit__(self, exc_type, exc_value, traceback):
        _ruleset_stack.pop()
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
                elif 'all' in switch_definition:
                    switch_definition = {'all': switch_definition['all']}
                elif 'any' in switch_definition:
                    switch_definition = {'any': switch_definition['any']}

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

def count(value):
    return {'count': value}

def pri(value):
    return {'pri': value}

def cap(value):
    return {'cap': value}

def distinct(value):
    return {'dist': value}

def sref(sid = None):
    return avalue('$sref', None, sid, None, None)

def select(name):
    for rset in _rulesets:
        if rset.name == name:
            return rset

    raise Exception('Ruleset {0} not found'.format(name))

m = value('$m')
s = value('$s')
item = value('$i', '$i')
c = closure()

_rule_stack = []
_ruleset_stack = []
_rulesets = []
_start_functions = []

def create_queue(ruleset_name, database = None, state_cache_size = 1024):
    return engine.Queue(ruleset_name, database, state_cache_size)

def create_host(databases = None, state_cache_size = 1024):
    ruleset_definitions = {}
    for rset in _rulesets:
        ruleset_name, ruleset_definition = rset.define()
        ruleset_definitions[ruleset_name] = ruleset_definition
        
    main_host = engine.Host(ruleset_definitions, databases, state_cache_size)
    for start in _start_functions:
        start(main_host)

    main_host.run()
    return main_host

def run_all(databases = None, host_name = '127.0.0.1', port = 5000, routing_rules = None, run = None, state_cache_size = 1024):
    main_host = create_host(databases, state_cache_size)
    main_app = interface.Application(main_host, host_name, port, routing_rules, run)
    main_app.run()

def run_server(run, databases = None, routing_rules = None, state_cache_size = 1024):
    run_all(databases, None, None, routing_rules, run, state_cache_size)

