Reference Manual
=====
### Table of contents
------
* [Local Setup](reference.md#local-setup)
* [Cloud Setup](reference.md#cloud-setup)
* [Rules](reference.md#rules)
  * [Simple Filter](reference.md#simple-filter)
  * [Correlated Sequence](reference.md#correlated-sequence)
  * [Choice of Sequences](reference.md#choice-of-sequences)
  * [Conflict Resolution](reference.md#conflict-resolution)
  * [Tumbling Window](reference.md#tumbling-window)
* [Data Model](reference.md#data-model)
  * [Events](reference.md#events)
  * [Facts](reference.md#facts)
  * [Context](reference.md#context)
  * [Timers](reference.md#timers) 
* [Flow Structures](reference.md#flow-structures)
  * [Statechart](reference.md#statechart)
  * [Nested States](reference.md#nested-states)
  * [Flowchart](reference.md#flowchart)

### Local Setup
------
durable_rules has been tested in MacOS X, Ubuntu Linux and Windows.
#### Redis install
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

#### First App
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
          host.post('test', {'id': 1, 'sid': 1, 'subject': 'World'})

  run_all()
  ```
7. In the terminal type `python test.py`  
8. You should see the message: `Hello World`  

Note: If you are using [Redis To Go](https://redistogo.com), replace the last line.
  ```python
  run_all([{'host': 'host_name', 'port': port, 'password': 'password'}]);
  ```

[top](reference.md#table-of-contents) 
### Cloud Setup
--------
#### Redis install
Redis To Go has worked well for me and is very fast if you are deploying an app using Heroku or AWS.   
1. Go to: [Redis To Go](https://redistogo.com)  
2. Create an account (the free instance with 5MB has enough space for you to evaluate durable_rules)  
3. Make sure you write down the host, port and password, which represents your new account  

### Rules
------
#### Simple Filter
Rules are the basic building blocks. All rules have a condition, which defines the events and facts that trigger an action.  
* The rule condition is an expression. Its left side represents an event or fact property, followed by a logical operator and its right side defines a pattern to be matched. By convention events or facts originated by calling post or assert are represented with the `m` name; events or facts originated by changing the context state are represented with the `s` name.  
* The rule action is a function to which the context is passed as a parameter. Actions can be synchronous and asynchronous. Asynchronous actions take a completion function as a parameter.  

Below is an example of the typical rule structure. 

Logical operator precedence:  
1. Unary: `-` (not exists), `+` (exists)  
2. Boolean operators: `|` (or) , `&` (and)  
3. Pattern matching: >, <, >=, <=, ==, !=  
```python
from durable.lang import *
with ruleset('a0'):
    @when_all((m.subject < 100) | (m.subject == 'approve') | (m.subject == 'ok'))
    def approved(c):
        print ('a0 approved ->{0}'.format(c.m.subject))
        
    @when_start
    def start(host):
        host.post('a0', {'id': 1, 'sid': 1, 'subject': 10})
        
run_all()
```
[top](reference.md#table-of-contents) 
#### Correlated Sequence
The ability to express and efficiently evaluate sequences of correlated events or facts represents the forward inference hallmark. The fraud detection rule in the example below shows a pattern of three events: the second event amount being more than 200% the first event amount and the third event amount greater than the average of the other two.  

The `when_all` decorator expresses a sequence of events or facts separated by `,`. The `<<` operator is used to name events or facts, which can be referenced in subsequent expressions. When referencing events or facts, all properties are available. Complex patterns can be expressed using arithmetic operators.  

Arithmetic operator precedence:  
1. `*`, `/`  
2. `+`, `-`  
```python
from durable.lang import *
with ruleset('fraud_detection'):
    @when_all(c.first << m.amount > 10,
              c.second << m.amount > c.first.amount * 2,
              c.third << m.amount > (c.first.amount + c.second.amount) / 2)
    def detected(c):
        print('fraud detected -> {0}'.format(c.first.amount))
        print('               -> {0}'.format(c.second.amount))
        print('               -> {0}'.format(c.third.amount))
        
    @when_start
    def start(host):
        host.post('fraud_detection', {'id': 1, 'sid': 1, 'amount': 50})
        host.post('fraud_detection', {'id': 2, 'sid': 1, 'amount': 200})
        host.post('fraud_detection', {'id': 3, 'sid': 1, 'amount': 300})

run_all()
```
[top](reference.md#table-of-contents)  
#### Choice of Sequences
durable_rules allows expressing and efficiently evaluating richer events sequences leveraging forward inference. In the example below any of the two event\fact sequences will trigger the `a4` action. 

The following two decorators can be used for the rule definition:  
* when_all: a set of event or fact patterns separated by `,`. All of them are required to match to trigger an action.  
* when_any: a set of event or fact patterns separated by `,`. Any one match will trigger an action.  

The following functions can be combined to form richer sequences:
* all: patterns separated by `,`, all of them are required to match.
* any: patterns separated by `,`, any of the patterns can match.    
* none: no event or fact matching the pattern.  
```python
from durable.lang import *
with ruleset('a4'):
    @when_any(all(m.subject == 'approve', m.amount == 1000), 
              all(m.subject == 'jumbo', m.amount == 10000))
    def action(c):
        print ('a4 action {0}'.format(c.s.sid))
    
    @when_start
    def start(host):
        host.post('a4', {'id': 1, 'sid': 2, 'subject': 'jumbo'})
        host.post('a4', {'id': 2, 'sid': 2, 'amount': 10000})

run_all()
```
[top](reference.md#table-of-contents) 
#### Conflict Resolution
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
#### Tumbling Window
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
### Data Model
------
#### Events
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

#### Facts
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

#### Context
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
    @when_all(m.amount < c.s.max_amount + c.s.id('global').min_amount)
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
#### Timers
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
### Flow Structures
-------
#### Statechart
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

run_all()
```
[top](reference.md#table-of-contents)  
#### Nested States
`durable_rules` supports nested states. Which implies that, along with the [statechart](reference.md#statechart) description from the previous section, most of the [UML statechart](http://en.wikipedia.org/wiki/UML_state_machine) semantics is supported. If a context is in the nested state, it also (implicitly) is in the surrounding state. The state machine will attempt to handle any event in the context of the substate, which conceptually is at the lower level of the hierarchy. However, if the substate does not prescribe how to handle the event, the event is not discarded, but it is automatically handled at the higher level context of the superstate.

The example below shows a statechart, where the `canceled` and reflective `work` transitions are reused for both the `enter` and the `process` states. 
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

        @to('work')
        @when_all(m.subject == 'reset')
        def reset(c):
            print('a6 resetting')

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

run_all()
```
[top](reference.md#table-of-contents)
#### Flowchart
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
        to('request').when_any(m.subject == 'approved', m.subject == 'ok')
    
    with stage('approve'):
        @run 
        def approved(c):
            print ('a3 approved from: {0}'.format(c.s.sid))

    with stage('deny'):
        @run
        def denied(c):
            print ('a3 denied from: {0}'.format(c.s.sid))

run_all()
```
[top](reference.md#table-of-contents)  

