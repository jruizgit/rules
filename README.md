  
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

Using your scripting language of choice, simply describe the event to match (antecedent) and the action to take (consequent). In this example the rule can be triggered by posting `{"id": 1, "subject": "World"}` to url `http://localhost:5000/test/1`. 

<sub>Tip: once the test is running, from a terminal type:   
`curl -H "Content-type: application/json" -X POST -d '{"id": 1, "subject": "World"}' http://localhost:5000/test/1`</sub>

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
### Node.js
JavaScript
```javascript
var d = require('durable');
var m = d.m, s = d.s, c = d.c;

d.ruleset('test', {
    // antecedent
    whenAll: m.subject.eq('World'),
    // consequent
    run: function(c) { console.log('Hello ' + c.m.subject); }
});

d.runAll();
```
TypeScript
```typescript
import * as d from 'durable';
let m = d.m, s = d.s, c = d.c;

d.ruleset('test', {
    // antecedent
    whenAll: m['subject'].eq('World'),
    // consequent
    run: (c) => { console.log('Hello ' + c.m['subject']); }
});

d.runAll();
```
## Pattern Matching

durable_rules provides string pattern matching. Expressions are compiled down to a DFA, guaranteeing linear execution time in the order of single digit nano seconds per character (note: backtracking expressions are not supported).

<sub>Tip: once the test is running, from a terminal type:  
`curl -H "Content-type: application/json" -X POST -d '{"id": 1, "subject": "375678956789765"}' http://localhost:5000/test/1`  
`curl -H "Content-type: application/json" -X POST -d '{"id": 2, "subject": "4345634566789888"}' http://localhost:5000/test/1`  
`curl -H "Content-type: application/json" -X POST -d '{"id": 3, "subject": "2228345634567898"}' http://localhost:5000/test/1`  
</sub>

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
### Node.js
JavaScript
```javascript
var d = require('durable');
var m = d.m, s = d.s, c = d.c;

d.ruleset('test', {
    whenAll: m.subject.mt('3[47][0-9]{13}'),
    run: function(c) { console.log('Amex detected in ' + c.m.subject); }
}, {
    whenAll: m.subject.mt('4[0-9]{12}([0-9]{3})?'),
    run: function(c) { console.log('Visa detected in ' + c.m.subject); }
}, {
    whenAll: m.subject.mt('(5[1-5][0-9]{2}|222[1-9]|22[3-9][0-9]|2[3-6][0-9]{2}|2720)[0-9]{12}'),
    run: function(c) { console.log('Mastercard detected in ' + c.m.subject); }
});

d.runAll();
```
TypeScript
```typescript
import * as d from 'durable';
let m = d.m, s = d.s, c = d.c;

d.ruleset('test', {
    whenAll: m['subject'].mt('3[47][0-9]{13}'),
    run: (c) => { console.log('Amex detected in ' + c.m['subject']); }
},{
    whenAll: m['subject'].mt('4[0-9]{12}([0-9]{3})?'),
    run: (c) => { console.log('Visa detected in ' + c.m['subject']); }
},{
    whenAll: m['subject'].mt('(5[1-5][0-9]{2}|222[1-9]|22[3-9][0-9]|2[3-6][0-9]{2}|2720)[0-9]{12}'),
    run: (c) => { console.log('Mastercard detected in ' + c.m['subject']); }
});

d.runAll();
```
## Forward Inference  

durable_rules super-power is the ability to define forward reasoning rules. In other words, rules to derive an action based on a set of correlated facts or observed events. The example below illustrates this basic building block by calculating the first 100 numbers of the Fibonacci series.

### Ruby
```ruby
require "durable"

Durable.ruleset :fibonacci do
  when_all(c.first = (m.value != 0),
           c.second = (m.id == first.id + 1)) do
    puts "Value: #{first.value}"
    if second.id > 100
        puts "Value: #{second.value}"
    else
        assert(:id => second.id + 1, :value => first.value + second.value)
        retract first
    end
  end
  when_start do
    assert :fibonacci, {:id => 1, :sid => 1, :value => 1}
    assert :fibonacci, {:id => 2, :sid => 1, :value => 1}
  end
end
Durable.run_all
```
### Python
```python
from durable.lang import *

with ruleset('fibonacci'):
    @when_all(c.first << (m.value != 0),
              c.second << (m.id == c.first.id + 1))
    def calculate(c):
        print('Value: {0}'.format(c.first.value))
        if c.second.id > 100:
            print('Value: {0}'.format(c.second.value))
        else:
            c.assert_fact({'id': c.second.id + 1, 'value': c.first.value + c.second.value})
            c.retract_fact(c.first)
    
    @when_start
    def start(host):
        host.assert_fact('fibonacci', {'id': 1, 'sid': 1, 'value': 1})
        host.assert_fact('fibonacci', {'id': 2, 'sid': 1, 'value': 1})
        
run_all()
```
### Node.js
JavaScript  
```javascript
var d = require('durable');
var m = d.m, s = d.s, c = d.c;

d.ruleset('fibonacci', {
        whenAll: [
            c.first = m.value.neq(0),
            c.second = m.id.eq(c.first.id.add(1))
        ],
        run: function(c) { 
            console.log('Value: ' + c.first.value);
            if (c.second.id > 100) {
                console.log('Value: ' + c.second.value);
            } else {
                c.assert({id: c.second.id + 1, value: c.first.value + c.second.value});
                c.retract(c.first);
            }
        }
    }, 
    function(host) {
        host.assert('fibonacci', { id: 1, sid: 1, value: 1 });
        host.assert('fibonacci', { id: 2, sid: 1, value: 1 });
    }
);

d.runAll();
```
TypeScript  
```typescript
import * as d from 'durable';
let m = d.m, s = d.s, c = d.c;

d.ruleset('fibonacci', {
        whenAll: [
            c['first'] = m['value'].neq(0),
            c['second'] = m['id'].eq(c['first']['id'].add(1))
        ],
        run: (c) => { 
            console.log('Value: ' + c['first']['value']);
            if (c['second']['id'] > 100) {
                console.log('Value: ' + c['second']['value']);
            } else {
                c.assert({id: c['second']['id']+1, value: c['first']['value'] + c['second']['value']});
                c.retract(c['first']);
            }
        }
    }, 
    function(host) {
        host.assert('fibonacci', { id: 1, sid: 1, value: 1 });
        host.assert('fibonacci', { id: 2, sid: 1, value: 1 });
    }
);

d.runAll();
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

