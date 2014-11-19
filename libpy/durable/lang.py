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
        elif self.op == '$eq':
            new_definition = {self.left: self.right}
        else:
            new_definition = {self.op: {self.left: self.right}}
        
        if self.type == '$s':
            return {'$s': new_definition}
        else:
            return new_definition


class rule(object):

    def __init__(self, operator, expression, func, multi = False):
        self.expression = expression
        self.operator = operator
        self.func = func
        self.multi = multi


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
                    
        if self.func:
            return {self.operator: defined_expression, 'run': self.func}
        else:
            return {self.operator: defined_expression}


class when_one(rule):

    def __init__(self, value, func = None):
        super(when_one, self).__init__('when', value, func)


class when_some(rule):

    def __init__(self, value, func = None):
        super(when_some, self).__init__('whenSome', value, func)


class when_all(rule):

    def __init__(self, value, func = None):
        super(when_all, self).__init__('whenAll', value, func, True)


class when_any(rule):

    def __init__(self, value, func = None):
        super(when_any, self).__init__('whenAny', value, func, True)


class when_start(object):

    def __init__(self, func):
        self.func = func

class ruleset(object):
    
    def __init__(self, name, *rules):
        self.name = name
        self.rules = rules
        _ruleset_definitions[name] = self.define()

    def define(self):
        index = 0
        new_definition = {}
        for rule in self.rules:
            if isinstance(rule, when_start):
                _start_functions.append(rule.func)
            else:
                new_definition['r_{0}'.format(index)] = rule.define()
                index += 1   
        
        return new_definition
        

class to(object):

    def __init__(self, state_name):
        self.state_name = state_name

    def when_one(self, value, func = None):
        self.rule = when_one(value, func)
        return self

    def wehn_all(self, value, func = None):
        self.rule = when_all(value, func)
        return self

    def when_any(self, value, func = None):
        self.rule = when_any(value, func)
        return self 

    def when_some(self, value, func = None):
        self.rule = when_some(value, func)
        return self

    def define(self):
        return self.state_name, self.rule.define()


class state(object):
    
    def __init__(self, state_name, *triggers):
        self.state_name = state_name
        self.triggers = triggers

    def define(self):
        index = 0
        new_definition = {}
        for trigger in self.triggers:
            trigger_to, trigger_rule = trigger.define()
            trigger_rule['to'] = trigger_to
            new_definition['t_{0}'.format(index)] = trigger_rule
            index += 1
        
        return self.state_name, new_definition


class statechart(object):
    
    def __init__(self, name, *states):
        self.name = name
        self.states = states
        ruleset_name, ruleset_definition = self.define()
        _ruleset_definitions[ruleset_name] = ruleset_definition

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

    def __init__(self, stage_name, func, *switches):
        self.stage_name = stage_name
        self.func = func
        self.switches = switches

    def define(self):
        new_definition = {}
        if self.func:
            new_definition['run'] = self.func

        if (len(self.switches)):
            to = {}
            for switch in self.switches:
                stage_name, switch_definition = switch.define()
                if 'when' in switch_definition:
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
    
    def __init__(self, name, *stages):
        self.name = name
        self.stages = stages
        ruleset_name, ruleset_definition = self.define()
        _ruleset_definitions[ruleset_name] = ruleset_definition

    def define(self):
        new_definition = {}
        for stage in self.stages:
            if isinstance(stage, when_start):
                _start_functions.append(stage.func)
            else:
                stage_name, stage_definition = stage.define()
                new_definition[stage_name] = stage_definition

        return '{0}$flow'.format(self.name), new_definition

m = value('$m')
s = value('$s')
_ruleset_definitions = {}
_start_functions = []

def run_all(databases = ['/tmp/redis.sock']):
    main_host = engine.Host(_ruleset_definitions, databases)
    for start in _start_functions:
        start(main_host)

    main_app = interface.Application(main_host)
    main_app.run()

