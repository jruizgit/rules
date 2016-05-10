exports = module.exports = durableEngine = function () {
    var bodyParser = require('body-parser');
    var express = require('express');
    var stat = require('node-static');
    var r = require('bindings')('rulesjs.node');

    var closureQueue = function () {
        var queuedPosts = [];
        var queuedAsserts = [];
        var queuedRetracts = [];

        that.getQueuedPosts = function () {
            return queuedPosts;
        };

        that.getQueuedAsserts = function () {
            return queuedAsserts;
        };

        that.getQueuedRetracts = function () {
            return queuedRetracts;
        };

        that.post = function (message) {
            message = copy(message);
            queuedPosts.push(message);
        };

        that.assert = function (message) {
            message = copy(message);
            queuedAsserts.push(message);
        };

        that.retract = function (message) {
            message = copy(message);
            queuedRetracts.push(message);
        };
    } 

    var closure = function (host, document, output, handle, rulesetName) {
        var that = {};
        var targetRulesets = {};
        var eventsDirectory = {};
        var queuesDirectory = {};
        var factsDirectory = {};
        var retractDirectory = {};
        var timerDirectory = {};
        var cancelledTimerDirectory = {};
        var branchDirectory = {};
        var startTime = new Date().getTime();
        var completed = false;

        that.s = document;

        if (output.constructor === Array) {
            that.m = output;
        } else {
            for (var name in output) {
                that[name] = output[name];
            }
        }

        that.getRulesetName = function () {
            return rulesetName;
        };

        that.getHandle = function () {
            return handle;
        };

        that.getHost = function() {
            return host;
        };

        that.getTargetRulesets = function () {
            var rulesetNames = [];
            for (var rulesetName in targetRulesets) {
                rulesetNames.push(rulesetName);
            }
            return rulesetNames;
        };

        that.getEvents = function () {
            return eventsDirectory;
        };

        that.getQueues = function () {
            return queuesDirectory;
        };

        that.getFacts = function () {
            return factsDirectory;
        };

        that.getRetract = function () {
            return retractDirectory;
        };

        that.getTimers = function () {
            return timerDirectory;
        };

        that.getCancelledTimers = function () {
            return cancelledTimerDirectory;
        };

        that.getQueue = function (rules) {
            if (!queuesDirectory[rules]) {
                queuesDirectory[rules] = closureQueue();
            }
            
            return queuesDirectory[rules];
        }

        that.post = function (rules, message) {
            if (!message) {
                message = rules;
                rules = rulesetName;
            }

            message = copy(message);

            if (!message.sid) {
                message.sid = that.s.sid;
            }

            var eventsList;
            if (eventsDirectory[rules]) {
                eventsList = eventsDirectory[rules];
            } else {
                eventsList = [];
                targetRulesets[rules] = true;
                eventsDirectory[rules] = eventsList;
            }

            eventsList.push(message);
        };

        that.queuePost = function (rules, message) {
            message = copy(message);
            var eventsList;
            if (queuedEventsDirectory[rules]) {
                eventsList = queuedEventsDirectory[rules];
            } else {
                eventsList = [];
                queuedEventsDirectory[rules] = eventsList;
            }

            eventsList.push(message);
        };

        that.assert = function (rules, fact) {
            if (!fact) {
                fact = rules;
                rules = rulesetName;
            }

            fact = copy(fact);

            if (!fact.sid) {
                fact.sid = that.s.sid;
            }

            var factsList;
            if (factsDirectory[rules]) {
                factsList = factsDirectory[rules];
            } else {
                factsList = [];
                targetRulesets[rules] = true;
                factsDirectory[rules] = factsList;
            }
            factsList.push(fact);
        };

        that.retract = function (rules, fact) {
            if (!fact) {
                fact = rules;
                rules = rulesetName;
            }

            fact = copy(fact);

            if (!fact.sid) {
                fact.sid = that.s.sid;
            }

            var retractList;
            if (retractDirectory[rules]) {
                retractList = retractDirectory[rules];
            } else {
                retractList = [];
                targetRulesets[rules] = true;
                retractDirectory[rules] = retractList;
            }
            retractList.push(fact);
        };

        that.startTimer = function (name, duration, id) {
            if (!id) {
                id = name;
            }

            if (timerDirectory[id]) {
                throw 'timer with id ' + id + ' already added';
            } else {
                timerDirectory[id] = [{sid: that.s.sid, id: id, $t: name}, duration];

            }
        };

        that.cancelTimer = function (name, duration, id) {
            if (!id) {
                id = name;
            }

            if (cancelledTimerDirectory[id]) {
                throw 'timer with id ' + id + ' already cancelled';
            } else {
                cancelledTimerDirectory[id] = {sid: that.s.sid, id: id, $t: name};
            }
        };

        that.renewActionLease = function() {
            if ((new Date().getTime() - startTime) < 10000) {
                startTime = new Date().getTime();
                host.renewActionLease(rulesetName, that.s.sid);
            }
        };

        that.hasCompleted = function () {
            if ((new Date().getTime() - startTime) > 10000) {
                completed = true;
            }

            return completed;
        };

        that.complete = function () {
            completed = true;
        }

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
            var timeoutCallback = function(maxTime) {
                if (new Date().getTime() > maxTime) {
                    c.s.exception = 'timeout expired';
                    complete(null, c)
                } else if (!c.hasCompleted()) {
                    c.renewActionLease();
                    setTimeout(timeoutCallback, 5000, maxTime);
                }
            }

            // complete should never throw
            if (sync) {
                try {
                    func(c);
                } catch (reason) {
                    console.log(reason.stack);
                    c.s.exception = String(reason);
                }

                if (next) {
                    next.run(c, complete);
                } else {
                    complete(null, c);
                }
            } else {
                try {
                    var timeLeft = func(c, function (err) {
                        if (err) {
                            c.s.exception = err;
                            complete(null, c);
                        } else if (next) {
                            next.run(c, complete);
                        } else {
                            complete(null, c);
                        }
                    });

                    if (timeLeft) {
                        setTimeout(timeoutCallback, 5000, new Date().getTime() + timeLeft * 1000);
                    }
                } catch (reason) {
                    console.log(reason.stack);
                    c.s.exception = String(reason);
                    complete(null, c);
                }
            }
        };

        return that;
    };

    var to = function (fromState, toState, assertState) {
        var execute = function (c) {
            c.s.running = true;
            if (fromState !== toState) {
                if (fromState) {
                    if (c.m && c.m.constructor === Array) {
                        c.retract(c.m[0].chart_context);
                    } else {
                        c.retract(c.chart_context);
                    }
                }

                if (assertState) {
                    c.assert({label: toState, chart: 1, id: Math.ceil(Math.random() * 1000000000 + 1)});
                } else {
                    c.post({label: toState, chart: 1, id: Math.ceil(Math.random() * 1000000000 + 1)});
                }
            }
        };

        var that = promise(execute);
        return that;
    };

    var copy = function (object, filter) {
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

        that.startAssertEvent = function (message) {
            return r.startAssertEvent(handle, JSON.stringify(message));
        };

        that.assertEvent = function (message) {
            return r.assertEvent(handle, JSON.stringify(message));
        };

        that.queueAssertEvent = function (sid, rulesetName, message) {
            return r.queueAssertEvent(handle, sid, rulesetName, JSON.stringify(message));
        };

        that.startAssertEvents = function (messages) {
            return r.startAssertEvents(handle, JSON.stringify(messages));
        };

        that.assertEvents = function (messages) {
            return r.assertEvents(handle, JSON.stringify(messages));
        };

        that.startAssertFact = function (fact) {
            return r.startAssertFact(handle, JSON.stringify(fact));
        };

        that.assertFact = function (fact) {
            return r.assertFact(handle, JSON.stringify(fact));
        };

        that.queueAssertFact = function (sid, rulesetName, message) {
            return r.queueAssertFact(handle, sid, rulesetName, JSON.stringify(message));
        };

        that.startAssertFacts = function (facts) {
            return r.startAssertFacts(handle, JSON.stringify(facts));
        };

        that.assertFacts = function (facts) {
            return r.assertFacts(handle, JSON.stringify(facts));
        };

        that.startRetractFact = function (fact) {
            return r.startRetractFact(handle, JSON.stringify(fact));
        };

        that.retractFact = function (fact) {
            return r.retractFact(handle, JSON.stringify(fact));
        };

        that.queueRetractFact = function (sid, rulesetName, message) {
            return r.queueRetractFact(handle, sid, rulesetName, JSON.stringify(message));
        };

        that.startRetractFacts = function (facts) {
            return r.startRetractFacts(handle, JSON.stringify(facts));
        };

        that.retractFacts = function (facts) {
            return r.retractFacts(handle, JSON.stringify(facts));
        };

        that.assertState = function (state) {
            return r.assertState(handle, JSON.stringify(state));
        };

        that.startTimer = function (sid, timer, timerDuration) {
            return r.startTimer(handle, sid, timerDuration, JSON.stringify(timer));
        };

        that.cancelTimer = function (sid, timer) {
            return r.cancelTimer(handle, sid, JSON.stringify(timer));
        };

        that.getState = function (sid) {
            return JSON.parse(r.getState(handle, sid));
        }

        that.renewActionLease = function (sid) {
            return r.renewActionLease(handle, sid);
        }

        that.getRulesetState = function (sid, complete) {
            return JSON.parse(r.getRulesetState(handle));
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

        var ensureRulesets = function (names, index, c, complete) {
            if (index == names.length) {
                complete(null, c);
            } else {
                var rulesetName = names[index];
                host.ensureRuleset(rulesetName, function (err) {
                    if (err) {
                        complete(err, c);
                    } else {
                        ensureRulesets(names, ++index, c, complete);
                    }
                });
            }
        }

        that.dispatch = function (complete, asyncResult) {
            var state = null;
            var actionHandle = null;
            var actionBinding = null;
            var resultContainer = {};
            if (asyncResult) {
                state = asyncResult[0];
                resultContainer = {'message': JSON.parse(asyncResult[1])};
                actionHandle = asyncResult[2];
                actionBinding = asyncResult[3];
            } else {
                try {
                    var result = r.startAction(handle);
                    if (result) {
                        state = JSON.parse(result[0]);
                        resultContainer = {'message': JSON.parse(result[1])};
                        actionHandle = result[2];
                        actionBinding = result[3];
                    }
                } catch (reason) {
                    complete(reason);
                    return;
                }
            }

            while (resultContainer['message']) {
                var actionName = null;
                var message = null;
                for (actionName in resultContainer['message']) {
                    message = resultContainer['message'][actionName];
                    break;
                }
                resultContainer['message'] = null;
                if (resultContainer['async']) {
                    resultContainer['async'] = null;
                }

                var c = closure(host, state, message, actionHandle, name);
                actions[actionName].run(c, function (err, c) {
                    if (err) {
                        r.abandonAction(handle, c.getHandle());
                        complete(err);
                    } else {
                        var rulesetNames = c.getTargetRulesets();
                        ensureRulesets(rulesetNames, 0, c, function(err, c) {
                            if (c.hasCompleted()) {
                                return;
                            } else {
                                c.complete();
                            }

                            if (err) {
                                r.abandonAction(handle, c.getHandle());
                                complete(err);
                                return;
                            }

                            try {
                                var rulesetName;
                                var facts;
                                var bindingReplies;
                                var binding = 0;
                                var replies = 0;
                                var pending = {};

                                var timers = c.getCancelledTimers();
                                for (var timerId in timers) {
                                    that.cancelTimer(c.s.sid, timers[timerId]);
                                }

                                timers = c.getTimers();
                                for (var timerId in timers) {
                                    var timerTuple = timers[timerId];
                                    that.startTimer(c.s.sid, timerTuple[0], timerTuple[1]);
                                }

                                var queues = c.getQueues();
                                for (var targetRuleset in queues) {
                                    var q = queues[targetRuleset];
                                    var messages = q.getQueuedPosts();
                                    for (var i = 0; i < messages.length; ++i) {
                                        that.queueAssertEvent(messages[i].sid, targetRuleset, messages[i]);
                                    }

                                    var messages = q.getQueuedAsserts();
                                    for (var i = 0; i < messages.length; ++i) {
                                        that.queueAssertFact(messages[i].sid, targetRuleset, messages[i]);
                                    }

                                    var messages = q.getQueuedRetracts();
                                    for (var i = 0; i < messages.length; ++i) {
                                        that.queueRetractFact(messages[i].sid, targetRuleset, messages[i]);
                                    }
                                }

                                var retractFacts = c.getRetract();
                                pending[actionBinding] = 0;
                                for (rulesetName in retractFacts) {
                                    facts = retractFacts[rulesetName];
                                    if (facts.length == 1) {
                                        bindingReplies = host.startRetract(rulesetName, facts[0]);
                                    } else {
                                        bindingReplies = host.startRetractFacts(rulesetName, facts);
                                    }
                                    binding = bindingReplies[0];
                                    replies = bindingReplies[1];

                                    if (pending[binding]) {
                                        pending[binding] = pending[binding] + replies;
                                    } else {
                                        pending[binding] = replies;
                                    }
                                }
                                var assertFacts = c.getFacts();
                                for (rulesetName in assertFacts) {
                                    facts = assertFacts[rulesetName];
                                    if (facts.length == 1) {
                                        bindingReplies = host.startAssert(rulesetName, facts[0]);
                                    } else {
                                        bindingReplies = host.startAssertFacts(rulesetName, facts);
                                    }
                                    binding = bindingReplies[0];
                                    replies = bindingReplies[1];

                                    if (pending[binding]) {
                                        pending[binding] = pending[binding] + replies;
                                    } else {
                                        pending[binding] = replies;
                                    }
                                }
                                var postEvents = c.getEvents();
                                for (rulesetName in postEvents) {
                                    var events = postEvents[rulesetName];
                                    if (events.length == 1) {
                                        bindingReplies = host.startPost(rulesetName, events[0]);
                                    } else {
                                        bindingReplies = host.startPostBatch(rulesetName, events);
                                    }
                                    binding = bindingReplies[0];
                                    replies = bindingReplies[1];
                                    if (pending[binding]) {
                                        pending[binding] = pending[binding] + replies;
                                    } else {
                                        pending[binding] = replies;
                                    }
                                }
                                bindingReplies = r.startUpdateState(handle, c.getHandle(), JSON.stringify(c.s));
                                binding = bindingReplies[0];
                                replies = bindingReplies[1];
                                if (pending[binding]) {
                                    pending[binding] = pending[binding] + replies;
                                } else {
                                    pending[binding] = replies;
                                }
                                for (binding in pending) {
                                    replies = pending[binding];
                                    binding = parseInt(binding);
                                    if (binding) {
                                        if (binding != actionBinding) {
                                            r.complete(binding, replies);
                                        } else {
                                            var newResult = r.completeAndStartAction(handle, replies, c.getHandle());
                                            if (newResult) {
                                                if (resultContainer['async']) {
                                                    that.dispatch(function (e) {}, [state, newResult, actionHandle, actionBinding]);
                                                } else {
                                                    resultContainer['message'] = JSON.parse(newResult);
                                                }
                                            }
                                        }
                                    }
                                }
                            } catch (reason) {
                                r.abandonAction(handle, c.getHandle());
                                complete(reason);
                            }
                        });
                    }
                });
                resultContainer['async'] = true;
            }

            complete(null);
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
            }

            delete(rule.run);
        }

        handle = r.createRuleset(name, JSON.stringify(rulesetDefinition), stateCacheSize);
        return that;
    }

    var stateChart = function (name, host, chartDefinition, stateCacheSize) {

        var transform = function (parentName, parentTriggers, parentStartState, host, chart, rules) {
            var startState = {};
            var reflexiveStates = {};
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
                state = chart[stateName];
                for (triggerName in state) {
                    trigger = state[triggerName];
                    if ((trigger.to && trigger.to === stateName) || trigger.count || trigger.cap || trigger.span) {
                        reflexiveStates[qualifiedStateName] = true;
                    }

                }
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
                        triggers[qualifiedStateName + '.' + parentTriggerName] = parentTriggers[parentTriggerName];
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
                        var stateTest = {chart_context: {$and:[{label: qualifiedStateName}, {chart: 1}]}};
                        if (trigger.pri) {
                            rule.pri = trigger.pri;
                        }

                        if (trigger.count) {
                            rule.count = trigger.count;
                        }

                        if (trigger.span) {
                            rule.span = trigger.span;
                        }

                        if (trigger.cap) {
                            rule.cap = trigger.cap;
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
                            }
                        }

                        if (trigger.to) {
                            var fromState = null;
                            if (reflexiveStates[qualifiedStateName]) {
                                fromState = qualifiedStateName;
                            }

                            var assertState = false;
                            if (reflexiveStates[trigger.to]) {
                                assertState = true;
                            }

                            if (rule.run) {
                                rule.run.continueWith(to(fromState, trigger.to, assertState));
                            } else {
                                rule.run = to(fromState, trigger.to, assertState);
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
                    rules[parentName + '$start'] = {all: [{chart_context: {$and: [{label: parentName}, {chart:1}]}}], run: to(null, stateName, false)};
                } else {
                    rules['$start'] = {all: [{chart_context: {$and: [{$nex: {running: 1}}, {$s: 1}]}}], run: to(null, stateName, false)};
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
            var reflexiveStages = {};
            var rule;
            var stageName;
            var stage;

            for (stageName in chart) {
                stage = chart[stageName];
                if (stage.to) {
                    if (typeof(stage.to) === 'string') {
                        if (stage.to === stageName) {
                            reflexiveStages[stageName] = true;
                        }
                    } else {
                        for (var transitionName  in stage.to) {
                            var transition = stage.to[transitionName];
                            if ((transitionName === stageName) || transition.count || transition.span || transition.cap) {
                                reflexiveStages[stageName] = true;
                            }
                        }
                    }
                }
            }

            for (stageName in chart) {
                stage = chart[stageName];
                var stageTest = {chart_context: {$and:[{label: stageName}, {chart:1}]}};
                var nextStage;
                var fromStage = null;
                if (reflexiveStages[stageName]) {
                    fromStage = stageName;
                }

                if (stage.to) {
                    if (typeof(stage.to) === 'string') {
                        rule = {};
                        rule.all = [stageTest];
                        nextStage = chart[stage.to];

                        var assertStage = false;
                        if (reflexiveStages[stage.to]) {
                            assertStage = true;
                        }

                        if (!nextStage.run) {
                            rule.run = to(fromStage, stage.to, assertStage);
                        } else {
                            if (typeof(nextStage.run) === 'string') {
                                rule.run = to(fromStage, stage.to, assertStage).continueWith(host.getAction(nextStage.run));
                            } else if (typeof(nextStage.run) === 'function' || nextStage.run.continueWith) {
                                rule.run = to(fromStage, stage.to, assertStage).continueWith(nextStage.run);
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

                            if (transition.span) {
                                rule.span = transition.span;
                            }

                            if (transition.cap) {
                                rule.cap = transition.cap;
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

                            var assertStage = false;
                            if (reflexiveStages[transitionName]) {
                                assertStage = true;
                            }

                            if (!nextStage.run) {
                                rule.run = to(transitionName);
                            } else {
                                if (typeof(nextStage.run) === 'string') {
                                    rule.run = to(fromStage, transitionName, assertStage).continueWith(host.getAction(nextStage.run));
                                } else if (typeof(nextStage.run) === 'function' || nextStage.run.continueWith) {
                                    rule.run = to(fromStage, transitionName, assertStage).continueWith(nextStage.run);
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
                    rule = {all: [{chart_context: {$and: [{$nex: {running: 1}}, {$s: 1}]}}]};
                    if (!stage.run) {
                        rule.run = to(null, stageName, false);
                    } else {
                        if (typeof(stage.run) === 'string') {
                            rule.run = to(null, stageName, false).continueWith(host.getAction(stage.run));
                        } else if (typeof(stage.run) === 'function' || stage.run.continueWith) {
                            rule.run = to(null, stageName, false).continueWith(stage.run);
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
        databases = databases || [{host: 'localhost', port: 6379, password:null}];
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

        that.ensureRuleset = function (rulesetName, complete) {
            if (rulesDirectory[rulesetName]) {
                complete(null);
            } else {
                that.loadRuleset(rulesetName, function (err, result) {
                    if (err) {
                        complete(err);
                    } else {
                        try {
                            that.registerRulesets(null, result);
                            complete(null);
                        } catch (reason) {
                            complete(reason);
                        }
                    }
                });
            }
        };

        that.getRuleset = function(rulesetName) {
            var rules = rulesDirectory[rulesetName];
            if (!rules) {
                throw 'Ruleset ' + rulesetName + ' not found';
            }

            return rules;
        }

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

        that.startPostBatch = function (rulesetName, messages) {
            return that.getRuleset(rulesetName).startAssertEvents(messages);
        };

        that.postBatch = function () {
            var rulesetName = arguments[0];
            var messages = [];
            var complete;
            if (arguments[1].constructor === Array) {
                messages = arguments[1];
            }
            else {
                for (var i = 1; i < arguments.length; ++ i) {
                    messages.push(arguments[i]);
                }
            }

            var rules = that.getRuleset(rulesetName);
            return rules.assertEvents(messages);
        };

        that.startPost = function (rulesetName, message) {
            return that.getRuleset(rulesetName).startAssertEvent(message);
        };

        that.post = function (rulesetName, message) {
            return that.getRuleset(rulesetName).assertEvent(message);
        };

        that.startAssert = function (rulesetName, fact) {
            return that.getRuleset(rulesetName).startAssertFact(fact);
        };

        that.assert = function (rulesetName, fact) {
            return that.getRuleset(rulesetName).assertFact(fact);
        };

        that.startAssertFacts = function (rulesetName, facts) {
            return that.getRuleset(rulesetName).startAssertFacts(facts);
        };

        that.assertFacts = function (rulesetName, facts) {
            return that.getRuleset(rulesetName).assertFacts(facts);
        };

        that.startRetract = function (rulesetName, fact) {
            return that.getRuleset(rulesetName).startRetractFact(fact);
        };

        that.retract = function (rulesetName, fact) {
            return that.getRuleset(rulesetName).retractFact(fact);
        };

        that.startRetractFacts = function (rulesetName, facts) {
            return that.getRuleset(rulesetName).startRetractFacts(facts);
        };

        that.retractFacts = function (rulesetName, facts) {
            return that.getRuleset(rulesetName).retractFacts(facts);
        };

        that.getState = function (rulesetName, sid) {
            return that.getRuleset(rulesetName).getState(sid);
        };

        that.patchState = function (rulesetName, state) {
            return that.getRuleset(rulesetName).assertState(state);
        };

        that.renewActionLease = function (rulesetName, sid) {
            return that.getRuleset(rulesetName).renewActionLease(sid);
        };

        var dispatchRules = function (index) {
            if (!rulesList.length) {
                setTimeout(dispatchRules, 100, index);
            } else {
                var rules = rulesList[index % rulesList.length];
                rules.dispatch(function (err) {
                    if (err) {
                        if (String(err).search('306') == -1) {
                            console.log('Exiting ' + err);
                            console.log(JSON.stringify(rules.getDefinition()))
                            process.exit(1);
                        }
                    }
                    setImmediate(dispatchRules, index + 1);
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

    var queue = function(rulesetName, database, stateCacheSize) {
        var that = {};
        var handle;
        database = database || {host: 'localhost', port: 6379, password:null};
        stateCacheSize = stateCacheSize || 1024;

        that.post = function (message) {
            return r.queueAssertEvent(handle, message.sid, rulesetName, JSON.stringify(message));
        };

        that.assert = function (message) {
            return r.queueAssertFact(handle, message.sid, rulesetName, JSON.stringify(message));
        };

        that.retract = function (message) {
            return r.queueRetractFact(handle, message.sid, rulesetName, JSON.stringify(message));
        };

        that.close = function() {
            return r.deleteClient(handle);
        };

        handle = r.createClient(rulesetName, stateCacheSize)
        if (typeof(database) === 'string') {
            r.bindRuleset(handle, database, 0, null);
        } else {
            r.bindRuleset(handle, database.host, database.port, database.password);
        }

        return that;
    }

    var application = function (host, port, basePath) {
        var that = express();
        var fileServer = new stat.Server(__dirname);
        basePath = basePath || '';
        port = port || 5000;
        if (basePath !== '' && basePath.indexOf('/') !== 0) {
            basePath = '/' + basePath;
        }

        that.use(bodyParser.json());
        that.run = function () {

            that.get(basePath + '/:rulesetName/:sid', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                host.ensureRuleset(request.params.rulesetName, function (err, result) {
                    if (err) {
                        response.send({ error: err }, 500);
                    }
                    else {
                        try {
                            response.send(host.getState(request.params.rulesetName, request.params.sid));
                        } catch (reason) {
                            response.send({ error: reason }, 500);
                        }
                    }
                });
            });

            that.post(basePath + '/:rulesetName/:sid', function (request, response) {
                response.contentType = "application/json; charset=utf-8";
                var message = request.body;
                message.sid = request.params.sid;

                host.ensureRuleset(request.params.rulesetName, function (err, result) {
                    if (err) {
                        response.send({ error: err }, 500);
                    }
                    else {
                        try {
                            host.post(request.params.rulesetName, message)
                            response.send();
                        } catch (reason) {
                            response.send({ error: reason }, 500);
                        }
                    }
                });
            });

            that.patch(basePath + '/:rulesetName/:sid', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                var document = request.body;
                document.id = request.params.sid;
                host.ensureRuleset(request.params.rulesetName, function (err, result) {
                    if (err)
                        response.send({ error: err }, 500);
                    else {
                        try {
                            host.patchState(request.params.rulesetName, document);
                            response.send();
                        } catch (reason) {
                            response.send({ error: reason }, 500);
                        }
                    }
                });
            });

            that.get(basePath + '/:rulesetName', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                host.ensureRuleset(request.params.rulesetName, function (err, result) {
                    if (err)
                        response.send({ error: err }, 500);
                    else {
                        try {
                            response.send(host.getRuleset(request.params.rulesetName).getDefinition());
                        } catch (reason) {
                            response.send({ error: reason }, 500);
                        }
                    }
                });
            });

            that.post(basePath + '/:rulesetName', function (request, response) {
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
        queue: queue,
        application: application
    };
}();