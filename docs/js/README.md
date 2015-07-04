  
# durable_rules    
#### for real time analytics
---
durable_rules is a polyglot micro-framework for real-time, consistent and scalable coordination of events. With durable_rules you can track and analyze information about things that happen (events) by combining data from multiple sources to infer more complicated circumstances.

A full forward chaining implementation (A.K.A. Rete) is used to evaluate facts and massive streams of events in real time. A simple, yet powerful meta-liguistic abstraction lets you define simple and complex rulesets, such as flowcharts, statecharts, nested statecharts and time driven flows. 

The durable_rules core engine is implemented in C, which enables ultra fast rule evaluation and inference as well as muti-language support. durable_rules relies on state of the art technologies: [Node.js](http://www.nodejs.org) to host rulesets. [Redis](http://www.redis.io) to cache inference state. This allows for fault tolerant execution and scale-out without giving up performance.

### Example 1  

durable_rules is simple: to define a rule, all you need to do is describe the event or fact pattern to match (antecedent) and the action to take (consequent).  

In this example the rule can be triggered by posting `{"id": 1, "subject": "World"}` to url `http://localhost:5000/test/1`.  

```javascript
var d = require('durable');

with (d.ruleset('test')) {
    // antecedent
    whenAll(m.subject.eq('World'), function (c) {
        // consequent
        console.log('Hello ' + c.m.subject);
    });
} 
d.runAll();
```

### Example 2  

Let’s consider a couple of fictitious fraud rules used in bank account management.  
Note: I'm paraphrasing the example presented in this [article](https://www.packtpub.com/books/content/drools-jboss-rules-50complex-event-processing).  

1. If there are two debit requests greater than 200% the average monthly withdrawal amount in a span of 2 minutes, flag the account as medium risk.
2. If there are three consecutive increasing debit requests, withdrawing more than 70% the average monthly balance in a span of three minutes, flag the account as high risk.

```javascript
var d = require('durable');

with (d.ruleset('fraudDetection')) {
    // compute monthly averages
    whenAll(span(86400), or(m.t.eq('debitCleared'), m.t.eq('creditCleared')), 
    function(c) {
        var debitTotal = 0;
        var creditTotal = 0;
        for (var i = 0; i < c.m.length; ++i) {
            if (c.m[i].t === 'debitCleared') {
                debitTotal += c.m[i].amount;
            } else {
                creditTotal += c.m[i].amount;
            }
        }

        c.s.balance = c.s.balance - debitTotal + creditTotal;
        c.s.avgBalance = (c.s.avgBalance * 29 + c.s.balance) / 30;
        c.s.avgWithdraw = (c.s.avgWithdraw * 29 + debitTotal) / 30;
    });

    // medium risk rule
    whenAll(c.first = and(m.t.eq('debitRequest'), 
                          m.amount.gt(c.s.avgWithdraw.mul(2))),
            c.second = and(m.t.eq('debitRequest'),
                           m.amount.gt(c.s.avgWithdraw.mul(2)),
                           m.stamp.gt(c.first.stamp),
                           m.stamp.lt(c.first.stamp.add(120))),
    function(c) {
        console.log('Medium risk');
    });

    // high risk rule 
    whenAll(c.first = m.t.eq('debitRequest'),
            c.second = and(m.t.eq('debitRequest'),
                           m.amount.gt(c.first.amount),
                           m.stamp.lt(c.first.stamp.add(180))),
            c.third = and(m.t.eq('debitRequest'),
                          m.amount.gt(c.second.amount),
                          m.stamp.lt(c.first.stamp.add(180))),
            s.avgBalance.lt(add(c.first.amount, c.second.amount, c.third.amount).div(0.7)),
    function(c) {
        console.log('High risk');
    });
}

d.runAll();
```

### Example 3  

durable_rules can also be used to solve traditional production bussiness rules problems. The example below is the 'Miss Manners' benchmark. Miss Manners has decided to throw a party. She wants to seat her guests such that adjacent guests are of opposite sex and share at least one hobby.  

To improve readability, with durable_rules, the ruleset flow structure can be defined using a statechart. The benchmark results compare well with other business rules systems both from an execution time as well as memory utilization perspective.  

```javascript
var d = require('durable');

with (d.statechart('missManners')) {
    with (state('start')) {
        to('assign').whenAll(m.t.eq('guest'), 
        function(c) {
            c.s.count = 0;
            c.s.gcount = 1000;
            c.assert({t: 'seating',
                      id: c.s.gcount,
                      tid: c.s.count,
                      pid: 0,
                      path: true,
                      leftSeat: 1,
                      leftGuestName: c.m.name,
                      rightSeat: 1,
                      rightGuestName: c.m.name});
            c.assert({t: 'path',
                      id: c.s.gcount + 1,
                      pid: c.s.count,
                      seat: 1,
                      guestName: c.m.name});
            c.s.count += 1;
            c.s.gcount += 2;
            console.log('assign ' + c.m.name);
        });
    }
    with (state('assign')) {
        to('make').whenAll(c.seating = and(m.t.eq('seating'), 
                                           m.path.eq(true)),
                           c.rightGuest = and(m.t.eq('guest'), 
                                              m.name.eq(c.seating.rightGuestName)),
                           c.leftGuest = and(m.t.eq('guest'), 
                                             m.sex.neq(c.rightGuest.sex), 
                                             m.hobby.eq(c.rightGuest.hobby)),
                           none(and(m.t.eq('path'),
                                    m.pid.eq(c.seating.tid),
                                    m.guestName.eq(c.leftGuest.name))),
                           none(and(m.t.eq('chosen'),
                                    m.cid.eq(c.seating.tid),
                                    m.guestName.eq(c.leftGuest.name),
                                    m.hobby.eq(c.rightGuest.hobby))), 
        function(c) {
            c.assert({t: 'seating',
                      id: c.s.gcount,
                      tid: c.s.count,
                      pid: c.seating.tid,
                      path: false,
                      leftSeat: c.seating.rightSeat,
                      leftGuestName: c.seating.rightGuestName,
                      rightSeat: c.seating.rightSeat + 1,
                      rightGuestName: c.leftGuest.name});
            c.assert({t: 'path',
                      id: c.s.gcount + 1,
                      pid: c.s.count,
                      seat: c.seating.rightSeat + 1,
                      guestName: c.leftGuest.name});
            c.assert({t: 'chosen',
                      id: c.s.gcount + 2,
                      cid: c.seating.tid,
                      guestName: c.leftGuest.name,
                      hobby: c.rightGuest.hobby});
            c.s.count += 1;
            c.s.gcount += 3;
        }); 
    }
    with (state('make')) {
        to('make').whenAll(cap(1000),
                           c.seating = and(m.t.eq('seating'),
                                           m.path.eq(false)),
                           c.path = and(m.t.eq('path'),
                                        m.pid.eq(c.seating.pid)),
                           none(and(m.t.eq('path'),
                                    m.pid.eq(c.seating.tid),
                                    m.guestName.eq(c.path.guestName))), 
        function(c) {
            for (var i = 0; i < c.m.length; ++i) {
                var frame = c.m[i];
                c.assert({t: 'path',
                          id: c.s.gcount,
                          pid: frame.seating.tid,
                          seat: frame.path.seat,
                          guestName: frame.path.guestName});
                c.s.gcount += 1;
            }
        });

        to('check').whenAll(pri(1), and(m.t.eq('seating'), m.path.eq(false)), 
        function(c) {
            c.retract(c.m);
            c.m.id = c.s.gcount;
            c.m.path = true;
            c.assert(c.m);
            c.s.gcount += 1;
            console.log('path sid: ' + c.m.tid + ', pid: ' + c.m.pid + ', left: ' + c.m.leftGuestName + ', right: ' + c.m.rightGuestName);
        });
    }
    with (state('check')) {
        to('end').whenAll(c.lastSeat = m.t.eq('lastSeat'),
                          and(m.t.eq('seating'), m.rightSeat.eq(c.lastSeat.seat)), 
        function(c) {
            console.log('end');
        });
        to('assign');
    }
    
    state('end');
}
```
### To Learn More  
Reference Manual:  
* [Ruby](https://github.com/jruizgit/rules/blob/master/docs/rb/reference.md)  
* [Python](https://github.com/jruizgit/rules/blob/master/docs/py/reference.md)  
* [JavaScript](https://github.com/jruizgit/rules/blob/master/docs/js/reference.md)  

Blog:  
* [Polyglot (03/2015)](http://jruizblog.com/2015/03/02/polyglot/)  
* [Rete_D (02/2015)](http://jruizblog.com/2015/02/23/rete_d/)
* [Boosting Performance with C (08/2014)](http://jruizblog.com/2014/08/19/boosting-performance-with-c/)
* [Rete Meets Redis (02/2014)](http://jruizblog.com/2014/02/02/rete-meets-redis/)
* [Inference: From Expert Systems to Cloud Scale Event Processing (01/2014)](http://jruizblog.com/2014/01/27/event-processing/)

