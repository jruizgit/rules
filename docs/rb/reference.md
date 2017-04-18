Reference Manual
=====
## Table of contents
* [Setup](reference.md#local-setup)
* [Rules](reference.md#rules)
  * [Simple Filter](reference.md#simple-filter)
  * [Pattern Matching](reference.md#pattern-matching)
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

## Setup
durable_rules has been tested in MacOS X, Ubuntu Linux and Windows.
### Redis install
durable_rules relies on Redis version 2.8 or higher 
 
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
3. In the new directory `gem install durable_rules` (this will download durable_rules and its dependencies)  
4. In that same directory create a test.rb file using your favorite editor  
5. Copy/Paste and save the following code:  

```ruby
require "durable"
Durable.ruleset :test do
  when_all (m.subject == "World") do
    puts "Hello #{m.subject}"
  end
  when_start do
    post :test, {:id => 1, :sid => 1, :subject => "World"}
  end
end
Durable.run_all
```  

7. In the terminal type `ruby test.rb`  
8. You should see the message: `Hello World`  

Note: If you are using [Redis To Go](https://redistogo.com), replace the last line.
  ```ruby
  Durable.run_all([{:host => "host_name", :port => "port", :password => "password"}])
  ```
[top](reference.md#table-of-contents) 

## Rules
### Simple Filter
Rules are the basic building blocks. All rules have a condition, which defines the events and facts that trigger an action.  
* The rule condition is an expression. Its left side represents an event or fact property, followed by a logical operator and its right side defines a pattern to be matched. By convention events or facts originated by calling post or assert are represented with the `m` name; events or facts originated by changing the context state are represented with the `s` name.  
* The rule action is a function to which the context is passed as a parameter. Actions can be synchronous and asynchronous. Asynchronous actions take a completion function as a parameter.  

Below is an example of the typical rule structure. 

Logical operator precedence:  
1. Unary: `-` (does not exist), `+` (exists)   
2. Logical operators: `|` (or) , `&` (and)   
3. Relational operators: >, <, >=, <=, ==, !=   
```ruby
require "durable"
Durable.ruleset :a0 do
  when_all (m.subject < 100) | (m.subject == "approve") | (m.subject == "ok") do
    puts "a0 approved ->#{m.subject}"
  end
  when_start do
    post :a0, {:id => 1, :sid => 1, :subject => 10}
  end
end
Durable.run_all
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

```ruby
require "durable"
Durable.ruleset :match do
  when_all (m.url.matches("(https?://)?([0-9a-z.-]+)%.[a-z]{2,6}(/[A-z0-9_.-]+/?)*")) do
    puts "match -> #{m.url}"
  end
  when_start do
    post :match, {:id => 1, :sid => 1, :url => "https://github.com"}
    post :match, {:id => 2, :sid => 1, :url => "http://github.com/jruizgit/rul!es"}
    post :match, {:id => 3, :sid => 1, :url => "https://github.com/jruizgit/rules/reference.md"}
    post :match, {:id => 4, :sid => 1, :url => "//rules"}
    post :match, {:id => 5, :sid => 1, :url => "https://github.c/jruizgit/rules"}
  end
end
Durable.run_all
```  
[top](reference.md#table-of-contents) 
### Correlated Sequence
The ability to express and efficiently evaluate sequences of correlated events or facts represents the forward inference hallmark. The fraud detection rule in the example below shows a pattern of three events: the second event amount being more than 200% the first event amount and the third event amount greater than the average of the other two.  

The `when_all` function expresses a sequence of events or facts separated by `,`. The assignment operator is used to name events or facts, which can be referenced in subsequent expressions. When referencing events or facts, all properties are available. Complex patterns can be expressed using arithmetic operators.  

Arithmetic operator precedence:  
1. `*`, `/`  
2. `+`, `-`  
```ruby
require "durable"
Durable.ruleset :fraud_detection do
  when_all c.first = m.t == "purchase",
           c.second = m.amount > first.amount * 2,
           c.third = m.amount > (first.amount + second.amount) / 2 do
    puts "fraud detected -> " + first.amount.to_s 
    puts "               -> " + second.amount.to_s
    puts "               -> " + third.amount.to_s 
  end
  when_start do
    post :fraud_detection, {:id => 1, :sid => 1, :t => "purchase", :amount => 50}
    post :fraud_detection, {:id => 2, :sid => 1, :t => "purchase", :amount => 200}
    post :fraud_detection, {:id => 3, :sid => 1, :t => "purchase", :amount => 300}
  end
end
Durable.run_all
```
[top](reference.md#table-of-contents)  
### Choice of Sequences
durable_rules allows expressing and efficiently evaluating richer events sequences leveraging forward inference. In the example below any of the two event\fact sequences will trigger the `a4` action. 

The following two functions can be used to define a rule:  
* when_all: a set of event or fact patterns separated by `,`. All of them are required to match to trigger an action.  
* when_any: a set of event or fact patterns separated by `,`. Any one match will trigger an action.  

The following functions can be combined to form richer sequences:
* all: patterns separated by `,`, all of them are required to match.
* any: patterns separated by `,`, any of the patterns can match.    
* none: no event or fact matching the pattern.  
```ruby
require "durable"
Durable.ruleset :a4 do
  when_any all(m.subject == "approve", m.amount == 1000),
           all(m.subject == "jumbo", m.amount == 10000) do
    puts "a4 action #{s.sid}"
  end
  when_start do
    post :a4, {:id => 1, :sid => 2, :subject => "jumbo"}
    post :a4, {:id => 2, :sid => 2, :amount => 10000}
  end
end
Durable.run_all
```
[top](reference.md#table-of-contents) 
### Conflict Resolution
Events or facts can produce multiple results in a single fact, in which case durable_rules will choose the result with the most recent events or facts. In addition events or facts can trigger more than one action simultaneously, the triggering order can be defined by setting the priority (salience) attribute on the rule.

In this example, notice how the last rule is triggered first, as it has the highest priority. In the last rule result facts are ordered starting with the most recent.
```ruby
require "durable"
Durable.ruleset :attributes do
  when_all pri(3), count(3), m.amount < 300 do
    puts "attributes ->" + m[0].amount.to_s
    puts "           ->" + m[1].amount.to_s
    puts "           ->" + m[2].amount.to_s
  end
  when_all pri(2), count(2), m.amount < 200 do
    puts "attributes ->" + m[0].amount.to_s
    puts "           ->" + m[1].amount.to_s
  end
  when_all pri(1), m.amount < 100  do
    puts "attributes ->" + m.amount.to_s
  end
  when_start do
    assert :attributes, {:id => 1, :sid => 1, :amount => 50}
    assert :attributes, {:id => 2, :sid => 1, :amount => 150}
    assert :attributes, {:id => 3, :sid => 1, :amount => 250}
  end
end
Durable.run_all
```
[top](reference.md#table-of-contents) 
### Tumbling Window
durable_rules enables aggregating events or observed facts over time with tumbling windows. Tumbling windows are a series of fixed-sized, non-overlapping and contiguous time intervals.  

Summary of rule attributes:  
* count: defines the number of events or facts, which need to be matched when scheduling an action.   
* span: defines the tumbling time in seconds between scheduled actions.  
* pri: defines the scheduled action order in case of conflict.  

```ruby
require "durable"
Durable.ruleset :t0 do
  when_all (timeout :my_timer) | (m.count == 0) do
    s.count += 1
    post :t0, {:id => s.count, :sid => 1, :t => "purchase"}
    start_timer(:my_timer, rand(3), "t_#{s.count}")
  end
  when_all span(5), m.t == "purchase" do 
    puts("t0 pulse -> #{m.count}")
  end 
  when_start do
    patch_state :t0, {:sid => 1, :count => 0}
  end
end
Durable.run_all
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
* `post ruleset_name, {event}`  
* `post_batch ruleset_name, {event}, {event}...`  
```ruby
require "durable"
Durable.ruleset :fraud_detection do
  when_all c.first = m.t == "purchase",
           c.second = m.location != first.location do
    puts "fraud detected ->" + first.location + ", " + second.location
  end
  when_start do
    post :fraud_detection, {:id => 1, :sid => 1, :t => "purchase", :location => "US"}
    post :fraud_detection, {:id => 2, :sid => 1, :t => "purchase", :location => "CA"}
  end
end
Durable.run_all
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
* `assert ruleset_name, {fact}`  
* `assert_facts ruleset_name, {fact}, {fact}...`
* `retract ruleset_name, {fact}`  
```ruby
require "durable"
Durable.ruleset :fraud_detection do
  when_all c.first = m.t == "purchase",
           c.second = m.location != first.location,
           count(2) do
    puts "fraud detected ->" + m[0].first.location + ", " + m[0].second.location
    puts "               ->" + m[1].first.location + ", " + m[1].second.location
  end
  when_start do
    assert :fraud_detection, {:id => 1, :sid => 1, :t => "purchase", :location => "US"}
    assert :fraud_detection, {:id => 2, :sid => 1, :t => "purchase", :location => "CA"}
  end
end
Durable.run_all
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
* `patch_state ruleset_name, {state}`  
* `state.property = ...`  
```ruby
require "durable"
Durable.ruleset :a8 do
  when_all (m.amount < s.max_amount) & (m.amount > s.ref_id(:global).min_amount) do
    puts "a8 approved " + m.amount.to_s
  end
  when_start do
    patch_state :a8, {:sid => 1, :max_amount => 500}
    patch_state :a8, {:sid => :global, :min_amount => 100}
    post :a8, {:id => 1, :sid => 1, :amount => 10}
    post :a8, {:id => 2, :sid => 1, :amount => 200}
  end
end
Durable.run_all
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
* `start_timer timer_name, seconds`  
* `when... timeout(timer_name)`  
```ruby
require "durable"
Durable.ruleset :t1 do
  when_all m.start == "yes" do
    s.start = Time.now
    start_timer(:my_timer, 5)
  end
  when_all timeout :my_timer do
    puts "t1 End"
    puts "t1 Started #{s.start}"
    puts "t1 Ended #{Time.now}"
  end
  when_start do
    post :t1, {:id => 1, :sid => 1, :start => "yes"}
  end
end
Durable.run_all
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
* `statechart ruleset_name do states_block`  
* `state state_name [do triggers_and_states_block]`  
* `to state_name, [rule] [do action_block]`  

```ruby
require "durable"
Durable.statechart :a2 do
  state :input do
    to :denied, when_all((m.subject == "approve") & (m.amount > 1000)) do
      puts "a2 state denied: #{s.sid}"
    end
    to :pending, when_all((m.subject == "approve") & (m.amount <= 1000)) do
      puts "a2 state request approval from: #{s.sid}"
      s.status? ? s.status = "approved": s.status = "pending"
    end
  end  
  state :pending do
    to :pending, when_any(m.subject == "approved", m.subject == "ok") do
      puts "a2 state received approval for: #{s.sid}"
      s.status = "approved"
    end
    to :approved, when_all(s.status == "approved")
    to :denied, when_all(m.subject == "denied") do
      puts "a2 state denied: #{s.sid}"
    end
  end
  state :approved
  state :denied
  when_start do
    post :a2, {:id => 1, :sid => 1, :subject => "approve", :amount => 100}
    post :a2, {:id => 2, :sid => 1, :subject => "approved"}
    post :a2, {:id => 3, :sid => 2, :subject => "approve", :amount => 100}
    post :a2, {:id => 4, :sid => 2, :subject => "denied"}
    post :a2, {:id => 5, :sid => 3, :subject => "approve", :amount => 10000}
  end
end
Durable.run_all
```
[top](reference.md#table-of-contents)  
### Nested States
`durable_rules` supports nested states. Which implies that, along with the [statechart](reference.md#statechart) description from the previous section, most of the [UML statechart](http://en.wikipedia.org/wiki/UML_state_machine) semantics is supported. If a context is in the nested state, it also (implicitly) is in the surrounding state. The state machine will attempt to handle any event in the context of the substate, which conceptually is at the lower level of the hierarchy. However, if the substate does not prescribe how to handle the event, the event is not discarded, but it is automatically handled at the higher level context of the superstate.

The example below shows a statechart, where the `canceled` transition is reused for both the `enter` and the `process` states. 
```ruby
require "durable"
Durable.statechart :a6 do
  state :work do   
    state :enter do
      to :process, when_all(m.subject == "enter") do
        puts "a6 continue process"
      end
    end
    state :process do
      to :process, when_all(m.subject == "continue") do
        puts "a6 processing"
      end
    end
    to :canceled, when_all(pri(1), m.subject == "cancel") do
      puts "a6 canceling"
    end
  end
  state :canceled
  when_start do
    post :a6, {:id => 1, :sid => 1, :subject => "enter"}
    post :a6, {:id => 2, :sid => 1, :subject => "continue"}
    post :a6, {:id => 3, :sid => 1, :subject => "continue"}
    post :a6, {:id => 4, :sid => 1, :subject => "cancel"}
  end
end
Durable.run_all
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
* `flowchart ruleset_name do stage_condition_block`  
* `stage stage_name [do action_block]`  
* `to stage_name, [rule]`  
Note: conditions have to be defined immediately after the stage definition  
```ruby
require "durable"
Durable.flowchart :a3 do
  stage :input
  to :request, when_all((m.subject == "approve") & (m.amount <= 1000))
  to :deny, when_all((m.subject == "approve") & (m.amount > 1000))
  
  stage :request do
    puts "a3 flow requesting approval for: #{s.sid}"
    s.status? ? s.status = "approved": s.status = "pending"
  end
  to :approve, when_all(s.status == "approved")
  to :deny, when_all(m.subject == "denied")
  to :request, when_any(m.subject == "approved", m.subject == "ok")
  
  stage :approve do
    puts "a3 flow aprroved: #{s.sid}"
  end

  stage :deny do
    puts "a3 flow denied: #{s.sid}"
  end

  when_start do
    post :a3, {:id => 1, :sid => 1, :subject => "approve", :amount => 100}
    post :a3, {:id => 2, :sid => 1, :subject => "approved"}
    post :a3, {:id => 3, :sid => 2, :subject => "approve", :amount => 100}
    post :a3, {:id => 4, :sid => 2, :subject => "denied"}
    post :a3, {:id => 5, :sid => 3, :subject => "approve", :amount => 10000}
  end
end
Durable.run_all
```
[top](reference.md#table-of-contents)  


