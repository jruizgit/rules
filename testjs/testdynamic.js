var stat = require('node-static');
var bodyParser = require('body-parser');
var express = require('express');    
var d = require('../libjs/durable');

var app = express();
var host = d.getHost();
var fileServer = new stat.Server(__dirname);
app.use(bodyParser.json());

function printAction(actionName) {
    return function(c) {
        c.s.label = actionName;
        console.log('Running ' + actionName);
    }
}

host.getAction = function(actionName) {
    return printAction(actionName);
};

app.get('/:rulesetName/state/:sid', function (request, response) {
    response.contentType = 'application/json; charset=utf-8';
    try {
        response.status(200).send(host.getState(request.params.rulesetName, request.params.sid));
    } catch (reason) {
        response.status(500).send({ error: reason });
    }
});

app.post('/:rulesetName/events/:sid', function (request, response) {
    response.contentType = "application/json; charset=utf-8";
    var message = request.body;
    message.sid = request.params.sid;

    host.post(request.params.rulesetName, message, function(e) {
        if (e) {
            response.status(500).send({ error: e });
        } else {
            response.status(200).send({});   
        }
    });
});

app.get('/:rulesetName/definition', function (request, response) {
    response.contentType = 'application/json; charset=utf-8';
    try {
        response.status(200).send(host.getRuleset(request.params.rulesetName).getDefinition());
    } catch (reason) {
        response.status(500).send({ error: reason });
    }
});

app.post('/:rulesetName/definition', function (request, response) {
    response.contentType = "application/json; charset=utf-8";
    host.setRulesets(request.body , function (e, result) {
        if (e) {
            response.status(500).send({ error: e });
        }
        else {
            response.status(200).send();
        }
    });
});

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

app.listen(5000, function () {
    console.log('Listening on 5000');
}); 
