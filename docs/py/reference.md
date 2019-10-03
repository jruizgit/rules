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
3. In the new directory `pip install durable_rules` (this will download durable_rules and its dependencies)  
4. In that same directory create a test.py file using your favorite editor  
5. Copy/Paste and save the following code:
  ```python
  from durable.lang import *
  with ruleset('test'):
      @when_all(m.subject == 'World')
      def say_hello(c):
          print ('Hello {0}'.format(c.m.subject))

  post('test', { 'subject': 'World' })
  ```
7. In the terminal type `python test.py`  
8. You should see the message: `Hello World`  

[top](reference.md#table-of-contents) 
## Basics
### Rules
A rule is the basic building block of the framework. The rule antecendent defines the conditions that need to be satisfied to execute the rule consequent (action). By convention `m` represents the data to be evaluated by a given rule.

* `when_all` and `when_any` annotate the antecendent definition of a rule
  
```python
from durable.lang import *

with ruleset('test'):
    # antecedent
    @when_all(m.subject == 'World')
    def say_hello(c):
        # consequent
        print('Hello {0}'.format(c.m.subject))

post('test', { 'subject': 'World' })
```
### Facts
Facts represent the data that defines a knowledge base. Facts are asserted as JSON objects and are stored until they are retracted. When a fact satisfies a rule antecedent, the rule consequent is executed.

```python
from durable.lang import *

with ruleset('animal'):
    # will be triggered by 'Kermit eats flies'
    @when_all((m.predicate == 'eats') & (m.object == 'flies'))
    def frog(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'frog' })

    @when_all((m.predicate == 'eats') & (m.object == 'worms'))
    def bird(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'bird' })

    # will be chained after asserting 'Kermit is frog'
    @when_all((m.predicate == 'is') & (m.object == 'frog'))
    def green(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'green' })

    @when_all((m.predicate == 'is') & (m.object == 'bird'))
    def black(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'black' })

    @when_all(+m.subject)
    def output(c):
        print('Fact: {0} {1} {2}'.format(c.m.subject, c.m.predicate, c.m.object))

assert_fact('animal', { 'subject': 'Kermit', 'predicate': 'eats', 'object': 'flies' })
```

[top](reference.md#table-of-contents)  

### Events
Events can be posted to and evaluated by rules. An event is an ephemeral fact, that is, a fact retracted right before executing a consequent. Thus, events can only be observed once. Events are stored until they are observed. 

```python
from durable.lang import *

with ruleset('risk'):
    @when_all(c.first << m.t == 'purchase',
              c.second << m.location != c.first.location)
    # the event pair will only be observed once
    def fraud(c):
        print('Fraud detected -> {0}, {1}'.format(c.first.location, c.second.location))
        
post('risk', {'t': 'purchase', 'location': 'US'})
post('risk', {'t': 'purchase', 'location': 'CA'})

```

**Note:**  

*Using facts in the example above will produce the following output:*   

<sub>`Fraud detected -> US, CA`</sub>  
<sub>`Fraud detected -> CA, US`</sub>  

*In the example both facts satisfy the first condition m.t == 'purchase' and each fact satisfies the second condition m.location != c.first.location in relation to the facts which satisfied the first.*  

*An event ia an ephemeral fact. As soon as a fact is scheduled to be dispatched, it is retracted. When using post in the example above, by the time the second pair is calculated the events have already been retracted.*  

*Retracting events before dispatch reduces the number of combinations to be calculated during action execution.*  

[top](reference.md#table-of-contents)  

### State
Context state is available when a consequent is executed. The same context state is passed across rule execution. Context state is stored until it is deleted. Context state changes can be evaluated by rules. By convention `s` represents the state to be evaluated by a rule.

```python
from durable.lang import *

with ruleset('flow'):
    # state condition uses 's'
    @when_all(s.status == 'start')
    def start(c):
        # state update on 's'
        c.s.status = 'next' 
        print('start')

    @when_all(s.status == 'next')
    def next(c):
        c.s.status = 'last' 
        print('next')

    @when_all(s.status == 'last')
    def last(c):
        c.s.status = 'end' 
        print('last')
        # deletes state at the end
        c.delete_state()

update_state('flow', { 'status': 'start' })
```

[top](reference.md#table-of-contents)  
### Identity
Facts with the same property names and values are considered equal when asserted or retracted. Events with the same property names and values are considered different when posted because the posting time matters. 

```python
from durable.lang import *

with ruleset('bookstore'):
    # this rule will trigger for events with status
    @when_all(+m.status)
    def event(c):
        print('bookstore-> Reference {0} status {1}'.format(c.m.reference, c.m.status))

    @when_all(+m.name)
    def fact(c):
        print('bookstore-> Added {0}'.format(c.m.name))
        
    # this rule will be triggered when the fact is retracted
    @when_all(none(+m.name))
    def empty(c):
        print('bookstore-> No books')

# will not throw because the fact assert was successful 
assert_fact('bookstore', {
    'name': 'The new book',
    'seller': 'bookstore',
    'reference': '75323',
    'price': 500
})

# will throw MessageObservedError because the fact has already been asserted 
try:
    assert_fact('bookstore', {
        'reference': '75323',
        'name': 'The new book',
        'price': 500,
        'seller': 'bookstore'
    })
except BaseException as e:
    print('bookstore expected {0}'.format(e.message))

# will not throw because a new event is being posted
post('bookstore', {
    'reference': '75323',
    'status': 'Active'
})

# will not throw because a new event is being posted
post('bookstore', {
    'reference': '75323',
    'status': 'Active'
})

retract_fact('bookstore', {
    'reference': '75323',
    'name': 'The new book',
    'price': 500,
    'seller': 'bookstore'
})
```

[top](reference.md#table-of-contents)  
### Error Codes

When asserting a fact, retracting a fact, posting an event or updating state context, the following exceptions can be thrown:

* MessageObservedException: The fact has already been asserted or the event has already been posted.
* MessageNotHandledException: The event or fact was not captured because it did not match any rule.

[top](reference.md#table-of-contents) 

## Antecendents
### Simple Filter
A rule antecedent is an expression. The left side of the expression represents an event or fact property. The right side defines a pattern to be matched. By convention events or facts are represented with the `m` name. Context state are represented with the `s` name.  

Logical operators:  
* Unary: - (does not exist), + (exists)  
* Logical operators: &, |  
* Relational operators: < , >, <=, >=, ==, !=  

```python
from durable.lang import *

with ruleset('expense'):
    @when_all((m.subject == 'approve') | (m.subject == 'ok'))
    def approved(c):
        print ('Approved subject: {0}'.format(c.m.subject))
        
post('expense', { 'subject': 'approve'})
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

```python
from durable.lang import *

with ruleset('match'):
    @when_all(m.url.matches('(https?://)?([0-9a-z.-]+)%.[a-z]{2,6}(/[A-z0-9_.-]+/?)*'))
    def approved(c):
        print ('match-> url {0}'.format(c.m.url))

def match_complete_callback(e, state):
    print('match -> expected {0}'.format(e.message))

post('match', { 'url': 'https://github.com' })
post('match', { 'url': 'http://github.com/jruizgit/rul!es' }, match_complete_callback)
post('match', { 'url': 'https://github.com/jruizgit/rules/reference.md' })
post('match', { 'url': '//rules'}, match_complete_callback)
post('match', { 'url': 'https://github.c/jruizgit/rules' }, match_complete_callback)
```  

[top](reference.md#table-of-contents)  

### String Operations  
The pattern matching dialect can be used for common string operations. The `imatches` function enables case insensitive pattern matching.

```python
from durable.lang import *

with ruleset('strings'):
    @when_all(m.subject.matches('hello.*'))
    def starts_with(c):
        print ('string starts with hello -> {0}'.format(c.m.subject))

    @when_all(m.subject.matches('.*hello'))
    def ends_with(c):
        print ('string ends with hello -> {0}'.format(c.m.subject))

    @when_all(m.subject.imatches('.*hello.*'))
    def contains(c):
        print ('string contains hello (case insensitive) -> {0}'.format(c.m.subject))
    
assert_fact('strings', { 'subject': 'HELLO world' })
assert_fact('strings', { 'subject': 'world hello' })
assert_fact('strings', { 'subject': 'hello hi' })
assert_fact('strings', { 'subject': 'has Hello string' })
assert_fact('strings', { 'subject': 'does not match' })
```  

[top](reference.md#table-of-contents) 

### Correlated Sequence
Rules can be used to efficiently evaluate sequences of correlated events or facts. The fraud detection rule in the example below shows a pattern of three events: the second event amount being more than 200% the first event amount and the third event amount greater than the average of the other two.  

By default a correlated sequence captures distinct messages. In the example below the second event satisfies the second and the third condition, however the event will be captured only for the second condition. Use the `distinct` attribute to disable distinct event or fact correlation.

The `when_all` annotation expresses a sequence of events or facts. The `<<` operator is used to name events or facts, which can be referenced in subsequent expressions. When referencing events or facts, all properties are available. Complex patterns can be expressed using arithmetic operators.  

Arithmetic operators: +, -, *, /
```python
from durable.lang import *

with ruleset('risk'):
    @when_all(# distinct(True),
              c.first << m.amount > 10,
              c.second << m.amount > c.first.amount * 2,
              c.third << m.amount > (c.first.amount + c.second.amount) / 2)
    def detected(c):
        print('fraud detected -> {0}'.format(c.first.amount))
        print('               -> {0}'.format(c.second.amount))
        print('               -> {0}'.format(c.third.amount))
        
post('risk', { 'amount': 50 })
post('risk', { 'amount': 200 })
post('risk', { 'amount': 251 })
```  

[top](reference.md#table-of-contents)  

### Choice of Sequences
durable_rules allows expressing and efficiently evaluating richer event sequences. In the example below each of the two event\fact sequences will trigger an action.

The following two functions can be used and combined to define richer event sequences:  
* all: a set of event or fact patterns. All of them are required to match to trigger an action.  
* any: a set of event or fact patterns. Any one match will trigger an action.  

```python
from durable.lang import *

with ruleset('expense'):
    @when_any(all(c.first << m.subject == 'approve', 
                  c.second << m.amount == 1000), 
              all(c.third << m.subject == 'jumbo', 
                  c.fourth << m.amount == 10000))
    def action(c):
        if c.first:
            print ('Approved {0} {1}'.format(c.first.subject, c.second.amount))
        else:
            print ('Approved {0} {1}'.format(c.third.subject, c.fourth.amount))
    

post('expense', { 'subject': 'approve' })
post('expense', { 'amount': 1000 })
post('expense', { 'subject': 'jumbo' })
post('expense', { 'amount': 10000 })
```
[top](reference.md#table-of-contents) 

### Lack of Information
In some cases lack of information is meaningful. The `none` function can be used in rules with correlated sequences to evaluate the lack of information.  

*Note: the `none` function requires information to reason about a lack of information. That is, it will not trigger any actions if no events or facts have been registered in the corresponding rule.*

```python
from durable.lang import *

with ruleset('risk'):
    @when_all(c.first << m.t == 'deposit',
              none(m.t == 'balance'),
              c.third << m.t == 'withdrawal',
              c.fourth << m.t == 'chargeback')
    def detected(c):
        print('fraud detected {0} {1} {2}'.format(c.first.t, c.third.t, c.fourth.t))
        
assert_fact('risk', { 't': 'deposit' })
assert_fact('risk', { 't': 'withdrawal' })
assert_fact('risk', { 't': 'chargeback' })

assert_fact('risk', { 'sid': 1, 't': 'balance' })
assert_fact('risk', { 'sid': 1, 't': 'deposit' })
assert_fact('risk', { 'sid': 1, 't': 'withdrawal' })
assert_fact('risk', { 'sid': 1, 't': 'chargeback' })
retract_fact('risk', { 'sid': 1, 't': 'balance' })

```

[top](reference.md#table-of-contents)  

### Nested Objects
Queries on nested events or facts are also supported. The `.` notation is used for defining conditions on properties in nested objects.  

```python
from durable.lang import *

with ruleset('expense'):
    # use the '.' notation to match properties in nested objects
    @when_all(c.bill << (m.t == 'bill') & (m.invoice.amount > 50),
              c.account << (m.t == 'account') & (m.payment.invoice.amount == c.bill.invoice.amount))
    def approved(c):
        print ('bill amount  ->{0}'.format(c.bill.invoice.amount))
        print ('account payment amount ->{0}'.format(c.account.payment.invoice.amount))
            
# one level of nesting
post('expense', {'t': 'bill', 'invoice': {'amount': 100}})
        
#two levels of nesting
post('expense', {'t': 'account', 'payment': {'invoice': {'amount': 100}}})
```  
[top](reference.md#table-of-contents)  

### Arrays

```python
from durable.lang import *

with ruleset('risk'):
    # matching primitive array
    @when_all(m.payments.allItems((item > 100) & (item < 500)))
    def rule1(c):
        print('fraud 1 detected {0}'.format(c.m.payments))

    # matching object array
    @when_all(m.payments.allItems((item.amount < 250) | (item.amount >= 300)))
    def rule2(c):
        print('fraud 2 detected {0}'.format(c.m.payments))

    # pattern matching string array
    @when_all(m.cards.anyItem(item.matches('three.*')))
    def rule3(c):
        print('fraud 3 detected {0}'.format(c.m.cards))

    # matching nested arrays
    @when_all(m.payments.anyItem(item.allItems(item < 100)))
    def rule4(c):
        print('fraud 4 detected {0}'.format(c.m.payments))
        
post('risk', {'payments': [ 150, 300, 450 ]})
post('risk', {'payments': [ { 'amount' : 200 }, { 'amount' : 300 }, { 'amount' : 450 } ]})
post('risk', {'cards': [ 'one card', 'two cards', 'three cards' ]})
post('risk', {'payments': [ [ 10, 20, 30 ], [ 30, 40, 50 ], [ 10, 20 ] ]}) 
```  
[top](reference.md#table-of-contents)  

### Facts and Events as rvalues

Aside from scalars (strings, number and boolean values), it is possible to use the fact or event observed on the right side of an expression.  

```python
from durable.lang import *

with ruleset('risk'):
    # compares properties in the same event, this expression is evaluated in the client 
    @when_all(m.debit > m.credit * 2)
    def fraud_1(c):
        print('debit {0} more than twice the credit {1}'.format(c.m.debit, c.m.credit))

    # compares two correlated events, this expression is evaluated in the backend
    @when_all(c.first << m.amount > 100,
              c.second << m.amount > c.first.amount + m.amount / 2)
    def fraud_2(c):
        print('fraud detected ->{0}'.format(c.first.amount))
        print('fraud detected ->{0}'.format(c.second.amount))
        
post('risk', { 'debit': 220, 'credit': 100 })
post('risk', { 'debit': 150, 'credit': 100 })
post('risk', { 'amount': 200 })
post('risk', { 'amount': 500 })
```

[top](reference.md#table-of-contents) 
 
## Consequents
### Conflict Resolution
Event and fact evaluation can lead to multiple consequents. The triggering order can be controlled by using the `pri` (salience) function. Actions with lower value are executed first. The default value for all actions is 0.

In this example, notice how the last rule is triggered first, as it has the highest priority.
```python
from durable.lang import *

with ruleset('attributes'):
    @when_all(pri(3), m.amount < 300)
    def first_detect(c):
        print('attributes P3 ->{0}'.format(c.m.amount))
        
    @when_all(pri(2), m.amount < 200)
    def second_detect(c):
        print('attributes P2 ->{0}'.format(c.m.amount))
        
    @when_all(pri(1), m.amount < 100)
    def third_detect(c):
        print('attributes P1 ->{0}'.format(c.m.amount))
                
assert_fact('attributes', { 'amount': 50 })
assert_fact('attributes', { 'amount': 150 })
assert_fact('attributes', { 'amount': 250 })
```
[top](reference.md#table-of-contents)  
### Action Batches
When a high number of events or facts satisfy a consequent, the consequent results can be delivered in batches.

* count: defines the exact number of times the rule needs to be satisfied before scheduling the action.   
* cap: defines the maximum number of times the rule needs to be satisfied before scheduling the action.  

This example batches exactly three approvals and caps the number of rejects to two:
```python
from durable.lang import *

with ruleset('expense'):
    # this rule will trigger as soon as three events match the condition
    @when_all(count(3), m.amount < 100)
    def approve(c):
        print('approved {0}'.format(c.m))

    # this rule will be triggered when 'expense' is asserted batching at most two results       
    @when_all(cap(2),
              c.expense << m.amount >= 100,
              c.approval << m.review == True)
    def reject(c):
        print('rejected {0}'.format(c.m))

post_batch('expense', [{ 'amount': 10 },
                                    { 'amount': 20 },
                                    { 'amount': 100 },
                                    { 'amount': 30 },
                                    { 'amount': 200 },
                                    { 'amount': 400 }])
assert_fact('expense', { 'review': True })
```
[top](reference.md#table-of-contents)  
### Async Actions  
The consequent action can be asynchronous. When the action is finished, the `complete` function has to be called. By default an action is considered abandoned after 5 seconds. This value can be changed by returning a different number in the action function or extended by calling `renew_action_lease`.

```python
from durable.lang import *
import threading

with ruleset('flow'):
    timer = None

    def start_timer(time, callback):
        timer = threading.Timer(time, callback)
        timer.daemon = True    
        timer.start()

    @when_all(s.state == 'first')
    # async actions take a callback argument to signal completion
    def first(c, complete):
        def end_first():
            c.s.state = 'second'     
            print('first completed')

            # completes the action after 3 seconds
            complete(None)
        
        start_timer(3, end_first)
        
    @when_all(s.state == 'second')
    def second(c, complete):
        def end_second():
            c.s.state = 'third'
            print('second completed')

            # completes the action after 6 seconds
            # use the first argument to signal an error
            complete(Exception('error detected'))

        start_timer(6, end_second)

        # overrides the 5 second default abandon timeout
        return 10
    
update_state('flow', { 'state': 'first' })
```
[top](reference.md#table-of-contents)  
### Unhandled Exceptions  
When exceptions are not handled by actions, they are stored in the context state. This enables writing exception-handling rules.

```python
from durable.lang import *

with ruleset('flow'):
    
    @when_all(m.action == 'start')
    def first(c):
        raise Exception('Unhandled Exception!')

    # when the exception property exists
    @when_all(+s.exception)
    def second(c):
        print(c.s.exception)
        c.s.exception = None
            
post('flow', { 'action': 'start' })
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

```python
from durable.lang import *

with statechart('expense'):
    # initial state 'input' with two triggers
    with state('input'):
        # trigger to move to 'denied' given a condition
        @to('denied')
        @when_all((m.subject == 'approve') & (m.amount > 1000))
        # action executed before state change
        def denied(c):
            print ('denied amount {0}'.format(c.m.amount))
        
        @to('pending')    
        @when_all((m.subject == 'approve') & (m.amount <= 1000))
        def request(c):
            print ('requesting approve amount {0}'.format(c.m.amount))
    
    # intermediate state 'pending' with two triggers
    with state('pending'):
        @to('approved')
        @when_all(m.subject == 'approved')
        def approved(c):
            print ('expense approved')
            
        @to('denied')
        @when_all(m.subject == 'denied')
        def denied(c):
            print ('expense denied')
    
    # 'denied' and 'approved' are final states    
    state('denied')
    state('approved')
        
# events directed to default statechart instance
post('expense', { 'subject': 'approve', 'amount': 100 })
post('expense', { 'subject': 'approved' })

# events directed to statechart instance with id '1'
post('expense', { 'sid': 1, 'subject': 'approve', 'amount': 100 })
post('expense', { 'sid': 1, 'subject': 'denied' })

# events directed to statechart instance with id '2'
post('expense', { 'sid': 2, 'subject': 'approve', 'amount': 10000 })
```
[top](reference.md#table-of-contents)  
### Nested States
Nested states allow for writing compact statecharts. If a context is in the nested state, it also (implicitly) is in the surrounding state. The statechart will attempt to handle any event in the context of the sub-state. If the sub-state does not  handle an event, the event is automatically handled at the context of the super-state.

```python
from durable.lang import *

with statechart('worker'):
    # super-state 'work' has two states and one trigger
    with state('work'):
        # sub-state 'enter' has only one trigger
        with state('enter'):
            @to('process')
            @when_all(m.subject == 'enter')
            def continue_process(c):
                print('start process')
    
        with state('process'):
            @to('process')
            @when_all(m.subject == 'continue')
            def continue_process(c):
                print('continue processing')

        # the super-state trigger will be evaluated for all sub-state triggers
        @to('canceled')
        @when_all(m.subject == 'cancel')
        def cancel(c):
            print('cancel process')

    state('canceled')

# will move the statechart to the 'work.process' sub-state
post('worker', { 'subject': 'enter' })

# will keep the statechart to the 'work.process' sub-state
post('worker', { 'subject': 'continue' })
post('worker', { 'subject': 'continue' })

# will move the statechart out of the work state
post('worker', { 'subject': 'cancel' })
```
[top](reference.md#table-of-contents)
### Flowchart
A flowchart is another way of organizing a ruleset flow. In a flowchart each stage represents an action to be executed. So (unlike the statechart state) when applied to the context state it results in a transition to another stage.  

Flowchart rules:  
* A flowchart can have one or more stages.  
* A flowchart requires an initial stage.  
* An initial stage is defined as a vertex without incoming edges.  
* A stage can have an action.  
* A stage can have zero or more conditions.  
* A condition has a rule and a destination stage.  

```python
from durable.lang import *

with flowchart('expense'):
    # initial stage 'input' has two conditions
    with stage('input'): 
        to('request').when_all((m.subject == 'approve') & (m.amount <= 1000))
        to('deny').when_all((m.subject == 'approve') & (m.amount > 1000))
    
    # intermediate stage 'request' has an action and three conditions
    with stage('request'):
        @run
        def request(c):
            print('requesting approve')
            
        to('approve').when_all(m.subject == 'approved')
        to('deny').when_all(m.subject == 'denied')
        # reflexive condition: if met, returns to the same stage
        to('request').when_all(m.subject == 'retry')
    
    with stage('approve'):
        @run 
        def approved(c):
            print('expense approved')

    with stage('deny'):
        @run
        def denied(c):
            print('expense denied')

# events for the default flowchart instance, approved after retry
post('expense', { 'subject': 'approve', 'amount': 100 })
post('expense', { 'subject': 'retry' })
post('expense', { 'subject': 'approved' })

# events for the flowchart instance '1', denied after first try
post('expense', { 'sid': 1, 'subject': 'approve', 'amount': 100})
post('expense', { 'sid': 1, 'subject': 'denied'})

# event for the flowchart instance '2' immediately denied
post('expense', { 'sid': 2, 'subject': 'approve', 'amount': 10000})
```
[top](reference.md#table-of-contents)  
### Timers
Events can be scheduled with timers. A timeout condition can be included in the rule antecedent. By default a timeout is triggered as an event (observed only once). Timeouts can also be triggered as facts by 'manual reset' timers and the timers can be reset during action execution (see last example).

* start_timer: starts a timer with the name and duration specified (manual_reset is optional).
* reset_timer: resets a 'manual reset' timer.
* cancel_timer: cancels ongoing timer.
* timeout: used as an antecedent condition.

```python
from durable.lang import *

with ruleset('timer'):
    
    @when_all(m.subject == 'start')
    def start(c):
        c.start_timer('MyTimer', 5)
        
    @when_all(timeout('MyTimer'))
    def timer(c):
        print('timer timeout')

post('timer', { 'subject': 'start' })
```

The example below uses a timer to detect a higher event rate:  

```python
from durable.lang import *

with statechart('risk'):
    with state('start'):
        @to('meter')
        def start(c):
            c.start_timer('RiskTimer', 5)

    with state('meter'):
        @to('fraud')
        @when_all(count(3), c.message << m.amount > 100)
        def fraud(c):
            for e in c.m:
                print(e.message) 

        @to('exit')
        @when_all(timeout('RiskTimer'))
        def exit(c):
            print('exit')

    state('fraud')
    state('exit')

# three events in a row will trigger the fraud rule
post('risk', { 'amount': 200 })
post('risk', { 'amount': 300 })
post('risk', { 'amount': 400 })

# two events will exit after 5 seconds
post('risk', { 'sid': 1, 'amount': 500 })
post('risk', { 'sid': 1, 'amount': 600 })
```

In this example a manual reset timer is used for measuring velocity. 

```python
from durable.lang import *

with statechart('risk'):
    with state('start'):
        @to('meter')
        def start(c):
            c.start_timer('VelocityTimer', 5, True)

    with state('meter'):
        @to('meter')
        @when_all(cap(5), 
                  m.amount > 100,
                  timeout('VelocityTimer'))
        def some_events(c):
            print('velocity: {0} in 5 seconds'.format(len(c.m)))
            # resets and restarts the manual reset timer
            c.reset_timer('VelocityTimer')
            c.start_timer('VelocityTimer', 5, True)

        @to('meter')
        @when_all(pri(1), timeout('VelocityTimer'))
        def no_events(c):
            print('velocity: no events in 5 seconds')
            c.reset_timer('VelocityTimer')
            c.start_timer('VelocityTimer', 5, True)

post('risk', { 'amount': 200 })
post('risk', { 'amount': 300 })
post('risk', { 'amount': 50 })
post('risk', { 'amount': 500 })
post('risk', { 'amount': 600 })
```

[top](reference.md#table-of-contents)  
