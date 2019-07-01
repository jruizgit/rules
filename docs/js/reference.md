Reference Manual
=====
## Table of contents
* [Setup](reference.md#setup)
* [Basics](reference.md#basics)
  * [Rules](reference.md#rules)
  * [Facts](reference.md#facts)
  * [Events](reference.md#events)
  * [State](reference.md#state)
  * [Identity](reference.md#identity)
  * [Error Codes](reference.md#error-codes)
* [Antecedents](reference.md#antecedents)
  * [Simple Filter](reference.md#simple-filter)
  * [Pattern Matching](reference.md#pattern-matching)
  * [String Operations](reference.md#string-operations)
  * [Correlated Sequence](reference.md#correlated-sequence)
  * [Choice of Sequences](reference.md#choice-of-sequences)
  * [Lack of Information](reference.md#lack-of-information)
  * [Nested Objects](reference.md#nested-objects)
  * [Arrays](reference.md#arrays)
  * [Facts and Events as rvalues](reference.md#facts-and-events-as-rvalues)
* [Consequents](reference.md#consequents)  
  * [Conflict Resolution](reference.md#conflict-resolution)
  * [Action Batches](reference.md#action-batches)
  * [Async Actions](reference.md#async-actions)
  * [Unhandled Exceptions](reference.md#unhandled-exceptions)
* [Flow Structures](reference.md#flow-structures) 
  * [Statechart](reference.md#statechart)
  * [Nested States](reference.md#nested-states)
  * [Flowchart](reference.md#flowchart)
  * [Timers](reference.md#timers)

## Setup

### First App
Let's write a simple rule:  

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
  });

  d.post('a0', { amount: 10 });
  ```
7. In the terminal type `node test.js`  
8. You should see the message: `a0 approved`  

Note: If you are running in Windows, you will need VS2013 express edition and Python 2.7 for the package to build during npm install. Make sure both the VS build tools and the python directory are in your path. 

[top](reference.md#table-of-contents) 

## Basics
### Rules
A rule is the basic building block of the framework. The rule antecendent defines the conditions that need to be satisfied to execute the rule consequent (action). By convention `m` represents the data to be evaluated by a given rule.

* `whenAll` and `whenAny` label the antecendent definition of a rule
* `run` and `runAsync` label the consequent definition of a rule 
  
```javascript
var d = require('durable');

d.ruleset('test', function() {
    // antecedent
    whenAll: m.subject == 'World'
    // consequent
    run: console.log('Hello ' + m.subject)
});

d.post('test', {subject: 'World'});
```
### Facts
Facts represent the data that defines a knowledge base. After facts are asserted as JSON objects. Facts are stored until they are retracted. When a fact satisfies a rule antecedent, the rule consequent is executed.

```javascript
var d = require('durable');

d.ruleset('animal', function() {
    // will be triggered by 'Kermit eats flies'
    whenAll: m.predicate == 'eats' && m.object == 'flies' 
    run: assert({ subject: m.subject, predicate: 'is', object: 'frog' })

    whenAll: m.predicate == 'eats' && m.object == 'worms' 
    run: assert({ subject: m.subject, predicate: 'is', object: 'bird' })

    // will be chained after asserting 'Kermit is frog'
    whenAll: m.predicate == 'is' && m.object == 'frog' 
    run: assert({ subject: m.subject, predicate: 'is', object: 'green'})

    whenAll: m.predicate == 'is' && m.object == 'bird' 
    run: assert({ subject: m.subject, predicate: 'is', object: 'black'})

    whenAll: +m.subject
    run: console.log('fact: ' + m.subject + ' ' + m.predicate + ' ' + m.object)
});

d.assert('animal', { subject: 'Kermit', predicate: 'eats', object: 'flies' });
```

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
});

// 'post' submits events, try 'assert' instead and to see differt behavior
d.post('risk', { t: 'purchase', location: 'US' });
d.post('risk', { t: 'purchase', location: 'CA' });
```

**Note from the autor:**  

*Using facts in the example above will produce the following output:*   

<sub>`Fraud detected -> US, CA`</sub>  
<sub>`Fraud detected -> CA, US`</sub>  

*The reason is because both facts satisfy the first condition m.t == 'purchase' and each fact satisfies the second condition m.location != c.first.location in relation to the facts which satisfied the first.*  

*Events are ephemeral facts, they are retracted before they are dispatched. When using post in the example above, by the time the second pair is calculated the events have already been retracted.*  

*Retracting events before dispatch reduces the number of combinations to be calculated during action execution.*  

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
});

// modifies context state
d.updateState('flow', { status: 'start' });
```

[top](reference.md#table-of-contents)  

### Identity
Facts and events with the same property names and values are considered equal. 

```javascript
var d = require('durable');

d.ruleset('bookstore', function() {
    // this rule will trigger for events with status
    whenAll: +m.status
    run: console.log('bookstore reference ' + m.reference + ' status ' + m.status)

    whenAll: +m.name
    run: { 
        console.log('bookstore added: ' + m.name);
    }

    // this rule will be triggered when the fact is retracted
    whenAll: none(+m.name)
    run: console.log('bookstore no books');
});

// will not throw because the fact assert was successful 
d.assert('bookstore', {
    name: 'The new book',
    seller: 'bookstore',
    reference: '75323',
    price: 500
});


// will throw MessageObservedError because the fact has already been asserted 
try {
    d.assert('bookstore', {
        reference: '75323',
        name: 'The new book',
        price: 500,
        seller: 'bookstore'
    });
} catch (err) {
    console.log('bookstore: ' + err.message);   
}

// will not throw because a new event is being posted
d.post('bookstore', {
    reference: '75323',
    status: 'Active'
});

// will not throw because a new event is being posted
d.post('bookstore', {
    reference: '75323',
    status: 'Active'
});

d.retract('bookstore', {
    reference: '75323',
    name: 'The new book',
    price: 500,
    seller: 'bookstore'
});

```

### Error Codes

When asserting a fact, retracting a fact, posting an event or updating state context, the following errors can be thrown:

* MessageObservedError: The fact has already been asserted or the event has already been posted.
* MessageNotHandledError: The event or fact was not captured because it did not match any rule.

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
});

d.post('expense', { subject: 'approve' });
```  
[top](reference.md#table-of-contents)
### Pattern Matching
durable_rules implements a simple pattern matching dialect. It uses % to escape, which vastly simplifies writing expressions. Expressions are compiled down into a deterministic state machine, thus backtracking is not supported. Event processing is O(n) guaranteed (n being the size of the event).  

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
});

d.post('match', {url: 'https://github.com'});
d.post('match', {url: 'http://github.com/jruizgit/rul!es'}, function(err, state){ console.log('match: ' + err.message) });
d.post('match', {url: 'https://github.com/jruizgit/rules/reference.md'});
d.post('match', {url: '//rules'}, function(err, state){ console.log('match: ' + err.message) });
d.post('match', {url: 'https://github.c/jruizgit/rules'}, function(err, state){ console.log('match: ' + err.message) });
```  
[top](reference.md#table-of-contents) 
### String Operations  
The pattern matching dialect can be used for common string operations. The `imatches` function enables case insensitive pattern matching.

```javascript
var d = require('durable');

d.ruleset('strings', function() {
    whenAll: m.subject.matches('hello.*')
    run: console.log('string starts with hello: ' + m.subject)

    whenAll: m.subject.matches('.*hello')
    run: console.log('string ends with hello: ' + m.subject)

    whenAll: m.subject.imatches('.*hello.*')
    run: console.log('string contains hello (case insensitive): ' + m.subject)
});

d.assert('strings', { subject: 'HELLO world' });
d.assert('strings', { subject: 'world hello' });
d.assert('strings', { subject: 'hello hi' });
d.assert('strings', { subject: 'has Hello string' });
d.assert('strings', { subject: 'does not match' }, function(err, state){ console.log('strings: ' + err.message) });
```  
[top](reference.md#table-of-contents)

### Correlated Sequence
Rules can be used to efficiently evaluate sequences of correlated events or facts. The fraud detection rule in the example below shows a pattern of three events: the second event amount being more than 200% the first event amount and the third event amount greater than the average of the other two.  

By default a correlated sequences capture distinct messages. In the example below the second event satisfies the second and the third condition, however the event will be captured only for the second condition. Use the `distinct` attribute to disable distinct event or fact correlation.  

The `whenAll` label expresses a sequence of events or facts. The assignment operator is used to name events or facts, which can be referenced in subsequent expressions. When referencing events or facts, all properties are available. Complex patterns can be expressed using arithmetic operators.  

Arithmetic operators: +, -, *, /
```javascript
var d = require('durable');

d.ruleset('risk', function() {
    whenAll: {
        first = m.amount > 10
        second = m.amount > first.amount * 2
        third = m.amount > (first.amount + second.amount) / 2
    }
    // distinct: true
    run: {
       	console.log('fraud detected -> ' + first.amount);
        console.log('               -> ' + second.amount);
        console.log('               -> ' + third.amount);
    }
});

d.post('risk', { amount: 50 });
d.post('risk', { amount: 200 });
d.post('risk', { amount: 251 });
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
});

d.post('expense', { subject: 'approve' });
d.post('expense', { amount: 1000 });
d.post('expense', { subject: 'jumbo' });
d.post('expense', { amount: 10000 });
```
[top](reference.md#table-of-contents) 

### Lack of Information
In some cases lack of information is meaningful. The `none` function can be used in rules with correlated sequences to evaluate the lack of information.

*Note: the `none` function requires information to reason about lack of information. That is, it will not trigger any actions if no events or facts have been registered in the corresponding rule.*

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
});

d.assert('risk', {t: 'deposit'});
d.assert('risk', {t: 'withrawal'});
d.assert('risk', {t: 'chargeback'});

d.assert('risk', {sid: 1, t: 'balance'});
d.assert('risk', {sid: 1, t: 'deposit'});
d.assert('risk', {sid: 1, t: 'withrawal'});
d.assert('risk', {sid: 1, t: 'chargeback'});
d.retract('risk', {sid: 1, t: 'balance'});

d.assert('risk', {sid: 2, t: 'deposit'});
d.assert('risk', {sid: 2, t: 'withrawal'});
d.assert('risk', {sid: 2, t: 'chargeback'});
d.assert('risk', {sid: 2, t: 'balance'});
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
});

// one level of nesting
d.post('expense4', {t: 'bill', invoice: {amount: 100}});  

// two levels of nesting
d.post('expense4', {t: 'account', payment: {invoice: {amount: 100}}}); 
```
[top](reference.md#table-of-contents)  

### Arrays
```javascript
var d = require('durable');

d.ruleset('risk', function() {
    
    // matching primitive array
    whenAll: {
        m.payments.allItems(item > 100)
    }
    run: console.log('fraud 1 detected ' + m.payments)

    // matching object array
    whenAll: {
        m.payments.allItems(item.amount < 250 || item.amount >= 300)
    }
    run: console.log('fraud 2 detected ' + JSON.stringify(m.payments))
   
    // pattern matching string array
    whenAll: {
        m.cards.anyItem(item.matches('three.*'))
    }
    run: console.log('fraud 3 detected ' + m.cards)

    // matching nested arrays
    whenAll: {
        m.payments.anyItem(item.allItems(item < 100))
    }
    run: console.log('fraud 4 detected ' + JSON.stringify(m.payments))
});

d.post('risk', { payments: [ 150, 350, 450 ] });
d.post('risk', { payments: [ { amount: 200 }, { amount: 300 }, { amount: 400 } ] });
d.post('risk', { cards: [ 'one card', 'two cards', 'three cards' ] });
d.post('risk', { payments: [ [ 10, 20, 30 ], [ 30, 40, 50 ], [ 10, 20 ] ]});    
```
[top](reference.md#table-of-contents) 
### Facts and Events as rvalues

Aside from scalars (strings, number and boolean values), it is possible to use the fact or event observed on the right side of an expression. 

```javascript
var d = require('durable');

d.ruleset('risk', function() {
    
    // compares properties in the same event
    whenAll: {
        m.debit > 2 * m.credit
    }
    run: console.log('debit ' + m.debit + ' more than twice the credit ' + m.credit)
   
    // compares two correlated events
    whenAll: {
        first = m.amount > 100
        second = m.amount > first.amount + m.amount / 2
    }
    run: {
        console.log('fraud detected -> ' + first.amount);
        console.log('fraud detected -> ' + second.amount);
    }
});

d.post('risk', { debit: 220, credit: 100 });
d.post('risk', { debit: 150, credit: 100 });
d.post('risk', {amount: 200});
d.post('risk', {amount: 500});
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
});

d.assert('attributes', { amount: 50 });
d.assert('attributes', { amount: 150 });
d.assert('attributes', { amount: 250 });
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
    run: console.log('approved ' + JSON.stringify(m))

    // this rule will be triggered when 'expense' is asserted batching at most two results
    whenAll: {
        expense = m.amount >= 100
        approval = m.review == true
    }
    cap: 2
    run: console.log('rejected ' + JSON.stringify(m))
});

d.postBatch('expense', [{ amount: 10 },
                        { amount: 20 },
                        { amount: 100 },
                        { amount: 30 },
                        { amount: 200 },
                        { amount: 400 }]);
d.assert('expense', { review: true })
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
});

d.updateState('flow', { state: 'first' });
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
});

d.post('flow', { action: 'start' });
```
[top](reference.md#table-of-contents)  
 
## Flow Structures

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
});

d.post('expense', { subject: 'approve', amount: 100 });
d.post('expense', { subject: 'approved' });

// events directed to statechart instance with id '1'
d.post('expense', { sid: 1, subject: 'approve', amount: 100 });
d.post('expense', { sid: 1, subject: 'denied' });

// events directed to statechart instance with id '2'
d.post('expense', { sid: 2, subject: 'approve', amount: 10000 });
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
});

// will move the statechart to the 'work.process' sub-state
d.post('worker', { subject: 'enter' });
        
// will keep the statechart to the 'work.process' sub-state
d.post('worker', { subject: 'continue' });
d.post('worker', { subject: 'continue' });

// will move the statechart out of the work state
d.post('worker', { subject: 'cancel' });
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
});
// events for the default flowchart instance, approved after retry
d.post('expense', { subject: 'approve', amount: 100 });
d.post('expense', { subject: 'retry' });
d.post('expense', { subject: 'approved' });

// events for the flowchart instance '1', denied after first try
d.post('expense', {sid: 1, subject: 'approve', amount: 100});
d.post('expense', {sid: 1, subject: 'denied'});

// event for the flowchart instance '2' immediately denied
d.post('expense', {sid: 2, subject: 'approve', amount: 10000});
```
[top](reference.md#table-of-contents)  
### Timers
Events can be scheduled with timers. A timeout condition can be included in the rule antecedent. By default a timeuot is triggered as an event (observed only once). Timeouts can also be triggered as facts by 'manual reset' timers, the timers can be reset during action execution (see last example). 

* startTimer: starts a timer with the name and duration specified (manual_reset is optional).
* resetTimer: resets a 'manual reset' timer.
* cancelTimer: cancels ongoing timer.
* timeout: used as an antecedent condition.

```javascript
var d = require('durable');

d.ruleset('timer', function() {
    
    whenAll: m.subject == 'start'
    run: startTimer('MyTimer', 5);

    whenAll: {
        timeout('MyTimer')    
    }
    run: {
        console.log('timer timeout'); 
    }
});


d.post('timer', {subject: 'start'});
```

The example below use a timer to detect higher event rate:  

```javascript
var d = require('durable');

d.statechart('risk', function() {
    start: {
        to: 'meter'
        run: startTimer('RiskTimer', 5)
    }

    meter: {
        to: 'fraud'
        whenAll: message = m.amount > 100
        count: 3
        run: m.forEach(function(e, i){ console.log('risk ' + JSON.stringify(e.message)) })

        to: 'exit'
        whenAll: timeout('RiskTimer')
        run: console.log('risk exit for ' + c.s.sid)
    }

    fraud: {}
    exit:{}
});

// three events in a row will trigger the fraud rule
d.post('risk', { amount: 200 }); 
d.post('risk', { amount: 300 }); 
d.post('risk', { amount: 400 }); 

// two events will exit after 5 seconds
d.post('risk', { sid: 1, amount: 500 }); 
d.post('risk', { sid: 1, amount: 600 });  
```

In this example a manual reset timer is used for measuring velocity.

```javascript
var d = require('durable');

d.statechart('risk', function() {
    start: {
        to: 'meter'
        // will start a manual reset timer
        run: startTimer('VelocityTimer', 5, true)
    }

    meter: {
        to: 'meter'
        whenAll: { 
            message = m.amount > 100
            timeout('VelocityTimer')
        }
        cap: 100
        run: {
            console.log('risk velocity: ' + m.length + ' events in 5 seconds');
            // resets and restarts the manual reset timer
            startTimer('VelocityTimer', 5, true);
        }  

        to: 'meter'
        whenAll: {
            timeout('VelocityTimer')
        }
        run: {
            console.log('risk velocity: no events in 5 seconds');
            cancelTimer('VelocityTimer');
        }
    }
});

d.post('risk', { amount: 200 }); 
d.post('risk', { amount: 300 }); 
d.post('risk', { amount: 500 }); 
d.post('risk', { amount: 600 }); 
```

[top](reference.md#table-of-contents)  
 

