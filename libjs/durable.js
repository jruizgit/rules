exports = module.exports = durableEngine = function () {
    var d = require('./durableEngine');
    var r = require('../build/release/rules.node');

    var m = r.createProxy(
        function(name) {
            return term('$m', name);
        },
        function(name, value) {
            return;
        }
    );

    var s = r.createProxy(
        function(name) {
            return term('$s', name);
        },
        function(name, value) {
            return;
        }
    );

    var argsToArray = function(args, array) {
        array = array || [];
        for (var i = 0; i < args.length; ++i) {
            array.push(args[i]);
        }

        return array;
    } 

    var exp = function (op, terms) {
        var that = {};
        
        that.and = function () {
            if (op === '$and') {
                argsToArray(arguments, terms);
            } else {
                terms.push(and.apply(this, arguments));
            }
        }

        that.or = function()  {
            if (op === '$or') {
                argsToArray(arguments, terms);
            } else {
                terms.push(or.apply(this, arguments));
            }
        }

        that.define = function() {
            var definitions = [];
            for (var i = 0; i < terms.length; ++ i) {
                definitions.push(terms[i].define());
            }

            var newDefinition = {};
            newDefinition[op] = definitions;
            return newDefinition;
        }

        return that;
    };

    var and = function () {
        that = exp('$and', argsToArray(arguments));
        return that;
    };

    var or = function () {
        that = exp('$or', argsToArray(arguments));
        return that;
    };

    var term = function (type, left) {
        var that = {};
        var op;
        var right;
        var sid;
        var time;
        var atLeast;
        var atMost;

        that.gt = function (rvalue) {
            op = '$gt';
            right = rvalue;
            return that;
        };

        that.gte = function (rvalue) {
            op = '$gte';
            right = rvalue;
            return that;
        };

        that.lt = function (rvalue) {
            op = '$lt';
            right = rvalue;
            return that;
        };

        that.lte = function (rvalue) {
            op = '$lte';
            right = rvalue;
            return that;
        };

        that.eq = function (rvalue) {
            op = '$eq';
            right = rvalue;
            return that;
        };

        that.neq = function (rvalue) {
            op = '$neq';
            right = rvalue;
            return that;
        };

        that.ex = function () {
            op = '$ex';
            return that;
        };

        that.nex = function () {
            op = '$nex';
            return that;
        };

        that.id = function (refid) {
            sid = refid;
            return that;
        };

        that.time = function (reftime) {
            time = reftime;
            return that;
        };

        that.atLeast = function (count) {
            atLeast = count;
            return that;
        };    

        that.atMost = function (count) {
            atMost = count;
            return that;
        };

        that.and = function () {
            var terms = [that];
            argsToArray(arguments, terms);
            return and.apply(this, terms);
        };

        that.or = function () {
            var terms = [that];
            argsToArray(arguments, terms);
            return or.apply(this, terms);
        };

        that.define = function () {
            var newDefinition = {};
            if (!op) {
                if (sid && time) {
                    newDefinition[type] = {name: left, id: sid, time: time};
                } else if (sid) {
                    newDefinition[type] = {name: left, id: sid};
                } else if (time) {
                    newDefinition[type] = {name: left, time: time};
                } else {
                    newDefinition[type] = left;
                }
                return newDefinition;
            } else {
                var rightDefinition = right;
                if (typeof(right) === 'function') {
                    rightDefinition = right.define();
                }

                if (op === '$eq') {
                    newDefinition[left] = rightDefinition;
                } else {
                    var innerDefinition = {};
                    innerDefinition[left] = right;
                    newDefinition[op] = innerDefinition;
                }

                if (atLeast) {
                    newDefinition['atLeast'] = atLeast;
                }

                if (atMost) {
                    newDefinition['atMost'] = atMost;
                }

                return type === '$s' ? {$s: newDefinition}: newDefinition;
            }
        };

        return that;
    };

    var rule = function(op, exp) {
        var that = {};

        that.define = function(name) {
            var newDefinition = {};
            var func;
            if (typeof(exp[exp.length - 1]) === 'function') {
                func = exp.pop();
            }

            if (!op) {
                if (name === undefined) {
                    newDefinition = {when: exp[0].define('m_1')};
                } else {
                    newDefinition = exp[0].define('m_1');
                }
            }
            else {
                var innerDefinition = {};
                for (var i = 0; i < exp.length; ++i) {
                    var expDefinition = exp[i].define('m_' + i);
                    if (expDefinition['$s']) {
                        innerDefinition['$s'] = expDefinition['$s'];
                    } else {
                        innerDefinition['m_' + i] = exp[i].define('m_' + i);
                    }
                }
                
                if (name !== undefined) {
                    newDefinition[name + op] = innerDefinition;
                } else {
                    newDefinition[op] = innerDefinition;
                }   
            }

            if (func) {
                newDefinition['run'] = func;
            }   

            return newDefinition;
        };

        return that;
    }

    var ruleset = function (name) {
        var that = {};
        var rules = [];
        var startFunc;

        that.getName = function () {
            return name;
        }

        that.getStart = function () {
            return startFunc;
        }  

        that.when = function () {
            rules.push(rule(null, argsToArray(arguments)));
        };

        that.whenAll = function () {
            rules.push(rule('whenAll', argsToArray(arguments)));
        };

        that.whenAny = function () {
            rules.push(rule('whenAny', argsToArray(arguments)));
        };

        that.whenStart = function (func) {
            startFunc = func;
        };

        that.define = function () {
            var newDefinition = {};
            for (var i = 0; i < rules.length; ++ i) {
                newDefinition['r_' + i] = rules[i].define();
            }

            return newDefinition;
        }

        that.m = m;
        that.s = s;
        rulesets.push(that);
        return that;
    };

    var to = function (stateName, flow) {
        var that = {};
        var condition;

        that.getName = function() {
            return stateName;
        }

        that.when = function () {
            condition = rule(null, argsToArray(arguments));
        };

        that.whenAll = function () {
            if (flow) {
                condition = rule('$all', argsToArray(arguments));
            } else {
                condition = rule('whenAll', argsToArray(arguments));
            }
        };

        that.whenAny = function () {
            if (flow) {
                condition = rule('$any', argsToArray(arguments));
            } else {
                condition = rule('whenAny', argsToArray(arguments));
            }
        };

        that.define = function(name)  {
            var newDefinition = condition.define(name);
            if (!flow) {
                newDefinition['to'] = stateName;
            }
            return newDefinition;
        }

        return that;
    };

    var state = function (name) {
        var that = {};
        var triggers = [];

        that.getName = function() {
            return name;
        }

        that.to = function (stateName) {
            var trigger = to(stateName);
            triggers.push(trigger);
            return trigger;
        };

        that.define = function() {
            var newDefinition = {};
            for (var i = 0; i < triggers.length; ++i) {
                newDefinition['t_' + i] = triggers[i].define();
            }

            return newDefinition;
        }

        that.m = m;
        that.s = s;
        return that;
    };

    var statechart = function (name) {
        var that = {};
        var states = [];
        var startFunc;

        that.getName = function () {
            return name + '$state';
        } 

        that.getStart = function () {
            return startFunc;
        }

        that.state = function (name) {
            var s = state(name);
            states.push(s);
            return s;
        };

        that.whenStart = function (func) {
            startFunc = func;
        };

        that.define = function() {
            var newDefinition = {};
            for (var i = 0; i < states.length; ++i) {
                var current = states[i];
                newDefinition[states[i].getName()] = states[i].define();
            }

            return newDefinition;
        }

        that.m = m;
        that.s = s;
        rulesets.push(that);
        return that;
    };

    var stage = function (name) {
        var that = {};
        var switches = [];
        var runFunc;

        that.getName = function() {
            return name;
        }

        that.run = function (func) {
            runFunc = func;
        };

        that.to = function (stageName) {
            var sw = to(stageName, true);
            switches.push(sw);
            return sw;
        };

        that.define = function () {
            var newDefinition = {};
            if (runFunc) {
                newDefinition['run'] = runFunc;
            }

            var switchDefinition = {};
            for (var i = 0; i < switches.length; ++i) {
                switchDefinition[switches[i].getName()] = switches[i].define('');
            }

            newDefinition['to'] = switchDefinition;
            return newDefinition;
        }

        that.m = m;
        that.s = s;
        return that;
    };

    var flowchart = function (name) {
        var that = {};
        var stages = [];
        var startFunc;

        that.getName = function () {
            return name + '$flow';
        } 

        that.getStart = function () {
            return startFunc;
        }

        that.stage = function (name) {
            var s = stage(name);
            stages.push(s);
            return s;
        };

        that.whenStart = function (func) {
            startFunc = func;
        };

        that.define = function() {
            var newDefinition = {};
            for (var i = 0; i < stages.length; ++ i) {
                newDefinition[stages[i].getName()] = stages[i].define();
            }

            return newDefinition;
        }

        that.m = m;
        that.s = s;
        rulesets.push(that);
        return that;
    };

    var rulesets = [];

    var runAll = function(databases, basePath, stateCacheSize) {
        var definitions = {};
        for (var i = 0; i < rulesets.length; ++ i) {
            definitions[rulesets[i].getName()] = rulesets[i].define();
        }

        var rulesHost = d.host(databases, stateCacheSize);
        rulesHost.registerRulesets(null, definitions);
        var app = d.application(rulesHost, basePath);
        for (var i = 0; i < rulesets.length; ++ i) {
            if (rulesets[i].getStart()) {
                rulesets[i].getStart()(rulesHost);
            }
        }

        app.run(); 
    } 

    var run = function (rulesetDefinitions, basePath, databases, start, stateCacheSize) {
        var rulesHost = d.host(databases, stateCacheSize);
        rulesHost.registerRulesets(null, rulesetDefinitions);
        
        var app = d.application(rulesHost, basePath);
        if (start) {
            start(rulesHost, app);
        }

        app.run();
    }

    return {
        m: m,
        s: s,
        and: and,
        or: or,
        state: state,
        statechart: statechart,
        stage: stage,
        flowchart: flowchart,
        ruleset: ruleset,
        runAll: runAll,
        run: run,
    };
}();