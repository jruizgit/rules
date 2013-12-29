Durable Rules
=====

Distributed Business Rules Engine  

# Introduction
Ditributed Rules enables real-time, consistent coordination of events. It leverages a variation of the RETE algorithm to evaluate massive streams of data. A simple, yet powerful meta-liguistic abstraction allows defining simple and complex rulesets.

```javascript
var d = require('durable');
d.run({
    approval: {
        r1: {
            whenAll: { 
			    request: { subject: 'approve' },
			    order: { $gt: { amount: 100 } }
            },
            run: deny
        },
        r2: {
            whenAll: { 
			    request: { subject: 'approve' },
                order: { $lte: { amount: 100 } } 				
            },
            run: approve
        }
    }
});

# Principles
1. Rules are the basic building blocks of the system. 
2. A rule is defined as a set of assertions followed by an action. 
3. Assertions can be made on events (messages) and user state. 
4. Actions are executed when a rule is satisfied. 
5. Actions are guaranteed to be executed at least once. 
6. Rules, events and user state are expressed in the JSON type system.
7. Forward chaining (a modification of the RETE algorithm) is used to enable rule inference. 
8. To scale the system out, redis is used as a memory cache to keep the join state.
9. More complex constructs such as state machines and flow charts are transformed into rulesets.

# Topics
Data Model
Events and user state
Assert events and user state
$and, $or 
$gt, $lt, $eq, $lte, $gte
$ex, $nex
Algebra
$all, $any
Ruleset
Assertions
Actions
Inference
Statechart
Triggers
Actions
Flowchart
Actions
Conditions
Paralel
Timers



