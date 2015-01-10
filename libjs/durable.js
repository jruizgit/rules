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

    var lexp = function (op, terms) {
        var that = {};
        var alias;
        
        that.and = function () {
            if (op === '$and') {
                argsToArray(arguments, terms);
                return that;
            } else {
                var newTerms = [that];
                argsToArray(arguments, newTerms);
                return and.apply(this, newTerms);
            }
        };

        that.or = function()  {
            if (op === '$or') {
                argsToArray(arguments, terms);
                return that;
            } else {
                var newTerms = [that];
                argsToArray(arguments, newTerms);
                return or.apply(this, newTerms);
            }
        };

        that.setAlias = function(name) {
            alias = name;
            return that;
        }

        that.define = function(proposedAlias) {
            var definitions = [];
            for (var i = 0; i < terms.length; ++ i) {
                definitions.push(terms[i].define());
            }

            var newDefinition = {};
            newDefinition[op] = definitions;
            var aliasedDefinition = {};
            if (alias) {
                aliasedDefinition[alias] = newDefinition;
            } else if (proposedAlias) {
                aliasedDefinition[proposedAlias] = newDefinition;
            } else {
                aliasedDefinition = newDefinition;
            }
            
            return aliasedDefinition;
        };

        return that;
    };

    var term = function (type, left) {
        var that = {};
        var op;
        var right;
        var alias;

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

        that.setAlias = function(name) {
            alias = name;
            return that;
        }

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

        that.define = function (proposedAlias) {
            var newDefinition = {};
            if (!op) {
                if (sid) {
                    newDefinition[type] = {name: left, id: sid};
                } else {
                    newDefinition[type] = left;
                }
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

                newDefinition = type === '$s' ? {$and: [newDefinition, {$s: 1}]}: newDefinition;
            }

            var aliasedDefinition = {};
            if (alias) {
                aliasedDefinition[alias] = newDefinition;
            } else if (proposedAlias) {
                aliasedDefinition[proposedAlias] = newDefinition;
            } else {
                aliasedDefinition = newDefinition;
            }
            
            return aliasedDefinition;
        };

        return that;
    };

    var aexp = function (op, idioms) {
        var that = {};
        var alias;
        
        that.add = function () {
            if (op === '$add') {
                argsToArray(arguments, idioms);
                return that;
            } else {
                var newIdioms = [that];
                argsToArray(arguments, newIdioms);
                return add.apply(this, newIdioms);
            }
        };

        that.sub = function () {
            if (op === '$sub') {
                argsToArray(arguments, idioms);
                return that;
            } else {
                var newIdioms = [that];
                argsToArray(arguments, newIdioms);
                return sub.apply(this, newIdioms);
            }
        };

        that.mul = function () {
            if (op === '$mul') {
                argsToArray(arguments, idioms);
                return that;
            } else {
                var newIdioms = [that];
                argsToArray(arguments, newIdioms);
                return mul.apply(this, newIdioms);
            }
        };

        that.div = function () {
            if (op === '$div') {
                argsToArray(arguments, idioms);
                return that;
            } else {
                var newIdioms = [that];
                argsToArray(arguments, newIdioms);
                return div.apply(this, newIdioms);
            }
        };

        that.define = function () {
            var currentNode;
            for (var i = idioms.length - 2; i >= 0; -- i) {
                var currentLeft;
                if (idioms[i].define) {
                    currentLeft = idioms[i].define();
                } else {
                    currentLeft = idioms[i];
                }

                var innerNode;
                if (currentNode) {
                    innerNode = {$l: currentLeft, $r: currentNode};
                } else {
                    var currentRight;
                    if (idioms[i + 1].define) {
                        currentRight = idioms[i + 1].define();
                    } else {
                        currentRight =idioms[i + 1];
                    }

                    innerNode = {$l: currentLeft, $r: currentRight};
                }
                currentNode = {};
                currentNode[op] = innerNode;
            }
            return currentNode;
        };

        return that;
    };

    var idiom = function (type) {
        var that = {};
        var op;
        var right = [];
        var left;
        var sid;

        that.add = function () {
            var idioms = [that];
            argsToArray(arguments, idioms);
            return add.apply(this, idioms);
        }

        that.sub = function (rvalue) {
            var idioms = [that];
            argsToArray(arguments, idioms);
            return sub.apply(this, idioms);
        }

        that.mul = function (rvalue) {
            var idioms = [that];
            argsToArray(arguments, idioms);
            return mul.apply(this, idioms);
        }

        that.div = function (rvalue) {
            var idioms = [that];
            argsToArray(arguments, idioms);
            return div.apply(this, idioms);
        }

        that.id = function (refid) {
            sid = refid;
            return r.createProxy(
                function(name) {
                    left = name;
                    return that;
                },
                function(name, value) {
                    return;
                }
            );
        }; 

        that.define = function () {
            var newDefinition;
            if (sid) {
                newDefinition = {name: left, id: sid};
            } else {
                newDefinition = left;
            }
            var leftDefinition = {};
            leftDefinition[type] = newDefinition;
            return leftDefinition;
        }

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
    }

    var dispatcher = function(func) {
        return function(c) {
            delete(c.m['$s']);
            var results = 0;
            for (var name in c.m) {
                c[name] = c.m[name];
                ++results;
            }

            if (results === 1 && c.m['m_0']) {
                c.m = c.m['m_0'];
            }

            return func(c);
        }
    }

    var rule = function(op, lexp) {
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

            if (typeof(lexp[lexp.length - 1]) === 'function') {
                func =  dispatcher(lexp.pop());
            } else if (rulesetArray.length) {
                var rulesetDefinitions = {};
                for (var i = 0; i < rulesetArray.length; ++i) {
                    rulesetDefinitions[rulesetArray[i].getName()] = rulesetArray[i].define();
                }

                func =  rulesetDefinitions;
            }

            var innerDefinition = [];
            var refName;
            var expObject;
            for (var i = 0; i < lexp.length; ++i) {
                refName = 'm_' + i;
                expObject = {};
                expDefinition = lexp[i].define(refName);
                if (expDefinition['count']) {
                    newDefinition['count'] = expDefinition['count'];
                } else if (expDefinition['pri']) {
                    newDefinition['pri'] = expDefinition['pri'];
                } else {
                    if (expDefinition[refName + '$all']) {
                        expObject[refName + '$all'] = expDefinition[refName + '$all'];
                    } else if (expDefinition[refName + '$any']) {
                        expObject[refName + '$any'] = expDefinition[refName + '$any'];
                    } else {
                        expObject = expDefinition;
                    }

                    innerDefinition.push(expObject);
                }
            }
            
            if (name !== undefined) {
                newDefinition[name + op] = innerDefinition;
            } else {
                newDefinition[op] = innerDefinition;
            }   
            
            if (func) {
                newDefinition['run'] = func;
            }   

            return newDefinition;
        };

        return that;
    };

    var and = function () {
        var that = lexp('$and', argsToArray(arguments));
        return that;
    };

    var or = function () {
        var that = lexp('$or', argsToArray(arguments));
        return that;
    };

    var add = function () {
        var that = aexp('$add', argsToArray(arguments));
        return that;
    };

    var sub = function () {
        var that = aexp('$sub', argsToArray(arguments));
        return that;
    };

    var mul = function () {
        var that = aexp('$mul', argsToArray(arguments));
        return that;
    };

    var div = function () {
        var that = aexp('$div', argsToArray(arguments));
        return that;
    };

    var c = r.createProxy(
        function(name) {
            if (name === "s") {
                return idiom("$s");
            }

            return idiom(name);
        },
        function(name, value) {
            value.setAlias(name);
            return value;
        }
    );

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

    var extend = function (obj) {        
        obj.add = add;
        obj.sub = sub;
        obj.mul = mul;
        obj.div = div;
        obj.and = and;
        obj.or = or;
        obj.c = c;
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
        
        obj.pri = function(pri) {
            var that = {};
            that.define = function () {
                return {pri: pri};
            }
            return that;
        };

        obj.count = function(count) {
            var that = {};
            that.define = function () {
                return {count: count};
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

        that.whenAll = function () {
            var newRule = rule('all', argsToArray(arguments));
            rules.push(newRule);
            return newRule;
        };

        that.whenAny = function () {
            var newRule = rule('any', argsToArray(arguments));
            rules.push(newRule);
            return newRule;
        };

        that.timeout = function(name) {
            return m.$t.eq(name);
        }

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

        that.whenAll = function () {
            condition = rule('all', argsToArray(arguments));
            return condition;
        };

        that.whenAny = function () {
            condition = rule('any', argsToArray(arguments));
            return condition;
        };

        that.define = function(name)  {
            if (!condition) {
                if (!flow) {
                    return {to: stateName};
                } else if (typeof(flow) === 'function') {
                    return {to: stateName, run: dispatcher(flow)};
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
                newDefinition['run'] = dispatcher(runFunc);
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