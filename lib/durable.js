exports = module.exports = durable = function () {
    var connect = require('connect');
    var express = require('express');
    var stat = require('node-static');
    var r = require('../build/release/rules.node');

    var session = function (document, output, handle, rulesetName) {
        var that = document;
        var messageRulesets = [];
        var messageDirectory = {};
        var timerNames = [];
        var timerDirectory = {};
        var branchNames = [];
        var branchDirectory = {};

        if (output && output.$m) {
            output = output.$m;
        }

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

        that.getBranchNames = function() {
            return branchNames;
        };

        that.getBranches = function () {
            return branchDirectory;
        };

        that.signal = function (message) {
            var nameIndex = rulesetName.lastIndexOf('.');
            var parentNamespace = rulesetName.slice(0, nameIndex);
            var name = rulesetName.slice(nameIndex + 1);
            message.sid = that.id;
            message.id = name + '.' + message.id;
            that.post(parentNamespace, message);
        };

        that.fork = function (branchName, branchSid, branchDocument) {
            if (branchDirectory[branchName]) {
                throw 'branch with name ' + branchName + ' already forked';
            } else {
                branchNames.push(branchName);
                branchDocument.id = branchSid;
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
        if (func.length <= 1) {
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

        that.run = function (s, complete) {
            // complete should never throw
            if (sync) {
                try {
                    func(s);
                } catch (reason) {
                    complete(reason, s);
                    return;
                }

                if (next) {
                    next.run(s, complete);
                } else {
                    complete(null, s);
                }
            } else {
                try {
                    func(s, function (err) {
                        if (err) {
                            complete(err, s);
                        } else if (next) {
                            next.run(s, complete);
                        } else {
                            complete(null, s);
                        }
                    });
                } catch (reason) {
                    complete(reason, s);
                }
            }
        };

        return that;
    };

    var to = function (state) {
        var execute = function (s) {
            s.label = state;
        };

        var that = promise(execute);
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

        var execute = function (s) {
            for (var i = 0; i < branchNames.length; ++i) {
                s.fork(branchNames[i], s.id, copy(s));
            }
        };

        var that = promise(execute);
        return that;
    };

    var ruleset = function(name, host, rulesetDefinition) {
        var that = {};
        var actions = {};
        var handle;
        
        that.bind = function(databases) {
            for (var i = 0; i < databases.length; ++i) {
                var db = databases[i];
                if (typeof(db) === 'string') {
                    r.bindRuleset(handle, db, 0, null);
                } else {
                    r.bindRuleset(handle, db.host, db.port, db.password);
                }
            }
        };

        that.assertEvent = function(message, complete) {
            try {
                complete(null, r.assertEvent(handle, JSON.stringify(message)));
            } catch(err) {
                complete(err);
            }
        };

        that.assertEvents = function(messages, complete) {
            try {
                complete(null, r.assertEvents(handle, JSON.stringify(messages)));
            } catch(err) {
                complete(err);
            }
        };

        that.assertState = function(state, complete) {
            try {
                complete(null, r.assertState(handle, JSON.stringify(state)));
            } catch(err) {
                complete(err);
            }
        };

        that.getState = function(sid, complete) {
            try {
                complete(null, JSON.parse(r.getState(handle, sid)));
            } catch(err) {
                complete(err);
            }
        }

        var postMessages = function(messages, names, index, s, complete) {
            if (index == names.length) {
                complete(null, s);
            } else {
                var rules = names[index];
                var messageList = messages[rules];
                if (messageList.length == 1) {
                    host.post(rules, messageList[0], function(err) {
                        if (err) {
                            complete(err, s);
                        } else {
                            postMessages(messages, names, ++index, s, complete);
                        }
                    });
                } else {
                    host.postBatch(rules, messageList, function(err) {
                        if (err) {
                            complete(err, s);
                        } else {
                            postMessages(messages, names, ++index, s, complete);
                        }
                    });
                }
            }
        }

        var startBranches = function(branches, names, index, s, complete) {
            if (index == names.length) {
                complete(null, s);
            } else {
                var branchName = names[index];
                var state = branches[branchName];
                host.patchState(branchName, state, function(err) {
                    if (err) {
                        complete(err, s);
                    } else {
                        startBranches(branches, names, ++index, s, complete);
                    }
                });
            }
        }

        var startTimers = function(timers, names, index, s, complete) {
            if (index == names.length) {
                complete(null, s);
            } else {
                var timerName = names[index];
                var duration = timers[timerName];
                try {
                    r.startTimer(handle, s.id, duration, JSON.stringify({ sid: s.id, id:timerName, $t:timerName }));
                    startTimers(timers, names, ++index, s, complete);
                } catch (reason) {
                    complete(reason, s);
                }
            }
        }

        that.dispatchTimers = function(complete) {
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
                var output = JSON.parse(result[2])[name];
                var actionName;
                for (actionName in output) {
                    output = output[actionName];
                    break;
                }

                var s = session(document, output, result[0], name);
                var action = actions[actionName]; 
                action.run(s, function(err, s) {
                    if (err) {
                        try {
                            r.abandonAction(handle, s.getHandle());
                        } catch (reason) {
                            complete(reason);
                            return;
                        }
                        complete(err);
                    } else {
                        var messages = s.getMessages();
                        var rulesets = s.getMessageRulesets();
                        postMessages(messages, rulesets, 0, s, function(err, s) {
                            if (err) {
                                try {
                                    r.abandonAction(handle, s.getHandle());
                                } catch (reason) {
                                    complete(reason);
                                    return;
                                }
                                complete(err);
                            } else {
                                var branches = s.getBranches();
                                var branchNames = s.getBranchNames();
                                startBranches(branches, branchNames, 0, s, function(err, s) {
                                    if (err) {
                                        try {
                                            r.abandonAction(handle, s.getHandle());
                                        } catch (reason) {
                                            complete(reason);
                                            return;
                                        }
                                        complete(err);
                                    } else {
                                        var timers = s.getTimers();
                                        var timerNames = s.getTimerNames();
                                        startTimers(timers, timerNames, 0, s, function(err, s) {
                                            if (err) {
                                                try {
                                                    r.abandonAction(handle, s.getHandle());
                                                } catch (reason) {
                                                    complete(reason);
                                                    return;
                                                }
                                                complete(err);
                                            } else {
                                                try {
                                                    r.completeAction(handle, s.getHandle(), JSON.stringify(s));   
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

        that.toJSON = function() {
            rules.$type = 'ruleset';
            return JSON.stringify(rules);
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

        handle = r.createRuleset(name, JSON.stringify(rulesetDefinition));
        return that;
    }

    var stateChart = function (name, host, chartDefinition) {
        
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
                    if (triggerName !== '$state') {
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
                        var stateTest = { label: qualifiedStateName };
                        if (trigger.when) {
                            if (trigger.when.$s) {
                                rule.when = { $s: { $and: [stateTest, trigger.when.$s]}};
                            } else {
                                rule.whenAll = { $s: stateTest, $m: trigger.when };
                            }
                        } else if (trigger.whenAll) {
                            var test = { $s: stateTest };
                            for (var testName in trigger.whenAll) {
                                if (testName !== '$s') {
                                    test[testName] = trigger.whenAll[testName];
                                } else {
                                    test.$s = { $and: [stateTest, trigger.whenAll.$s]};
                                }
                            }

                            rule.whenAll = test;
                        } else if (trigger.whenAny) {
                            rule.whenAll = { $s: stateTest, m$any: trigger.whenAny };
                        } else if (trigger.whenSome) {
                            rule.whenAll = { $s: stateTest, $m$some: trigger.whenSome };
                        } else {
                            rule.when = { $s: stateTest };
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
                    rules[parentName + '$start'] = { when: { $s: { label: parentName }}, run: to(stateName)};
                } else {
                    rules['$start'] = { when: { $s: { $nex: { label: 1 }} }, run: to(stateName)};
                }

                started = true;
            }

            if (!started) {
                throw 'chart ' + parentName + ' has no start state';
            }
        };

        var rules = {};
        transform(null, null, null, host, chartDefinition, rules);
        var that = ruleset(name, host, rules);
        that.toJSON = function() {
            chartDefinition.$type = 'stateChart';
            return JSON.stringify(chartDefinition);
        };

        return that;
    };

    var flowChart = function (name, host, chartDefinition) {
        
        var transform = function (host, chart, rules) {
            var visited = {};
            var rule;
            var stageName;
            var stage;
            for (stageName in chart) {
                stage = chart[stageName];
                var stageTest = { label: stageName };
                var nextStage;
                if (stage.to) {
                    if (typeof(stage.to) === 'string') {
                        rule = {};
                        rule.when = { $s: stageTest };
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
                            if (transition.$s) {
                                rule.when = { $s: { $and: [ stageTest, transition.$s]}};
                            } else if (transition.$all) {
                                for (var testName in transition.$all) {
                                    var test = { $s: stageTest };
                                    if (testName !== '$s') {
                                        test[testName] = transition.$all[testName];
                                    } else {
                                        test.$s = { $and: [ stageTest, transition.$all.$s ]};
                                    }
                                }

                                rule.whenAll = test;
                            } else if (transition.$any) {
                                rule.whenAll = { $s: stageTest, m$any: transition.$any };
                            } else if (transition.$some) {
                                rule.whenAll = { $s: stageTest, $m$some: transition.$some };
                            } else {
                                rule.whenAll = { $s: stageTest, $m: transition };
                            }

                            nextStage = chart[transitionName];
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
                        throw 'chart ' + namespace + ' has more than one start state';
                    }

                    stage = chart[stageName];
                    if (!stage.run) {
                        rule = { when: { $s: { $nex: { label: 1 }} }, run: to(stageName) };
                    } else {
                        if (typeof(stage.run) === 'string') {
                            rule = { when: { $s: { $nex: { label: 1 }} }, run: to(stageName).continueWith(host.getAction(stage.run))};
                        } else if (typeof(stage.run) === 'function' || stage.run.continueWith) {
                            rule = { when: { $s: { $nex: { label: 1 }} }, run: to(stageName).continueWith(stage.run)};
                        } else {
                            rule = { when: { $s: { $nex: { label: 1 }} } };
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
        var that = ruleset(name, host, rules);
        that.toJSON = function() {
            chartDefinition.$type = 'flowChart';
            return JSON.stringify(chartDefinition);
        };

        return that;
    };

    var createRulesets = function (parentName, host, rulesetDefinitions) {
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

                branches[name] = stateChart(name, host, currentDefinition);
            } else {
                index = name.indexOf('$flow');
                if (index !== -1) {
                    name = name.slice(0, index);
                    if (parentName) {
                        name = parentName + '.' + name;
                    }

                    branches[name] = flowChart(name, host, currentDefinition);
                } else {
                    if (parentName) {
                        name = parentName + '.' + name;
                    }

                    branches[name] = ruleset(name, host, currentDefinition);
                }
            }
        }

        return branches;
    };

    var host = function (databases) {
        var that = {};
        var rulesDirectory = {};
        var instanceDirectory = {};
        var rulesList = [];
        databases = databases || ['/tmp/redis.sock'];
        
        that.getAction = function (actionName) {
            throw 'No getAction implementation provided';
        }

        that.loadRuleset = function(rulesetName, complete) {
            complete('Ruleset ' + rulesetName + ' not found');
        }

        that.getState = function (rulesetName, sid, complete) {
            that.getRuleset(rulesetName, function(err, rules) {
                if (err) {
                    complete(err);
                } else {
                    rules.getState(sid, complete);
                }
            });
        };

        that.patchState = function (rulesetName, state, complete) {
            that.getRuleset(rulesetName, function(err, rules) {
                if (err) {
                    complete(err);
                } else {
                    rules.assertState(state, complete);
                }
            });
        };

        that.getRuleset = function (rulesetName, complete) {
            var rules = rulesDirectory[rulesetName];
            if (rules) {
                complete(null, rules);
            } else {   
                that.loadRuleset(rulesetName, function(err, result) {
                    if (err) {
                        complete(err);
                    } else {
                        try {
                            that.registerRulesets(result);
                            rules = rulesDirectory[rulesetName];
                            complete(null, rules);
                        } catch (err) {
                            complete(err);
                        }
                    }
                });
            }
        };

        that.registerRulesets = function (parentName, rulesetDefinitions) {
            var rulesets = createRulesets(parentName, that, rulesetDefinitions);
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

        that.postBatch = function (rulesetName, messages, complete) {
            that.getRuleset(rulesetName, function(err, rules) {
                if (err) {
                    complete(err);
                } else {
                    rules.assertEvents(messages, complete);
                }
            }); 
        };

        that.post = function (rulesetName, message, complete) {
            that.getRuleset(rulesetName, function(err, rules) {
                if (err) {
                    complete(err);
                } else {
                    rules.assertEvent(message, complete);
                }
            });
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

    var application = function(host, basePath) {
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

            that.get(basePath + '/:rules/:sid/admin.html', function (request, response) {
                request.addListener('end',function () {
                    fileServer.serveFile('/admin.html', 200, {}, request, response);
                }).resume();
            });

            that.get(basePath + '/:rules/:sid', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                host.getState(request.params.rules,
                    request.params.sid,
                    function (err, result) {
                        if (err) {
                            response.send({ error: err }, 500);
                        }
                        else {
                            response.send(result);
                        }
                    });
            });

            that.post(basePath + '/:rules/:sid', function (request, response) {
                response.contentType = "application/json; charset=utf-8";
                var message = request.body;
                var rules = request.params.rules;
                message.sid = request.params.sid;
                host.post(rules, message, function (err) {
                    if (err)
                        response.send({ error: err }, 500);
                    else
                        response.send();
                });
            });

            that.patch(basePath + '/:rules/:sid', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                var document = request.body;
                var rules = request.params.rules;
                document.id = request.params.sid;
                host.patchState(rules, document, function (err) {
                    if (err) {
                        response.send({ error: err }, 500);
                    } else {
                        response.send();
                    }
                });
            });

            that.get(basePath + '/:rules', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                var rules = request.params.rules;
                host.getRuleset(rules, function (err, result) {
                    if (err) {
                        response.send({ error: err }, 500);
                    }
                    else {
                        response.send(result);
                    }
                });
            });


            that.listen(port, function () {
                console.log('Listening on ' + port);
            });    
        };

        return that;
    };

    var run = function(rulesetDefinitions, basePath, databases, start) {
        var rulesHost = host(databases);
        rulesHost.registerRulesets(null, rulesetDefinitions);
        
        var app = application(rulesHost, basePath);
        if (start) {
            start(rulesHost, app);
        }

        app.run();
    }

    return {
        promise: promise,
        run: run,
        host: host,
        application: application
    };
}();