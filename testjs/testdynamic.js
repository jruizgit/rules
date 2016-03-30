var stat = require('node-static');
var redis = require('redis');
var d = require('../libjs/durableEngine');

var host = d.host();
var app = d.application(host);
var db = redis.createClient();
var fileServer = new stat.Server(__dirname);

function printAction(actionName) {
    return function(c) {
        c.s.label = actionName;
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

host.saveRuleset = function(rulesetName, rulesetDefinition, complete) {
    db.hset('rulesets', rulesetName, JSON.stringify(rulesetDefinition), complete);
};

host.getAction = function(actionName) {
    return printAction(actionName);
};

app.get('/durableVisual.js', function (request, response) {
    request.addListener('end',function () {
        fileServer.serveFile('/durableVisual.js', 200, {}, request, response);
    }).resume();
});

app.get('/testdynamic.html', function (request, response) {
    request.addListener('end', function () {
        fileServer.serveFile('/testdynamic.html', 200, {}, request, response);
    }).resume();
});

app.get('/:rulesetName/:sid/admin.html', function (request, response) {
    request.addListener('end',function () {
        fileServer.serveFile('/admin.html', 200, {}, request, response);
    }).resume();
});

app.run();