<sub>*note 1: The new node.js syntax was just pushed as of npm 0.36.57. The old syntax is still supported. Your feedback is welcomed.*</sub>  
<sub>*note 2: Passing 'sid' and 'id' in events and facts is no longer required as of npm 0.36.59, gem 0.34.25 and pypi 0.33.96.*</sub> 


# durable_rules    
#### for real time analytics (a Ruby, Python and Node.js Rules Engine)
[![Build Status](https://travis-ci.org/jruizgit/rules.svg?branch=master)](https://travis-ci.org/jruizgit/rules)
[![Gem Version](https://badge.fury.io/rb/durable_rules.svg)](https://badge.fury.io/rb/durable_rules)
[![npm version](https://badge.fury.io/js/durable.svg)](https://badge.fury.io/js/durable)
[![PyPI version](https://badge.fury.io/py/durable_rules.svg)](https://badge.fury.io/py/durable_rules)  

durable_rules is a polyglot micro-framework for real-time, consistent and scalable coordination of events. With durable_rules you can track and analyze information about things that happen (events) by combining data from multiple sources to infer more complicated circumstances.

A full forward chaining implementation (A.K.A. Rete) is used to evaluate facts and massive streams of events in real time. A simple, yet powerful meta-liguistic abstraction lets you define simple and complex rulesets as well as control flow structures such as flowcharts, statecharts, nested statecharts and time driven flows. 

The durable_rules core engine is implemented in C, which enables ultra fast rule evaluation as well as muti-language support. durable_rules relies on state of the art technologies: [Node.js](http://www.nodejs.org), [Werkzeug](http://werkzeug.pocoo.org/), [Sinatra](http://www.sinatrarb.com/) are used to host rulesets written in JavaScript, Python and Ruby respectively. Inference state is cached using [Redis](http://www.redis.io). This allows for fault tolerant execution and scale-out without giving up performance.  

durable_rules is cloud ready. It can easily be hosted and scaled in environments such as Amazon Web Services with EC2 instances and ElastiCache clusters. Or Heroku using web dynos and RedisLabs or RedisToGo.  

## Getting Started  

Using your scripting language of choice, simply describe the event to match (antecedent) and the action to take (consequent). In this example the rule can be triggered by posting `{ "subject": "World" }` to url `http://localhost:5000/test/events`. 

<sub>Tip: once the test is running, from a terminal type:   
`curl -H "Content-type: application/json" -X POST -d '{"subject": "World"}' http://localhost:5000/test/events`</sub>  

### Node.js
```javascript
var d = require('durable');

d.ruleset('test', function() {
    // antecedent
    whenAll: m.subject == 'World'
    // consequent
    run: console.log('Hello ' + m.subject)
});

d.runAll();
```  
### Python
```python
from durable.lang import *

with ruleset('test'):
    # antecedent
    @when_all(m.subject == 'World')
    def say_hello(c):
        # consequent
        print ('Hello {0}'.format(c.m.subject))

run_all()
```  
### Ruby
```ruby
require "durable"
Durable.ruleset :test do
  # antecedent
  when_all (m.subject == "World") do
    # consequent
    puts "Hello #{m.subject}"
  end
end
Durable.run_all
```  
## Forward Inference  

durable_rules super-power is the foward-chaining evaluation of rules. In other words, the repeated application of logical [modus ponens](https://en.wikipedia.org/wiki/Modus_ponens) to a set of facts or observed events to derive a conclusion. The example below shows a set of rules applied to a small knowledge base (set of facts).

### Node.js
```javascript
var d = require('durable');

d.ruleset('animal', function() {
    whenAll: {
        first = m.verb == 'eats' && m.predicate == 'flies' 
        m.verb == 'lives' && m.predicate == 'water' && m.subject == first.subject
    }
    run: assert({ subject: first.subject, verb: 'is', predicate: 'frog' })

    whenAll: {
        first = m.verb == 'eats' && m.predicate == 'flies' 
        m.verb == 'lives' && m.predicate == 'land' && m.subject == first.subject
    }
    run: assert({ subject: first.subject, verb: 'is', predicate: 'chameleon' })

    whenAll: m.verb == 'eats' && m.predicate == 'worms' 
    run: assert({ subject: m.subject, verb: 'is', predicate: 'bird' })

    whenAll: m.verb == 'is' && m.predicate == 'frog'
    run: assert({ subject: m.subject, verb: 'is', predicate: 'green' })

    whenAll: m.verb == 'is' && m.predicate == 'chameleon'
    run: assert({ subject: m.subject, verb: 'is', predicate: 'green' })

    whenAll: m.verb == 'is' && m.predicate == 'bird' 
    run: assert({ subject: m.subject, verb: 'is', predicate: 'black' })

    whenAll: +m.subject
    count: 11
    run: m.forEach(function(f, i) {console.log('fact: ' + f.subject + ' ' + f.verb + ' ' + f.predicate)})

    whenStart: {
        assert('animal', { subject: 'Kermit', verb: 'eats', predicate: 'flies' });
        assert('animal', { subject: 'Kermit', verb: 'lives', predicate: 'water' });
        assert('animal', { subject: 'Greedy', verb: 'eats', predicate: 'flies' });
        assert('animal', { subject: 'Greedy', verb: 'lives', predicate: 'land' });
        assert('animal', { subject: 'Tweety', verb: 'eats', predicate: 'worms' });
    }
});

d.runAll();
```
### Python
```python
from durable.lang import *

with ruleset('animal'):
    @when_all(c.first << (m.verb == 'eats') & (m.predicate == 'flies'),
              (m.verb == 'lives') & (m.predicate == 'water') & (m.subject == c.first.subject))
    def frog(c):
        c.assert_fact({ 'subject': c.first.subject, 'verb': 'is', 'predicate': 'frog' })

    @when_all(c.first << (m.verb == 'eats') & (m.predicate == 'flies'),
              (m.verb == 'lives') & (m.predicate == 'land') & (m.subject == c.first.subject))
    def chameleon(c):
        c.assert_fact({ 'subject': c.first.subject, 'verb': 'is', 'predicate': 'chameleon' })

    @when_all((m.verb == 'eats') & (m.predicate == 'worms'))
    def bird(c):
        c.assert_fact({ 'subject': c.m.subject, 'verb': 'is', 'predicate': 'bird' })

    @when_all((m.verb == 'is') & (m.predicate == 'frog'))
    def green(c):
        c.assert_fact({ 'subject': c.m.subject, 'verb': 'is', 'predicate': 'green' })

    @when_all((m.verb == 'is') & (m.predicate == 'chameleon'))
    def grey(c):
        c.assert_fact({ 'subject': c.m.subject, 'verb': 'is', 'predicate': 'grey' })

    @when_all((m.verb == 'is') & (m.predicate == 'bird'))
    def black(c):
        c.assert_fact({ 'subject': c.m.subject, 'verb': 'is', 'predicate': 'black' })

    @when_all(count(11), +m.subject)
    def output(c):
        for f in c.m:
            print ('Fact: {0} {1} {2}'.format(f.subject, f.verb, f.predicate))

    @when_start
    def start(host):
        host.assert_fact('animal', { 'subject': 'Kermit', 'verb': 'eats', 'predicate': 'flies' })
        host.assert_fact('animal', { 'subject': 'Kermit', 'verb': 'lives', 'predicate': 'water' })
        host.assert_fact('animal', { 'subject': 'Greedy', 'verb': 'eats', 'predicate': 'flies' })
        host.assert_fact('animal', { 'subject': 'Greedy', 'verb': 'lives', 'predicate': 'land' })
        host.assert_fact('animal', { 'subject': 'Tweety', 'verb': 'eats', 'predicate': 'worms' })
        
run_all()
```
### Ruby
```ruby
require "durable"

Durable.ruleset :animal do
  when_all c.first = (m.verb == "eats") & (m.predicate == "flies"),  
          (m.verb == "lives") & (m.predicate == "water") & (m.subject == first.subject) do
    assert :subject => first.subject, :verb => "is", :predicate => "frog"
  end

  when_all c.first = (m.verb == "eats") & (m.predicate == "flies"),  
          (m.verb == "lives") & (m.predicate == "land") & (m.subject == first.subject) do
    assert :subject => first.subject, :verb => "is", :predicate => "chameleon"
  end

  when_all (m.verb == "eats") & (m.predicate == "worms") do
    assert :subject => m.subject, :verb => "is", :predicate => "bird"
  end
  
  when_all (m.verb == "is") & (m.predicate == "frog") do
    assert :subject => m.subject, :verb => "is", :predicate => "green"
  end
    
  when_all (m.verb == "is") & (m.predicate == "chameleon") do
    assert :subject => m.subject, :verb => "is", :predicate => "green"
  end

  when_all (m.verb == "is") & (m.predicate == "bird") do
    assert :subject => m.subject, :verb => "is", :predicate => "black"
  end
    
  when_all +m.subject, count(11) do
    m.each { |f| puts "fact: #{f.subject} #{f.verb} #{f.predicate}" }
  end
    
  when_start do
    assert :animal, { :subject => "Kermit", :verb => "eats", :predicate => "flies" }
    assert :animal, { :subject => "Kermit", :verb => "lives", :predicate => "water" }
    assert :animal, { :subject => "Greedy", :verb => "eats", :predicate => "flies" }
    assert :animal, { :subject => "Greedy", :verb => "lives", :predicate => "land" }
    assert :animal, { :subject => "Tweety", :verb => "eats", :predicate => "worms" }
  end
end

Durable.run_all
```
## Flow Structures

The combination of forward inference and durable_rules tolerance to failures on rule action dispatch, enables work coordination with data flow structures such as statecharts, nested states and flowcharts. 

<sub>Tip: once the test is running, from a terminal type:   
`curl -H "Content-type: application/json" -X POST -d '{"subject": "approve", "amount": 100}' http://localhost:5000/expense/events`  
`curl -H "Content-type: application/json" -X POST -d '{"subject": "approved"}' http://localhost:5000/expense/events`  
`curl -H "Content-type: application/json" -X POST -d '{"subject": "approve", "amount": 100}' http://localhost:5000/expense/events/2`  
`curl -H "Content-type: application/json" -X POST -d '{"subject": "denied"}' http://localhost:5000/expense/events/2`  
</sub>
### Node.js
```javascript
d.statechart('expense', function() {
    input: {
        to: 'denied'
        whenAll: m.subject == 'approve' && m.amount > 1000
        run: console.log('expense denied')

        to: 'pending'
        whenAll: m.subject == 'approve' && m.amount <= 1000
        run: console.log('requesting expense approval')
    }

    pending: {
        to: 'approved'
        whenAll: m.subject == 'approved'
        run: console.log('expense approved')
            
        to: 'denied'
        whenAll: m.subject == 'denied'
        run: console.log('expense denied')
    }
    
    denied: {}
    approved: {}
});

d.runAll();
```
### Python
```python
from durable.lang import *

with statechart('expense'):
    with state('input'):
        @to('denied')
        @when_all((m.subject == 'approve') & (m.amount > 1000))
        def denied(c):
            print ('expense denied')
        
        @to('pending')    
        @when_all((m.subject == 'approve') & (m.amount <= 1000))
        def request(c):
            print ('requesting expense approval')
        
    with state('pending'):
        @to('approved')
        @when_all(m.subject == 'approved')
        def approved(c):
            print ('expense approved')
            
        @to('denied')
        @when_all(m.subject == 'denied')
        def denied(c):
            print ('expense denied')
        
    state('denied')
    state('approved')
    
run_all()
```
### Ruby
```ruby
require "durable"

Durable.statechart :expense do
  state :input do
    to :denied, when_all((m.subject == "approve") & (m.amount > 1000)) do
      puts "expense denied"
    end
    to :pending, when_all((m.subject == "approve") & (m.amount <= 1000)) do
      puts "requesting expense approval"
    end
  end  
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
end

Durable.run_all
```
## Pattern Matching

durable_rules provides string pattern matching. Expressions are compiled down to a DFA, guaranteeing linear execution time in the order of single digit nano seconds per character (note: backtracking expressions are not supported).

<sub>Tip: once the test is running, from a terminal type:  
`curl -H "Content-type: application/json" -X POST -d '{"subject": "375678956789765"}' http://localhost:5000/test/events`  
`curl -H "Content-type: application/json" -X POST -d '{"subject": "4345634566789888"}' http://localhost:5000/test/events`  
`curl -H "Content-type: application/json" -X POST -d '{"subject": "2228345634567898"}' http://localhost:5000/test/events`  
</sub>

### Node.js
```javascript
var d = require('durable');

d.ruleset('test', function() {
    whenAll: m.subject.matches('3[47][0-9]{13}')
    run: console.log('Amex detected in ' + m.subject)
    
    whenAll: m.subject.matches('4[0-9]{12}([0-9]{3})?')
    run: console.log('Visa detected in ' + m.subject)
    
    whenAll: m.subject.matches('(5[1-5][0-9]{2}|222[1-9]|22[3-9][0-9]|2[3-6][0-9]{2}|2720)[0-9]{12}')
    run: console.log('Mastercard detected in ' + m.subject)
});

d.runAll();
```
### Python
```python
from durable.lang import *

with ruleset('test'):
    @when_all(m.subject.matches('3[47][0-9]{13}'))
    def amex(c):
        print ('Amex detected {0}'.format(c.m.subject))

    @when_all(m.subject.matches('4[0-9]{12}([0-9]{3})?'))
    def visa(c):
        print ('Visa detected {0}'.format(c.m.subject))

    @when_all(m.subject.matches('(5[1-5][0-9]{2}|222[1-9]|22[3-9][0-9]|2[3-6][0-9]{2}|2720)[0-9]{12}'))
    def mastercard(c):
        print ('Mastercard detected {0}'.format(c.m.subject))

run_all()
```
### Ruby
```ruby
require "durable"
Durable.ruleset :test do
  when_all m.subject.matches('3[47][0-9]{13}') do
    puts "Amex detected in #{m.subject}"
  end
  when_all m.subject.matches('4[0-9]{12}([0-9]{3})?') do
    puts "Visa detected in #{m.subject}"
  end
  when_all m.subject.matches('(5[1-5][0-9]{2}|222[1-9]|22[3-9][0-9]|2[3-6][0-9]{2}|2720)[0-9]{12}') do
    puts "Mastercard detected in #{m.subject}"
  end
end
Durable.run_all
```
## Business Rules and Miss Manners 

durable_rules can also be used to solve traditional Production Bussiness Rules problems. This example is an industry benchmark. Miss Manners has decided to throw a party. She wants to seat her guests such that adjacent people are of opposite sex and share at least one hobby. 

Note how the benchmark flow structure is defined using a statechart to improve code readability without sacrificing performance nor altering the combinatorics required by the benchmark. For 128 guests, 438 facts, the execution time is less than 2 seconds in JavaScript and Python slightly above 2 seconds in Ruby. More details documented in this [blog post](http://jruizblog.com/2015/07/20/miss-manners-and-waltzdb/).   

<div align="center"><img src="https://raw.github.com/jruizgit/rules/master/docs/manners.jpg" width="800px" height="300px" /></div>  

_IMac, 4GHz i7, 32GB 1600MHz DDR3, 1.12 TB Fusion Drive_    

### [Ruby](https://github.com/jruizgit/rules/blob/master/testrb/manners.rb)

### [Python](https://github.com/jruizgit/rules/blob/master/testpy/manners.py)

### [Node.js](https://github.com/jruizgit/rules/blob/master/testjs/manners.js)

## Image recognition and Waltzdb

Waltzdb is a constraint propagation problem for image recognition: given a set of lines in a 2D space, the system needs to interpret the 3D depth of the image. The first part of the algorithm consists of identifying four types of junctions, then labeling the junctions following Huffman-Clowes notation. Pairs of adjacent junctions constraint each other’s edge labeling. So, after choosing the labeling for an initial junction, the second part of the algorithm iterates through the graph, propagating the labeling constraints by removing inconsistent labels.  

In this case too, the benchmark flow structure is defined using a statechart to improve code readability. The benchmark requirements are not altered. Execution time is around 3 seconds for the case of 4 regions and around 20 for the case of 50. More details documented in this [blog post](http://jruizblog.com/2015/07/20/miss-manners-and-waltzdb/).  

<div align="center"><img src="https://raw.github.com/jruizgit/rules/master/docs/waltzdb.jpg" width="800px" height="300px" /></div>  

_IMac, 4GHz i7, 32GB 1600MHz DDR3, 1.12 TB Fusion Drive_    

### [Ruby](https://github.com/jruizgit/rules/blob/master/testrb/waltzdb.rb)

### [Python](https://github.com/jruizgit/rules/blob/master/testpy/waltzdb.py)

### [Node.js](https://github.com/jruizgit/rules/blob/master/testjs/waltzdb.js)

## To Learn More  
Reference Manual:  
* [Ruby](https://github.com/jruizgit/rules/blob/master/docs/rb/reference.md)  
* [Python](https://github.com/jruizgit/rules/blob/master/docs/py/reference.md)  
* [Node.js](https://github.com/jruizgit/rules/blob/master/docs/js/reference.md)  

Blog:  
* [Miss Manners and Waltzdb (07/2015)](http://jruizblog.com/2015/07/20/miss-manners-and-waltzdb/)
* [Polyglot (03/2015)](http://jruizblog.com/2015/03/02/polyglot/)  
* [Rete_D (02/2015)](http://jruizblog.com/2015/02/23/rete_d/)
* [Boosting Performance with C (08/2014)](http://jruizblog.com/2014/08/19/boosting-performance-with-c/)
* [Rete Meets Redis (02/2014)](http://jruizblog.com/2014/02/02/rete-meets-redis/)
* [Inference: From Expert Systems to Cloud Scale Event Processing (01/2014)](http://jruizblog.com/2014/01/27/event-processing/)

