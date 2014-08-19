Durable Rules
=====
Durable Rules provides real-time, consistent and scalable coordination of events. With Durable Rules you can track and analyze information about things that happen (events) by combining data from multiple sources to infer more complicated circumstances.

A forward chaining algorithm (A.K.A. Rete) is used to evaluate massive streams of data. A simple, yet powerful meta-liguistic abstraction lets you define simple and complex rulesets. Below is an example on how easy it is to define a couple of rules that act on incoming web messages.

```javascript
var d = require('durable');
d.run({
    approve: {
        r1: {
            when: { $lt: { amount: 1000 } },
            run: function(s) { s.status = 'pending' }
        },
        r2: {
            whenAll: {
                $m: { subject: 'approved' },
                $s: { status: 'pending' }
            },
            run: function(s) { console.log('approved'); }
        }
    }
});
```

Durable Rules relies on state of the art technologies:

* [Node.js](http://www.nodejs.org) is used as the host. This allows leveraging the vast amount of available libraries.
* Inference state is cached using [Redis](http://www.redis.io), which lets scaling out without giving up performance.
* A web client based on [D3.js](http://www.d3js.org) provides powerful data visualization and test tools.

#### Resources
To learn more:
* [Setup](https://github.com/jruizgit/rules/blob/master/setup.md)
* [Tutorial](https://github.com/jruizgit/rules/blob/master/tutorial.md)
* [Concepts](https://github.com/jruizgit/rules/blob/master/concepts.md)  
 
Blog:
* [Boosting Performance with C (08/2014)](http://jruizblog.com/2014/08/19/boosting-performance-with-c/)
* [Rete Meets Redis (02/2014)](http://jruizblog.com/2014/02/02/rete-meets-redis/)
* [Inference: From Expert Systems to Cloud Scale Event Processing (01/2014)](http://jruizblog.com/2014/01/27/event-processing/)



