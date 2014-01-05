Durable Rules
=====
Durable Rules provides real-time, consistent and scalable coordination of events. A forward chaining algorithm (A.K.A. Rete) is used to evaluate massive streams of data. With a simple, yet powerful meta-liguistic abstraction you can define simple and complex rulesets. An example to illustrate the point:  

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

![Rete tree](/rete.jpg)

#### Resources  
To learn more:
* [Setup] (http://www.github.com/jruizgit/rules/wiki/setup)
* [Approve Tutorial] (http://www.github.com/jruizgit/rules/wiki/approve-tutorial)
* [Order Tutorial] (http://www.github.com/jruizgit/rules/wiki/order-tutorial)
* [Concepts] (http://www.github.com/jruizgit/rules/wiki/concepts)
