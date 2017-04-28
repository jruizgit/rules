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
  * [Tumbling Window](reference.md#tumbling-window)  
* [Flow Structures](reference.md#flow-structures)
  * [Timers](reference.md#timers) 
  * [Statechart](reference.md#statechart)
  * [Nested States](reference.md#nested-states)
  * [Flowchart](reference.md#flowchart)

## Setup
durable_rules has been tested in MacOS X, Ubuntu Linux and Windows.
### Redis install
durable_rules relies on Redis version 2.8  
 
_Mac_  
1. Download [Redis](http://download.redis.io/releases/redis-2.8.4.tar.gz)   
2. Extract code, compile and start Redis

For more information go to: http://redis.io/download  

_Windows_  
1. Download Redis binaries from [MSTechOpen](https://github.com/MSOpenTech/redis/releases)  
2. Extract binaries and start Redis  

For more information go to: https://github.com/MSOpenTech/redis  

Note: To test applications locally you can also use a Redis [cloud service](reference.md#cloud-setup) 

### First App
Now that your cache ready, let's write a simple rule:  

1. Start a terminal  
2. Create a directory for your app: `mkdir firstapp` `cd firstapp`  
3. In the new directory `sudo pip install durable_rules` (this will download durable_rules and its dependencies)  
4. In that same directory create a test.py file using your favorite editor  
5. Copy/Paste and save the following code:
  ```python
  from durable.lang import *
  with ruleset('test'):
      @when_all(m.subject == 'World')
      def say_hello(c):
          print ('Hello {0}'.format(c.m.subject))

      @when_start
      def start(host):
          host.post('test', { 'subject': 'World' })

  run_all()
  ```
7. In the terminal type `python test.py`  
8. You should see the message: `Hello World`  

Note 1: If you are using a redis service outside your local host, replace the last line with:
  ```python
  run_all([{'host': 'host_name', 'port': port, 'password': 'password'}]);
  ```

[top](reference.md#table-of-contents) 
## Basics
### Rules
A rule is the basic building block of the framework. The rule antecendent defines the conditions that need to be satisfied to execute the rule consequent (action). By convention `m` represents the data to be evaluated by a given rule.

* `when_all` and `when_any` annotate the antecendent definition of a rule
* `when_start` annotates the action to be taken when starting the ruleset  
  
```python
from durable.lang import *

with ruleset('test'):
    # antecedent
    @when_all(m.subject == 'World')
    def say_hello(c):
        # consequent
        print('Hello {0}'.format(c.m.subject))

    # on ruleset start
    @when_start
    def start(host):    
        host.post('test', { 'subject': 'World' })

run_all()
```
### Facts
Facts represent the data that defines a knowledge base. After facts are asserted as JSON objects. Facts are stored until they are retracted. When a fact satisfies a rule antecedent, the rule consequent is executed.

```python
from durable.lang import *

with ruleset('animal'):
    # will be triggered by 'Kermit eats flies'
    @when_all((m.verb == 'eats') & (m.predicate == 'flies'))
    def frog(c):
        c.assert_fact({ 'subject': c.m.subject, 'verb': 'is', 'predicate': 'frog' })

    @when_all((m.verb == 'eats') & (m.predicate == 'worms'))
    def bird(c):
        c.assert_fact({ 'subject': c.m.subject, 'verb': 'is', 'predicate': 'bird' })

    # will be chained after asserting 'Kermit is frog'
    @when_all((m.verb == 'is') & (m.predicate == 'frog'))
    def green(c):
        c.assert_fact({ 'subject': c.m.subject, 'verb': 'is', 'predicate': 'green' })

    @when_all((m.verb == 'is') & (m.predicate == 'bird'))
    def black(c):
        c.assert_fact({ 'subject': c.m.subject, 'verb': 'is', 'predicate': 'black' })

    @when_all(count(3), +m.subject)
    def output(c):
        for f in c.m:
            print('Fact: {0} {1} {2}'.format(f.subject, f.verb, f.predicate))

    @when_start
    def start(host):
        host.assert_fact('animal', { 'subject': 'Kermit', 'verb': 'eats', 'predicate': 'flies' })

run_all()
```

Facts can also be asserted using the http API. For the example above, run the following command:  

<sub>`curl -H "content-type: application/json" -X POST -d '{"subject": "Tweety", "verb": "eats", "predicate": "worms"}' http://localhost:5000/animal/facts`</sub>

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

    @when_start
    def start(host):
        # 'post' submits events, try 'assert' instead and to see differt behavior
        host.post('risk', {'t': 'purchase', 'location': 'US'});
        host.post('risk', {'t': 'purchase', 'location': 'CA'});

run_all()
```

Events can be posted using the http API. When the example above is listening, run the following commands:  

<sub>`curl -H "content-type: application/json" -X POST -d '{"t": "purchase", "location": "BR"}' http://localhost:5000/risk/events`</sub>  
<sub>`curl -H "content-type: application/json" -X POST -d '{"t": "purchase", "location": "JP"}' http://localhost:5000/risk/events`</sub>  

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

    @when_start
    def on_start(host):
        # modifies default context state
        host.patch_state('flow', { 'status': 'start' })

run_all()
```
State can also be retrieved and modified using the http API. When the example above is running, try the following commands:  
<sub>`curl -H "content-type: application/json" -X POST -d '{"state": "next"}' http://localhost:5000/flow/state`</sub>  

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
        
    @when_start
    def start(host):
        host.post('expense', { 'subject': 'approve'})
        
run_all()
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

```python
from durable.lang import *

with ruleset('match'):
    @when_all(m.url.matches('(https?://)?([0-9a-z.-]+)%.[a-z]{2,6}(/[A-z0-9_.-]+/?)*'))
    def approved(c):
        print ('match url ->{0}'.format(c.m.url))

    @when_start
    def start(host):
        host.post('match', { 'url': 'https://github.com' })
        host.post('match', { 'url': 'http://github.com/jruizgit/rul!es' })
        host.post('match', { 'url': 'https://github.com/jruizgit/rules/reference.md' })
        host.post('match', { 'url': '//rules'})
        host.post('match', { 'url': 'https://github.c/jruizgit/rules' })

run_all()
```  

[top](reference.md#table-of-contents)  

### Correlated Sequence
Rules can be used to efficiently evaluate sequences of correlated events or facts. The fraud detection rule in the example below shows a pattern of three events: the second event amount being more than 200% the first event amount and the third event amount greater than the average of the other two.  

The `when_all` annotation expresses a sequence of events or facts. The `<<` operator is used to name events or facts, which can be referenced in subsequent expressions. When referencing events or facts, all properties are available. Complex patterns can be expressed using arithmetic operators.  

Arithmetic operators: +, -, *, /
```python
from durable.lang import *

with ruleset('risk'):
    @when_all(c.first << m.amount > 10,
              c.second << m.amount > c.first.amount * 2,
              c.third << m.amount > (c.first.amount + c.second.amount) / 2)
    def detected(c):
        print('fraud detected -> {0}'.format(c.first.amount))
        print('               -> {0}'.format(c.second.amount))
        print('               -> {0}'.format(c.third.amount))
        
    @when_start
    def start(host):
        host.post('risk', { 'amount': 50 })
        host.post('risk', { 'amount': 200 })
        host.post('risk', { 'amount': 300 })

run_all()
```  

[top](reference.md#table-of-contents)  
### Lack of Information
In some cases lack of information is meaningful. The `none` function can be used in rules with correlated sequences to evaluate the lack of information.
```python
from durable.lang import *

with ruleset('risk'):
    @when_all(c.first << m.t == 'deposit',
              none(m.t == 'balance'),
              c.third << m.t == 'withrawal',
              c.fourth << m.t == 'chargeback')
    def detected(c):
        print('fraud detected {0} {1} {2}'.format(c.first.t, c.third.t, c.fourth.t))
        
    @when_start
    def start(host):
        host.post('risk', { 't': 'deposit' })
        host.post('risk', { 't': 'withrawal' })
        host.post('risk', { 't': 'chargeback' })
        
run_all()
```

[top](reference.md#table-of-contents)  
### Choice of Sequences
durable_rules allows expressing and efficiently evaluating richer events sequences In the example below any of the two event\fact sequences will trigger an action. 

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
    
    @when_start
    def start(host):
        host.post('expense', { 'subject': 'approve' })
        host.post('expense', { 'amount': 1000 })
        host.post('expense', { 'subject': 'jumbo' })
        host.post('expense', { 'amount': 10000 })

run_all()
```
[top](reference.md#table-of-contents) 
## Consequents
### Conflict Resolution
Events or facts can produce multiple results in a single fact, in which case durable_rules will choose the result with the most recent events or facts. In addition events or facts can trigger more than one action simultaneously, the triggering order can be defined by setting the priority (salience) attribute on the rule.

In this example, notice how the last rule is triggered first, as it has the highest priority. In the last rule result facts are ordered starting with the most recent.
```python
from durable.lang import *
with ruleset('attributes'):
    @when_all(pri(3), count(3), m.amount < 300)
    def first_detect(c):
        print('attributes ->{0}'.format(c.m[0].amount))
        print('           ->{0}'.format(c.m[1].amount))
        print('           ->{0}'.format(c.m[2].amount))

    @when_all(pri(2), count(2), m.amount < 200)
    def second_detect(c):
        print('attributes ->{0}'.format(c.m[0].amount))
        print('           ->{0}'.format(c.m[1].amount))

    @when_all(pri(1), m.amount < 100)
    def third_detect(c):
        print('attributes ->{0}'.format(c.m.amount))
        
    @when_start
    def start(host):
        host.assert_fact('attributes', {'id': 1, 'sid': 1, 'amount': 50})
        host.assert_fact('attributes', {'id': 2, 'sid': 1, 'amount': 150})
        host.assert_fact('attributes', {'id': 3, 'sid': 1, 'amount': 250})

run_all()
```
[top](reference.md#table-of-contents) 
### Tumbling Window
durable_rules enables aggregating events or observed facts over time with tumbling windows. Tumbling windows are a series of fixed-sized, non-overlapping and contiguous time intervals.  

Summary of rule attributes:  
* count: defines the number of events or facts, which need to be matched when scheduling an action.   
* span: defines the tumbling time in seconds between scheduled actions.  
* pri: defines the scheduled action order in case of conflict.  

```python
from durable.lang import *
import random

with ruleset('t0'):
    @when_all(timeout('my_timer') | (s.count == 0))
    def start_timer(c):
        c.s.count += 1
        c.post('t0', {'id': c.s.count, 'sid': 1, 't': 'purchase'})
        c.start_timer('my_timer', random.randint(1, 3), 't_{0}'.format(c.s.count))

    @when_all(span(5), m.t == 'purchase')
    def pulse(c):
        print('t0 pulse -> {0}'.format(len(c.m)))

    @when_start
    def start(host):
        host.patch_state('t0', {'sid': 1, 'count': 0})

run_all()
```
[top](reference.md#table-of-contents)  
## Data Model
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
* `c.post(ruleset_name, {event})`
* `c.post_batch(ruleset_name, {event}, {event}...)`
* `host.post(ruleset_name, {event})`
* `host.post_batch(ruleset_name, {event}, {event}...)`
```python
from durable.lang import *
with ruleset('fraud_detection'):
    @when_all(c.first << m.t == 'purchase',
              c.second << m.location != c.first.location)
    def detected(c):
        print ('fraud detected ->{0}, {1}'.format(c.first.location, c.second.location))

    @when_start
    def start(host):
        host.post('fraud_detection', {'id': 1, 'sid': 1, 't': 'purchase', 'location': 'US'})
        host.post('fraud_detection', {'id': 2, 'sid': 1, 't': 'purchase', 'location': 'CA'})

run_all()
```
[top](reference.md#table-of-contents)  

### Facts
Facts are used for defining more permanent state, which lifetime spans at least more than one action execution.

Fact rules:  
* Facts can be asserted in the `start` handler via the host parameter.   
* Facts can asserted in an `action` handler using the context parameter.   
* Facts have to be explicitly retracted.  
* Once retracted all related scheduled actions are cancelled.  
* Facts can co-exist with events.  
* The assert and retract fact operations are idempotent.  

This example shows how asserting two facts lead to scheduling two actions: one for each combination.  

API:  
* `host.assert_fact(ruleset_name, {fact})`  
* `host.assert_facts(ruleset_name, {fact}, {fact}...)`  
* `host.retract_fact(ruleset_name, {fact})`  
* `c.assert_fact(ruleset_name, {fact})`  
* `c.assert_facts(ruleset_name, {fact}. {fact}...)`  
* `c.retract_fact(ruleset_name, {fact})`  
```python
from durable.lang import *
with ruleset('fraud_detection'):
    @when_all(c.first << m.t == 'purchase',
              c.second << m.location != c.first.location,
              count(2))
    def detected(c):
        print ('fraud detected ->{0}, {1}'.format(c.m[0].first.location, c.m[0].second.location))
        print ('               ->{0}, {1}'.format(c.m[1].first.location, c.m[1].second.location))

    @when_start
    def start(host):
        host.assert_fact('fraud_detection', {'id': 1, 'sid': 1, 't': 'purchase', 'location': 'US'})
        host.assert_fact('fraud_detection', {'id': 2, 'sid': 1, 't': 'purchase', 'location': 'CA'})

run_all()
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
* `host.patch_state(ruleset_name, {state})`  
* `c.state.property = ...`  
```python
from durable.lang import *
with ruleset('a8'):
    @when_all(m.amount < c.s.max_amount + c.s.ref_id('global').min_amount)
    def approved(c):
        print ('a8 approved {0}'.format(c.m.amount))

    @when_start
    def start(host):
        host.patch_state('a8', {'sid': 1, 'max_amount': 500})
        host.patch_state('a8', {'sid': 'global', 'min_amount': 100})
        host.post('a8', {'id': 1, 'sid': 1, 'amount': 10})

run_all()
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

The example shows an event scheduled to be raised after 5 seconds and a rule which reacts to such an event.  

API:  
* `host.start_timer(timer_name, seconds)`
* `c.start_timer(timer_name, seconds)`  
* `when... timeout(timer_name)`  
```python
from durable.lang import *
import datetime

with ruleset('t1'): 
    @when_all(m.start == 'yes')
    def start_timer(c):
        c.s.start = datetime.datetime.now().strftime('%I:%M:%S%p')
        c.start_timer('my_timer', 5)

    @when_all(timeout('my_timer'))
    def end_timer(c):
        print('t1 started @%s' % c.s.start)
        print('t1 ended @%s' % datetime.datetime.now().strftime('%I:%M:%S%p'))

    @when_start
    def start(host):
        host.post('t1', {'id': 1, 'sid': 1, 'start': 'yes'})

run_all()
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

The example shows an approval state machine, which waits for two consecutive events (`subject = "approve"` and `subject = "approved"`) to reach the `approved` state.  

API:  
* `with statechart(ruleset_name): states_block`  
* `with state(state_name): [triggers_and_states_block]`  

Action decorators:  
* `@to(state_name)`  
* `@rule`  
```python
from durable.lang import *
with statechart('a2'):
    with state('input'):
        @to('denied')
        @when_all((m.subject == 'approve') & (m.amount > 1000))
        def denied(c):
            print ('a2 denied from: {0}'.format(c.s.sid))
        
        @to('pending')    
        @when_all((m.subject == 'approve') & (m.amount <= 1000))
        def request(c):
            print ('a2 request approval from: {0}'.format(c.s.sid))
        
    with state('pending'):
        @to('pending')
        @when_all(m.subject == 'approved')
        def second_request(c):
            print ('a2 second request approval from: {0}'.format(c.s.sid))
            c.s.status = 'approved'

        @to('approved')
        @when_all(s.status == 'approved')
        def approved(c):
            print ('a2 approved from: {0}'.format(c.s.sid))
        
        @to('denied')
        @when_all(m.subject == 'denied')
        def denied(c):
            print ('a2 denied from: {0}'.format(c.s.sid))
        
    state('denied')
    state('approved')
    @when_start
    def start(host):
        host.post('a2', {'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100})
        host.post('a2', {'id': 2, 'sid': 1, 'subject': 'approved'})
        host.post('a2', {'id': 3, 'sid': 2, 'subject': 'approve', 'amount': 100})
        host.post('a2', {'id': 4, 'sid': 2, 'subject': 'denied'})
        host.post('a2', {'id': 5, 'sid': 3, 'subject': 'approve', 'amount': 10000})

run_all()
```
[top](reference.md#table-of-contents)  
### Nested States
`durable_rules` supports nested states. Which implies that, along with the [statechart](reference.md#statechart) description from the previous section, most of the [UML statechart](http://en.wikipedia.org/wiki/UML_state_machine) semantics is supported. If a context is in the nested state, it also (implicitly) is in the surrounding state. The state machine will attempt to handle any event in the context of the substate, which conceptually is at the lower level of the hierarchy. However, if the substate does not prescribe how to handle the event, the event is not discarded, but it is automatically handled at the higher level context of the superstate.

The example below shows a statechart, where the `canceled` transition is reused for both the `enter` and the `process` states. 
```python
from durable.lang import *
with statechart('a6'):
    with state('work'):
        with state('enter'):
            @to('process')
            @when_all(m.subject == 'enter')
            def continue_process(c):
                print('a6 continue_process')
    
        with state('process'):
            @to('process')
            @when_all(m.subject == 'continue')
            def continue_process(c):
                print('a6 processing')

        @to('canceled')
        @when_all(m.subject == 'cancel')
        def cancel(c):
            print('a6 canceling')

    state('canceled')
    @when_start
    def start(host):
        host.post('a6', {'id': 1, 'sid': 1, 'subject': 'enter'})
        host.post('a6', {'id': 2, 'sid': 1, 'subject': 'continue'})
        host.post('a6', {'id': 3, 'sid': 1, 'subject': 'continue'})
        host.post('a6', {'id': 4, 'sid': 1, 'subject': 'cancel'})
        
run_all()
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
* `flowchart(ruleset_name): stage_condition_block`  
* `with stage(stage_name): [action_condition_block]`  
* `to(stage_name).[rule]`  

Decorators:  
* `@run`  
Note: conditions have to be defined immediately after the stage definition  
```python
from durable.lang import *
with flowchart('a3'):
    with stage('input'): 
        to('request').when_all((m.subject == 'approve') & (m.amount <= 1000))
        to('deny').when_all((m.subject == 'approve') & (m.amount > 1000))
    
    with stage('request'):
        @run
        def request(c):
            print ('a3 request approval from: {0}'.format(c.s.sid))
            if c.s.status:
                c.s.status = 'approved'
            else:
                c.s.status = 'pending'

        to('approve').when_all(s.status == 'approved')
        to('deny').when_all(m.subject == 'denied')
        to('request').when_all(m.subject == 'approved')
    
    with stage('approve'):
        @run 
        def approved(c):
            print ('a3 approved from: {0}'.format(c.s.sid))

    with stage('deny'):
        @run
        def denied(c):
            print ('a3 denied from: {0}'.format(c.s.sid))

    @when_start
    def start(host):
        host.post('a3', {'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100})
        host.post('a3', {'id': 2, 'sid': 1, 'subject': 'approved'})
        host.post('a3', {'id': 3, 'sid': 2, 'subject': 'approve', 'amount': 100})
        host.post('a3', {'id': 4, 'sid': 2, 'subject': 'denied'})
        host.post('a3', {'id': 5, 'sid': 3, 'subject': 'approve', 'amount': 10000})

run_all()
```
[top](reference.md#table-of-contents)  

