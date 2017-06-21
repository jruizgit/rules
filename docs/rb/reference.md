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
  * [Facts and Events as rvalues](reference.md#facts-and-events-as-rvalues)
* [Consequents](reference.md#consequents)  
  * [Conflict Resolution](reference.md#conflict-resolution)
  * [Action Batches](reference.md#action-batches)
  * [Async Actions](reference.md#async-actions)
  * [Unhandled Exceptions](reference.md#unhandled-exceptions)
  * [Fault Tolerance](reference.md#fault-tolerance)
* [Flow Structures](reference.md#flow-structures) 
  * [Statechart](reference.md#statechart)
  * [Nested States](reference.md#nested-states)
  * [Flowchart](reference.md#flowchart)
  * [Timers](reference.md#timers)
  
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
    post :test, { :subject => "World"}
  end
end
Durable.run_all
```  

7. In the terminal type `ruby test.rb`  
8. You should see the message: `Hello World`  

Note: If you are using a redis service outside your local host, replace the last line with:
  ```ruby
  Durable.run_all([{:host => "host_name", :port => "port", :password => "password"}])
  ```
[top](reference.md#table-of-contents) 

## Basics
### Rules
A rule is the basic building block of the framework. The rule antecendent defines the conditions that need to be satisfied to execute the rule consequent (action). By convention `m` represents the data to be evaluated by a given rule.

* `when_all` and `when_any` annotate the antecendent definition of a rule
* `when_start` annotates the action to be taken when starting the ruleset  
  
```ruby
require "durable"

Durable.ruleset :test do
  # antecedent
  when_all m.subject == "World" do
    # consequent
    puts "Hello #{m.subject}"
  end
  # on ruleset start
  when_start do
    post :test, { :subject => "World" }
  end
end

Durable.run_all
```  
### Facts
Facts represent the data that defines a knowledge base. After facts are asserted as JSON objects. Facts are stored until they are retracted. When a fact satisfies a rule antecedent, the rule consequent is executed.

```ruby
require "durable"

Durable.ruleset :animal do
  # will be triggered by 'Kermit eats flies'
  when_all c.first = (m.predicate == "eats") & (m.object == "flies") do
    assert :subject => first.subject, :predicate => "is", :object => "frog"
  end

  when_all (m.predicate == "eats") & (m.object == "worms") do
    assert :subject => m.subject, :predicate => "is", :object => "bird"
  end
  
  # will be chained after asserting 'Kermit is frog'
  when_all (m.predicate == "is") & (m.object == "frog") do
    assert :subject => m.subject, :predicate => "is", :object => "green"
  end
    
  when_all (m.predicate == "is") & (m.object == "bird") do
    assert :subject => m.subject, :predicate => "is", :object => "black"
  end
    
  when_all +m.subject do
    puts "fact: #{m.subject} #{m.predicate} #{m.object}"
  end
    
  when_start do
    assert :animal, { :subject => "Kermit", :predicate => "eats", :object => "flies" }
  end
end

Durable.run_all
```  

Facts can also be asserted using the http API. For the example above, run the following command:  

<sub>`curl -H "content-type: application/json" -X POST -d '{"subject": "Tweety", "predicate": "eats", "object": "worms"}' http://localhost:4567/animal/facts`</sub>

[top](reference.md#table-of-contents)  

### Events
Events can be posted to and evaluated by rules. An event is an ephemeral fact, that is, a fact retracted right before executing a consequent. Thus, events can only be observed once. Events are stored until they are observed. 

```ruby
require "durable"

Durable.ruleset :risk do
  when_all c.first = m.t == "purchase",
           c.second = m.location != first.location do
    # the event pair will only be observed once
    puts "fraud detected -> #{first.location}, #{second.location}"
  end

  when_start do
    # 'post' submits events, try 'assert' instead and to see differt behavior
    post :risk, { :t => "purchase", :location => "US" }
    post :risk, { :t => "purchase", :location => "CA" }
  end
end

Durable.run_all
```  

Events can be posted using the http API. When the example above is listening, run the following commands:  

<sub>`curl -H "content-type: application/json" -X POST -d '{"t": "purchase", "location": "BR"}' http://localhost:4567/risk/events`</sub>  
<sub>`curl -H "content-type: application/json" -X POST -d '{"t": "purchase", "location": "JP"}' http://localhost:4567/risk/events`</sub>  

**Note:**  

*Using facts in the example above will produce the following output:*   

<sub>`Fraud detected -> US, CA`</sub>  
<sub>`Fraud detected -> CA, US`</sub>  

*The reason is because both facts satisfy the first condition m.t == 'purchase' and each fact satisfies the second condition m.location != c.first.location in relation to the facts which satisfied the first.*  

*Given that, you might be wondering why post behaves differently: the reason is because an event is an ephemeral fact, as soon as it is scheduled to be dispatched, it is retracted. When using post in the example above, by the time the second pair is calculated the events have already been retracted.*  

*And why is the difference between events and facts important? Retracting events before dispatch reduces the number of combinations to be calculated for dispatch. Thus, processing events is much more efficient (orders of magnitude faster in some cases).*  


[top](reference.md#table-of-contents)  

### State
Context state is available when a consequent is executed. The same context state is passed across rule execution. Context state is stored until it is deleted. Context state changes can be evaluated by rules. By convention `s` represents the state to be evaluated by a rule.

```ruby
require "durable"

Durable.ruleset :flow do
  # state condition uses 's'
  when_all s.status == "start" do
    # state update on 's'
    s.status = "next"
    puts "start"
  end

  when_all s.status == "next" do
    s.status = "last"
    puts "next"
  end

  when_all s.status == "last" do
    s.status = "end"
    puts "last"
    # deletes state at the end
    delete_state
  end
  # modifies context state
  when_start do
    patch_state :flow, { :status => "start"}
  end
end

Durable.run_all
```  
State can also be retrieved and modified using the http API. When the example above is running, try the following commands:  
<sub>`curl -H "content-type: application/json" -X POST -d '{"status": "next"}' http://localhost:4567/flow/state`</sub>  

[top](reference.md#table-of-contents)  

### Identity
Facts with the same property names and values are considered equal when asserted or retracted. Events with the same property names and values are considered different when posted because the posting time matters. 

```ruby
require "durable"

Durable.ruleset :bookstore do
  # this rule will trigger for events with status
  when_all +m.status do
    puts "Reference #{m.reference} status #{m.status}"
  end

  when_all +m.name do
    puts "Added: #{m.name}"
    retract(:name => 'The new book',
            :reference => '75323',
            :price => 500,
            :seller => 'bookstore')
  end

  when_all none(+m.name) do
    puts "No books"
  end  

  when_start do
    # will return 0 because the fact assert was successful 
    puts assert :bookstore, {
                :name => 'The new book',
                :seller => 'bookstore',
                :reference => '75323',
                :price => 500}

    # will return 212 because the fact has already been asserted 
    puts assert :bookstore, {
                :reference => '75323',
                :name => 'The new book',
                :price => 500,
                :seller => 'bookstore'}

    # will return 0 because a new event is being posted
    puts post :bookstore, {
              :reference => '75323',
              :status => 'Active'}

    # will return 0 because a new event is being posted
    puts post :bookstore, {
              :reference => '75323',
              :status => 'Active'}
  end
end

Durable.run_all
```

[top](reference.md#table-of-contents)  

### Error Codes

When the run_all command fails, it can return the following error codes:

* 0 - OK
* 1 - Out of memory (uncommon)
* 2 - Unexpected type (uncommon)
* 5 - Unexpected name (uncommon)
* 6 - Rule limit exceeded (uncommon)
* 8 - Rule beta limit exceeded (uncommon)
* 9 - Rule without qualifier (uncommon)
* 10 - Invalid rule attribute (uncommon)
* 101 - Error parsing JSON value (uncommon)
* 102 - Error parsing JSON string (uncommon)
* 103 - Error parsing JSON number (uncommon)
* 104 - Error parsing JSON object (uncommon)
* 301 - Could not establish Redis connection
* 302 - Redis returned an error
* 501 - Could not parse regex
* 502 - Max regex state transitions reached (uncommon)
* 503 - Max regex states reached (uncommon)
* 504 - Regex DFA transform queue full (uncommon)
* 505 - Regex DFA transform list full (uncommon)
* 506 - Regex DFA transform set full (uncommon)
* 507 - Conflict in regex transform (uncommon)

When asserting a fact or posting an event via the when_start function or the web API, these error codes can be returned:

* 0 - OK
* 101 - Error parsing JSON value (uncommon)
* 102 - Error parsing JSON string (uncommon)
* 103 - Error parsing JSON number (uncommon)
* 104 - Error parsing JSON object (uncommon)
* 201 - The event or fact was not captured because it did not match any rule
* 202 - Too many properties in the event or fact
* 203 - Max rule stack size reached due to complex ruleset (uncommon) 
* 209 - Max number of command actions reached (uncommon)
* 210 - Max number of add actions reached (uncommon)
* 211 - Max number of eval actions reached (uncommon)
* 212 - The event or fact has already been observed
* 302 - Redis returned an error

[top](reference.md#table-of-contents) 

## Antecendents
### Simple Filter
A rule antecedent is an expression. The left side of the expression represents an event or fact property. The right side defines a pattern to be matched. By convention events or facts are represented with the `m` name. Context state are represented with the `s` name.  

Logical operators:  
* Unary: - (does not exist), + (exists)  
* Logical operators: &, |  
* Relational operators: < , >, <=, >=, ==, !=  

```ruby
require "durable"

Durable.ruleset :expense do
  when_all (m.subject == "approve") | (m.subject == "ok") do
    puts "Approved subject: #{m.subject}"
  end

  when_start do
    post :expense, { :subject => "approve" }
  end
end

Durable.run_all
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

```ruby
require "durable"

Durable.ruleset :match do
  when_all (m.url.matches("(https?://)?([0-9a-z.-]+)%.[a-z]{2,6}(/[A-z0-9_.-]+/?)*")) do
    puts "match -> #{m.url}"
  end
  when_start do
    post :match, { :url => "https://github.com" }
    post :match, { :url => "http://github.com/jruizgit/rul!es" }
    post :match, { :url => "https://github.com/jruizgit/rules/blob/master/docs/rb/reference.md" }
    post :match, { :url => "//rules" }
    post :match, { :url => "https://github.c/jruizgit/rules" }
  end
end

Durable.run_all
```  

[top](reference.md#table-of-contents)  

### String Operations  
The pattern matching dialect can be used for common string operations. The `imatches` function enables case insensitive pattern matching.

```ruby
require "durable"

Durable.ruleset :strings do
  when_all m.subject.matches("hello.*") do
    puts "string starts with hello: #{m.subject}"
  end

  when_all m.subject.matches(".*hello") do
    puts "string ends with hello: #{m.subject}"
  end

  when_all m.subject.imatches(".*Hello.*") do
    puts "string contains hello (case insensitive): #{m.subject}"
  end

  when_start do
    assert :strings, { :subject => "HELLO world" }
    assert :strings, { :subject => "world hello" }
    assert :strings, { :subject => "hello hi" }
    assert :strings, { :subject => "has Hello string" }
    assert :strings, { :subject => "does not match" }
  end
end

Durable.run_all
```  

[top](reference.md#table-of-contents) 

### Correlated Sequence
Rules can be used to efficiently evaluate sequences of correlated events or facts. The fraud detection rule in the example below shows a pattern of three events: the second event amount being more than 200% the first event amount and the third event amount greater than the average of the other two.  

The `when_all` annotation expresses a sequence of events or facts. The `=` operator is used to name events or facts, which can be referenced in subsequent expressions. When referencing events or facts, all properties are available. Complex patterns can be expressed using arithmetic operators.  

Arithmetic operators: +, -, *, /
```ruby
require "durable"

Durable.ruleset :risk do
  when_all c.first = m.t == "purchase",
           c.second = m.amount > first.amount * 2,
           c.third = m.amount > first.amount + second.amount do
    puts "fraud detected -> #{first.amount}" 
    puts "               -> #{second.amount}"
    puts "               -> #{third.amount}"
  end
  
  when_start do
    post :risk, { :t => "purchase", :amount => 50 }
    post :risk, { :t => "purchase", :amount => 200 }
    post :risk, { :t => "purchase", :amount => 300 } 
  end
end

Durable.run_all
```  

[top](reference.md#table-of-contents)  

### Choice of Sequences
durable_rules allows expressing and efficiently evaluating richer events sequences In the example below any of the two event\fact sequences will trigger an action. 

The following two functions can be used and combined to define richer event sequences:  
* all: a set of event or fact patterns. All of them are required to match to trigger an action.  
* any: a set of event or fact patterns. Any one match will trigger an action.  

```ruby
require "durable"

Durable.ruleset :expense do
  when_any all(c.first = m.subject == "approve", 
               c.second = m.amount == 1000),
           all(c.third = m.subject == "jumbo", 
               c.fourth = m.amount == 10000) do
    if first
      puts "Approved #{first.subject} #{second.amount}"
    else
      puts "Approved #{third.subject} #{fourth.amount}"
    end
  end
  
  when_start do
    post :expense, { :subject => "approve" }
    post :expense, { :amount => 1000 }
    post :expense, { :subject => "jumbo" }
    post :expense, { :amount => 10000 }
  end
end

Durable.run_all
```  
[top](reference.md#table-of-contents)  

### Lack of Information
In some cases lack of information is meaningful. The `none` function can be used in rules with correlated sequences to evaluate the lack of information.  

*Note: the `none` function requires information to reason about lack of information. That is, it will not trigger any actions if no events or facts have been registered in the corresponding rule.*

```ruby
require "durable"

Durable.ruleset :risk do
  when_all c.first = m.t == "deposit",
           none(m.t == "balance"),
           c.third = m.t == "withrawal",
           c.fourth = m.t == "chargeback" do
    puts "fraud detected #{first.t} #{third.t} #{fourth.t}"
  end
  
  when_start do
    post :risk, { :t => "deposit" }
    post :risk, { :t => "withrawal" }
    post :risk, { :t => "chargeback" }
  end
end

Durable.run_all
```  

[top](reference.md#table-of-contents)  

### Nested Objects
Queries on nested events or facts are also supported. The `.` notation is used for defining conditions on properties in nested objects.  

```ruby
require "durable"

Durable.ruleset :expense do
  when_all c.bill = (m.t == "bill") & (m.invoice.amount > 50),
           c.account = (m.t == "account") & (m.payment.invoice.amount == bill.invoice.amount) do
    puts "bill amount -> #{bill.invoice.amount}" 
    puts "account payment amount -> #{account.payment.invoice.amount}" 
  end
  
  when_start do
    post :expense, { t:"bill", :invoice => { :amount => 1000 }}
    post :expense, { t:"account", :payment => { :invoice => { :amount => 1000 }}}
  end
end

Durable.run_all
```  
[top](reference.md#table-of-contents)  

### Facts and Events as rvalues

Aside from scalars (strings, number and boolean values), it is possible to use the fact or event observed on the right side of an expression. This allows for efficient evaluation in the scripting client before reaching the Redis backend.  

```ruby
require "durable"

Durable.ruleset :risk do
  # compares properties in the same event, this expression is evaluated in the client 
  when_all m.debit > m.credit * 2 do
    puts "debit #{m.debit} more than twice the credit #{m.credit}"
  end
  # compares two correlated events, this expression is evaluated in the backend
  when_all c.first = m.amount > 100,
           c.second = m.amount > first.amount + m.amount / 2  do
    puts "fraud detected -> #{first.amount}"
    puts "fraud detected -> #{second.amount}"
  end

  when_start do
    post :risk, { :debit => 220, :credit => 100 }
    post :risk, { :debit => 150, :credit => 100 }
    post :risk, { :amount => 200 }
    post :risk, { :amount => 500 }
  end
end

Durable.run_all
```

[top](reference.md#table-of-contents) 

## Consequents
### Conflict Resolution
Event and fact evaluation can lead to multiple consequents. The triggering order can be controlled by using the `pri` (salience) function. Actions with lower value are executed first. The default value for all actions is 0.

In this example, notice how the last rule is triggered first, as it has the highest priority.
```ruby
require "durable"

Durable.ruleset :attributes do
  when_all pri(3), m.amount < 300 do
    puts "attributes P3 -> #{m.amount}"
  end

  when_all pri(2), m.amount < 200 do
    puts "attributes P2 -> #{m.amount}"
  end

  when_all pri(1), m.amount < 100  do
    puts "attributes P1 -> #{m.amount}"
  end

  when_start do
    assert :attributes, { :amount => 50 }
    assert :attributes, { :amount => 150 }
    assert :attributes, { :amount => 250 }
  end
end

Durable.run_all
```  
[top](reference.md#table-of-contents)  
### Action Batches
When a high number of events or facts satisfy a consequent, the consequent results can be delivered in batches.

* count: defines the exact number of times the rule needs to be satisfied before scheduling the action.   
* cap: defines the maximum number of times the rule needs to be satisfied before scheduling the action.  

This example batches exaclty three approvals and caps the number of rejects to two:  
```ruby
require "durable"

Durable.ruleset :expense do
  # this rule will trigger as soon as three events match the condition
  when_all count(3), m.amount < 100 do
    for f in m do
      puts "approved ->#{f}"
    end
  end

  # this rule will be triggered when 'expense' is asserted batching at most two results
  when_all cap(2), 
           c.expense = m.amount >= 100,
           c.approval = m.review == true do
    for f in m do
      puts "rejected ->#{f}"
    end
  end

  when_start do
    post_batch :expense, { :amount => 10 },
                         { :amount => 20 },
                         { :amount => 100 },
                         { :amount => 30 },
                         { :amount => 200 },
                         { :amount => 400 }
    assert :expense, { :review => true }
  end
end

Durable.run_all
```  
[top](reference.md#table-of-contents)  
### Async Actions  
The consequent action can be asynchronous. When the action is finished, the `complete` function has to be called. By default an action is considered abandoned after 5 seconds. This value can be changed by returning a different number in the action function or extended by calling `renew_action_lease`.

```ruby
require "durable"

Durable.ruleset :flow do
  # async actions take a callback argument to signal completion
  when_all s.state == "first" do |c, complete|
    Thread.new do
      sleep 3
      s.state = "second"
      puts "first completed"
      complete.call nil
    end
  end

  when_all s.state == "second" do |c, complete|
    Thread.new do
      sleep 6
      s.state = "third"
      puts "second completed"

      # completes the action after 6 seconds
      # use the first argument to signal an error
      complete.call Exception('error detected')
    end
    # overrides the 5 second default abandon timeout
    10
  end
  
  when_start do
    patch_state :flow, { :state => "first" }
  end
end

Durable.run_all
```  
[top](reference.md#table-of-contents)  
### Unhandled Exceptions  
When exceptions are not handled by actions, they are stored in the context state. This enables writing exception handling rules.

```ruby
require "durable"

Durable.ruleset :flow do
  when_all m.action == "start" do
    raise "Unhandled Exception!"
  end
  
  # when the exception property exists
  when_all +s.exception do
    puts "#{s.exception}"
    s.exception = nil
  end

  when_start do
    post :flow, { :action => "start" }
  end
end

Durable.run_all
```  
[top](reference.md#table-of-contents)  
### Fault Tolerance  
Consequent execution is transactional and atomic. That is, if the process crashes in the middle of a consequent execution, no facts will be asserted nor retracted, no events will posted and the context state will not be changed. The consequent will be retried a few seconds after process restart.  

```ruby
require "durable"

Durable.ruleset :flow do

  when_all m.status == "start" do
    post :status => "next"
    puts "start"
  end
  # the process will always exit here every time the action is run
  # when restarting the process this action will be retried after a few seconds
  when_all m.status == "next" do
    post :status => "last"
    puts "next"
    Process.kill 9, Process.pid
  end

  when_all m.status == "last" do
    puts "last"
  end
  
  when_start do
    post :flow, { :status => "start" }
  end
end


Durable.run_all
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

```ruby
require "durable"

Durable.statechart :expense do
  # initial state :input with two triggers
  state :input do
    # trigger to move to :denied given a condition
    to :denied, when_all((m.subject == "approve") & (m.amount > 1000)) do
      # action executed before state change
      puts "denied amount #{m.amount}"
    end

    to :pending, when_all((m.subject == "approve") & (m.amount <= 1000)) do
      puts "requesting approve amount #{m.amount}"
    end
  end  

  # intermediate state :pending with two triggers
  state :pending do
    to :approved, when_all(m.subject == "approved") do
      puts "expense approved"
    end

    to :denied, when_all(m.subject == "denied") do
      puts "expense denied"
    end
  end

  state :approved
  state :denied

  when_start do
    # events directed to default statechart instance
    post :expense, { :subject => 'approve', :amount => 100 }
    post :expense, { :subject => 'approved' }

    # events directed to statechart instance with id '1'
    post :expense, { :sid => 1, :subject => 'approve', :amount => 100 }
    post :expense, { :sid => 1, :subject => 'denied' }

    # events directed to statechart instance with id '2'
    post :expense, { :sid => 2, :subject => 'approve', :amount => 10000 }
  end
end

Durable.run_all
```  
[top](reference.md#table-of-contents)  
### Nested States
Nested states allow for writing compact statecharts. If a context is in the nested state, it also (implicitly) is in the surrounding state. The statechart will attempt to handle any event in the context of the sub-state. If the sub-state does not  handle an event, the event is automatically handled at the context of the super-state.

```ruby
require "durable"

Durable.statechart :worker do
  # super-state :work has two states and one trigger
  state :work do
    # sub-sate :enter has only one trigger   
    state :enter do
      to :process, when_all(m.subject == "enter") do
        puts "start process"
      end
    end

    state :process do
      to :process, when_all(m.subject == "continue") do
        puts "continue processing"
      end
    end

    # the super-state trigger will be evaluated for all sub-state triggers
    to :canceled, when_all(pri(1), m.subject == "cancel") do
      puts "cancel process"
    end
  end

  state :canceled
  
  when_start do
    # will move the statechart to the 'work.process' sub-state
    post :worker, { :subject => "enter" }

     # will keep the statechart to the 'work.process' sub-state
    post :worker, { :subject => "continue" }
    post :worker, { :subject => "continue" }

    # will move the statechart out of the work state
    post :worker, { :subject => "cancel" }
  end
end

Durable.run_all
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

```ruby
require "durable"

Durable.flowchart :expense do
  # initial stage :input has two conditions
  stage :input
  to :request, when_all((m.subject == "approve") & (m.amount <= 1000))
  to :deny, when_all((m.subject == "approve") & (m.amount > 1000))
  
  # intermediate stage :request has an action and three conditions
  stage :request do
    puts "requesting approve"
  end
  to :approve, when_all(m.subject == "approved")
  to :deny, when_all(m.subject == "denied")
  # reflexive condition: if met, returns to the same stage
  to :request, when_any(m.subject == "retry")
  
  stage :approve do
    puts "expense approved"
  end

  stage :deny do
    puts "expense denied"
  end

  when_start do
    # events for the default flowchart instance, approved after retry
    post :expense, { :subject => "approve", :amount => 100 }
    post :expense, { :subject => "retry" }
    post :expense, { :subject => "approved" }

    # events for the flowchart instance '1', denied after first try
    post :expense, {:sid => 1, :subject => "approve", :amount => 100}
    post :expense, {:sid => 1, :subject => "denied"}

     # event for the flowchart instance '2' immediately denied    
    post :expense, {:sid => 2, :subject => "approve", :amount => 10000}
  end
end

Durable.run_all
```  
[top](reference.md#table-of-contents)  
### Timers
Events can be scheduled with timers. A timeout condition can be included in the rule antecedent. By default a timeuot is triggered as an event (observed only once). Timeouts can also be triggered as facts by 'manual reset' timers, the timers can be reset during action execution (see last example). 

* start_timer: starts a timer with the name and duration specified (manual_reset is optional).
* reset_timer: resets a 'manual reset' timer.
* cancel_timer: cancels ongoing timer.
* timeout: used as an antecedent condition.

In this example, the timer can be canceled by running the following command:  

<sub>`curl -H "content-type: application/json" -X POST -d '{"cancel": true}' http://localhost:4567/timer/events`</sub>  

```ruby
require "durable"

Durable.ruleset :timer do
  when_any all(s.count == 0),
           # will trigger when MyTimer expires
           all(s.count < 5, 
               timeout("MyTimer")) do
    s.count += 1
    # MyTimer will expire in 5 seconds
    start_timer "MyTimer", 5
    puts "pulse -> #{Time.now}"
  end

  when_all m.cancel == true do
    cancel_timer "MyTimer"
    puts "canceled timer"
  end

  when_start do
    patch_state :timer, { :count => 0 }
  end
end

Durable.run_all
```  

The example below uses a timer to detect higher event rate:  

```ruby
require "durable"

Durable.statechart :risk do
  state :start do
    to :meter do
      start_timer "RiskTimer", 5
    end
  end  

  state :meter do
    to :fraud, when_all(count(3), c.message = m.amount > 100) do
      for e in m do
        puts e.message
      end
    end

    to :exit, when_all(timeout("RiskTimer")) do
      puts "exit"
    end
  end

  state :fraud
  state :exit

  when_start do
    # three events in a row will trigger the fraud rule
    post 'risk', { :amount => 200 } 
    post 'risk', { :amount => 300 } 
    post 'risk', { :amount => 400 }

    # two events will exit after 5 seconds
    post 'risk', { :sid => 1, :amount => 500 } 
    post 'risk', { :sid => 1, :amount => 600 } 
  end
end

Durable.run_all
```  

In this example a manual reset timer is used for measuring velocity. Try issuing the command below multiple times.

<sub>`curl -H "content-type: application/json" -X POST -d '{"amount": 200}' http://localhost:4567/risk/events`</sub>  

```ruby
require "durable"

Durable.statechart :risk do
  state :start do
    to :meter do
      # will start a manual reset timer
      start_timer "VelocityTimer", 5, true
    end
  end  

  state :meter do
    to :meter, when_all(cap(100), 
                        c.message = m.amount > 100,
                        timeout("VelocityTimer")) do
      puts "velocity: #{m.length} events in 5 seconds"
      # resets and restarts the manual reset timer
      reset_timer "VelocityTimer"
      start_timer "VelocityTimer", 5, true
    end

    to :meter, when_all(timeout("VelocityTimer")) do
      puts "velocity: no events in 5 seconds"
      reset_timer "VelocityTimer"
      start_timer "VelocityTimer", 5, true
    end
  end

  when_start do
    # the velocity will 4 events in 5 seconds
    post 'risk', { :amount => 200 } 
    post 'risk', { :amount => 300 } 
    post 'risk', { :amount => 50 }
    post 'risk', { :amount => 300 } 
    post 'risk', { :amount => 400 }
  end
end

Durable.run_all
```  

[top](reference.md#table-of-contents)  

