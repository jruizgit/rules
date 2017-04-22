Reference Manual
=====
## Table of contents
* [Setup](reference.md#setup)
* [Basics](reference.md#basics)
  * [Rules](reference.md#rules)
  * [Facts](reference.md#facts)
  * [Events](reference.md#events)
  * [State](reference.md#state)
* [Antecedents](reference.md#antecedents)
  * [Simple Filter](reference.md#simple-filter)
  * [Pattern Matching](reference.md#pattern-matching)
  * [Correlated Sequence](reference.md#correlated-sequence)
  * [Lack of Information](reference.md#lack-of-information)
  * [Choice of Sequences](reference.md#choice-of-sequences)
* [Consequents](reference.md#consequents)  
  * [Conflict Resolution](reference.md#conflict-resolution)
  * [Action Windows](reference.md#action-windows)
  * [Async Actions](reference.md#async-actions)
* [Flow Structures](reference.md#flow-structures)
  * [Timers](reference.md#timers) 
  * [Statechart](reference.md#statechart)
  * [Nested States](reference.md#nested-states)
  * [Flowchart](reference.md#flowchart) 

## Setup
durable_rules has been tested in MacOS X, Ubuntu Linux and Windows.
### Redis install
durable.js relies on Redis version 2.8  
 
_Mac_  
1. Download [Redis](http://download.redis.io/releases/redis-2.8.4.tar.gz)   
2. Extract code, compile and start Redis

For more information go to: http://redis.io/download  

_Windows_  
1. Download Redis binaries from [MSTechOpen](https://github.com/MSOpenTech/redis/releases)  
2. Extract binaries and start Redis  

For more information go to: https://github.com/MSOpenTech/redis  

Note: To test applications locally you can also use a Redis [cloud service](reference.md#cloud-setup) 
### Node.js install
durable.js uses Node.js version  0.10.15.    

1. Download [Node.js](http://nodejs.org/dist/v0.10.15)  
2. Run the installer and follow the instructions  
3. The installer will set all the necessary environment variables, so you are ready to go  

For more information go to: http://nodejs.org/download  

### First App
Now that your cache and web server are ready, let's write a simple rule:  

1. Start a terminal  
2. Create a directory for your app: `mkdir firstapp` `cd firstapp`  
3. In the new directory `npm install durable` (this will download durable.js and its dependencies)  
4. In that same directory create a test.js file using your favorite editor  
5. Copy/Paste and save the following code:
  ```javascript
  var d = require('durable');
  d.ruleset('a0', function() {
      whenAll: m.amount < 100
      run: console.log('a0 approved from ' + s.sid)
    
      whenStart: {
          post('a0', {id: 1, sid: 1, amount: 10});
      }
  });
  d.runAll();
  ```
7. In the terminal type `node test.js`  
8. You should see the message: `a0 approved from 1`  

Note 1: If you are using a redis service outside your local host, replace the last line with:
  ```javascript
  d.runAll([{host: 'hostName', port: port, password: 'password'}]);
  ```
Note 2: If you are running in Windows, you will need VS2013 express edition and Python 2.7 for the package to build during npm install. Make sure both the VS build tools and the python directory are in your path. 

[top](reference.md#table-of-contents) 

## Basics
### Rules
A rule is the basic building block of the framework. The rule antecendent defines the conditions that need to be satisfied to execute the rule consequent (action). The 'test' rule in this example illustrates this simple concept.

* `whenAll` and `whenAny` label the antecendent definition of a rule
* `run` and `runAsync` label the consequent definition of a rule 
* `whenStart` labels the action to be taken when starting the ruleset  
  
```javascript
var d = require('durable');

d.ruleset('test', function() {
    // antecedent
    whenAll: m.subject == 'World'
    // consequent
    run: console.log('Hello ' + m.subject)
    // on ruleset start
    whenStart: post('test', {id: 1, sid: 1, subject: 'World'})
});

d.runAll();
```
### Facts
Facts are used to define the knowledge base. Facts are the data stored by the rules engine. Facts can be asserted and retracted.

```javascript
var d = require('durable');
var m = d.m, s = d.s, c = d.c;

d.ruleset('fraudDetection', {
        whenAll: [
            c.first = m.t.eq('purchase'),
            c.second = m.location.neq(c.first.location)
        ],
        count: 2,
        run: function(c) {
            console.log('fraud detected ->' + c.m[0].first.location + ', ' + c.m[0].second.location);
            console.log('               ->' + c.m[1].first.location + ', ' + c.m[1].second.location);
        }
    },
    function (host) {
        host.assert('fraudDetection', {id: 1, sid: 1, t: 'purchase', location: 'US'});
        host.assert('fraudDetection', {id: 2, sid: 1, t: 'purchase', location: 'CA'});
    }
);
d.runAll();
```
[top](reference.md#table-of-contents)  

### Events
Inference based on events is the main purpose of `durable_rules`. What makes events unique is they can only be consumed once by an action. Events are removed from inference sets as soon as they are scheduled for dispatch. The join combinatorics are significantly reduced, thus improving the rule evaluation performance, in some cases, by orders of magnitude.  

Event rules:  
* Events can be posted in the `start` handler via the host parameter.   
* Events be posted in an `action` handler using the context parameter.   
* Events can be posted one at a time or in batches.  
* Events don't need to be retracted.  
* Events can co-exist with facts.  
* The post event operation is idempotent.    

The example below shows how two events will cause only one action to be scheduled, as a given event can only be observed once. You can contrast this with the example in the facts section, which will schedule two actions.

API:  
* `c.post(rulesetName, {event})`  
* `c.postBatch(rulesetName, {event}, {event}...)`  
* `host.post(rulesetName, {event})`  
* `host.postBatch(rulesetName, {event}, {event}...)`  
```javascript
var d = require('durable');
var m = d.m, s = d.s, c = d.c;

d.ruleset('fraudDetection', {
        whenAll: [
            c.first = m.t.eq('purchase'),
            c.second = m.location.neq(c.first.location)
        ],
        run: function(c) {
            console.log('fraud detected ->' + c.first.location + ', ' + c.second.location);
        }
    },
    function (host) {
        host.post('fraudDetection', {id: 1, sid: 1, t: 'purchase', location: 'US'});
        host.post('fraudDetection', {id: 2, sid: 1, t: 'purchase', location: 'CA'});
    }
);
d.runAll();
```

[top](reference.md#table-of-contents)  

## Antecendents
### Simple Filter
Rules are the basic building blocks. All rules have a condition, which defines the events and facts that trigger an action.  
* The rule condition is an expression. Its left side represents an event or fact property, followed by a logical operator and its right side defines a pattern to be matched. By convention events or facts originated by calling post or assert are represented with the `m` name; events or facts originated by changing the context state are represented with the `s` name.  
* The rule action is a function to which the context is passed as a parameter. Actions can be synchronous and asynchronous. Asynchronous actions take a completion function as a parameter.  

Below is an example of the typical rule structure. 

Logical operators:  
* Unary: ~ (does not exist), + (exists)  
* Logical operators: &&, ||  
* Relational operators: < , >, <=, >=, ==, !=  
```javascript
var d = require('durable');

d.ruleset('a0', function() {
    whenAll: m.subject < 100 || m.subject == 'approve' || m.subject == 'ok'
    run: console.log('a0 approved from ' + s.sid)
    
    whenStart: post('a0', {id: 1, sid: 1, subject: 10})
});

d.runAll();
```  
[top](reference.md#table-of-contents)
### Pattern Matching
durable_rules implements a simple pattern matching dialect. Similar to lua, it uses % to escape, which vastly simplifies writing expressions. Expressions are compiled down into a deterministic state machine, thus backtracking is not supported. The expressiveness of the dialect is not as rich as that of ruby, python or jscript. Event processing is O(n) guaranteed (n being the size of the event).  

**Repetition**  
\+ 1 or more repetitions  
\* 0 or more repetitions  
? optional (0 or 1 occurrence)  

**Special**  
() group  
| disjunct  
[] range  
{} repeat  

**Character classes**  
.	all characters  
%a	letters  
%c	control characters  
%d	digits  
%l	lower case letters  
%p	punctuation characters  
%s	space characters  
%u	upper case letters  
%w	alphanumeric characters  
%x	hexadecimal digits  

```javascript
var d = require('durable');

d.ruleset('match', function() {
    whenAll: m.url.matches('(https?://)?([%da-z.-]+)%.[a-z]{2,6}(/[%w_.-]+/?)*') 
    run: console.log('match url ' + m.url)
        
    whenStart: {
        post('match', {id: 1, sid: 1, url: 'https://github.com'});
        post('match', {id: 2, sid: 1, url: 'http://github.com/jruizgit/rul!es'});
        post('match', {id: 3, sid: 1, url: 'https://github.com/jruizgit/rules/reference.md'});
        post('match', {id: 4, sid: 1, url: '//rules'});
        post('match', {id: 5, sid: 1, url: 'https://github.c/jruizgit/rules'});
    }
});

d.runAll();
```  
[top](reference.md#table-of-contents) 
### Correlated Sequence
The ability to express and efficiently evaluate sequences of correlated events or facts represents the forward inference hallmark. The fraud detection rule in the example below shows a pattern of three events: the second event amount being more than 200% the first event amount and the third event amount greater than the average of the other two.  

The `whenAll` label expresses a sequence of events or facts. The assignment operator is used to name events or facts, which can be referenced in subsequent expressions. When referencing events or facts, all properties are available. Complex patterns can be expressed using arithmetic operators.  

Arithmetic operators: +, -, *, /
```javascript
var d = require('durable');

d.ruleset('fraudDetection', function() {
    whenAll: {
        first = m.amount > 100
        second = m.amount > first.amount * 2
        third = m.amount > (first.amount + second.amount) / 2
    }
    run: {
       	console.log('fraud detected -> ' + first.amount);
        console.log('               -> ' + second.amount);
        console.log('               -> ' + third.amount);
    }

    whenStart: {
        host.post('fraudDetection', {id: 1, sid: 1, amount: 200});
        host.post('fraudDetection', {id: 2, sid: 1, amount: 500});
        host.post('fraudDetection', {id: 3, sid: 1, amount: 1000});
    }
});

d.runAll();
```
[top](reference.md#table-of-contents) 
### Lack of Information
In some cases lack of information is meaningful. The `none` function enables such tests. 
```javascript
var d = require('durable');

d.ruleset('fraud6', function() {
    whenAll: {
        first = m.t == 'deposit'
        none(m.t == 'balance')
        third = m.t == 'withrawal'
        fourth = m.t == 'chargeback'
    }
    run: console.log('fraud6 detected ' + first.t + ' ' + third.t + ' ' + fourth.t + ' from ' + s.sid);

    whenStart: {
        post('fraud6', {id: 1, sid: 1, t: 'deposit'});
        post('fraud6', {id: 2, sid: 1, t: 'withrawal'});
        post('fraud6', {id: 3, sid: 1, t: 'chargeback'});
    }
});

d.runAll();
```

[top](reference.md#table-of-contents)  
### Choice of Sequences
durable_rules allows expressing and efficiently evaluating richer events sequences leveraging forward inference. In the example below any of the two event\fact sequences will trigger an action. 

The following two labels can be used and combined to define richer event sequences:  
* whenAll: a set of event or fact patterns. All of them are required to match to trigger an action.  
* whenAny: a set of event or fact patterns. Any one match will trigger an action.  

```javascript
var d = require('durable');

d.ruleset('a8', function() {
    whenAny: {
        whenAll: {
            first = m.subject == 'approve'
            second = m.amount == 1000
        }
        whenAll: { 
            third = m.subject == 'jumbo'
            fourth = m.amount == 10000
        }
    }
    run: {
        if (first) {
            console.log('a8 action from: ' + s.sid + ' ' + first.subject + ' ' + second.amount);     
        } else {
            console.log('a8 action from: ' + s.sid + ' ' + third.subject + ' ' + fourth.amount);        
        }
    }

    whenStart: {
        post('a8', {id: 1, sid: 1, subject: 'approve'});
        post('a8', {id: 2, sid: 1, amount: 1000});
        post('a8', {id: 3, sid: 2, subject: 'jumbo'});
        post('a8', {id: 4, sid: 2, amount: 10000});
    }
});

d.runAll();
```
[top](reference.md#table-of-contents) 
### Conflict Resolution
Event and fact evaluation can lead to multiple actions.The triggering order can be defined by setting the priority (salience) attribute on the rule.

In this example, notice how the last rule is triggered first, as it has the highest priority.
```javascript
var d = require('durable');

d.ruleset('attributes', function() {
    whenAll: m.amount < 300
    pri: 3 
    run: console.log('attributes P3 ->' + m.amount);
        
    whenAll: m.amount < 200
    pri: 2
    run: console.log('attributes P2 ->' + m.amount);     
            
    whenAll: m.amount < 100
    pri: 1
    run: console.log('attributes P1 ->' + m.amount);
       
    whenStart: {
        assert('attributes', {id: 1, sid: 1, amount: 50});
        assert('attributes', {id: 2, sid: 1, amount: 150});
        assert('attributes', {id: 3, sid: 1, amount: 250});
    }
});

d.runAll();
```
[top](reference.md#table-of-contents) 
### Action Windows
durable_rules enables aggregating actions using count windows or time tumbling windows. Tumbling windows are a series of fixed-sized, non-overlapping and contiguous time intervals.  

Summary of rule attributes:  
* count: defines the number of times the rule needs to be satisfied befure scheduling the action.   
* span: defines the tumbling time in seconds between scheduled actions.  

This example generates events at random times and schedules an action every 5 seconds (tumbling window).
```javascript
var d = require('durable');

d.ruleset('t0', function() {
    whenAll: m.count == 0 || timeout('myTimer')
    run: {
        s.count += 1;
        post('t0', {id: s.count, sid: 1, t: 'purchase'});
        startTimer('myTimer', Math.random() * 2 + 1, 't0_' + s.count);
    }

    whenAll: m.t == 'purchase'
    span: 5
    run: console.log('t0 pulse ->' + s.count);

    whenStart: {
        patchState('t0', {sid: 1, count: 0}); 
    }
});

d.runAll();
```

In this example, when the rule is satisfied three times, the action is scheduled.
```javascript
d.ruleset('a15', function() {
    whenAll: {
        first = m.amount < 100
        second = m.subject == 'approve'
    }
    count: 3
    run: console.log('a15 approved ->' + JSON.stringify(m));

    whenStart: {
        postBatch('a15', {id: 1, sid: 1, amount: 10},
                         {id: 2, sid: 1, amount: 10},
                         {id: 3, sid: 1, amount: 10},
                         {id: 4, sid: 1, subject: 'approve'});
        postBatch('a15', {id: 5, sid: 1, subject: 'approve'},
                         {id: 6, sid: 1, subject: 'approve'});
    }
});

d.runAll();
```
[top](reference.md#table-of-contents)  

### Context
Context state is permanent. It is used for controlling the ruleset flow or for storing configuration information. `durable_rules` implements a client cache with LRU eviction policy to reference contexts by id, this helps reducing the combinatorics in joins which otherwise would be used for configuration facts.  

Context rules:
* Context state can be modified in the `start` handler via the host parameter.   
* Context state can modified in an `action` handler simply by modifying the context state object.  
* All events and facts are addressed to a context id.  
* Rules can be written for context changes. By convention the `s` name is used for naming the context state.  
* The right side of a rule can reference a context, the references will be resolved in the Rete tree alpha nodes.  

API:  
* `host.patchState(rulesetName, {state})`  
* `c.state.property = ...`  
```javascript
var d = require('durable');
var m = d.m, s = d.s, c = d.c, add = d.add;

d.ruleset('a8', {
        whenAll: [ m.amount.lt(add(c.s.maxAmount, c.s.refId('global').minAmount)) ],
        run: function(c) {
            console.log('a8 approved ' +  c.m.amount);
        }
    },
    function (host) {
        host.patchState('a8', {sid: 1, maxAmount: 500});
        host.patchState('a8', {sid: 'global', minAmount: 100});
        host.post('a8', {id: 1, sid: 1, amount: 10});
    }
);
d.runAll();
```
[top](reference.md#table-of-contents)  
### Timers
`durable_rules` supports scheduling timeout events and writing rules, which observe such events.  

Timer rules:  
* Timers can be started in the `start` handler via the host parameter.   
* Timers can started in an `action` handler using the context parameter.   
* A timeout is an event. 
* A timeout is raised only once.  
* Timeouts can be observed in rules given the timer name.  
* The start timer operation is idempotent.  

This example shows an event scheduled to be raised after 5 seconds and a rule which reacts to such an event.  

API:  
* `host.startTimer(timerName, seconds)`
* `c.startTimer(timerName, seconds)`  
* `when... timeout(timerName)`  
```javascript
var d = require('durable');
var m = d.m, s = d.s, c = d.c, timeout = d.timeout;

d.ruleset('t1', {
        whenAll: m.start.eq('yes'),
        run: function(c) {
            c.s.start = new Date();
            c.startTimer('myTimer', 5);
        }
    }, {
        whenAll: timeout('myTimer'),
        run: function(c) {
            console.log('t1 end');
            console.log('t1 started ' + c.s.start);
            console.log('t1 ended ' + new Date());
        }
    },
    function (host) {
        host.post('t1', {id: 1, sid: 1, start: 'yes'});
    }
);
d.runAll();
```
[top](reference.md#table-of-contents)  
## Flow Structures
### Statechart
`durable_rules` lets you organize the ruleset flow such that its context is always in exactly one of a number of possible states with well-defined conditional transitions between these states. Actions depend on the state of the context and a triggering event.  

Statechart rules:  
* A statechart can have one or more states.  
* A statechart requires an initial state.  
* An initial state is defined as a vertex without incoming edges.  
* A state can have zero or more triggers.  
* A state can have zero or more states (see [nested states](reference.md#nested-states)).  
* A trigger has a destination state.  
* A trigger can have a rule (absence means state enter).  
* A trigger can have an action.  

The example shows an approval state machine, which detects an anomaly if three consecutive events happen in less than 30 seconds.

API:  
* `with (statechart(rulesetName)) statesBlock`  
* `with (state(stateName)) triggersAndStatesBlock`  
* `to(stateName, [actionBlock]).[ruleAntecedent, actionBlock]`   
```javascript
var d = require('durable');
var m = d.m, s = d.s, c = d.c, timeout = d.timeout;

d.statechart('fraud', {
    start: {
        to: 'metering',
        whenAll: m.amount.gt(100),
        run: function (c) { c.startTimer('velocity', 30); }
    },
    metering: [{
        to: 'fraud',
        count: 3,
        whenAll: m.amount.gt(100),
        run: function (c) { console.log('fraud detected'); }
    }, {
        to: 'cleared',
        whenAll: timeout('velocity'),
        run: function (c) { console.log('fraud cleared'); }
    }],
    cleared: {},
    fraud: {},
    whenStart: function (host) {
        host.post('fraud', {id: 1, sid: 1, amount: 200});
        host.post('fraud', {id: 2, sid: 1, amount: 200});
        host.post('fraud', {id: 3, sid: 1, amount: 200});
        host.post('fraud', {id: 4, sid: 1, amount: 200});
    }
});

d.runAll();
```
[top](reference.md#table-of-contents)  
### Nested States
`durable_rules` supports nested states. Which implies that, along with the [statechart](reference.md#statechart) description from the previous section, most of the [UML statechart](http://en.wikipedia.org/wiki/UML_state_machine) semantics is supported. If a context is in the nested state, it also (implicitly) is in the surrounding state. The state machine will attempt to handle any event in the context of the substate, which conceptually is at the lower level of the hierarchy. However, if the substate does not prescribe how to handle the event, the event is not discarded, but it is automatically handled at the higher level context of the superstate.

The example below shows a statechart, where the `canceled` transition is reused for both the `enter` and the `process` states. 

```javascript
var d = require('durable');
var m = d.m, s = d.s, c = d.c;

d.statechart('a6', {
    work: [{
        enter: {
            to: 'process',
            whenAll: m.subject.eq('enter'),
            run: function (c) { console.log('a6 continue process'); }
        },
        process: {
            to: 'process',
            whenAll: m.subject.eq('continue'),
            run: function (c) { console.log('a6 processing'); }
        }
    }, {
        to: 'canceled',
        pri: 1,
        whenAll: m.subject.eq('cancel'),
        run: function (c) { console.log('a6 canceling');}
    }],
    canceled: {},
    whenStart: function (host) {
        host.post('a6', {id: 1, sid: 1, subject: 'enter'});
        host.post('a6', {id: 2, sid: 1, subject: 'continue'});
        host.post('a6', {id: 3, sid: 1, subject: 'continue'});
        host.post('a6', {id: 4, sid: 1, subject: 'cancel'});
    }
});
d.runAll();
```
[top](reference.md#table-of-contents)
### Flowchart
In addition to [statechart](reference.md#statechart), flowchart is another way for organizing a ruleset flow. In a flowchart each stage represents an action to be executed. So (unlike the statechart state), when applied to the context state, it results in a transition to another stage.  

Flowchart rules:  
* A flowchart can have one or more stages.  
* A flowchart requires an initial stage.  
* An initial stage is defined as a vertex without incoming edges.  
* A stage can have an action.  
* A stage can have zero or more conditions.  
* A condition has a rule and a destination stage.  

API:  
* `flowchart(rulesetName) stageConditionBlock`  
* `with (stage(stageName)) [actionConditionbBlock]` 
* `run(actionFunction)`  
* `to(stageName).[rule]`  
Note: conditions have to be defined immediately after the stage definition  
```javascript
var d = require('durable');
var m = d.m, s = d.s, c = d.c;

d.flowchart('a3', {
    input: {
        request: { whenAll: m.subject.eq('approve').and(m.amount.lte(1000)) }, 
        deny: { whenAll: m.subject.eq('approve').and(m.amount.gt(1000)) }, 
    },
    request: {
        run: function (c) {
            console.log('a3_0 request approval from: ' + c.s.sid);
            if (c.s.status) 
                c.s.status = 'approved';
            else
                c.s.status = 'pending';
        },
        approve: { whenAll: s.status.eq('approved') },
        deny: { whenAll: m.subject.eq('denied') },
        request: { whenAll: m.subject.eq('approved') }
    },
    approve: {
        run: function (c) { console.log('a3 approved from: ' + c.s.sid); }
    },
    deny: {
        run: function (c) { console.log('a3 denied from: ' + c.s.sid); }
    },
    whenStart: function (host) {
        host.post('a3', {id: 1, sid: 1, subject: 'approve', amount: 100});
        host.post('a3', {id: 2, sid: 1, subject: 'approved'});
        host.post('a3', {id: 3, sid: 2, subject: 'approve', amount: 100});
        host.post('a3', {id: 4, sid: 2, subject: 'denied'});
        host.post('a3', {id: 5, sid: 3, subject: 'approve', amount: 10000});
    }
});
d.runAll();
```
[top](reference.md#table-of-contents)  

 

