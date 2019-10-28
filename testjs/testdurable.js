var d = require('../libjs/durable');

var redisLib = require('redis');

var provideDurability = function(host, redisHostName, port, options) {
    var redisClient = redisLib.createClient(port, redisHostName, options);

    var getHsetName = function(ruleset, sid) {
        return 'h!' + ruleset + '!' + sid;
    }

    var getListName = function(ruleset, sid) {
        return 'l!' + ruleset + '!' + sid;
    }

    var getSsetName = function(ruleset) {
        return 's!' + ruleset;
    }

    var formatMessage = function (actionType, content) {
        return actionType + ',' + content;
    }

    var formatMessages = function(results) {
        if (!results) {
            return 0;
        }

        var messages = '[';
        for (var i =0; i < results.length; ++i) {
            messages += results[i];
            if (i < results.length - 1) {
                messages += ',';
            }
        }

        messages += ']';
        return messages;
    }

    var storeMessageCallback = function(ruleset, sid, mid, actionType, content) {
        try {
            redisClient.hset(getHsetName(ruleset,sid), mid, formatMessage(actionType, content));
            return 0;
        } catch (reason) {
            return 601;
        }
    }

    var deleteMessageCallback = function(ruleset, sid, mid) {
        try {
            redisClient.hdel(getHsetName(ruleset,sid), mid);
            return 0;
        } catch (reason) {
            return 602;
        }
    }

    var queueMessageCallback = function(ruleset, sid, actionType, content) {
        try {
            redisClient.zscore(getSsetName(ruleset), sid, function(err, result) {
                if (!err && !result) {
                    var current = new Date() - 0;
                    redisClient.zadd(getSsetName(ruleset), current, sid);
                }
            });
                
            redisClient.rpush(getListName(ruleset,sid), formatMessage(actionType, content));
            return 0;
        } catch (reason) {
            return 603;
        }
    }

    var getQueuedMessagesCallback = function(ruleset, sid) {
        try {
            var current = new Date() - 0;
            redisClient.zadd(getSsetName(ruleset), current + 5000, sid, function(err, result) {
                if (!err) {         
                    redisClient.lrange(getListName(ruleset,sid), 0, -1, function(err, messages) {
                        if (!err && messages.length) {
                            redisClient.del(getListName(ruleset,sid), function(err) {
                                host.completeGetQueuedMessages(ruleset, sid, formatMessages(messages));
                            });
                        }
                    });
                }
            });

            return 0;
        } catch (reason) {
            return 604;
        }
    }

    var getIdleStateCallback = function(ruleset) {
        try {
            var current = new Date() - 0;
            redisClient.zrangebyscore(getSsetName(ruleset), 0, current, function(err, results) {
                if (!err && results.length) {
                    var sid = results[0];
                    redisClient.hvals(getHsetName(ruleset, sid), function(err, messages) {
                        if (!err) {
                            host.completeGetIdleState(ruleset, sid, formatMessages(messages));
                        }
                    });
                }
            });

            return 0;
        } catch (reason) {
            console.log(reason);
            return 606;
        }
    }

    host.setStoreMessageCallback(storeMessageCallback);
    host.setDeleteMessageCallback(deleteMessageCallback);
    host.setQueueMessageCallback(queueMessageCallback);
    host.setGetIdleStateCallback(getIdleStateCallback);
    host.setGetQueuedMessagesCallback(getQueuedMessagesCallback);
}

provideDurability(d.getHost());

d.ruleset('test', function() {
    whenAll: {
        first = m.subject == 'Hello'
        second = m.subject == 'World'
    }
    run: {
        if (!s.count) {
            s.count = 1;
        } else {
            s.count = s.count + 1;
        }

        console.log(first.subject + ' ' + second.subject + ' from: ' + first.sid + ' count: ' + s.count);
    }
});

d.post('test', {subject: 'Hello', sid: 'a'});
d.post('test', {subject: 'World', sid: 'b'});

d.post('test', {subject: 'Hello', sid: 'b'});
d.post('test', {subject: 'World', sid: 'a'});
