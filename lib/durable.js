exports = module.exports = durable = function () {
    var express = require('express');
    var stat = require('node-static');
    var r = require('../build/release/rules.node');

    var session = function (document, output, handle, namespace) {
        var that = document;
        var messageDirectory = {};
        var messageRulesets = [];
        var timerDirectory = {};
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

        that.getTimers = function () {
            return timerDirectory;
        };

        that.getBranches = function () {
            return branchDirectory;
        };

        that.signal = function (message) {
            var idIndex = that.id.lastIndexOf('.');
            if (idIndex !== -1) {
                var parentId = that.id.slice(0, idIndex);
                var nameIndex = namespace.lastIndexOf('.');
                var parentNamespace = namespace.slice(0, nameIndex);
                var name = namespace.slice(nameIndex + 1);
                message.sid = parentId;
                that.post(parentNamespace, message);
            }
        };

        that.fork = function (branchName, branchSid, branchDocument) {
            branchSid = that.id + '.' + branchSid;
            branchName = namespace + '.' + branchName;
            if (branchDirectory[branchName]) {
                throw 'branch with name ' + branchName + ' already forked';
            } else {
                branchDocument.id = branchSid;
                branchDirectory[branchName] = branchDocument;
            }
        };

        that.post = function (ruleset, message) {
            var messageList;
            if (messageDirectory[ruleset]) {
                messageList = messageDirectory[ruleset];
            } else {
                messageList = [];
                messageRulesets.push(ruleset);
                messageDirectory[ruleset] = messageList;
            }

            messageList.push(message);
        };

        that.startTimer = function (name, duration) {
            if (timerDirectory[name]) {
                throw 'timer with name ' + name + ' already added';
            } else {
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
                s.fork(branchNames[i], branchNames[i], copy(s));
            }
        };

        var that = promise(execute);
        return that;
    };

    var ruleSet = function(name, host, rules) {
        var that = {};
        var actions = {};
        var handle;
        
        that.bind = function(databases) {
            for (var i = 0; i < databases.length; ++i) {
                r.bindRuleset(handle, databases[i]);
            }
        }

        that.assertEvent = function(message, complete) {
            try {
                complete(null, r.assertEvent(handle, JSON.stringify(message)));
            }
            catch(err) {
                complete(err);
            }
        }

        that.assertEvents = function(messages, complete) {
            try {
                complete(null, r.assertEvents(handle, JSON.stringify(messages)));
            }
            catch(err) {
                complete(err);
            }
        }

        var postMessages = function(messages, names, index, s, complete) {
            if (index == names.length) {
                complete(null, s);
            } else {
                var messageList = messages[names[index]];
                if (messageList.length == 1) {
                    host.post(messageList[0], function(err) {
                        if (err) {
                            complete(err, s);
                        }

                        postMessages(messages, names, ++index, s, complete);
                    });
                } else {
                    host.postBatch(messageList, function(err) {
                        if (err) {
                            complete(err, s);
                        }

                        postMessages(messages, names, ++index, s, complete);
                    });
                }
            }
        }

        that.dispatch = function (complete) {
            var result = r.startAction(handle);
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
                        var names = s.getMessageRulesets();
                        postMessages(messages, names, 0, s, function(err, s) {
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

                                complete();
                            }
                        }); 
                    }
                });
            }
        }

        that.toJSON = function() {
            rules.$type = 'ruleSet';
            return JSON.stringify(rules);
        };

        for (var actionName in rules) {
            var rule = rules[actionName];
            if (typeof(rule.run) === 'function') {
                actions[actionName] = promise(rule.run);
            } else if (rule.run.continueWith) {
                actions[actionName] = rule.run.getRoot();
            } else {
                actions[actionName] = fork(parseDefinitions(rule.run, host, namespace));
            }

            delete(rule.run);
        }

        handle = r.createRuleset(name, JSON.stringify(rules));
        return that;
    }

    var stateChart = function (namespace, host, mainChart) {
        if (!mainChart) {
            throw 'chart argument is not defined';
        }

        var processChart = function (parentName, parentTriggers, parentStartState, chart, rules, host) {
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
                    processChart(qualifiedStateName, triggers, startState, state.$chart, rules);
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
                        } else {
                            rule.when = { $s: stateTest };
                        }

                        if (trigger.run) {
                            if (typeof(trigger.run) === 'function') {
                                rule.run = promise(trigger.run);
                            } else if (trigger.run.continueWith) {
                                rule.run = trigger.run;
                            } else {
                                rule.run = fork(parseDefinitions(trigger.run, namespace), function (pName) {
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
        processChart(null, null, null, mainChart, rules, host);
        var that = ruleSet(namespace, host, rules);
        that.toJSON = function() {
            mainChart.$type = 'stateChart';
            return JSON.stringify(mainChart);
        };

        return that;
    };

    var flowChart = function (namespace, host, mainChart) {
        if (!mainChart) {
            throw 'chart argument is not defined';
        }

        var processChart = function (chart, rules, host) {
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
                            if (typeof(nextStage.run) === 'function' || nextStage.run.continueWith) {
                                rule.run = to(stage.to).continueWith(nextStage.run);
                            } else {
                                rule.run = to(stage.to).continueWith(fork(parseDefinitions(nextStage.run, namespace), function (pName) {
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

                                    rule.whenAll = test;
                                }
                            } else if (transition.$any) {
                                rule.whenAll = { $s: stageTest, m$any: transition.$any };
                            } else {
                                rule.whenAll = { $s: stageTest, $m: transition };
                            }

                            nextStage = chart[transitionName];
                            if (!nextStage.run) {
                                rule.run = to(transitionName);
                            } else {
                                if (typeof(nextStage.run) === 'function' || nextStage.run.continueWith) {
                                    rule.run = to(transitionName).continueWith(nextStage.run);
                                } else {
                                    rule.run = to(transitionName).continueWith(fork(parseDefinitions(nextStage.run, namespace), function (pName) {
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
                        if (typeof(stage.run) === 'function' || stage.run.continueWith) {
                            rule = { when: { $s: { $nex: { label: 1 }} }, run: to(stageName).continueWith(stage.run)};
                        } else {
                            rule = { when: { $s: { $nex: { label: 1 }} } };
                            rule.run = to(stageName).continueWith(fork(parseDefinitions(stage.run, namespace), function (pName) {
                                return (pName !== 'label');
                            }));
                        }
                    } 

                    rules['$start.' + stageName] = rule;
                }
            }
        };

        var rules = {};
        processChart(mainChart, rules, host);
        var that = ruleSet(namespace, host, rules);
        that.toJSON = function() {
            mainChart.$type = 'flowChart';
            return JSON.stringify(mainChart);
        };

        return that;
    };

    var parseDefinitions = function (definitions, host, parentNamespace) {
        var names = [];
        for (var name in definitions) {
            var currentDefinition = definitions[name];
            var rules;
            var index = name.indexOf('$state');
            if (index !== -1) {
                name = name.slice(0, index);
                if (parentNamespace) {
                    name = parentNamespace + '.' + name;
                }

                rules = stateChart(name, host, currentDefinition);
            } else {
                index = name.indexOf('$flow');
                if (index !== -1) {
                    name = name.slice(0, index);
                    if (parentNamespace) {
                        name = parentNamespace + '.' + name;
                    }

                    rules = flowChart(name, host, currentDefinition);
                } else {
                    if (parentNamespace) {
                        name = parentNamespace + '.' + name;
                    }

                    rules = ruleSet(name, host, currentDefinition);
                }
            }

            host.register(name, rules);
            names.push(name);
        }

        return names;
    };

    var host = function () {
        var that = {};
        var rulesDirectory = {};
        var instanceDirectory = {};
        var rulesList = [];
        var app = express.createServer();

        var getStatus = function (rulesName, sid, complete) {
            var rules = rulesDirectory[rulesName];
            if (rules) {
            } else {
                complete('Could not find ' + message.rules);
            }
        };

        var patchSession = function (rulesName, document, complete) {
        };

        var getrules = function (rulesName, complete) {
            var rules = rulesDirectory[rulesName];
            if (rules) {
                complete(null, rules);
            } else {
                complete('Could not find ' + rulesName);
            }
        };

        that.getApp = function() {
            return app;
        };

        that.register = function (rulesName, rules) {
            if (rulesDirectory[rulesName]) {
                throw 'rules ' + rulesName + ' already registered';
            } else {
                rulesDirectory[rulesName] = rules;
                rulesList.push(rules);
            }
        };

        var bind = function (databases) {
            for (var i = 0; i < rulesList.length; ++i) {
                rulesList[i].bind(databases);
            }
        };

        that.postBatch = function (ruleset, messages, complete) {
            var rules = rulesDirectory[ruleset];
            if (rules) {
                rules.assertEvents(messages, complete);
            } else {
                complete('Could not find ' + ruleset);
            }   
        }

        that.post = function (ruleset, message, complete) {
            var rules = rulesDirectory[ruleset];
            if (rules) {
                rules.assertEvent(message, complete);
            } else {
                complete('Could not find ' + ruleset);
            }
        };

        var dispatchrules = function (index) {
            var rules = rulesList[index % rulesList.length];
            rules.dispatch(function (err) {
                if (err) {
                    console.log(err);
                }
                if (index % 10) {
                    dispatchrules(index + 1);
                }
                else {
                    setTimeout(dispatchrules, 1, index + 1);
                }
            });
        };

        that.run = function (basePath, databases, start) {
            var port = 5000;
            var fileServer = new stat.Server(__dirname);
            basePath = basePath || '';
            if (basePath !== '' && basePath.indexOf('/') !== 0) {
                basePath = '/' + basePath;
            }
            if (!databases) {
                databases = [ '/tmp/redis.sock' ];
            }

            bind(databases);
            if (start) {
                start(that);
            }

            app.get('/durableVisual.js', function (request, response) {
                request.addListener('end',function () {
                    fileServer.serveFile('/durableVisual.js', 200, {}, request, response);
                }).resume();
            });

            app.get(basePath + '/:rules/:sid/admin.html', function (request, response) {
                request.addListener('end',function () {
                    fileServer.serveFile('/admin.html', 200, {}, request, response);
                }).resume();
            });

            app.get(basePath + '/:rules/:sid', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                getStatus(request.params.rules,
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

            app.post(basePath + "/:rules/:sid", function (request, response) {
                response.contentType = "application/json; charset=utf-8";
                var message = request.body;
                var rules = request.params.rules;
                message.sid = request.params.sid;
                that.post(rules, message, function (err) {
                    if (err)
                        response.send({ error: err }, 500);
                    else
                        response.send();
                });
            });

            app.patch(basePath + '/:rules/:sid', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                var document = request.body;
                var rules = request.params.rules;
                document.id = request.params.sid;
                patchSession(rules, document, function (err) {
                    if (err) {
                        response.send({ error: err }, 500);
                    } else {
                        response.send();
                    }
                });
            });

            app.get(basePath + '/:rules', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                var rules = request.params.rules;
                getrules(rules, function (err, result) {
                    if (err) {
                        response.send({ error: err }, 500);
                    }
                    else {
                        response.send(result);
                    }
                });
            });

            setTimeout(dispatchrules, 100, 1);
               
            app.listen(port, function () {
                console.log('Listening on ' + port);
            });
        };

        return that;
    };

    function run(definitions, basePath, databases, start) {
        var rulesHost = host();
        parseDefinitions(definitions, rulesHost);
        rulesHost.run(basePath, databases, start);
    }

    return {
        promise: promise,
        run: run
    };
}();