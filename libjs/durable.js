exports = module.exports = durableEngine = function () {
    var d = require('./durableEngine');
    var r = require('../build/release/rules.node');

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
                return that;
            } else {
                var andExp = and.apply(this, arguments);
                terms.push(andExp);
                return andExp;
            }
        };

        that.or = function()  {
            if (op === '$or') {
                argsToArray(arguments, terms);
                return that;
            } else {
                var orExp = or.apply(this, arguments); 
                terms.push(orExp);
                return orExp;
            }
        };

        that.define = function() {
            var definitions = [];
            for (var i = 0; i < terms.length; ++ i) {
                definitions.push(terms[i].define());
            }

            var newDefinition = {};
            newDefinition[op] = definitions;
            return newDefinition;
        };

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
            right  = 1;
            return that;
        };

        that.nex = function () {
            op = '$nex';
            right  = 1;
            return that;
        };

        that.id = function (refid) {
            sid = refid;
            return r.createProxy(
                function(name) {
                    if (name === 'time') {
                        return that.time; 
                    }

                    left = name;
                    return that;
                },
                function(name, value) {
                    return;
                }
            );
        };  

        that.time = function (reftime) {
            time = reftime;
            return r.createProxy(
                function(name) {
                    if (name === 'id') {
                        return that.id; 
                    }

                    left = name;
                    return that;
                },
                function(name, value) {
                    return;
                }
            );
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
                if (typeof(right) === 'object') {
                    rightDefinition = right.define();
                }

                if (op === '$eq') {
                    newDefinition[left] = rightDefinition;
                } else {
                    var innerDefinition = {};
                    innerDefinition[left] = rightDefinition;
                    newDefinition[op] = innerDefinition;
                }

                if (atLeast) {
                    newDefinition['$atLeast'] = atLeast;
                }

                if (atMost) {
                    newDefinition['$atMost'] = atMost;
                }

                return type === '$s' ? {$s: newDefinition}: newDefinition;
            }
        };

        return that;
    };

    var rule = function(op, exp) {
        var that = {};
        var func;
        var rulesetArray = [];

        that.ruleset = function(name) {
            var newRuleset = ruleset(name);
            rulesets.pop();
            rulesetArray.push(newRuleset);
            return newRuleset;
        }

        that.statechart = function(name) {
            var newRuleset = statechart(name);
            rulesets.pop();
            rulesetArray.push(newRuleset);
            return newRuleset;
        }

        that.flowchart = function(name) {
            var newRuleset = flowchart(name);
            rulesets.pop();
            rulesetArray.push(newRuleset);
            return newRuleset;
        }

        that.define = function(name) {
            var newDefinition = {};
            var expDefinition;
            var func;

            if (typeof(exp[exp.length - 1]) === 'function') {
                func =  exp.pop();
            } else if (rulesetArray.length) {
                var rulesetDefinitions = {};
                for (var i = 0; i < rulesetArray.length; ++i) {
                    rulesetDefinitions[rulesetArray[i].getName()] = rulesetArray[i].define();
                }

                func =  rulesetDefinitions;
            }

            if (!op) {
                if (name === undefined) {
                    newDefinition = {when: exp[0].define('m_1')};
                } else {
                    newDefinition = exp[0].define('m_1');
                }
                for (var i = 1; i < exp.length; ++i) {
                    expDefinition = exp[i].define();
                    if (expDefinition['$least']) {
                        newDefinition['$atLeast'] = expDefinition['$least'];
                    } else if (expDefinition['$most']) {
                        newDefinition['$atMost'] = expDefinition['$most'];
                    }
                }
            }
            else {
                var innerDefinition = {};
                for (var i = 0; i < exp.length; ++i) {
                    expDefinition = exp[i].define('m_' + i);
                    if (expDefinition['$s']) {
                        innerDefinition['$s'] = expDefinition['$s'];
                    } else if (expDefinition['m_' + i + '$all']) {
                        innerDefinition['m_' + i + '$all'] = expDefinition['m_' + i + '$all'];
                    } else if (expDefinition['m_' + i + '$any']) {
                        innerDefinition['m_' + i + '$any'] = expDefinition['m_' + i + '$any'];
                    } else if (expDefinition['$least']) {
                        innerDefinition['$atLeast'] = expDefinition['$least'];
                    } else if (expDefinition['$most']) {
                        innerDefinition['$atMost'] = expDefinition['$most'];
                    } else {
                        innerDefinition['m_' + i] = expDefinition;
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
    };

    and = function () {
        var that = exp('$and', argsToArray(arguments));
        return that;
    };

    or = function () {
        var that = exp('$or', argsToArray(arguments));
        return that;
    };

    m = r.createProxy(
            function(name) {
                return term('$m', name);
            },
            function(name, value) {
                return;
            }
        );

    s = r.createProxy(
        function(name) {
            if (name === 'id') {
                return term('$s').id;    
            }

            if (name === 'time') {
                return term('$s').time; 
            }

            return term('$s', name);
        },
        function(name, value) {
            return;
        }
    );


    var extend = function (obj) {        
        obj.and = and;
        obj.or = or;
        obj.m = m;
        obj.s = s;

        obj.all = function() {
            var that = rule('$all', argsToArray(arguments));
            return that;
        };

        obj.any = function() {
            var that = rule('$any', argsToArray(arguments));
            return that;
        };
        
        obj.atLeast = function (count) {
            var that = {};
            that.define = function () {
                return {$least: count};
            }
            return that;
        };

        obj.atMost = function (count) {
            var that = {};
            that.define = function () {
                return {$most: count};
            }
            return that;
        };

        obj.timeout = function(name) {
            return m.$t.eq(name);
        }
    };

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
            var newRule = rule(null, argsToArray(arguments));
            rules.push(newRule);
            return newRule;
        };

        that.whenAll = function () {
            var newRule = rule('whenAll', argsToArray(arguments));
            rules.push(newRule);
            return newRule;
        };

        that.whenAny = function () {
            var newRule = rule('whenAny', argsToArray(arguments));
            rules.push(newRule);
            return newRule;
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

        extend(that);
        rulesets.push(that);
        return that;
    };

    var to = function (stateName, flow) {
        var that = {};
        var condition;

        that.getName = function() {
            return stateName;
        };

        that.when = function () {
            condition = rule(null, argsToArray(arguments));
            return condition;
        };

        that.whenAll = function () {
            if (flow) {
                condition = rule('$all', argsToArray(arguments));
            } else {
                condition = rule('whenAll', argsToArray(arguments));
            }
            return condition;
        };

        that.whenAny = function () {
            if (flow) {
                condition = rule('$any', argsToArray(arguments));
            } else {
                condition = rule('whenAny', argsToArray(arguments));
            }
            return condition;
        };

        that.define = function(name)  {
            if (!condition) {
                if (!flow) {
                    return {to: stateName};
                } else if (typeof(flow) === 'function') {
                    return {to: stateName, run: flow};
                }
                else {
                    return stateName;
                }
            } 

            var newDefinition = condition.define(name);
            if (!flow) {
                newDefinition['to'] = stateName;
            }
            return newDefinition;
        };

        return that;
    };

    var state = function (name) {
        var that = {};
        var triggers = [];
        var states = [];

        that.getName = function() {
            return name;
        };

        that.to = function (stateName, func) {
            var trigger = to(stateName, func);
            triggers.push(trigger);
            return trigger;
        };

        that.state = function(stateName) {
            var newState = state(stateName);
            states.push(newState);
            return newState;
        };

        that.define = function() {
            var newDefinition = {};
            for (var i = 0; i < triggers.length; ++i) {
                newDefinition['t_' + i] = triggers[i].define();
            }

            if (states.length) {
                var chart = {};
                for (var i = 0; i < states.length; ++i) {
                    chart[states[i].getName()] = states[i].define();
                }

                newDefinition['$chart'] = chart;
            }

            return newDefinition;
        };

        extend(that);
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
                newDefinition[states[i].getName()] = states[i].define();
            }

            return newDefinition;
        }

        extend(that);
        rulesets.push(that);
        return that;
    };

    var stage = function (name) {
        var that = {};
        var switches = [];
        var runFunc;
        var rulesetArray = [];

        that.getName = function() {
            return name;
        }

        that.run = function (func) {
            runFunc = func;
        };

        that.ruleset = function(name) {
            var newRuleset = ruleset(name);
            rulesets.pop();
            rulesetArray.push(newRuleset);
            return newRuleset;
        }

        that.statechart = function(name) {
            var newRuleset = statechart(name);
            rulesets.pop();
            rulesetArray.push(newRuleset);
            return newRuleset;
        }

        that.flowchart = function(name) {
            var newRuleset = flowchart(name);
            rulesets.pop();
            rulesetArray.push(newRuleset);
            return newRuleset;
        }

        that.to = function (stageName) {
            var sw = to(stageName, true);
            switches.push(sw);
            return sw;
        };

        that.define = function () {
            var newDefinition = {};
            if (runFunc) {
                newDefinition['run'] = runFunc;
            } else if (rulesetArray.length) {
                var rulesetDefinitions = {};
                for (var i = 0; i < rulesetArray.length; ++i) {
                    rulesetDefinitions[rulesetArray[i].getName()] = rulesetArray[i].define();
                }

                newDefinition['run'] =  rulesetDefinitions;   
            }

            var switchesDefinition = {};
            for (var i = 0; i < switches.length; ++i) {
                var switchDefinition = switches[i].define('');
                if (typeof(switchDefinition) === 'string') {
                    switchesDefinition = switchDefinition;
                    break;
                }

                switchesDefinition[switches[i].getName()] = switches[i].define('');
            }

            newDefinition['to'] = switchesDefinition;
            return newDefinition;
        }

        extend(that);
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

        extend(that);
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
        state: state,
        statechart: statechart,
        stage: stage,
        flowchart: flowchart,
        ruleset: ruleset,
        runAll: runAll,
        run: run,
    };
}();