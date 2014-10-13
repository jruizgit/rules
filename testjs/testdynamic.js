var stat = require('node-static');
var redis = require('redis');
var d = require('../libjs/durable');

var host = d.host(['/tmp/redis.sock']);
var app = d.application(host);
var db = redis.createClient('/tmp/redis.sock');
var fileServer = new stat.Server(__dirname);

function printAction(actionName) {
    return function(s) {
        console.log('Executing ' + actionName);
    }
}

host.loadRuleset = function(rulesetName, complete) {
    db.hget('rulesets', rulesetName, function(err, result) {
        if (err) {
            complete(err);
        } else {
            complete(null, JSON.parse(result));
        }
    });
};

host.getAction = function(actionName) {
    return printAction(actionName);
}

app.post('/:rulesetName', function (request, response) {
    response.contentType = "application/json; charset=utf-8";
    try {
        host.registerRulesets(null, request.body);
        db.hset('rulesets', request.params.rulesetName, JSON.stringify(request.body), function(err) {
            if (err)
                response.send({ error: err }, 500);
            else
                response.send();
        });
    } catch (err) {
        response.send({ error: err + '' }, 500);
    }
});

app.get('/testdynamic.html', function (request, response) {
    request.addListener('end', function () {
        fileServer.serveFile('/testdynamic.html', 200, {}, request, response);
    }).resume();
});

app.run();