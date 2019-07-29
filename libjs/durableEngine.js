exports = module.exports = durableEngine = function () {
    var r = require('bindings')('rulesjs.node');

    class MessageNotHandledError extends Error {
      constructor(message) {
        super('Could not handle message: ' + message);

        if (Error.captureStackTrace) {
          Error.captureStackTrace(this, MessageNotHandledError);
        }

        this.name = 'MessageNotHandledError';
      }
    }

    class MessageObservedError extends Error {
      constructor(message) {
        super('Message has already been observed: ' + message);

        if (Error.captureStackTrace) {
          Error.captureStackTrace(this, MessageObservedError);
        }

        this.name = 'MessageObservedError';
      }
    }

    var closure = function (host, ruleset, document, output, handle) {
        var that = {};
        var startTime = new Date().getTime();
        var ended = false;
        var deleted = false;

        that.s = document;
        
        if (output && output.constructor !== Array) {
            for (var name in output) {
                that[name] = output[name];
            }
        } else if (output) {
            that.m = [];
            for (var i = 0; i < output.length; ++i) {
                if (!output[i].m) {
                     that.m.push(output[i]);
                } else {
                    var keyCount = 0;
                    for (var key in output[i]) {
                        keyCount += 1;
                        if (keyCount > 1) {
                            break;
                        }
                    }
                    if (keyCount == 1) {
                        that.m.push(output[i].m);
                    } else {
                        that.m.push(output[i]);
                    }
                }
            }
        }            
        
        that.getHandle = function () {
            return handle;
        };

        that.getHost = function() {
            return host;
        };

        that.post = function (rules, message) {
            if (message) {
                if (!message.sid) {
                    message.sid = that.s.sid;
                }

                host.getRuleset(rules).assertEvent(message);
            } else {
                message = rules;
                if (!message.sid) {
                    message.sid = that.s.sid;
                }

                ruleset.assertEvent(message);
            }   
        };

        that.assert = function (rules, fact) {
            if (fact) {
                if (!fact.sid) {
                    fact.sid = that.s.sid;
                }

                host.getRuleset(rules).assertFact(fact);
            } else {
                fact = rules;
                if (!fact.sid) {
                    fact.sid = that.s.sid;
                }

                ruleset.assertFact(fact);
            }
        };

        that.retract = function (rules, fact) {
            if (fact) {
                if (!fact.sid) {
                    fact.sid = that.s.sid;
                }

                host.getRuleset(rules).retractFact(fact);
            } else {
                fact = rules;
                if (!fact.sid) {
                    fact.sid = that.s.sid;
                }

                ruleset.retractFact(fact);
            }
        };

        that.deleteState = function () {
            deleted = true;
        };

        that.startTimer = function (name, duration, manualReset) {
            manualReset = manualReset ? 1 : 0;
            ruleset.startTimer(that.s.sid, name, duration, manualReset);      
        };

        that.cancelTimer = function (name) {
            ruleset.cancelTimer(that.s.sid, name);
        };

        that.renewActionLease = function() {
            if ((new Date().getTime() - startTime) < 10000) {
                startTime = new Date().getTime();
                ruleset.renewActionLease(that.s.sid);
            }
        };

        that.hasEnded = function () {
            if ((new Date().getTime() - startTime) > 10000) {
                ended = true;
            }

            return ended;
        };

        that.end = function () {
            ended = true;
        }

        that.isDeleted = function () {
            return deleted;
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
            var timeoutCallback = function(maxTime) {
                if (new Date().getTime() > maxTime) {
                    c.s.exception = 'timeout expired';
                    complete(null, c)
                } else if (!c.hasEnded()) {
                    c.renewActionLease();
                    setTimeout(timeoutCallback, 5000, maxTime);
                }
            }

            // complete should never throw
            if (sync) {
                try {
                    func(c);
                } catch (reason) {
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
                        } 

                        if (next) {
                            next.run(c, complete);
                        } else {
                            complete(null, c);
                        }
                    });

                    if (timeLeft) {
                        setTimeout(timeoutCallback, 5000, new Date().getTime() + timeLeft * 1000);
                    }
                } catch (reason) {
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
                    c.assert({label: toState, chart: 1});
                } else {
                    c.post({label: toState, chart: 1});
                }
            }
        };

        var that = promise(execute);
        return that;
    };


    var ruleset = function (name, host, rulesetDefinition) {
        var that = {};
        var actions = {};
        var handle;
        
        var handleResult = function(result, message) {
            if (result[0] === 1) {
                throw new MessageNotHandledError(message);
            } else if (result[0] === 2) {
                throw new MessageObservedError(message);  
            } else if (result[0] === 3) {
                return 0;
            } 

            return result[1];
        };

        that.assertEvent = function (message) {
            var json = JSON.stringify(message);
            return handleResult(r.assertEvent(handle, json), json);
        };

        that.assertEvents = function (messages) {
            var json = JSON.stringify(messages);
            return handleResult(r.assertEvents(handle, json), json);
        };

        that.assertFact = function (fact) {
            var json = JSON.stringify(fact);
            return handleResult(r.assertFact(handle, json), json);
        };

        that.assertFacts = function (facts) {
            var json = JSON.stringify(facts);
            return handleResult(r.assertFacts(handle, json), json);
        };

        that.retractFact = function (fact) {
            var json = JSON.stringify(fact);
            return handleResult(r.retractFact(handle, json), json);
        };

        that.retractFacts = function (facts) {
            var json = JSON.stringify(facts);
            return handleResult(r.retractFacts(handle, json), json);
        };

        that.updateState = function (state) {
            state['$s'] = 1;
            return handleResult(r.updateState(handle, JSON.stringify(state)));
        };

        that.startTimer = function (sid, timer, timerDuration, manualReset) {
            r.startTimer(handle, sid, timerDuration, manualReset, timer);
        };

        that.cancelTimer = function (sid, timer_name) {
            r.cancelTimer(handle, sid, timer_name);
        };        

        that.getState = function (sid) {
            return JSON.parse(r.getState(handle, sid));
        }

        that.deleteState = function (sid) {
            r.deleteState(handle, sid);
        }

        that.renewActionLease = function (sid) {
            r.renewActionLease(handle, sid);
        }

        that.dispatchTimers = function () { 
            r.assertTimers(handle);
        };

        that.setStoreMessageCallback = function (func) {
            r.setStoreMessageCallback(handle, func);
        };

        that.setDeleteMessageCallback = function (func) {
            r.setDeleteMessageCallback(handle, func);
        };

        that.setQueueMessageCallback = function (func) {
            r.setQueueMessageCallback(handle, func);
        };

        that.setGetStoredMessagesCallback = function (func) {
            r.setGetStoredMessagesCallback(handle, func);
        };

        that.setGetQueuedMessagesCallback = function (func) {
            r.setGetQueuedMessagesCallback(handle, func);
        };

        that.setGetIdleStateCallback = function (func) {
            r.setGetIdleStateCallback(handle, func);
        };

        var flushActions = function (state, resultContainer, stateOffset, complete) {
            while (resultContainer['message']) {
                var actionName = null;
                var message = null;
                for (actionName in resultContainer['message']) {
                    message = resultContainer['message'][actionName];
                    break;
                }
                resultContainer['message'] = null;
                var c = closure(host, that, state, message, stateOffset);
                actions[actionName].run(c, function (err, c) {
                    if (err) {
                        r.abandonAction(handle, c.getHandle());
                        complete(err);
                    } else {
                        if (c.hasEnded()) {
                            return;
                        } else {
                            c.end();
                        }

                        try {
                            r.updateState(handle, JSON.stringify(c.s));
                            
                            var newResult = r.completeAndStartAction(handle, c.getHandle());
                            if (newResult) {
                                resultContainer['message'] = JSON.parse(newResult);
                            } else {
                                complete(null, state);
                            }

                        } catch (reason) {
                            r.abandonAction(handle, c.getHandle());
                            complete(reason);
                        }

                        if (c.isDeleted()) {
                            try {
                                host.deleteState(name, c.s.sid);
                            } catch (reason) {
                            }
                        }   
                    }
                });
            }
        };

        that.doActions = function (stateHandle, complete) {
            try {
                var result = r.startActionForState(handle, stateHandle);
                if (!result) {
                    complete(null);
                } else {
                    flushActions(JSON.parse(result[0]), {'message': JSON.parse(result[1])}, stateHandle, complete);
                }
            } catch (reason) {
                complete(reason);
            }
        };        

        that.dispatch = function () {            
            var result = r.startAction(handle);
            if (result) {
                flushActions(JSON.parse(result[0]), {'message': JSON.parse(result[1])}, result[2], function(reason, state) {});
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
            }

            delete(rule.run);
        }
        handle = r.createRuleset(name, JSON.stringify(rulesetDefinition));
        return that;
    }

    var stateChart = function (name, host, chartDefinition) {
        
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
                    if ((trigger.to && trigger.to === stateName) || trigger.count || trigger.cap) {
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
        var that = ruleset(name, host, rules);
        that.getDefinition = function () {
            chartDefinition.$type = 'stateChart';
            return chartDefinition;
        };

        return that;
    };

    var flowChart = function (name, host, chartDefinition) {
        
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
                            if ((transitionName === stageName) || transition.count || transition.cap) {
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
        var that = ruleset(name, host, rules);

        that.getDefinition = function () {
            chartDefinition.$type = 'flowChart';
            return chartDefinition;
        };

        return that;
    };

    var createRulesets = function (host, rulesetDefinitions) {
        
        var branches = {};
        for (var name in rulesetDefinitions) {
            var currentDefinition = rulesetDefinitions[name];
            var rules;
            var index = name.indexOf('$state');
            if (index !== -1) {
                name = name.slice(0, index);
                branches[name] = stateChart(name, host, currentDefinition);
            } else {
                index = name.indexOf('$flow');
                if (index !== -1) {
                    name = name.slice(0, index);
                    branches[name] = flowChart(name, host, currentDefinition);
                } else {
                    branches[name] = ruleset(name, host, currentDefinition);
                }
            }
        }

        return branches;
    };

    var host = function () {
        var that = {};
        var rulesDirectory = {};
        var instanceDirectory = {};
        var rulesList = [];
        var storeMessageCallback = null;
        var deleteMessageCallback = null;
        var queueMessageCallback = null;
        var getStoredMessagesCallback = null;
        var getQueuedMessagesCallback = null;
        var getIdleStateCallback = null;
        
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

        that.setRulesets = function (rulesetDefinitions, complete) {
            try {
                that.registerRulesets(rulesetDefinitions);
                for (var rulesetName in rulesetDefinitions) {
                    var rulesetDefinition = rulesetDefinitions[rulesetName];
                    that.saveRuleset(rulesetName, rulesetDefinition, complete);
                }
            } catch (reason) {
                complete(reason);
            }
        };

        that.registerRulesets = function (rulesetDefinitions) {
            var rulesets = createRulesets(that, rulesetDefinitions);
            var names = [];
            for (var rulesetName in rulesets) {
                var ruleset = rulesets[rulesetName];
                if (rulesDirectory[rulesetName]) {
                    throw 'ruleset ' + rulesetName + ' already registered';
                } else {
                    rulesDirectory[rulesetName] = ruleset;
                    rulesList.push(ruleset);
                    names.push(rulesetName);
                    if (storeMessageCallback) {
                        ruleset.setStoreMessageCallback(storeMessageCallback);
                    }

                    if (deleteMessageCallback) {
                        ruleset.setDeleteMessageCallback(deleteMessageCallback);
                    }

                    if (queueMessageCallback) {
                        ruleset.setQueueMessageCallback(queueMessageCallback);
                    }

                    if (getStoredMessagesCallback) {
                        ruleset.setGetStoredMessagesCallback(getStoredMessagesCallback);
                    }

                    if (getQueuedMessagesCallback) {
                        ruleset.setGetQueuedMessagesCallback(getQueuedMessagesCallback);
                    }

                    if (getIdleStateCallback) {
                        ruleset.setGetIdleStateCallback(getIdleStateCallback);
                    }
                }
            }

            return names;
        };

        handleFunction = function(rules, func, args, complete) {
            if (!complete) {
                var error = null;
                var result = null;
                rules.doActions(func(args), function(reason, state) {
                    error = reason;
                    result = state;
                });

                if (error) {
                    throw error;
                }

                return result;

            } else {
                try {
                    rules.doActions(func(args), complete);
                } catch (reason) {
                    complete(reason);
                }
            }
        }

        that.postEvents = function (rulesetName, messages, complete) { 
            var rules = that.getRuleset(rulesetName);
            return handleFunction(rules, rules.assertEvents, messages, complete);
        };

        that.post = function (rulesetName, message, complete) {
            var rules = that.getRuleset(rulesetName);
            return handleFunction(rules, rules.assertEvent, message, complete);
        };

        that.assertFacts = function (rulesetName, facts, complete) {
            var rules = that.getRuleset(rulesetName);
            return handleFunction(rules, rules.assertFacts, facts, complete);
        };

        that.assert = function (rulesetName, fact, complete) {
            var rules = that.getRuleset(rulesetName);
            return handleFunction(rules, rules.assertFact, fact, complete);
        };

        that.retractFacts = function (rulesetName, facts, complete) {
            var rules = that.getRuleset(rulesetName);
            return handleFunction(rules, rules.retractFacts, facts, complete);
        };

        that.retract = function (rulesetName, fact, complete) {
            var rules = that.getRuleset(rulesetName);
            return handleFunction(rules, rules.retractFact, fact, complete);
        };

        that.updateState = function (rulesetName, state, complete) {
            var rules = that.getRuleset(rulesetName);
            return handleFunction(rules, rules.updateState, state, complete);
        };

        that.getState = function (rulesetName, sid) {
            return that.getRuleset(rulesetName).getState(sid);
        };

        that.deleteState = function (rulesetName, sid) {
            that.getRuleset(rulesetName).deleteState(sid);
        };

        that.renewActionLease = function (rulesetName, sid) {
            that.getRuleset(rulesetName).renewActionLease(sid);
        };

        that.setStoreMessageCallback = function (func) {
            storeMessageCallback = func;

            for (var ruleset in rulesList) {
                ruleset.setStoreMessageCallback(func);
            }
        };

        that.setDeleteMessageCallback = function (func) {
            deleteMessageCallback = func;

            for (var ruleset in rulesList) {
                ruleset.setDeleteMessageCallback(func);
            }
        };

        that.setQueueMessageCallback = function (func) {
            queueMessageCallback = func;

            for (var ruleset in rulesList) {
                ruleset.setQueueMessageCallback(func);
            }
        };

        that.setGetQueuedMessagesCallback = function (func) {
            getQueuedMessagesCallback = func;

            for (var ruleset in rulesList) {
                ruleset.setGetQueuedMessagesCallback(func);
            }
        };

        that.setGetStoredMessagesCallback = function (func) {
            getStoredMessagesCallback = func;

            for (var ruleset in rulesList) {
                ruleset.setGetStoredMessagesCallback(func);
            }
        };

        that.setGetIdleStateCallback = function (func) {
            getIdleStateCallback = func;

            for (var ruleset in rulesList) {
                ruleset.setGetIdleStateCallback(func);
            }
        };

        var dispatchRules = function (index) {
            if (!rulesList.length) {
                setTimeout(dispatchRules, 500, index);
            } else {
                var rules = rulesList[index];
                try {
                    rules.dispatch();
                } catch (reason) {
                    console.log(reason);
                }

                var timeout = 0;
                if (index === (rulesList.length -1)) {
                    timeout = 200;
                }

                setTimeout(dispatchRules, timeout, (index + 1) % rulesList.length);
            }
        };

        var dispatchTimers = function (index) {
            if (!rulesList.length) {
                setTimeout(dispatchTimers, 500, index);
            } else {
                var rules = rulesList[index];
                try {
                    rules.dispatchTimers();
                } catch (reason) {
                    console.log(reason);
                }

                var timeout = 0;
                if (index === (rulesList.length -1)) {
                    timeout = 200;
                }

                setTimeout(dispatchTimers, timeout, (index + 1) % rulesList.length);
            }
        };

        setTimeout(dispatchRules, 100, 0);
        setTimeout(dispatchTimers, 100, 0);
        return that;
    };

    return {
        closure: closure,
        promise: promise,
        host: host
    };
}();
