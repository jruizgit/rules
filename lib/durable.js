exports = module.exports = durable = function () {
    var redis = require('redis');
    var express = require('express');

    var session = function (document, output, namespace) {
        var that = document;
        var messageList = [];
        var messageDirectory = {};
        var timerDirectory = {};
        var branchDirectory = {};

        that.getOutput = function () {
            return output;
        };

        that.getMessages = function () {
            return messageList;
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
                message.program = parentNamespace;
                that.post(message);
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

        that.post = function (message) {
            if (messageDirectory[message.id]) {
                throw 'message with id' + message.id + ' already posted';
            } else {
                messageDirectory[message.id] = message;
                messageList.push(message);
            }
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

    var promise = function (func, async) {
        var that = {};
        var root = that;
        var next;

        that.setRoot = function (rootPromise) {
            root = rootPromise;
        };

        that.getRoot = function () {
            return root;
        };

        that.continueWith = function (nextPromise) {
            if (!nextPromise) {
                throw 'promise or function argument is not defined';
            } else if (typeof nextPromise === 'function') {
                next = promise(nextPromise);
            } else {
                next = nextPromise;
            }

            next.setRoot(root);
            return next;
        };

        that.run = function (s, complete) {
            try {
                if (!async) {
                    func(s);
                    if (next) {
                        next.run(s, complete);
                    } else {
                        complete(null, s);
                    }
                } else {
                    func(s, function (err) {
                        if (err) {
                            complete(err);
                        } else if (next) {
                            next.run(s, complete);
                        } else {
                            complete(null, s);
                        }
                    });
                }
            } catch (reason) {
                complete(reason);
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
                    var propertyType = typeof object[pName];
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

    var ruleSet = function (namespace, rules) {
        var that = {};
        var retes;
        var retem;
        var actionDirectory = {};
        var messageDirectoryKey;
        var sessionDirectoryKey;
        var actionQueueKey;
        var hashIndex;

        var alpha = function (t) {
            var that = {};
            var next = {};
            var nextAssertions = [];
            var nextNegations = [];
            var nextAddssertions = [];

            that.getNames = function (childName) {
                if (childName) {
                    childName = childName + '.';
                }

                return childName + t;
            };

            that.evaluate = function (childName, sid, result) {
                if (t === "$s") {
                    return {};
                }

                if (childName) {
                    childName = childName + '.';
                }

                var value = result[childName + t + '.' + sid];
                if (!value) {
                    return false;
                }

                return value;
            };

            that.assert = function (transaction, sid, parentName, c, actions) {
                if (that.test(c)) {
                    var handled = false;
                    for (var i = 0; i < nextAssertions.length; ++i) {
                        if (nextAssertions[i](transaction, sid, parentName, c, actions)) {
                            handled = true;
                        }
                    }

                    return handled;
                }

                return false;
            };

            that.addssert = function (transaction, sid, resourceKey, acHash, parentName, c, actions) {
                if (that.test(c)) {
                    var handled = false;
                    for (var i = 0; i < nextAssertions.length; ++i) {
                        if (nextAddssertions[i](transaction, sid, resourceKey, acHash, parentName, c, actions)) {
                            handled = true;
                        }
                    }

                    return handled;
                }

                return false;
            };

            that.negate = function (transaction, sid, parentName, c) {
                if (that.test(c)) {
                    var handled = false;
                    for (var i = 0; i < nextAssertions.length; ++i) {
                        if (nextNegations[i](transaction, sid, parentName, c)) {
                            handled = true;
                        }
                    }

                    return handled;
                }

                return false;
            };

            that.setNext = function (node) {
                var n = next[node.hash];
                if (!n) {
                    next[node.hash] = node;
                    nextAssertions.push(node.assert);
                    nextNegations.push(node.negate);
                    nextAddssertions.push(node.addssert);
                    n = node;
                }
                return n;
            };

            return that;
        };

        var betaConnector = function (qName, name, beta) {
            var that = {};
            var next;

            that.getNames = function (childName) {
                return beta.getNames(childName);
            };

            that.evaluate = function (childName, sid, result) {
                return beta.evaluate(childName, sid, result);
            };

            that.assert = function (transaction, sid, parentName, c, actions) {
                if (parentName) {
                    parentName = '.' + parentName;
                }

                return next.assert(transaction, sid, name + parentName, c, actions);
            };

            that.addssert = function (transaction, sid, resourceKey, acHash, parentName, c, actions) {
                if (parentName) {
                    parentName = '.' + parentName;
                }

                return next.addssert(transaction, sid, resourceKey, acHash, name + parentName, c, actions);
            };

            that.negate = function (transaction, sid, parentName, c) {
                if (parentName) {
                    parentName = '.' + parentName;
                }

                return next.negate(transaction, sid, name + parentName, c);
            };

            that.setNext = function (node) {
                next = node;
            };

            that.hash = qName + '.' + name;
            return that;
        };

        var beta = function (qName, parentNodes) {
            var that = {};
            var connectors = [];
            var names = [];

            that.getConnectors = function () {
                return connectors;
            };

            that.getNames = function (childName) {
                return that.buildNames(childName, names, parentNodes);
            };

            that.evaluate = function (childName, sid, result) {
                return that.tryEvaluate(childName, sid, names, parentNodes, result);
            };

            for (var currentName in parentNodes) {
                var currentNodes = parentNodes[currentName];
                var connector = betaConnector(qName, currentName, that);
                connectors.push(connector);
                names.push(currentName);
                for (var i = 0; i < currentNodes.length; ++i) {
                    currentNodes[i].setNext(connector);
                }
            }

            return that;
        };

        var action = function (qName, name, promise) {
            var that = {};
            var parent;
            var queryNames;

            that.setParent = function (parentNode) {
                parent = parentNode;
                queryNames = parent.getNames('');
            };

            var calculateOne = function (transaction, query, seHash, sid) {
                var args = [seHash, query.length + 1];
                args.push(actionQueueKey);
                for (var ii = 0; ii < query.length; ++ii) {
                    args.push(that.hash + '.' + query[ii] + '.' + sid);
                }

                args.push(that.hash);
                args.push(sid);
                args.push(new Date().getTime());
                transaction.evalsha(args);
            };

            that.calculate = function (transaction, seHash, sid) {
                if (typeof(queryNames) === 'string') {
                    calculateOne(transaction, [queryNames], seHash, sid);
                } else if (typeof(queryNames[0]) === 'string') {
                    calculateOne(transaction, queryNames, seHash, sid);
                } else {
                    for (var i = 0; i < queryNames.length; ++i) {
                        calculateOne(transaction, queryNames[i], seHash, sid);
                    }
                }
            };

            that.assert = function (transaction, sid, parentName, c, actions) {
                transaction.zadd(that.hash + '.' + parentName + '.' + sid, new Date().getTime(), c.id);
                actions.push({ action: that, sid: sid });
                return true;
            };

            that.addssert = function (transaction, sid, resourceKey, acHash, parentName, c, actions) {
                var assertKey = that.hash + '.' + parentName + '.' + sid;
                transaction.evalsha(acHash, 2, resourceKey, assertKey, c.id, JSON.stringify(c), new Date().getTime());
                actions.push({ action: that, sid: sid });
                return true;
            };

            that.negate = function (transaction, sid, parentName, c) {
                transaction.zrem(that.hash + '.' + parentName + '.' + sid, c.id);
                return true;
            };

            var postMessages = function (programHost, index, messageList, complete) {
                if (index !== messageList.length) {
                    programHost.post(messageList[index], function (err) {
                        if (err) {
                            complete(err);
                        } else {
                            postMessages(programHost, ++index, messageList, complete);
                        }
                    });
                } else {
                    complete();
                }
            };

            var createSessions = function (programHost, index, branchList, branchDirectory, complete) {
                if (index !== branchList.length) {
                    var branchName = branchList[index];
                    var branchSession = branchDirectory[branchName];
                    programHost.createSession(branchName, branchSession, function (err) {
                        if (err) {
                            complete(err);
                        } else {
                            createSessions(programHost, ++index, branchList, branchDirectory, complete);
                        }
                    });
                } else {
                    complete();
                }
            };

            that.dispatch = function (programHost, document, messages, complete) {
                var args = parent.evaluate(that.hash, document.id, messages);
                var currentSession = session(document, args, namespace);
                promise.getRoot().run(currentSession, function (err, s) {
                    if (err) {
                        complete(err);
                    } else {
                        var messageList = s.getMessages();
                        postMessages(programHost, 0, messageList, function (err) {
                            if (err) {
                                complete(err);
                            } else {
                                var branchDirectory = s.getBranches();
                                var branchList = [];
                                for (var branchName in branchDirectory) {
                                    branchList.push(branchName);
                                }

                                createSessions(programHost, 0, branchList, branchDirectory, function (err){
                                    if (err) {
                                        complete(err);
                                    } else {
                                        complete(null, s);
                                    }
                                });
                            }
                        });
                    }
                });
            };

            that.hash = qName + '.' + name;
            return that;
        };

        var lte = function (field, rvalue, t) {
            var that = alpha(t);

            that.test = function (c) {
                var lvalue = c[field];
                return (lvalue <= rvalue);
            };

            that.hash = 'lte_' + field + '_' + rvalue;

            return that;
        };

        var lt = function (field, rvalue, t) {
            var that = alpha(t);

            that.test = function (c) {
                var lvalue = c[field];
                return (lvalue < rvalue);
            };

            that.hash = 'lt_' + field + '_' + rvalue;

            return that;
        };

        var gte = function (field, rvalue, t) {
            var that = alpha(t);

            that.test = function (c) {
                var lvalue = c[field];
                return (lvalue >= rvalue);
            };

            that.hash = 'gte_' + field + '_' + rvalue;

            return that;
        };

        var gt = function (field, rvalue, t) {
            var that = alpha(t);

            that.test = function (c) {
                var lvalue = c[field];
                return (lvalue > rvalue);
            };

            that.hash = 'gt_' + field + '_' + rvalue;

            return that;
        };

        var nex = function (field, t) {
            var that = alpha(t);

            that.test = function (c) {
                return !(c[field]);
            };

            that.hash = 'nex_' + field;

            return that;
        };

        var ex = function (field, t) {
            var that = alpha(t);

            that.test = function (c) {
                return (c[field]);
            };

            that.hash = 'ex_' + field;

            return that;
        };

        var ne = function (field, rvalue, t) {
            var that = alpha(t);

            that.test = function (c) {
                var lvalue = c[field];
                return (lvalue !== rvalue);
            };

            that.hash = 'ne_' + field + '_' + rvalue;

            return that;
        };

        var eq = function (field, rvalue, t) {
            var that = alpha(t);

            that.test = function (c) {
                var lvalue = c[field];
                return (lvalue === rvalue);
            };

            that.hash = 'eq_' + field + '_' + rvalue;

            return that;
        };

        var fn = function (func, t) {
            var that = alpha(t);

            that.test = function (c) {
                return func(c);
            };

            hashIndex = hashIndex + 1;
            that.hash = 'fn-' + hashIndex;
        };

        var type = function (t) {
            var that = alpha(t);

            that.test = function () {
                return true;
            };

            that.hash = 'type';
            return that;
        };

        var single = function (current, name, value, t) {
            var field;
            var i;
            var ii;
            var currentValue;
            var nextValue;
            var n;

            switch (name) {
                case '$fn':
                    for (field in value) {
                        return fn(value[field], t);
                    }
                    break;
                case '$ne':
                    for (field in value) {
                        return ne(field, value[field], t);
                    }
                    break;
                case '$lte':
                    for (field in value) {
                        return lte(field, value[field], t);
                    }
                    break;
                case '$lt':
                    for (field in value) {
                        return lt(field, value[field], t);
                    }
                    break;
                case '$gte':
                    for (field in value) {
                        return gte(field, value[field], t);
                    }
                    break;
                case '$gt':
                    for (field in value) {
                        return gt(field, value[field], t);
                    }
                    break;
                case '$ex':
                    for (field in value) {
                        return ex(field, t);
                    }
                    break;
                case '$nex':
                    for (field in value) {
                        return nex(field, t);
                    }
                    break;
                case '$and':
                    for (i = 0; i < value.length; ++i) {
                        currentValue = value[i];
                        for (field in currentValue) {
                            nextValue = currentValue[field];
                        }

                        if (field) {
                            n = single(current, field, nextValue, t);
                            if (!n.hash) {
                                current = n;
                            }
                            else {
                                var newCurrent = [];
                                for (ii = 0; ii < current.length; ++ii) {
                                    newCurrent.push(current[ii].setNext(n));
                                }

                                current = newCurrent;
                            }
                        } else {
                            break;
                        }
                    }
                    return current;
                case '$or':
                    var orResult = [];
                    for (i = 0; i < value.length; ++i) {
                        currentValue = value[i];
                        for (field in currentValue) {
                            nextValue = currentValue[field];
                        }

                        if (!field) {
                            break;
                        }
                        else {
                            n = single(current, field, nextValue, t);
                            if (!n.hash) {
                                orResult.concat(n);
                            }
                            else {
                                for (ii = 0; ii < current.length; ++ii) {
                                    orResult.push(current[ii].setNext(n));
                                }
                            }
                        }
                    }
                    return orResult;
                default:
                    return eq(name, value, t);
            }

            throw 'incorrect syntax function:' + name + ', value:' + value;

        };

        var m = function (assertion) {
            return single([retem], '$and', [assertion], '$m');
        };

        var s = function (assertion) {
            return single([retes], '$and', [assertion], '$s');
        };

        var all = function (qName, nodes) {
            var that = beta(qName, nodes);
            var i1;
            var i2;

            that.buildNames = function (childName, nodeNames, parentNodes) {
                var currentName;
                var currentResult;
                if (childName) {
                    childName = childName + '.';
                }

                var result;
                var newResult;
                for (var i = 0; i < nodeNames.length; ++i) {
                    currentName = nodeNames[i];

                    currentResult = parentNodes[currentName][0].getNames(childName + currentName);
                    if (!result) {
                        if (typeof(currentResult) === 'string') {
                            currentResult = [currentResult];
                        }

                        result = currentResult;
                    }
                    else {
                        if (typeof(currentResult) === 'string') {
                            if (typeof(result[0]) === 'string') {
                                result.push(currentResult);
                            }
                            else {
                                for (i1 = 0; i1 < result.length; ++ i1) {
                                    result[i1].push(currentResult);
                                }
                            }
                        }
                        else if (typeof(currentResult[0]) === 'string') {
                            if (typeof(result[0]) === 'string') {
                                result = result.concat(currentResult);
                            }
                            else {
                                newResult = [];
                                for (i1 = 0; i1 < result.length; ++ i1) {
                                    newResult.push(result[i1].concat(currentResult));
                                }

                                result = newResult;
                            }
                        }
                        else {
                            newResult = [];
                            for (i1 = 0; i1 < currentResult.length; ++i1) {

                                for (i2 = 0; i2 < result.length; ++i2) {
                                    var currentArray = result[i2];
                                    if (typeof(currentArray) === 'string') {
                                        currentArray = [currentArray];
                                    }

                                    newResult.push(currentArray.concat(currentResult[i1]));
                                }
                            }

                            result = newResult;
                        }
                    }
                }
                return result;
            };

            that.tryEvaluate = function (childName, sid, names, parentNodes, query) {
                var currentName;
                var currentValue;
                var value = {};
                if (childName) {
                    childName = childName + '.';
                }

                for (var i = 0; i < names.length; ++i) {
                    currentName = names[i];
                    currentValue = parentNodes[currentName][0].evaluate(childName + currentName, sid, query);
                    if (!currentValue) {
                        return false;
                    }
                    else {
                        value[currentName] = currentValue;
                    }
                }

                return value;
            };

            return that;
        };

        var any = function (qName, nodes) {
            var that = beta(qName, nodes);
            var i;

            that.buildNames = function (childName, nodeNames, parentNodes) {
                var currentName;
                var currentResult;
                if (childName) {
                    childName = childName + '.';
                }

                var result = [];
                for (i = 0; i < nodeNames.length; ++i) {
                    currentName = nodeNames[i];
                    currentResult = parentNodes[currentName][0].getNames(childName + currentName);
                    if (typeof(currentResult) === 'string') {
                        result.push([currentResult]);
                    }
                    else if (typeof(currentResult[0]) === 'string') {
                        result.push(currentResult);
                    }
                    else {
                        result = result.concat(currentResult);
                    }
                }

                return result;
            };

            that.tryEvaluate = function (childName, sid, names, parentNodes, query) {
                var currentName;
                var currentValue;
                var value = {};
                if (childName) {
                    childName = childName + '.';
                }

                for (var i = 0; i < names.length; ++i) {
                    currentName = names[i];
                    currentValue = parentNodes[currentName][0].evaluate(childName + currentName, sid, query);
                    if (currentValue) {
                        value[currentName] = currentValue;
                        return value;
                    }
                }

                return false;
            };

            return that;
        };

        var resolveMany = function (qName, definitions) {
            var resolvedDefinitions = {};
            var currentName;
            for (var assertionName in definitions) {
                if (assertionName.indexOf('$all') !== -1) {
                    currentName = assertionName.substring(0, assertionName.indexOf('$'));
                    resolvedDefinitions[currentName] = resolveAll(qName + '.' + currentName, definitions[assertionName]);
                }
                else if (assertionName.indexOf('$any') !== -1) {
                    currentName = assertionName.substring(0, assertionName.indexOf('$'));
                    resolvedDefinitions[currentName] = resolveAny(qName + '.' + currentName, definitions[assertionName]);
                }
                else if (assertionName === '$s') {
                    resolvedDefinitions[assertionName] = s(definitions[assertionName]);
                }
                else {
                    resolvedDefinitions[assertionName] = m(definitions[assertionName]);
                }
            }

            return resolvedDefinitions;
        };

        var resolveAll = function (qName, definitions) {
            definitions = resolveMany(qName, definitions);
            return all(qName, definitions).getConnectors();
        };

        var resolveAny = function (qName, definitions) {
            definitions = resolveMany(qName, definitions);
            return any(qName, definitions).getConnectors();
        };

        that.initialize = function (host, parentNamespace) {
            if (parentNamespace) {
                namespace = parentNamespace + '.' + namespace;

            }

            messageDirectoryKey = namespace + '.m';
            sessionDirectoryKey = namespace + '.s';
            actionQueueKey = namespace + '.a';

            host.register(namespace, that);
            if (rules) {
                for (var ruleName in rules) {
                    var currentRule = rules[ruleName];
                    that.addRule(ruleName, currentRule, host);
                }
            }
        };

        that.addRule = function (ruleName, definition, host) {
            var ruleAction;
            if (!ruleName) {
                throw 'ruleName argument is not defined';
            }

            if (!definition) {
                throw 'definition argument is not defined';
            }

            if (!definition.run) {
                throw 'rule action not defined';
            }

            if (typeof(definition.run) === 'function') {
                definition.run = promise(definition.run);
            } else if (!definition.run.continueWith) {
                definition.run = fork(parseDefinitions(definition.run, host, namespace));
            }

            ruleAction = action(namespace, ruleName, definition.run);
            actionDirectory[ruleAction.hash] = ruleAction;
            var nodes = {};
            if (definition.when) {
                if (!definition.when.$s && !definition.when.$m) {
                    nodes = resolveMany(ruleName, { $m: definition.when });
                }
                else {
                    nodes = resolveMany(ruleName, definition.when);
                }
            }
            else if (definition.whenAll) {
                nodes.$all = resolveAll(ruleName, definition.whenAll);
            }
            else if (definition.whenAny) {
                nodes.$any = resolveAny(ruleName, definition.whenAny);
            }
            else {
                throw 'rule assertion not defined';
            }

            for (var currentName in nodes) {
                var currentNodes = nodes[currentName];
                for (var i = 0; i < currentNodes.length; ++i) {
                    currentNodes[i].setNext(ruleAction);
                    ruleAction.setParent(currentNodes[i]);
                }
            }
        };

        var createArguments = function (args, messages) {
            var value = {};
            for (var argName in args) {
                var currentArgs = args[argName];
                if (argName === '$m') {
                    if (typeof(currentArgs) === 'string') {
                        return messages[currentArgs];
                    }
                    else {
                        return createArguments(currentArgs, messages);
                    }
                }
                if (argName !== '$s') {
                    value[argName] = createArguments(currentArgs, messages);
                }
            }

            return value;
        };

        var dispatchPromise = function (programHost, queryResult, complete) {
            var queuedAction = queryResult[1];
            var messages = {};
            var document = JSON.parse(queryResult[2]);
            var documentCopy = JSON.parse(queryResult[2]);
            var currentAction = actionDirectory[queuedAction];

            for (var i = 3; i < queryResult.length; i += 2) {
                var currentMessage = JSON.parse(queryResult[i + 1]);
                if (!currentMessage) {
                    currentMessage = document;
                }
                messages[queryResult[i]] = currentMessage;
            }

            currentAction.dispatch(programHost, document, messages, function (err, s) {
                if (err) {
                    complete(err);
                }
                else {
                    complete(null, documentCopy, messages, s);
                }
            });
        };

        var propagateActions = function (transaction, seHash, actions, complete) {
            var visitedActions = {};
            var currentAction;

            for (var i = 0; i < actions.length; ++i) {
                currentAction = actions[i];
                if (!visitedActions[currentAction.action.hash]) {
                    currentAction.action.calculate(transaction, seHash, currentAction.sid);
                    visitedActions[currentAction.action.hash] = currentAction;
                }
            }

            transaction.exec(complete);
        };

        that.dispatchAction = function (programHost, partition, complete) {
            partition.db.evalsha(partition.sdHash, 3, sessionDirectoryKey, messageDirectoryKey, actionQueueKey, new Date().getTime(), function (err, result) {
                if (err) {
                    complete(err);
                } else if (!result) {
                    complete();
                } else {
                    dispatchPromise(programHost, result, function (err, documentCopy, messages, s) {
                        if (err) {
                            complete(err);
                        } else {
                            var actions = [];
                            var sid = s.id;
                            var currentMessage;
                            var messagePath;

                            var transaction = partition.db.multi();

                            //add timers
                            var timers = s.getTimers();
                            for (var timerName in timers) {
                                var message = { id: timerName, program: namespace, sid: s.id, $t: timerName };
                                var duration = timers[timerName];
                                programHost.addTimer(transaction, duration, message);
                            }

                            //delete old messages
                            for (messagePath in messages) {
                                currentMessage = messages[messagePath];
                                if (currentMessage) {
                                    transaction.hdel(messageDirectoryKey, currentMessage.id);
                                }
                            }

                            //delete old action
                            transaction.zrem(actionQueueKey, result[0]);

                            //remove session and message tree state
                            retes.negate(transaction, sid, '$s', documentCopy);
                            for (messagePath in messages) {
                                currentMessage = messages[messagePath];
                                if (currentMessage) {
                                    retem.negate(transaction, sid, '$m', currentMessage);
                                }
                            }

                            //store session 
                            transaction.hset(sessionDirectoryKey, sid, JSON.stringify(s));

                            //assert new session
                            retes.assert(transaction, sid, '$s', s, actions);

                            propagateActions(transaction, partition.seHash, actions, complete);
                        }
                    });
                }
            });
        };

        that.createSession = function (partition, session, complete) {
            var transaction = partition.db.multi();
            var actions = [];
            transaction.hsetnx(sessionDirectoryKey, session.id, JSON.stringify(session));
            if (retes.assert(transaction, session.id, '$s', session, actions)) {
                propagateActions(transaction, partition.seHash, actions, function (err) {
                    if (err) {
                        complete(err);
                    } else {
                        complete();
                    }
                });
            } else {
                complete();
            }
        };

        that.dispatchMessage = function (partition, message, complete) {
            var actions = [];
            var transaction = partition.db.multi();
            var sid = message.sid;

            transaction.hexists(sessionDirectoryKey, sid);
            if (!retem.addssert(transaction, sid, messageDirectoryKey, partition.acHash, '$m', message, actions)) {
                complete('message with id ' + message.id + ' not handled');
            } else {
                propagateActions(transaction, partition.seHash, actions, function (err, result) {
                    if (err) {
                        complete(err);
                    } else if (!result[1]) {
                        complete('message with id ' + message.id + ' already handled');
                    } else if (!result[0]) {
                        actions = [];
                        transaction = partition.db.multi();
                        if (retes.addssert(transaction, sid, sessionDirectoryKey, partition.acHash, '$s', { id: sid }, actions)) {
                            propagateActions(transaction, partition.seHash, actions, function (err) {
                                if (err) {
                                    complete(err);
                                } else {
                                    complete();
                                }
                            });
                        } else {
                            transaction.hsetnx(sessionDirectoryKey, sid, '{ \"id\": \"' + sid + '\"}');
                            transaction.exec(function (err) {
                                if (err) {
                                    complete(err);
                                } else {
                                    complete();
                                }
                            });
                        }
                    } else {
                        complete();
                    }
                });
            }
        };

        that.dispatchTimer = function(partition, transaction, message, complete) {
            var actions = [];
            var sid = message.sid;

            if (!retem.addssert(transaction, sid, messageDirectoryKey, partition.acHash, '$m', message, actions)) {
                complete('message with id ' + message.id + ' not handled');
            } else {
                propagateActions(transaction, partition.seHash, actions, function (err, result) {
                    if (err) {
                        complete(err);
                    } else {
                        complete();
                    }
                });
            }
        };

        that.toJSON = function() {
            return JSON.stringify(rules);
        }

        retes = type('$s');
        retem = type('$m');
        return that;
    };

    var stateChart = function (namespace, mainChart) {
        var that = ruleSet(namespace);
        var superInitialize = that.initialize;

        if (!mainChart) {
            throw 'chart argument is not defined';
        }

        var processChart = function (parentName, parentTriggers, parentStartState, chart, host) {
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
                    processChart(qualifiedStateName, triggers, startState, state.$chart, host);
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
                                rule.run = fork(parseDefinitions(trigger.run, host, namespace), function (pName) {
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

                        that.addRule(triggerName, rule, host);
                    }
                }
            }

            var started = false;
            for (stateName in startState) {
                if (started) {
                    throw 'chart ' + parentName + ' has more than one start state';
                }

                if (parentName) {
                    that.addRule(parentName + '$start', { when: { $s: { label: parentName }}, run: to(stateName)}, host);
                } else {
                    that.addRule('$start', { when: { $s: { $nex: { label: 1 }} }, run: to(stateName)}, host);
                }

                started = true;
            }

            if (!started) {
                throw 'chart ' + parentName + ' has no start state';
            }
        };

        that.initialize  = function (host, parentNamespace) {
            superInitialize(host, parentNamespace);
            processChart(null, null, null, mainChart, host);
        };

        that.toJSON = function() {
            return JSON.stringify(mainChart);
        };

        return that;
    };

    var flowChart = function (namespace, chart) {
        var that = ruleSet(namespace);
        var superInitialize = that.initialize;

        if (!chart) {
            throw 'chart argument is not defined';
        }

        var processChart = function (host) {
            var visited = {};
            var rule;
            var stageName;
            var stage;
            for (stageName in chart) {
                stage = chart[stageName];
                var stageTest = { label: stageName };
                var nextStage;
                if (stage.to) {
                    if (typeof stage.to === 'string') {
                        rule = {};
                        rule.when = { $s: stageTest };
                        nextStage = chart[stage.to];
                        if (nextStage.run) {
                            rule.run = to(stage.to).continueWith(nextStage.run);
                        } else {
                            rule.run = to(stage.to);
                        }
                        that.addRule(stageName + '.' + stage.to, rule, host);
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
                            if (nextStage.run) {
                                rule.run = to(transitionName).continueWith(nextStage.run);
                            } else if (nextStage.run.continueWith) {
                                rule.run = to(transitionName);
                            } else {
                                rule.run = fork(parseDefinitions(nextStage.run, host, namespace));
                            }

                            that.addRule(stageName + '.' + transitionName, rule, host);
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
                    if (stage.run) {
                        rule = { when: { $s: { $nex: { label: 1 }} }, run: to(stageName).continueWith(stage.run)};
                    } else {
                        rule = { when: { $s: { $nex: { label: 1 }} }, run: to(stageName) };
                    }

                    that.addRule('$start.' + stageName, rule, host);
                }
            }
        };

        that.initialize  = function (host, parentNamespace) {
            superInitialize(host, parentNamespace);
            processChart(host);
        };

        that.toJSON = function() {
            return JSON.stringify(chart);
        };

        return that;
    };

    var parseDefinitions = function (definitions, host, parentNamespace) {
        var names = [];
        for (var name in definitions) {
            var currentDefinition = definitions[name];
            var program;
            var index = name.indexOf('$state');
            if (index !== -1) {
                name = name.slice(0, index);
                program = stateChart(name, currentDefinition);
            } else {
                index = name.indexOf('$flow');
                if (index !== -1) {
                    name = name.slice(0, index);
                    program = flowChart(name, currentDefinition);
                } else {
                    program = ruleSet(name, currentDefinition);
                }
            }

            names.push(name);
            program.initialize(host, parentNamespace);
        }

        return names;
    };

    var host = function () {
        var that = {};
        var dbList = [];
        var programDirectory = {};
        var instanceDirectory = {};
        var programList = [];
        var timerSetKey = 't';

        var sd = 'local signature = redis.call("zrange", KEYS[3], 0, 0, "withscores")\n';
        sd += 'if (signature[2] ~= nil and (tonumber(signature[2]) < (tonumber(ARGV[1]) + 100))) then\n';
        sd += '   local newscore = tonumber(ARGV[1]) + 60000\n';
        sd += '   redis.call("zadd", KEYS[3], newscore, signature[1])\n';
        sd += '   local i = 0\n';
        sd += '   local res = { signature[1] }\n';
        sd += '   for token in string.gmatch(signature[1], "([%w%.%-%$_]+)") do\n';
        sd += '       if (i % 2 == 0) then\n';
        sd += '           table.insert(res, token)\n';
        sd += '       else\n';
        sd += '           if (i == 1) then\n';
        sd += '               table.insert(res, redis.call("hget", KEYS[1], token))\n';
        sd += '           else\n';
        sd += '               table.insert(res, redis.call("hget", KEYS[2], token))\n';
        sd += '           end\n';
        sd += '       end\n';
        sd += '       i = i + 1\n';
        sd += '   end\n';
        sd += '   return res\n';
        sd += 'end\n';
        sd += 'return false\n';

        var se = 'local signature = ARGV[1] .. "," .. ARGV[2]\n';
        se += 'for i = 2, #KEYS, 1 do\n';
        se += '   local res = redis.call("zrange", KEYS[i], 0, 0)\n';
        se += '   if (not res[1]) then\n';
        se += '       return false\n';
        se += '   end\n';
        se += '   if (not string.match(KEYS[i], "%$s")) then\n';
        se += '       signature = signature .. "," .. KEYS[i] .. "," .. res[1]\n';
        se += '   end\n';
        se += 'end\n';
        se += 'local res = redis.call("zrank", KEYS[1], signature)\n';
        se += 'if (not res) then\n';
        se += '    redis.call("zadd", KEYS[1], ARGV[3], signature)\n';
        se += '    return true\n';
        se += 'end\n';
        se += 'return false\n';

        var ac = 'local res = redis.call("hsetnx", KEYS[1], ARGV[1], ARGV[2])\n';
        ac += 'if (res == 1) then\n';
        ac += '   redis.call("zadd", KEYS[2], ARGV[3], ARGV[1])\n';
        ac += '   return true\n';
        ac += 'else\n';
        ac += '   res = redis.call("hget", KEYS[1], ARGV[1])\n';
        ac += '   if (res == ARGV[2]) then\n';
        ac += '       return true\n';
        ac += '   end\n';
        ac += '   return false\n';
        ac += 'end\n';

        var pm = 'local res = redis.call("hget", KEYS[1], ARGV[1])\n';
        pm += 'if (not res) then\n';
        pm += '   res = redis.call("hincrby", KEYS[1], "index", 1)\n';
        pm += '   res = res % tonumber(ARGV[2])\n';
        pm += '   redis.call("hset", KEYS[1], ARGV[1], res)\n';
        pm += 'end\n';
        pm += 'return res\n';

        var getSessions = function (programName, query, skip, top, complete) {
        };

        var getHistory = function (programName, sessionId, query, skip, top, complete) {
        };

        var getStatus = function (programName, sessionId, complete) {
        };

        var patchSession = function (programName, document, complete) {
        };

        var getProgram = function (programName, complete) {
            var program = programDirectory[programName];
            if (program) {
                complete(null, JSON.stringify(program));
            } else {
                complete('Could not find ' + programName);
            }
        };

        var resolvePartition = function (program, sid, complete) {
            if (dbList.length === 1) {
                complete(null, dbList[0]);
            } else {
                var instanceKey = program + '.' + sid;
                var currentIndex = instanceDirectory[instanceKey];
                if (currentIndex) {
                    complete(null, dbList[currentIndex]);
                } else {
                    var primary = dbList[0];
                    primary.db.evalsha(primary.pmHash, 1, 'map', instanceKey, dbList.length, function (err, result) {
                        if (err) {
                            complete(err);
                        } else {
                            instanceDirectory[instanceKey] = result;
                            complete(null, dbList[result]);
                        }
                    });
                }
            }
        };

        that.register = function (namespace, program) {
            if (programDirectory[namespace]) {
                throw 'program ' + namespace + ' already registered';
            } else {
                programDirectory[namespace] = program;
                programList.push(program);
            }
        };

        that.createSession = function(namespace, session, complete) {
            var program = programDirectory[namespace];
            if (program) {
                resolvePartition(namespace, session.id , function (err, result) {
                    if (err) {
                        complete(err);
                    } else {
                        program.createSession(result, session, complete);
                    }
                });
            } else {
                complete('Could not find ' + namespace);
            }
        };

        that.post = function (message, complete) {
            var program = programDirectory[message.program];
            if (program) {
                resolvePartition(message.program, message.sid , function (err, result) {
                    if (err) {
                        complete(err);
                    } else {
                        program.dispatchMessage(result, message, complete);
                    }
                });
            } else {
                complete('Could not find ' + message.program);
            }
        };

        that.addTimer = function (transaction, duration ,message) {
            transaction.zadd(timerSetKey, new Date().getTime() + duration * 1000, JSON.stringify(message));
        };

        var loadProcedures = function (databases, index, complete) {
            if (index === databases.length) {
                complete();
                return;
            }

            var dbEntry = {};
            dbList.push(dbEntry);
            dbEntry.dbString = databases[index];
            dbEntry.db = redis.createClient(databases[index]);
            dbEntry.db.script('load', sd, function (err, result) {
                dbEntry.sdHash = result;
                dbEntry.db.script('load', se, function (err, result) {
                    dbEntry.seHash = result;
                    dbEntry.db.script('load', ac, function (err, result) {
                        dbEntry.acHash = result;
                        dbEntry.db.script('load', pm, function (err, result) {
                            dbEntry.pmHash = result;
                            loadProcedures(databases, ++index, complete);
                        });
                    });
                });
            });
        };

        var dispatchProgram = function (index) {
            var space = index % (dbList.length * programList.length);
            var partition = dbList[Math.floor(space / programList.length)];
            var program = programList[space % programList.length];
            program.dispatchAction(that, partition, function (err) {
                if (err) {
                    console.log(err);
                }
                if (index % 20) {
                    dispatchProgram(index + 1);
                }
                else {
                    setTimeout(dispatchProgram, 1, index + 1);
                }
            });
        };


        var dispatchOneTimer = function(partition, timers, index) {
            if (index < timers.length) {
                var message = JSON.parse(timers[index]);
                var program = programDirectory[message.program];
                var transaction = partition.db.multi();
                transaction.zrem(timerSetKey, timers[index]);
                program.dispatchTimer(partition, transaction, message, function(err) {
                   if (err) {
                       console.log(err);
                   } else {
                        dispatchOneTimer(partition, timers, ++index);
                   }
                });
            }
        };

        var dispatchTimers = function(index) {
            var partition = dbList[index % dbList.length];
            partition.db.zrangebyscore(timerSetKey, 0, new Date().getTime(), function(err, result) {
                if (err) {
                    console.log(err);
                } else {
                    dispatchOneTimer(partition, result, 0);
                }
            });

            setTimeout(dispatchTimers, 100, index + 1);
        };

        that.run = function (basePath, databases, start) {
            var port = 5000;
            basePath = basePath || '';

            var app = express();
            app.use(express.bodyParser());

            var stat = require('node-static');
            var fileServer = new stat.Server(__dirname);

            if (basePath !== '' && basePath.indexOf('/') !== 0) {
                basePath = '/' + basePath;
            }

            app.get('/durableVisual.js', function (request, response) {
                request.addListener('end',function () {
                    fileServer.serveFile('/durableVisual.js', 200, {}, request, response);
                }).resume();
            });

            app.get(basePath + '/:program/:sid/admin.html', function (request, response) {
                request.addListener('end',function () {
                    fileServer.serveFile('/admin.html', 200, {}, request, response);
                }).resume();
            });

            app.get(basePath + '/:program/sessions', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                var top;
                var skip;
                var query;
                if (request.query.$top) {
                    top = parseInt(request.query.$top);
                }

                if (request.query.$skip) {
                    skip = parseInt(request.query.$skip);
                }

                if (request.query.$filter) {
                    try {
                        query = JSON.parse(request.query.$filter);
                    }
                    catch (reason) {
                        response.send({ error: 'Invalid Query' }, 500);
                        return;
                    }
                }

                getSessions(request.params.program,
                    query,
                    skip,
                    top,
                    function (err, result) {
                        if (err) {
                            response.send({ error: err }, 500);
                        }
                        else {
                            response.send(result);
                        }
                    });
            });

            app.get(basePath + '/:program/:sid/history', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                var top;
                var skip;
                var query;
                if (request.query.$top) {
                    top = parseInt(request.query.$top);
                }

                if (request.query.$skip) {
                    skip = parseInt(request.query.$skip);
                }

                if (request.query.$filter) {
                    try {
                        query = JSON.parse(request.query.$filter);
                    }
                    catch (reason) {
                        response.send({ error: 'Invalid Query' }, 500);
                        return;
                    }
                }

                getHistory(request.params.program,
                    request.params.sid,
                    query,
                    skip,
                    top,
                    function (err, result) {
                        if (err) {
                            response.send({ error: err }, 500);
                        }
                        else {
                            response.send(result);
                        }
                    });
            });

            app.get(basePath + '/:program/:sid', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                getStatus(request.params.program,
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

            app.post(basePath + '/:program/:sid', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                var message = request.body;
                message.program = request.params.program;
                message.sid = request.params.sid;
                that.post(message, null, null, function (err) {
                    if (err) {
                        response.send({ error: err }, 500);
                    } else {
                        response.send();
                    }
                });
            });

            app.patch(basePath + '/:program/:sid', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                var document = request.body;
                var program = request.params.program;
                document.id = request.params.sid;
                patchSession(program, document, function (err) {
                    if (err) {
                        response.send({ error: err }, 500);
                    } else {
                        response.send();
                    }
                });
            });

            app.get(basePath + '/:program', function (request, response) {
                response.contentType = 'application/json; charset=utf-8';
                var program = request.params.program;
                getProgram(program, function (err, result) {
                    if (err) {
                        response.send({ error: err }, 500);
                    }
                    else {
                        response.send(result);
                    }
                });
            });

            if (!databases) {
                databases = ['/tmp/redis.sock'];
            }

            loadProcedures(databases, 0, function () {
                if (start) {
                    start(that);
                }

                setTimeout(dispatchProgram, 100, 1);
                setTimeout(dispatchTimers, 100, 1);
            });

            app.listen(port, function () {
                console.log('Listening on ' + port);
            });
        };

        return that;
    };

    function run(definitions, basePath, databases, start) {
        var programHost = host();
        parseDefinitions(definitions, programHost);
        programHost.run(basePath, databases, start);
    }

    return {
        promise: promise,
        run: run
    };
}();