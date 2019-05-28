exports = module.exports = durableEngine = function () {
    var bodyParser = require('body-parser');
    var express = require('express');
    var stat = require('node-static');
    var r = require('bindings')('rulesjs.node');

    var closure = function (host, document, output, handle, rulesetName) {
        var that = {};
        var targetRulesets = {};
        var eventDirectory = {};
        var queueDirectory = {};
        var factDirectory = {};
        var retractDirectory = {};
        var timerDirectory = {};
        var cancelledTimerDirectory = {};
        var branchDirectory = {};
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
            return eventDirectory;
        };

        that.getFacts = function () {
            return factDirectory;
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
            if (eventDirectory[rules]) {
                eventsList = eventDirectory[rules];
            } else {
                eventsList = [];
                targetRulesets[rules] = true;
                eventDirectory[rules] = eventsList;
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
            if (factDirectory[rules]) {
                factsList = factDirectory[rules];
            } else {
                factsList = [];
                targetRulesets[rules] = true;
                factDirectory[rules] = factsList;
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

        that.deleteState = function () {
            deleted = true;
        };

        that.startTimer = function (name, duration, manualReset) {
            manualReset = manualReset ? 1 : 0;
            if (timerDirectory[name]) {
                throw 'timer with name ' + name + ' already added';
            } else {
                timerDirectory[name] = [name, duration, manualReset];
            }
        };

        that.cancelTimer = function (name) {
            if (cancelledTimerDirectory[name]) {
                throw 'timer with name ' + name + ' already cancelled';
            } else {
                cancelledTimerDirectory[name] = true;
            }
        };

        that.renewActionLease = function() {
            if ((new Date().getTime() - startTime) < 10000) {
                startTime = new Date().getTime();
                host.renewActionLease(rulesetName, that.s.sid);
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
                    c.assert({label: toState, chart: 1});
                } else {
                    c.post({label: toState, chart: 1});
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
                    if (propertyType === 'object' && object[pName] !== null) {
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

    var ruleset = function (name, host, rulesetDefinition) {
        var that = {};
        var actions = {};
        var handle;
        
        that.assertEvent = function (message) {
            return r.assertEvent(handle, JSON.stringify(message));
        };

        that.assertEvents = function (messages) {
            return r.assertEvents(handle, JSON.stringify(messages));
        };

        that.assertFact = function (fact) {
            return r.assertFact(handle, JSON.stringify(fact));
        };

        that.assertFacts = function (facts) {
            return r.assertFacts(handle, JSON.stringify(facts));
        };

        that.retractFact = function (fact) {
            return r.retractFact(handle, JSON.stringify(fact));
        };

        that.retractFacts = function (facts) {
            return r.retractFacts(handle, JSON.stringify(facts));
        };

        that.updateState = function (state) {
            state['$s'] = 1;
            return r.updateState(handle, JSON.stringify(state));
        };

        that.startTimer = function (sid, timer, timerDuration, manualReset) {
            return r.startTimer(handle, sid, timerDuration, manualReset, timer);
        };

        that.cancelTimer = function (sid, timer_name) {
            return r.cancelTimer(handle, sid, timer_name);
        };        

        that.getState = function (sid) {
            return JSON.parse(r.getState(handle, sid));
        }

        that.deleteState = function (sid) {
            return r.deleteState(handle, sid);
        }

        that.renewActionLease = function (sid) {
            return r.renewActionLease(handle, sid);
        }

        that.dispatchTimers = function () { 
            return r.assertTimers(handle);
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
                var c = closure(host, state, message, stateOffset, name);
                actions[actionName].run(c, function (err, c) {
                    if (err) {
                        r.abandonAction(handle, c.getHandle());
                        complete(err);
                    } else {
                        var rulesetNames = c.getTargetRulesets();
                        ensureRulesets(rulesetNames, 0, c, function(err, c) {
                            if (c.hasEnded()) {
                                return;
                            } else {
                                c.end();
                            }

                            if (err) {
                                r.abandonAction(handle, c.getHandle());
                                complete(err);
                                return;
                            }

                            try {
                                var rulesetName;
                                var facts;
                                var currentStateOffset = 0;
                                var replies = 0;
                                var pending = {};
                                
                                var timers = c.getCancelledTimers();
                                for (var timerName in timers) {
                                    try {
                                        that.cancelTimer(c.s.sid, timerName);
                                    } catch (reason) {
                                        c.s.rulesetException = { action: 'cancelTimer', timer: timerName, reason: reason};
                                    }
                                }
                                timers = c.getTimers();
                                for (var timerName in timers) {
                                    var timerTuple = timers[timerName];
                                    try {   
                                        that.startTimer(c.s.sid, timerTuple[0], timerTuple[1], timerTuple[2]);
                                    } catch (reason) {
                                        c.s.rulesetException = { action: 'startTimer', timer: timerName, reason: reason};
                                    }
                                }                 
                                var retractFacts = c.getRetract();
                                pending[stateOffset] = 0;
                                for (rulesetName in retractFacts) {
                                    facts = retractFacts[rulesetName];
                                    try {                   
                                        if (facts.length == 1) {
                                            currentStateOffset = host.retract(rulesetName, facts[0]);
                                        } else {
                                            currentStateOffset = host.retractFacts(rulesetName, facts);
                                        }
                                        
                                        if (!pending[currentStateOffset]) {
                                            pending[currentStateOffset] = true;
                                        }
                                    } catch (reason) {
                                        c.s.rulesetException = { action: 'retract', facts: facts, reason: reason};
                                    }
                                }
                                var assertFacts = c.getFacts();
                                for (rulesetName in assertFacts) {
                                    facts = assertFacts[rulesetName];
                                    try {
                                        if (facts.length == 1) {
                                            currentStateOffset = host.assert(rulesetName, facts[0]);
                                        } else {
                                            currentStateOffset = host.assertFacts(rulesetName, facts);
                                        }
                                       
                                        if (!pending[currentStateOffset]) {
                                            pending[currentStateOffset] = true;
                                        }
                                    } catch (reason) {
                                        c.s.rulesetException = { action: 'assert', facts: facts, reason: reason};
                                    }
                                }
                                var postEvents = c.getEvents();
                                for (rulesetName in postEvents) {
                                    var events = postEvents[rulesetName];
                                    try {
                                        if (events.length == 1) {
                                            currentStateOffset = host.post(rulesetName, events[0]);
                                        } else {
                                            currentStateOffset = host.postEvents(rulesetName, events);
                                        }
                                        
                                        if (!pending[currentStateOffset]) {
                                            pending[currentStateOffset] = true;
                                        }
                                    } catch (reason) {
                                        c.s.rulesetException = { action: 'post', events: events, reason: reason};
                                    }
                                }

                                try {
                                    currentStateOffset = r.updateState(handle, JSON.stringify(c.s));

                                    if (!pending[currentStateOffset]) {
                                        pending[currentStateOffset] = true;
                                    }
                                } catch (reason) {
                                    c.s.rulesetException = { action: 'updateState', events: events, reason: reason};
                                }
    
                                for (currentStateOffset in pending) {
                                    if (currentStateOffset) {
                                        if (currentStateOffset != stateOffset) {
                                            r.complete(currentStateOffset);
                                        } else {
                                            var newResult = r.completeAndStartAction(handle, c.getHandle());
                                            if (newResult) {
                                                resultContainer['message'] = JSON.parse(newResult);
                                            } else {
                                                complete(null, state);
                                            }
                                        }
                                    }                                    
                                }
                            } catch (reason) {
                                r.abandonAction(handle, c.getHandle());
                                complete(reason);
                            }

                            if (c.isDeleted()) {
                                try {
                                    host.deleteState(name, c.s.sid);
                                } catch (reason) {
                                    complete(reason);
                                }
                            }   
                        });
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
                flushActions(JSON.parse(result[0]), {'message': JSON.parse(result[1])}, result[2], function(reason, state) {
                    if (reason) {
                       console.log(reason);
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

        that.setRuleset = function (rulesetDefinition, complete) {
            try {
                that.registerRulesets(rulesetDefinition);
                that.saveRuleset(rulesetName, rulesetDefinition, complete);
            } catch (reason) {
                complete(reason);
            }
        };

        that.registerRulesets = function (rulesetDefinitions) {
            var rulesets = createRulesets(that, rulesetDefinitions);
            var names = [];
            for (var rulesetName in rulesets) {
                var rulesetDefinition = rulesets[rulesetName];
                if (rulesDirectory[rulesetName]) {
                    throw 'ruleset ' + rulesetName + ' already registered';
                } else {
                    rulesDirectory[rulesetName] = rulesetDefinition;
                    rulesList.push(rulesetDefinition);
                    names.push(rulesetName);
                }
            }

            return names;
        };

        that.postEvents = function (rulesetName, messages, complete) { 
            if (!complete) {
                return that.getRuleset(rulesetName).assertEvents(messages);
            } else {
                try {
                    var rules = that.getRuleset(rulesetName);
                    rules.doActions(rules.assertEvents(messages), complete);
                } catch (reason) {
                    complete(reason);
                }
            }
        };

        that.post = function (rulesetName, message, complete) {
            if (!complete) {
                return that.getRuleset(rulesetName).assertEvent(message);
            } else {
                try {
                    var rules = that.getRuleset(rulesetName);
                    rules.doActions(rules.assertEvent(message), complete);
                } catch (reason) {
                    complete(reason);
                }
            }
        };

        that.assertFacts = function (rulesetName, facts, complete) {
            if (!complete) {
                return that.getRuleset(rulesetName).assertFacts(facts);
            } else {
                try {
                    var rules = that.getRuleset(rulesetName);
                    rules.doActions(rules.assertFacts(facts), complete);
                } catch (reason) {
                    complete(reason);
                }
            }
        };

        that.assert = function (rulesetName, fact, complete) {
            if (!complete) {
                return that.getRuleset(rulesetName).assertFact(fact);
            } else {
                try {
                    var rules = that.getRuleset(rulesetName);
                    rules.doActions(rules.assertFact(fact), complete);
                } catch (reason) {
                    complete(reason);
                }
            }
        };

        that.retractFacts = function (rulesetName, facts, complete) {
            if (!complete) {
                return that.getRuleset(rulesetName).retractFacts(facts);
            } else {
                try {
                    var rules = that.getRuleset(rulesetName);
                    rules.doActions(rules.retractFacts(facts), complete);
                } catch (reason) {
                    complete(reason);
                }
            }
        };

        that.retract = function (rulesetName, fact, complete) {
           if (!complete) {
                return that.getRuleset(rulesetName).retractFact(fact);
            } else {
                try {
                    var rules = that.getRuleset(rulesetName);
                    rules.doActions(rules.retractFact(fact), complete);
                } catch (reason) {
                    complete(reason);
                }
            }
        };

        that.updateState = function (rulesetName, state, complete) {
            if (!complete) {
                return that.getRuleset(rulesetName).updateState(fact);
            } else {
                try {
                    var rules = that.getRuleset(rulesetName);
                    rules.doActions(rules.updateState(state), complete);
                } catch (reason) {
                    complete(reason);
                }
            }
        };

        that.getState = function (rulesetName, sid) {
            return that.getRuleset(rulesetName).getState(sid);
        };

        that.deleteState = function (rulesetName, sid) {
            return that.getRuleset(rulesetName).deleteState(sid);
        };

        that.renewActionLease = function (rulesetName, sid) {
            return that.getRuleset(rulesetName).renewActionLease(sid);
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

            that.get(basePath + '/:rulesetName/state/:sid', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                host.ensureRuleset(request.params.rulesetName, function (err, result) {
                    if (err) {
                        response.status(500).send({ error: err });
                    }
                    else {
                        try {
                            response.status(200).send(host.getState(request.params.rulesetName, request.params.sid));
                        } catch (reason) {
                            response.status(500).send({ error: reason });
                        }
                    }
                });
            });

            that.get(basePath + '/:rulesetName/state', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                host.ensureRuleset(request.params.rulesetName, function (err, result) {
                    if (err) {
                        response.status(500).send({ error: err });
                    }
                    else {
                        try {
                            response.status(200).send(host.getState(request.params.rulesetName, null));
                        } catch (reason) {
                            response.status(500).send({ error: reason });
                        }
                    }
                });
            });


            that.post(basePath + '/:rulesetName/state/:sid', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                var document = request.body;
                document.id = request.params.sid;
                host.ensureRuleset(request.params.rulesetName, function (err, result) {
                    if (err)
                        response.status(500).send({ error: err });
                    else {
                        try {
                            var result = host.patchState(request.params.rulesetName, document);
                            response.status(200).send({ outcome: result });
                        } catch (reason) {
                            response.status(500).send({ error: reason });
                        }
                    }
                });
            });

            that.post(basePath + '/:rulesetName/state', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                var document = request.body;
                host.ensureRuleset(request.params.rulesetName, function (err, result) {
                    if (err)
                        response.status(500).send({ error: err });
                    else {
                        try {
                            var result = host.patchState(request.params.rulesetName, document);
                            response.status(200).send({ outcome: result });
                        } catch (reason) {
                            response.status(500).send({ error: reason });
                        }
                    }
                });
            });

            that.post(basePath + '/:rulesetName/events/:sid', function (request, response) {
                response.contentType = "application/json; charset=utf-8";
                var message = request.body;
                message.sid = request.params.sid;

                host.ensureRuleset(request.params.rulesetName, function (err, result) {
                    if (err) {
                        response.status(500).send({ error: err });
                    }
                    else {
                        try {
                            var result = host.post(request.params.rulesetName, message);
                            response.status(200).send({ outcome: result });
                        } catch (reason) {
                            response.status(500).send({ error: reason });
                        }
                    }
                });
            });

            that.post(basePath + '/:rulesetName/events', function (request, response) {
                response.contentType = "application/json; charset=utf-8";
                var message = request.body;
                host.ensureRuleset(request.params.rulesetName, function (err, result) {
                    if (err) {
                        response.status(500).send({ error: err });
                    }
                    else {
                        try {
                            var result = host.post(request.params.rulesetName, message);
                            response.status(200).send({ outcome: result });
                        } catch (reason) {
                            response.status(500).send({ error: reason });
                        }
                    }
                });
            });

            that.post(basePath + '/:rulesetName/facts/:sid', function (request, response) {
                response.contentType = "application/json; charset=utf-8";
                var message = request.body;
                message.sid = request.params.sid;

                host.ensureRuleset(request.params.rulesetName, function (err, result) {
                    if (err) {
                        response.status(500).send({ error: err });
                    }
                    else {
                        try {
                            var result = host.assert(request.params.rulesetName, message);
                            response.status(200).send({ outcome: result });
                        } catch (reason) {
                            response.status(500).send({ error: reason });
                        }
                    }
                });
            });

            that.post(basePath + '/:rulesetName/facts', function (request, response) {
                response.contentType = "application/json; charset=utf-8";
                var message = request.body;
                host.ensureRuleset(request.params.rulesetName, function (err, result) {
                    if (err) {
                        response.status(500).send({ error: err });
                    }
                    else {
                        try {
                            var result = host.assert(request.params.rulesetName, message);
                            response.status(200).send({ outcome: result });
                        } catch (reason) {
                            response.status(500).send({ error: reason });
                        }
                    }
                });
            });

            that.get(basePath + '/:rulesetName/definition', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                host.ensureRuleset(request.params.rulesetName, function (err, result) {
                    if (err)
                        response.status(500).send({ error: err });
                    else {
                        try {
                            response.status(200).send(host.getRuleset(request.params.rulesetName).getDefinition());
                        } catch (reason) {
                            response.status(500).send({ error: reason });
                        }
                    }
                });
            });

            that.post(basePath + '/:rulesetName/definition', function (request, response) {
                response.contentType = "application/json; charset=utf-8";
                host.setRuleset(request.params.rulesetName, request.body, function (err, result) {
                    if (err) {
                        response.status(500).send({ error: err });
                    }
                    else {
                        response.status(200).send();
                    }
                });
            });

            return that.listen(port, function () {
                console.log('Listening on ' + port);
            });    
        };

        return that;
    };

    return {
        closure: closure,
        promise: promise,
        host: host,
        application: application
    };
}();
