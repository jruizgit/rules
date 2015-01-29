exports = module.exports = durableEngine = function () {
    var connect = require('connect');
    var express = require('express');
    var stat = require('node-static');
    var r = require('../build/release/rules.node');

    var closure = function (document, output, handle, rulesetName) {
        var that = {};
        var messageRulesets = [];
        var messageDirectory = {};
        var timerNames = [];
        var timerDirectory = {};
        var branchNames = [];
        var branchDirectory = {};
        that.s = document;
        that.m = output;

        that.getRulesetName = function () {
            return rulesetName;
        };

        that.getHandle = function () {
            return handle;
        };

        that.getOutput = function () {
            return output;
        };

        that.getMessageRulesets = function () {
            return messageRulesets;
        };

        that.getMessages = function () {
            return messageDirectory;
        };

        that.getTimerNames = function () {
            return timerNames;
        };

        that.getTimers = function () {
            return timerDirectory;
        };

        that.getBranchNames = function () {
            return branchNames;
        };

        that.getBranches = function () {
            return branchDirectory;
        };

        that.signal = function (message) {
            var nameIndex = rulesetName.lastIndexOf('.');
            var parentNamespace = rulesetName.slice(0, nameIndex);
            var name = rulesetName.slice(nameIndex + 1);
            message.sid = that.s.sid;
            message.id = name + '.' + message.id;
            that.post(parentNamespace, message);
        };

        that.fork = function (branchName, branchSid, branchDocument) {
            if (branchDirectory[branchName]) {
                throw 'branch with name ' + branchName + ' already forked';
            } else {
                branchNames.push(branchName);
                branchDocument.sid = branchSid;
                branchDirectory[branchName] = branchDocument;
            }
        };

        that.post = function (rules, message) {
            var messageList;
            if (messageDirectory[rules]) {
                messageList = messageDirectory[rules];
            } else {
                messageList = [];
                messageRulesets.push(rules);
                messageDirectory[rules] = messageList;
            }

            messageList.push(message);
        };

        that.startTimer = function (name, duration) {
            if (timerDirectory[name]) {
                throw 'timer with name ' + name + ' already added';
            } else {
                timerNames.push(name);
                timerDirectory[name] = duration;
            }
        };

        return that;
    };

    var promise = function (func) {
        var that = {};
        var root = that;
        var next;
        var sync;
        if (func.length == 1) {
            sync = true;
        }
        else if (func.length === 2) {
            sync = false;
        }
        else {
            throw 'Invalid function signature';
        }

        that.setRoot = function (rootPromise) {
            root = rootPromise;
        };

        that.getRoot = function () {
            return root;
        };

        that.continueWith = function (nextPromise) {
            if (!nextPromise) {
                throw 'promise or function argument is not defined';
            } else if (typeof(nextPromise) === 'function') {
                next = promise(nextPromise);
            } else {
                next = nextPromise;
            }

            next.setRoot(root);
            return next;
        };

        that.run = function (c, complete) {
            // complete should never throw
            if (sync) {
                try {
                    func(c);
                } catch (reason) {
                    complete(reason, c);
                    return;
                }

                if (next) {
                    next.run(c, complete);
                } else {
                    complete(null, c);
                }
            } else {
                try {
                    func(c, function (err) {
                        if (err) {
                            complete(err, c);
                        } else if (next) {
                            next.run(c, complete);
                        } else {
                            complete(null, c);
                        }
                    });
                } catch (reason) {
                    complete(reason, c);
                }
            }
        };

        return that;
    };

    var to = function (state) {

        var execute = function (c) {
            c.s.label = state;
        };

        var that = promise(execute);
        that.getState = function() {
            return state;
        }

        return that;
    };

    var fork = function (branchNames, filter) {

        var copy = function (object) {
            var newObject = {};
            for (var pName in object) {
                if (!filter || filter(pName)) {
                    var propertyType = typeof(object[pName]);
                    if (propertyType !== 'function') {
                        if (propertyType === 'object') {
                            newObject[pName] = copy(object[pName]);
                        }
                        else {
                            newObject[pName] = object[pName];
                        }
                    }
                }
            }

            return newObject;
        };

        var execute = function (c) {
            for (var i = 0; i < branchNames.length; ++i) {
                c.fork(branchNames[i], c.s.sid, copy(c.s));
            }
        };

        var that = promise(execute);
        return that;
    };

    var ruleset = function (name, host, rulesetDefinition, stateCacheSize) {
        var that = {};
        var actions = {};
        var handle;
        
        that.bind = function (databases) {
            for (var i = 0; i < databases.length; ++i) {
                var db = databases[i];
                if (typeof(db) === 'string') {
                    r.bindRuleset(handle, db, 0, null);
                } else {
                    r.bindRuleset(handle, db.host, db.port, db.password);
                }
            }
        };

        that.assertEvent = function (message, complete) {
            if (complete) {
                try {
                    complete(null, r.assertEvent(handle, JSON.stringify(message)));
                } catch(err) {
                    complete(err);
                }
            } else {
                r.assertEvent(handle, JSON.stringify(message));
            }
        };

        that.assertEvents = function (messages, complete) {
            if (complete) {
                try {
                    complete(null, r.assertEvents(handle, JSON.stringify(messages)));
                } catch(err) {
                    complete(err);
                }
            } else {
                r.assertEvents(handle, JSON.stringify(messages));
            }
        };

        that.assertFact = function (message, complete) {
            if (complete) {
                try {
                    complete(null, r.assertFact(handle, JSON.stringify(message)));
                } catch(err) {
                    complete(err);
                }
            } else {
                r.assertFact(handle, JSON.stringify(message));
            }
        };

         that.retractFact = function (message, complete) {
            if (complete) {
                try {
                    complete(null, r.retractFact(handle, JSON.stringify(message)));
                } catch(err) {
                    complete(err);
                }
            } else {
                r.retractFact(handle, JSON.stringify(message));
            }
        };

        that.assertState = function (state, complete) {
            if (complete) {
                try {
                    complete(null, r.assertState(handle, JSON.stringify(state)));
                } catch(err) {
                    complete(err);
                }
            } else {
                r.assertState(handle, JSON.stringify(state));
            }
        };

        that.startTimer = function (sid, timerName, timerDuration, complete) {
            var timer = {sid: sid, id: Math.random() * 10000000 + 1, $t: timerName};
            if (complete) {
                try {
                    complete(null, r.startTimer(handle, sid, timerDuration, JSON.stringify(timer)));
                } catch(err) {
                    complete(err);
                }
            } else {
                r.startTimer(handle, sid, timerDuration, JSON.stringify(timer));
            }
        };


        that.getState = function (sid, complete) {
            try {
                complete(null, JSON.parse(r.getState(handle, sid)));
            } catch(err) {
                complete(err);
            }
        }

        that.getRulesetState = function (sid, complete) {
            try {
                complete(null, JSON.parse(r.getRulesetState(handle)));
            } catch(err) {
                complete(err);
            }
        }

        var postMessages = function (messages, names, index, c, complete) {
            if (index == names.length) {
                complete(null, c);
            } else {
                var rules = names[index];
                var messageList = messages[rules];
                if (messageList.length == 1) {
                    host.post(rules, messageList[0], function (err) {
                        if (err) {
                            complete(err, c);
                        } else {
                            postMessages(messages, names, ++index, c, complete);
                        }
                    });
                } else {
                    host.postBatch(rules, messageList, function (err) {
                        if (err) {
                            complete(err, c);
                        } else {
                            postMessages(messages, names, ++index, c, complete);
                        }
                    });
                }
            }
        }

        var startBranches = function (branches, names, index, c, complete) {
            if (index == names.length) {
                complete(null, c);
            } else {
                var branchName = names[index];
                var state = branches[branchName];
                host.patchState(branchName, state, function (err) {
                    if (err) {
                        complete(err, c);
                    } else {
                        startBranches(branches, names, ++index, c, complete);
                    }
                });
            }
        }

        var startTimers = function (timers, names, index, c, complete) {
            if (index == names.length) {
                complete(null, c);
            } else {
                var timerName = names[index];
                var duration = timers[timerName];
                that.startTimer(c.s.sid, timerName, duration, function (err) {
                    if (err) {
                        complete(err, c);
                    } else {
                        startTimers(timers, names, ++index, c, complete);
                    }
                });
            }
        }

        that.dispatchTimers = function (complete) {
            try {
                r.assertTimers(handle);
            } catch (reason) {
                complete(reason);
                return;
            }

            complete();
        };

        that.dispatch = function (complete) {
            var result;
            try {
                result = r.startAction(handle);
            } catch (reason) {
                complete(reason);
                return;
            }

            if (!result) {
                complete();
            } 
            else {
                var document = JSON.parse(result[1]);
                var output = JSON.parse(result[2]);
                var actionName;
                for (actionName in output) {
                    output = output[actionName];
                    break;
                }

                var c = closure(document, output, result[0], name);
                var action = actions[actionName]; 
                action.run(c, function (err, c) {
                    if (err) {
                        try {
                            r.abandonAction(handle, c.getHandle());
                        } catch (reason) {
                            complete(reason);
                            return;
                        }
                        complete(err);
                    } else {
                        var messages = c.getMessages();
                        var rulesets = c.getMessageRulesets();
                        postMessages(messages, rulesets, 0, c, function (err, c) {
                            if (err) {
                                try {
                                    r.abandonAction(handle, c.getHandle());
                                } catch (reason) {
                                    complete(reason);
                                    return;
                                }
                                complete(err);
                            } else {
                                var branches = c.getBranches();
                                var branchNames = c.getBranchNames();
                                startBranches(branches, branchNames, 0, c, function (err, c) {
                                    if (err) {
                                        try {
                                            r.abandonAction(handle, c.getHandle());
                                        } catch (reason) {
                                            complete(reason);
                                            return;
                                        }
                                        complete(err);
                                    } else {
                                        var timers = c.getTimers();
                                        var timerNames = c.getTimerNames();
                                        startTimers(timers, timerNames, 0, c, function (err, c) {
                                            if (err) {
                                                try {
                                                    r.abandonAction(handle, c.getHandle());
                                                } catch (reason) {
                                                    complete(reason);
                                                    return;
                                                }
                                                complete(err);
                                            } else {
                                                try {
                                                    r.completeAction(handle, c.getHandle(), JSON.stringify(c.s));   
                                                } catch (reason) {
                                                    complete(reason);
                                                    return;
                                                }
                                            }
                                        });
                                    }
                                });
                                complete();
                            }
                        }); 
                    }
                });
            }
        };

        that.getDefinition = function () {
            return rulesetDefinition;
        };

        for (var actionName in rulesetDefinition) {
            var rule = rulesetDefinition[actionName];
            if (typeof(rule.run) === 'string') {
                actions[actionName] = promise(host.getAction(rule.run));
            } else if (typeof(rule.run) === 'function') {
                actions[actionName] = promise(rule.run);
            } else if (rule.run.continueWith) {
                actions[actionName] = rule.run.getRoot();
            } else {
                actions[actionName] = fork(host.registerRulesets(name, rule.run));
            }

            delete(rule.run);
        }

        handle = r.createRuleset(name, JSON.stringify(rulesetDefinition), stateCacheSize);
        return that;
    }

    var stateChart = function (name, host, chartDefinition, stateCacheSize) {
        
        var transform = function (parentName, parentTriggers, parentStartState, host, chart, rules) {
            var startState = {};
            var state;
            var qualifiedStateName;
            var trigger;
            var triggers;
            var triggerName;
            var stateName;

            for (stateName in chart) {
                qualifiedStateName = stateName;
                if (parentName) {
                    qualifiedStateName = parentName + '.' + stateName;
                }

                startState[qualifiedStateName] = true;
            }

            for (stateName in chart) {
                state = chart[stateName];
                qualifiedStateName = stateName;
                if (parentName) {
                    qualifiedStateName = parentName + '.' + stateName;
                }

                triggers = {};
                if (parentTriggers) {
                    for (var parentTriggerName in parentTriggers) {
                        triggerName = parentTriggerName.substring(parentTriggerName.lastIndexOf('.') + 1);
                        triggers[qualifiedStateName + '.' + triggerName] = parentTriggers[parentTriggerName];
                    }
                }

                for (triggerName in state) {
                    if (triggerName !== '$chart') {
                        trigger = state[triggerName];
                        if (trigger.to && parentName) {
                            trigger.to = parentName + '.' + trigger.to;
                        }

                        triggers[qualifiedStateName + '.' + triggerName] = trigger;
                    }
                }

                if (state.$chart) {
                    transform(qualifiedStateName, triggers, startState, host, state.$chart, rules);
                }
                else {
                    for (triggerName in triggers) {
                        trigger = triggers[triggerName];
                        var rule = {};
                        var stateTest = {$s: {$and:[{label: qualifiedStateName}, {$s:1}]}};
                        if (trigger.pri) {
                            rule.pri = trigger.pri;
                        }

                        if (trigger.count) {
                            rule.count = trigger.count;
                        }

                        if (trigger.all) {
                            rule.all = trigger.all.concat(stateTest);
                        } else if (trigger.any) {
                            rule.all = [stateTest, {m$any: trigger.any}];
                        } else {
                            rule.all = [stateTest];
                        }    

                        if (trigger.run) {
                            if (typeof(trigger.run) === 'string') {
                                rule.run = promise(host.getAction(trigger.run));
                            } else if (typeof(trigger.run) === 'function') {
                                rule.run = promise(trigger.run);
                            } else if (trigger.run.continueWith) {
                                rule.run = trigger.run;
                            } else {
                                rule.run = fork(host.registerRulesets(name, trigger.run), function (pName) {
                                    return (pName !== 'label');
                                });
                            }
                        }

                        if (trigger.to) {
                            if (rule.run) {
                                rule.run.continueWith(to(trigger.to));
                            } else {
                                rule.run = to(trigger.to);
                            }
                            delete(startState[trigger.to]);
                            if (parentStartState) {
                                delete(parentStartState[trigger.to]);
                            }
                        } else {
                            throw 'trigger: ' + triggerName + ' destination not defined';
                        }

                        rules[triggerName] = rule;
                    }
                }
            }

            var started = false;
            for (stateName in startState) {
                if (started) {
                    throw 'chart ' + parentName + ' has more than one start state';
                }

                if (parentName) {
                    rules[parentName + '$start'] = {all:[{$s: {$and: [{label: parentName}, {$s:1}]}}], run: to(stateName)};
                } else {
                    rules['$start'] = {all: [{$s: {$and: [{$nex: {label: 1}}, {$s:1}]}}], run: to(stateName)};
                }

                started = true;
            }

            if (!started) {
                throw 'chart ' + name + ' has no start state';
            }
        };

        var rules = {};
        transform(null, null, null, host, chartDefinition, rules);
        var that = ruleset(name, host, rules, stateCacheSize);
        that.getDefinition = function () {
            chartDefinition.$type = 'stateChart';
            return chartDefinition;
        };

        return that;
    };

    var flowChart = function (name, host, chartDefinition, stateCacheSize) {
        
        var transform = function (host, chart, rules) {
            var visited = {};
            var rule;
            var stageName;
            var stage;
            for (stageName in chart) {
                stage = chart[stageName];
                var stageTest = {$s: {$and:[{label: stageName}, {$s:1}]}};
                var nextStage;
                if (stage.to) {
                    if (typeof(stage.to) === 'string') {
                        rule = {};
                        rule.all = [stageTest];
                        nextStage = chart[stage.to];

                        if (!nextStage.run) {
                            rule.run = to(stage.to);
                        } else {
                            if (typeof(nextStage.run) === 'string') {
                                rule.run = to(stage.to).continueWith(host.getAction(nextStage.run));
                            } else if (typeof(nextStage.run) === 'function' || nextStage.run.continueWith) {
                                rule.run = to(stage.to).continueWith(nextStage.run);
                            } else {
                                rule.run = to(stage.to).continueWith(fork(host.registerRulesets(name, nextStage.run), function (pName) {
                                    return (pName !== 'label');
                                }));
                            }
                        }

                        rules[stageName + '.' + stage.to] = rule;
                        visited[stage.to] = true;
                    } else {
                        for (var transitionName  in stage.to) {
                            var transition = stage.to[transitionName];
                            rule = {};
                            if (transition.pri) {
                                rule.pri = transition.pri;
                            }

                            if (transition.count) {
                                rule.count = transition.count;
                            }

                            if (transition.all) {
                                rule.all = transition.all.concat(stageTest);   
                            } else if (transition.any) {
                                rule.all = [stageTest, {m$any: transition.any}];
                            } else {
                                rule.all = [stageTest];
                            }

                            nextStage = chart[transitionName];
                            if (!nextStage) {
                                throw 'stage ' + transitionName + ' not found'
                            }

                            if (!nextStage.run) {
                                rule.run = to(transitionName);
                            } else {
                                if (typeof(nextStage.run) === 'string') {
                                    rule.run = to(transitionName).continueWith(host.getAction(nextStage.run));
                                } else if (typeof(nextStage.run) === 'function' || nextStage.run.continueWith) {
                                    rule.run = to(transitionName).continueWith(nextStage.run);
                                } else {
                                    rule.run = to(transitionName).continueWith(fork(host.registerRulesets(name, nextStage.run), function (pName) {
                                        return (pName !== 'label');
                                    }));
                                }
                            }

                            rules[stageName + '.' + transitionName] = rule;
                            visited[transitionName] = true;
                        }
                    }
                }
            }

            var started;
            for (stageName in chart) {
                if (!visited[stageName]) {
                    if (started) {
                        throw 'chart ' + name + ' has more than one start state';
                    }

                    stage = chart[stageName];
                    rule = {all: [{$s: {$and: [{$nex: {label:1}}, {$s: 1}]}}]};
                    if (!stage.run) {
                        rule.run = to(stageName);
                    } else {
                        if (typeof(stage.run) === 'string') {
                            rule.run = to(stageName).continueWith(host.getAction(stage.run));
                        } else if (typeof(stage.run) === 'function' || stage.run.continueWith) {
                            rule.run = to(stageName).continueWith(stage.run);
                        } else {
                            rule.run = to(stageName).continueWith(fork(host.registerRulesets(name, stage.run), function (pName) {
                                return (pName !== 'label');
                            }));
                        }
                    } 

                    rules['$start.' + stageName] = rule;
                    started = true;
                }
            }
        };

        var rules = {};
        transform(host, chartDefinition, rules);
        var that = ruleset(name, host, rules, stateCacheSize);

        that.getDefinition = function () {
            chartDefinition.$type = 'flowChart';
            return chartDefinition;
        };

        return that;
    };

    var createRulesets = function (parentName, host, rulesetDefinitions, stateCacheSize) {
        var branches = {};
        for (var name in rulesetDefinitions) {
            var currentDefinition = rulesetDefinitions[name];
            var rules;
            var index = name.indexOf('$state');
            if (index !== -1) {
                name = name.slice(0, index);
                if (parentName) {
                    name = parentName + '.' + name;
                }

                branches[name] = stateChart(name, host, currentDefinition, stateCacheSize);
            } else {
                index = name.indexOf('$flow');
                if (index !== -1) {
                    name = name.slice(0, index);
                    if (parentName) {
                        name = parentName + '.' + name;
                    }

                    branches[name] = flowChart(name, host, currentDefinition, stateCacheSize);
                } else {
                    if (parentName) {
                        name = parentName + '.' + name;
                    }

                    branches[name] = ruleset(name, host, currentDefinition, stateCacheSize);
                }
            }
        }

        return branches;
    };

    var host = function (databases, stateCacheSize) {
        var that = {};
        var rulesDirectory = {};
        var instanceDirectory = {};
        var rulesList = [];
        databases = databases || ['/tmp/redis.sock'];
        stateCacheSize = stateCacheSize || 1024;
        
        that.getAction = function (actionName) {
            throw 'Action ' + actionName + ' not found';
        };

        that.loadRuleset = function (rulesetName, complete) {
            complete('Ruleset ' + rulesetName + ' not found');
        };

        that.saveRuleset = function (rulesetName, rulesetDefinition, complete) {
            complete();
        };

        that.getState = function (rulesetName, sid, complete) {
            var lastError;
            that.getRuleset(rulesetName, function (err, rules) {
                if (err) {
                    if (complete) {
                        complete(err);
                    } else {
                        lastError = err;
                    }
                } else {
                    rules.getState(sid, complete);
                }
            });

            if (lastError) {
                throw lastError;
            }
        };

        that.patchState = function (rulesetName, state, complete) {
            var lastError;
            that.getRuleset(rulesetName, function (err, rules) {
                if (err) {
                    if (complete) {
                        complete(err);
                    } else {
                        lastError = err;
                    }
                } else {
                    rules.assertState(state, complete);
                }
            });

            if (lastError) {
                throw lastError;
            }
        };

        that.getRuleset = function (rulesetName, complete) {
            var rules = rulesDirectory[rulesetName];
            if (rules) {
                complete(null, rules);
            } else {   
                that.loadRuleset(rulesetName, function (err, result) {
                    if (err) {
                        complete(err);
                    } else {
                        try {
                            that.registerRulesets(null, result);
                        } catch (reason) {
                            complete(reason);
                        }
                    }
                });
            }
        };

        that.setRuleset = function (rulesetName, rulesetDefinition, complete) {
            try {
                that.registerRulesets(null, rulesetDefinition);
                that.saveRuleset(rulesetName, rulesetDefinition, complete);
            } catch (reason) {
                complete(reason);
            }
        };

        that.registerRulesets = function (parentName, rulesetDefinitions) {
            var rulesets = createRulesets(parentName, that, rulesetDefinitions, stateCacheSize);
            var names = [];
            for (var rulesetName in rulesets) {
                var rulesetDefinition = rulesets[rulesetName];
                if (rulesDirectory[rulesetName]) {
                    throw 'ruleset ' + rulesetName + ' already registered';
                } else {
                    rulesDirectory[rulesetName] = rulesetDefinition;
                    rulesList.push(rulesetDefinition);
                    rulesetDefinition.bind(databases);
                    names.push(rulesetName);
                }
            }

            return names;
        };

        that.postBatch = function () {
            var rulesetName = arguments[0];
            var messages = [];
            var complete;
            if (arguments[1].constructor === Array) {
                messages = arguments[1];
                if (arguments.length === 3) {
                    complete = arguments[2];
                }
            }
            else {
                for (var i = 1; i < arguments.length; ++ i) {
                    if (typeof(arguments[i]) === 'function') {
                        complete = arguments[i];
                    } else {
                        messages.push(arguments[i]);
                    }
                }
            }

            var lastError;
            that.getRuleset(rulesetName, function (err, rules) {
                if (err) {
                    if (complete) {
                        complete(err);
                    } else {
                        lastError = err;
                    }
                } else {
                    rules.assertEvents(messages, complete);
                }
            });

            if (lastError) {
                throw lastError;
            }
        };

        that.post = function (rulesetName, message, complete) {
            var lastError;
            that.getRuleset(rulesetName, function (err, rules) {
                if (err) {
                    if (complete) {
                        complete(err);
                    } else {
                        lastError = err;
                    }
                } else {
                    rules.assertEvent(message, complete);
                }
            });

            if (lastError) {
                throw lastError;
            }
        };

         that.assert = function (rulesetName, message, complete) {
            var lastError;
            that.getRuleset(rulesetName, function (err, rules) {
                if (err) {
                    if (complete) {
                        complete(err);
                    } else {
                        lastError = err;
                    }
                } else {
                    rules.assertFact(message, complete);
                }
            });

            if (lastError) {
                throw lastError;
            }
        };

        that.retract = function (rulesetName, message, complete) {
            var lastError;
            that.getRuleset(rulesetName, function (err, rules) {
                if (err) {
                    if (complete) {
                        complete(err);
                    } else {
                        lastError = err;
                    }
                } else {
                    rules.retractFact(message, complete);
                }
            });

            if (lastError) {
                throw lastError;
            }
        };

        that.startTimer = function (rulesetName, sid, timerName, timerDuration, complete) {
            var lastError;
            that.getRuleset(rulesetName, function (err, rules) {
                if (err) {
                    if (complete) {
                        complete(err);
                    } else {
                        lastError = err;
                    }
                } else {
                    rules.startTimer(sid, timerName, timerDuration, complete);
                }
            });

            if (lastError) {
                throw lastError;
            }
        };

        var dispatchRules = function (index) {
            if (!rulesList.length) {
                setTimeout(dispatchRules, 100, index);
            } else {
                var rules = rulesList[index % rulesList.length];
                rules.dispatch(function (err) {
                    if (err) {
                        console.log(err);
                    }
                    if (index % 10) {
                        dispatchRules(index + 1);
                    }
                    else {
                        setTimeout(dispatchRules, 1, index + 1);
                    }
                });
            }
        };

        var dispatchTimers = function (index) {
            if (!rulesList.length) {
                setTimeout(dispatchTimers, 100, index);
            } else {
                var rules = rulesList[index % rulesList.length];
                rules.dispatchTimers(function (err) {
                    if (err) {
                        console.log(err);
                    }
                    if (index % 10) {
                        dispatchTimers(index + 1);
                    }
                    else {
                        setTimeout(dispatchTimers, 50, index + 1);
                    }
                });
            }
        };

        setTimeout(dispatchRules, 100, 1);
        setTimeout(dispatchTimers, 100, 1);
        return that;
    }

    var application = function (host, basePath) {
        var that = express();
        var port = 5000;
        var fileServer = new stat.Server(__dirname);
        basePath = basePath || '';
        if (basePath !== '' && basePath.indexOf('/') !== 0) {
            basePath = '/' + basePath;
        }

        that.use(connect.json());           

        that.get('/durableVisual.js', function (request, response) {
            request.addListener('end',function () {
                fileServer.serveFile('/durableVisual.js', 200, {}, request, response);
            }).resume();
        });

        that.run = function () {

            that.get(basePath + '/:rulesetName/:sid/admin.html', function (request, response) {
                request.addListener('end',function () {
                    fileServer.serveFile('/admin.html', 200, {}, request, response);
                }).resume();
            });

            that.get(basePath + '/:rulesetName/:sid', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                host.getState(request.params.rulesetName, request.params.sid, function (err, result) {
                        if (err) {
                            response.send({ error: err }, 404);
                        }
                        else {
                            response.send(result);
                        }
                    });
            });

            that.post(basePath + '/:rulesetName/:sid', function (request, response) {
                response.contentType = "application/json; charset=utf-8";
                var message = request.body;
                message.sid = request.params.sid;
                host.post(request.params.rulesetName, message, function (err) {
                    if (err)
                        response.send({ error: err }, 500);
                    else
                        response.send();
                });
            });

            that.patch(basePath + '/:rulesetName/:sid', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                var document = request.body;
                document.id = request.params.sid;
                host.patchState(request.params.rulesetName, document, function (err) {
                    if (err) {
                        response.send({ error: err }, 500);
                    } else {
                        response.send();
                    }
                });
            });

            that.get(basePath + '/:rulesetName', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                host.getRuleset(request.params.rulesetName, function (err, result) {
                    if (err) {
                        response.send({ error: err }, 404);
                    }
                    else {
                        response.send(result.getDefinition());
                    }
                });
            });

            that.post('/:rulesetName', function (request, response) {
                response.contentType = "application/json; charset=utf-8";
                host.setRuleset(request.params.rulesetName, request.body, function (err, result) {
                    if (err) {
                        response.send({ error: err }, 500);
                    }
                    else {
                        response.send();
                    }
                });
            });

            that.listen(port, function () {
                console.log('Listening on ' + port);
            });    
        };

        return that;
    };

    return {
        promise: promise,
        host: host,
        application: application
    };
}();