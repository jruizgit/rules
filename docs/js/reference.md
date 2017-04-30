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
  * [Nested Objects](reference.md#nested-objects)
  * [Lack of Information](reference.md#lack-of-information)
  * [Choice of Sequences](reference.md#choice-of-sequences)
* [Consequents](reference.md#consequents)  
  * [Conflict Resolution](reference.md#conflict-resolution)
  * [Action Batches](reference.md#action-batches)
  * [Tumbling Window](reference.md#tumbling-window)
  * [Async Actions](reference.md#async-actions)
  * [Unhandled Exceptions](reference.md#unhandled-exceptions)
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
      run: console.log('a0 approved')
    
      whenStart: {
          post('a0', { amount: 10 });
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
A rule is the basic building block of the framework. The rule antecendent defines the conditions that need to be satisfied to execute the rule consequent (action). By convention `m` represents the data to be evaluated by a given rule.

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
    whenStart: post('test', { subject: 'World' })
});

d.runAll();
```
### Facts
Facts represent the data that defines a knowledge base. After facts are asserted as JSON objects. Facts are stored until they are retracted. When a fact satisfies a rule antecedent, the rule consequent is executed.

```javascript
var d = require('durable');

d.ruleset('animal', function() {
    // will be triggered by 'Kermit eats flies'
    whenAll: m.verb == 'eats' && m.predicate == 'flies' 
    run: assert({ subject: m.subject, verb: 'is', predicate: 'frog' })

    whenAll: m.verb == 'eats' && m.predicate == 'worms' 
    run: assert({ subject: m.subject, verb: 'is', predicate: 'bird' })

    // will be chained after asserting 'Kermit is frog'
    whenAll: m.verb == 'is' && m.predicate == 'frog' 
    run: assert({ subject: m.subject, verb: 'is', predicate: 'green'})

    whenAll: m.verb == 'is' && m.predicate == 'bird' 
    run: assert({ subject: m.subject, verb: 'is', predicate: 'black'})

    whenAll: +m.subject
    run: console.log('fact: ' + m.subject + ' ' + m.verb + ' ' + m.predicate)

    whenStart: {
        assert('animal', { subject: 'Kermit', verb: 'eats', predicate: 'flies' });
    }
});

d.runAll();
```

Facts can also be asserted using the http API. For the example above, run the following command:  

<sub>`curl -H "content-type: application/json" -X POST -d '{"subject": "Tweety", "verb": "eats", "predicate": "worms"}' http://localhost:5000/animal/facts`</sub>

[top](reference.md#table-of-contents)  

### Events
Events can be posted to and evaluated by rules. An event is an ephemeral fact, that is, a fact retracted right before executing a consequent. Thus, events can only be observed once. Events are stored until they are observed. 

```javascript
var d = require('durable');

d.ruleset('risk', function() {
    whenAll: {
        first = m.t == 'purchase'
        second = m.location != first.location
    }
    // the event pair will only be observed once
    run: console.log('fraud detected ->' + first.location + ', ' + second.location)
   
    whenStart: {
        // 'post' submits events, try 'assert' instead and to see differt behavior
        post('risk', { t: 'purchase', location: 'US' });
        post('risk', { t: 'purchase', location: 'CA' });
    }
});

d.runAll();
```

Events can be posted using the http API. When the example above is listening, run the following commands:  

<sub>`curl -H "content-type: application/json" -X POST -d '{"t": "purchase", "location": "BR"}' http://localhost:5000/risk/events`</sub>  
<sub>`curl -H "content-type: application/json" -X POST -d '{"t": "purchase", "location": "JP"}' http://localhost:5000/risk/events`</sub>  

[top](reference.md#table-of-contents)  
### State
Context state is available when a consequent is executed. The same context state is passed across rule execution. Context state is stored until it is deleted. Context state changes can be evaluated by rules. By convention `s` represents the state to be evaluated by a rule.

```javascript
var d = require('durable');

d.ruleset('flow', function() {
    // state condition uses 's'
    whenAll: s.status == 'start'
    run: {
        // state update on 's'
        s.status = 'next';
        console.log('start');
    }

    whenAll: s.status == 'next'
    run: {
        s.status = 'last';
        console.log('next');
    }

    whenAll: s.status == 'last'
    run: {
        s.status = 'end';
        console.log('last');
        // deletes state at the end
        deleteState();
    }

    // modifies default context state
    whenStart: patchState('flow', { status: 'start' })
});

d.runAll();
```
State can also be retrieved and modified using the http API. When the example above is running, try the following commands:  
<sub>`curl -H "content-type: application/json" -X POST -d '{"state": "next"}' http://localhost:5000/flow/state`</sub>  

[top](reference.md#table-of-contents)  

## Antecendents
### Simple Filter
A rule antecedent is an expression. The left side of the expression represents an event or fact property. The right side defines a pattern to be matched. By convention events or facts are represented with the `m` name. Context state are represented with the `s` name.  

Logical operators:  
* Unary: ~ (does not exist), + (exists)  
* Logical operators: &&, ||  
* Relational operators: < , >, <=, >=, ==, !=  

```javascript
var d = require('durable');

d.ruleset('expense', function() {
    whenAll: m.subject == 'approve' || m.subject == 'ok'
    run: console.log('Approved')
    
    whenStart: post('expense', { subject: 'approve' })
});

d.runAll();
```  
[top](reference.md#table-of-contents)
### Pattern Matching
durable_rules implements a simple pattern matching dialect. Similar to lua, it uses % to escape, which vastly simplifies writing expressions. Expressions are compiled down into a deterministic state machine, thus backtracking is not supported. Event processing is O(n) guaranteed (n being the size of the event).  

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
        post('match', { url: 'https://github.com' });
        post('match', { url: 'http://github.com/jruizgit/rul!es' });
        post('match', { url: 'https://github.com/jruizgit/rules/reference.md' });
        post('match', { url: '//rules'});
        post('match', { url: 'https://github.c/jruizgit/rules' });
    }
});

d.runAll();
```  
[top](reference.md#table-of-contents) 
### Correlated Sequence
Rules can be used to efficiently evaluate sequences of correlated events or facts. The fraud detection rule in the example below shows a pattern of three events: the second event amount being more than 200% the first event amount and the third event amount greater than the average of the other two.  

The `whenAll` label expresses a sequence of events or facts. The assignment operator is used to name events or facts, which can be referenced in subsequent expressions. When referencing events or facts, all properties are available. Complex patterns can be expressed using arithmetic operators.  

Arithmetic operators: +, -, *, /
```javascript
var d = require('durable');

d.ruleset('risk', function() {
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
        host.post('risk', { amount: 200 });
        host.post('risk', { amount: 500 });
        host.post('risk', { amount: 1000 });
    }
});

d.runAll();
```
[top](reference.md#table-of-contents)  

### Nested Objects
Queries on nested events or facts are also supported. The `.` notation is used for defining conditions on properties in nested objects.  

```javascript
var d = require('durable');

d.ruleset('expense4', function() {
    // use the '.' notation to match properties in nested objects
    whenAll: {
        bill = m.t == 'bill' && m.invoice.amount > 50
        account = m.t == 'account' && m.payment.invoice.amount == bill.invoice.amount
    }
    run: {
        console.log('bill amount ->' + bill.invoice.amount);
        console.log('account payment amount ->' + account.payment.invoice.amount);
    }

    whenStart: {
        // one level of nesting
        post('expense4', {t: 'bill', invoice: {amount: 100}});  

        // two levels of nesting
        post('expense4', {t: 'account', payment: {invoice: {amount: 100}}}); 
    }
});

d.runAll();
```
[top](reference.md#table-of-contents)  

### Lack of Information
In some cases lack of information is meaningful. The `none` function can be used in rules with correlated sequences to evaluate the lack of information.
```javascript
var d = require('durable');

d.ruleset('risk', function() {
    whenAll: {
        first = m.t == 'deposit'
        none(m.t == 'balance')
        third = m.t == 'withrawal'
        fourth = m.t == 'chargeback'
    }
    run: console.log('fraud detected ' + first.t + ' ' + third.t + ' ' + fourth.t);

    whenStart: {
        post('risk', { t: 'deposit' });
        post('risk', { t: 'withrawal' });
        post('risk', { t: 'chargeback' });
    }
});

d.runAll();
```

[top](reference.md#table-of-contents)  
### Choice of Sequences
durable_rules allows expressing and efficiently evaluating richer events sequences In the example below any of the two event\fact sequences will trigger an action. 

The following two labels can be used and combined to define richer event sequences:  
* whenAll: a set of event or fact patterns. All of them are required to match to trigger an action.  
* whenAny: a set of event or fact patterns. Any one match will trigger an action.  

```javascript
var d = require('durable');

d.ruleset('expense', function() {
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
            console.log('Approved ' + first.subject + ' ' + second.amount);     
        } else {
            console.log('Approved ' + third.subject + ' ' + fourth.amount);        
        }
    }

    whenStart: {
        post('expense', { subject: 'approve' });
        post('expense', { amount: 1000 });
        post('expense', { subject: 'jumbo' });
        post('expense', { amount: 10000 });
    }
});

d.runAll();
```
[top](reference.md#table-of-contents) 

## Consequents
### Conflict Resolution
Event and fact evaluation can lead to multiple consequents. The triggering order can be controlled by using the `pri` (salience) attribute. Actions with lower value are executed first. The default value for all actions is 0.

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
        assert('attributes', { amount: 50 });
        assert('attributes', { amount: 150 });
        assert('attributes', { amount: 250 });
    }
});

d.runAll();
```
[top](reference.md#table-of-contents) 
### Action Batches
When a high number of events or facts satisfy a consequent, the consequent results can be delivered in batches.

* count: defines the exact number of times the rule needs to be satisfied before scheduling the action.   
* cap: defines the maximum number of times the rule needs to be satisfied before scheduling the action.  

This example batches exaclty three approvals and caps the number of rejects to two:  
```javascript
var d = require('durable');

d.ruleset('expense', function() {
    // this rule will trigger as soon as three events match the condition
    whenAll: m.amount < 100
    count: 3
    run: console.log('approved ' + JSON.stringify(m));

    // this rule will be triggered when 'expense' is asserted batching at most two results
    whenAll: {
        expense = m.amount >= 100
        approval = m.review == true
    }
    cap: 2
    run: console.log('rejected ' + JSON.stringify(m));

    whenStart: {
        postBatch('expense', { amount: 10 },
                             { amount: 20 },
                             { amount: 100 },
                             { amount: 30 },
                             { amount: 200 },
                             { amount: 400 });
        assert('expense', { review: true })
    }
});

d.runAll();
```
[top](reference.md#table-of-contents)  
### Tumbling Window
Actions can also be batched using time tumbling windows. Tumbling windows are a series of fixed-sized, non-overlapping and contiguous time intervals.  

* span: defines the tumbling time in seconds between scheduled actions.  

This example generates events with random amounts. An action is scheduled every 5 seconds (tumbling window).
```javascript
var d = require('durable');

d.ruleset('risk', function() {
    // the action will be called every 5 seconds
    whenAll: m.amount > 100
    span: 5
    run: console.log('high value purchases ->' + JSON.stringify(m));

    whenStart: {
        // will post an event every second
        var callback = function() {
            post('risk', { amount: Math.random() * 200 });
            setTimeout(callback, 1000); 
        }
        callback();
    }
});

d.runAll();
```
[top](reference.md#table-of-contents)  

### Async Actions  
The consequent action can be asynchronous. When the action is finished, the `complete` function has to be called. By default an action is considered abandoned after 5 seconds. This value can be changed by returning a different number in the action function or extended by calling `renewActionLease`.

```javascript
var d = require('durable');

d.ruleset('flow', function() {
    whenAll: s.state == 'first'
    // runAsync labels an async action
    runAsync: {
        setTimeout(function() {
            s.state = 'second';
            console.log('first completed');
            
            // completes the async action after 3 seconds
            complete();
        }, 3000);
    }

    whenAll: s.state == 'second'
    runAsync: {
        setTimeout(function() {
            s.state = 'third';
            console.log('second completed');
            
            // completes the async action after 6 seconds
            // use the first argument to signal an error
            complete('error detected');
        }, 6000);
        
        // overrides the 5 second default abandon timeout
        return 10;
    }

    whenStart: {
        patchState('flow', { state: 'first' });
    }
});

d.runAll();
```
[top](reference.md#table-of-contents)  
### Unhandled Exceptions  
When exceptions are not handled by actions, they are stored in the context state. This enables writing exception handling rules.

```javascript
var d = require('durable');

d.ruleset('flow', function() {
    whenAll: m.action == 'start'
    run: throw 'Unhandled Exception!'

    // when the exception property exists
    whenAll: +s.exception
    run: {
        console.log(s.exception);
        delete(s.exception); 
    }

    whenStart: {
        post('flow', { action: 'start' });
    }
});

d.runAll();
```
[top](reference.md#table-of-contents)  
## Flow Structures
### Timers
Events can be scheduled with timers. A timeout condition can be included in the rule antecedent.   

* startTimer: starts a timer with the name and duration specified. id is optional if planning to cancel the timer.
* cancelTimer: cancels ongoing timer, name and id are required.
* timeout: used as an antecedent condition.

```javascript
var d = require('durable');

d.ruleset('timer', function() {
    // when first timer or less than 5 timeouts
    whenAny: {
        whenAll: s.count == 0
        whenAll: {
            s.count < 5 
            // when a timeout for 'Timer' occurs
            timeout('Timer')
        }
    }
    run: {
        s.count += 1;
        // start the 'Timer', wait 3 seconds for timeout
        startTimer('Timer', 3);
        console.log('Pusle ->' + new Date());
    }

    whenStart: {
        patchState('timer', { count: 0 }); 
    }
});

d.runAll();
```
[top](reference.md#table-of-contents)  

### Statechart
Rules can be organized using statecharts. A statechart is a deterministic finite automaton (DFA). The state context is in one of a number of possible states with conditional transitions between these states. 

Statechart rules:  
* A statechart can have one or more states.  
* A statechart requires an initial state.  
* An initial state is defined as a vertex without incoming edges.  
* A state can have zero or more triggers.  
* A state can have zero or more states (see [nested states](reference.md#nested-states)).  
* A trigger has a destination state.  
* A trigger can have a rule (absence means state enter).  
* A trigger can have an action.  

```javascript
var d = require('durable');

d.statechart('expense', function() {
    // initial state 'input' with two triggers
    input: {
        // trigger to move to 'denied' given a condition
        to: 'denied'
        whenAll: m.subject == 'approve' && m.amount > 1000
        // action executed before state change
        run: console.log('Denied amount: ' + m.amount)

        to: 'pending'
        whenAll: m.subject == 'approve' && m.amount <= 1000
        run: console.log('Requesting approve amount: ' + m.amount);
    }

    // intermediate state 'pending' with two triggers
    pending: {
        to: 'approved'
        whenAll: m.subject == 'approved'
        run: console.log('Expense approved')

        to: 'denied'
        whenAll: m.subject == 'denied'
        run: console.log('Expense denied')
    }
    
    // 'denied' and 'approved' are final states
    denied: {}
    approved: {}
    
    whenStart: {
        // events directed to default statechart instance
        post('expense', { subject: 'approve', amount: 100 });
        post('expense', { subject: 'approved' });
        
        // events directed to statechart instance with id '1'
        post('expense', { sid: 1, subject: 'approve', amount: 100 });
        post('expense', { sid: 1, subject: 'denied' });
        
        // events directed to statechart instance with id '2'
        post('expense', { sid: 2, subject: 'approve', amount: 10000 });
    }
});

d.runAll();
```
[top](reference.md#table-of-contents)  
### Nested States
Nested states allow for writing compact statecharts. If a context is in the nested state, it also (implicitly) is in the surrounding state. The statechart will attempt to handle any event in the context of the sub-state. If the sub-state does not  handle an event, the event is automatically handled at the context of the super-state.

```javascript
var d = require('durable');

d.statechart('worker', function() {
    // super-state 'work' has two states and one trigger
    work: {
        // sub-sate 'enter' has only one trigger
        enter: {
            to: 'process'
            whenAll: m.subject == 'enter'
            run: console.log('start process')
        }

        process: {
            to: 'process'
            whenAll: m.subject == 'continue'
            run: console.log('continue processing')
        }
    
        // the super-state trigger will be evaluated for all sub-state triggers
        to: 'canceled'
        whenAll: m.subject == 'cancel'
        run: console.log('cancel process')
    }

    canceled: {}
    whenStart: {
        // will move the statechart to the 'work.process' sub-state
        post('worker', { subject: 'enter' });
        
        // will keep the statechart to the 'work.process' sub-state
        post('worker', { subject: 'continue' });
        post('worker', { subject: 'continue' });
        
        // will move the statechart out of the work state
        post('worker', { subject: 'cancel' });
    }
});

d.runAll();
```
[top](reference.md#table-of-contents)
### Flowchart
A flowchart is another way of organizing a ruleset flow. In a flowchart each stage represents an action to be executed. So (unlike the statechart state), when applied to the context state, it results in a transition to another stage.  

Flowchart rules:  
* A flowchart can have one or more stages.  
* A flowchart requires an initial stage.  
* An initial stage is defined as a vertex without incoming edges.  
* A stage can have an action.  
* A stage can have zero or more conditions.  
* A condition has a rule and a destination stage.  

```javascript
var d = require('durable');

d.flowchart('expense', function() {
    // initial stage 'input' has two conditions
    input: {
        request: m.subject == 'approve' && m.amount <= 1000 
        deny:  m.subject == 'approve' && m.amount > 1000
    }

    // intermediate stage 'request' has an action and three conditions
    request: {
        run: console.log('Requesting approve')
        approve: m.subject == 'approved'
        deny: m.subject == 'denied'
        // self is a reflexive condition: if met, returns to the same stage
        self: m.subject == 'retry'
    }

    // two final stages 'approve' and 'deny' with no conditions
    approve: {
        run: console.log('Expense approved')
    }

    deny: {
        run: console.log('Expense denied')
    }

    whenStart: {
        // events for the default flowchart instance, approved after retry
        post('expense', { subject: 'approve', amount: 100 });
        post('expense', { subject: 'retry' });
        post('expense', { subject: 'approved' });
        
        // events for the flowchart instance '1', denied after first try
        post('expense', {sid: 1, subject: 'approve', amount: 100});
        post('expense', {sid: 1, subject: 'denied'});
        
        // event for the flowchart instance '2' immediately denied
        post('expense', {sid: 2, subject: 'approve', amount: 10000});
    }
});

d.runAll();
```
[top](reference.md#table-of-contents)  

 

