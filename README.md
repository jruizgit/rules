  
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

Using your scripting language of choice, you simply need to describe the event to match (antecedent) and the action to take (consequent). In this example the rule can be triggered by posting `{"id": 1, "subject": "World"}` to url `http://localhost:5000/test/1`. 

<sub>Tip: once the test is running, from a terminal type: `curl -H "Content-type: application/json" -X POST -d '{"id": 1, "subject": "World"}' http://localhost:5000/test/1`</sub>

#### Ruby
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
#### Python
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
#### Node.js
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

#### Ruby
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
#### Python
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
#### Node.js
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
## Event Processing and Fraud Detection  

Let’s consider a couple of fictitious fraud rules used in bank account management.  
Note: I'm paraphrasing the example presented in this [article](https://www.packtpub.com/books/content/drools-jboss-rules-50complex-event-processing).  

1. If there are two debit requests greater than 200% the average monthly withdrawal amount in a span of 2 minutes, flag the account as medium risk.
2. If there are three consecutive increasing debit requests, withdrawing more than 70% the average monthly balance in a span of three minutes, flag the account as high risk.


#### Ruby
```ruby
require "durable"

Durable.ruleset :fraud_detection do
  # compute monthly averages
  when_all span(86400), (m.t == "debit_cleared") | (m.t == "credit_cleared") do
    debit_total = 0
    credit_total = 0
    for tx in m do
      if tx.t == "debit_cleared"
        debit_total += tx.amount
      else
        credit_total += tx.amount
      end
    end

    s.balance = s.balance - debit_total + credit_total
    s.avg_balance = (s.avg_balance * 29 + s.balance) / 30
    s.avg_withdraw = (s.avg_withdraw * 29 + debit_total) / 30
  end

  # medium risk rule
  when_all c.first = (m.t == "debit_request") & 
                     (m.amount > s.avg_withdraw * 2),
           c.second = (m.t == "debit_request") & 
                      (m.amount > s.avg_withdraw * 2) & 
                      (m.stamp > first.stamp) &
                      (m.stamp < first.stamp + 120) do
    puts "Medium risk"
  end

  # high risk rule
  when_all c.first = m.t == "debit_request",
           c.second = (m.t == "debit_request") &
                      (m.amount > first.amount) & 
                      (m.stamp < first.stamp + 180),
           c.third = (m.t == "debit_request") & 
                     (m.amount > second.amount) & 
                     (m.stamp < first.stamp + 180),
           s.avg_balance < (first.amount + second.amount + third.amount) / 0.7 do
    puts "High risk"
  end
end

Durable.run_all
```
#### Python
```python
from durable.lang import *

with ruleset('fraud_detection'):
    # compute monthly averages
    @when_all(span(86400), (m.t == 'debit_cleared') | (m.t == 'credit_cleared'))
    def handle_balance(c):
        debit_total = 0
        credit_total = 0
        for tx in c.m:
            if tx.t == 'debit_cleared':
                debit_total += tx.amount
            else:
                credit_total += tx.amount

        c.s.balance = c.s.balance - debit_total + credit_total
        c.s.avg_balance = (c.s.avg_balance * 29 + c.s.balance) / 30
        c.s.avg_withdraw = (c.s.avg_withdraw * 29 + debit_total) / 30
    
    # medium risk rule
    @when_all(c.first << (m.t == 'debit_request') & 
                         (m.amount > c.s.avg_withdraw * 2),
              c.second << (m.t == 'debit_request') & 
                          (m.amount > c.s.avg_withdraw * 2) & 
                          (m.stamp > c.first.stamp) &
                          (m.stamp < c.first.stamp + 120))
    def first_rule(c):
        print('Medium Risk')

    # high risk rule
    @when_all(c.first << m.t == 'debit_request',
              c.second << (m.t == 'debit_request') &
                          (m.amount > c.first.amount) & 
                          (m.stamp < c.first.stamp + 180),
              c.third << (m.t == 'debit_request') & 
                         (m.amount > c.second.amount) & 
                         (m.stamp < c.first.stamp + 180),
              s.avg_balance < (c.first.amount + c.second.amount + c.third.amount) / 0.7)
    def second_rule(c):
        print('High Risk')

run_all()
```
#### Node.js
```javascript
var d = require('durable');

with (d.ruleset('fraudDetection')) {
    // compute monthly averages
    whenAll(span(86400), or(m.t.eq('debitCleared'), m.t.eq('creditCleared')), 
    function(c) {
        var debitTotal = 0;
        var creditTotal = 0;
        for (var i = 0; i < c.m.length; ++i) {
            if (c.m[i].t === 'debitCleared') {
                debitTotal += c.m[i].amount;
            } else {
                creditTotal += c.m[i].amount;
            }
        }

        c.s.balance = c.s.balance - debitTotal + creditTotal;
        c.s.avgBalance = (c.s.avgBalance * 29 + c.s.balance) / 30;
        c.s.avgWithdraw = (c.s.avgWithdraw * 29 + debitTotal) / 30;
    });

    // medium risk rule
    whenAll(c.first = and(m.t.eq('debitRequest'), 
                          m.amount.gt(c.s.avgWithdraw.mul(2))),
            c.second = and(m.t.eq('debitRequest'),
                           m.amount.gt(c.s.avgWithdraw.mul(2)),
                           m.stamp.gt(c.first.stamp),
                           m.stamp.lt(c.first.stamp.add(120))),
    function(c) {
        console.log('Medium risk');
    });

    // high risk rule 
    whenAll(c.first = m.t.eq('debitRequest'),
            c.second = and(m.t.eq('debitRequest'),
                           m.amount.gt(c.first.amount),
                           m.stamp.lt(c.first.stamp.add(180))),
            c.third = and(m.t.eq('debitRequest'),
                          m.amount.gt(c.second.amount),
                          m.stamp.lt(c.first.stamp.add(180))),
            s.avgBalance.lt(add(c.first.amount, c.second.amount, c.third.amount).div(0.7)),
    function(c) {
        console.log('High risk');
    });
}

d.runAll();
```

## Business Rules and Miss Manners 

durable_rules can also be used to solve traditional Production Bussiness Rules problems. This example is an industry benchmark. Miss Manners has decided to throw a party. She wants to seat her guests such that adjacent people are of opposite sex and share at least one hobby. 

Note how the benchmark flow structure is defined using a statechart to improve code readability without sacrificing performance nor altering the combinatorics required by the benchmark. For 128 guests, 438 facts, the execution time is less than 2 seconds in JavaScript and Python slightly above 2 seconds in Ruby. More details documented in this [blog post](http://jruizblog.com/2015/07/20/miss-manners-and-waltzdb/).   

<div align="center"><img src="https://raw.github.com/jruizgit/rules/master/docs/manners.jpg" width="800px" height="300px" /></div>  

_IMac, 4GHz i7, 32GB 1600MHz DDR3, 1.12 TB Fusion Drive_    

#### [Ruby](https://github.com/jruizgit/rules/blob/master/testrb/manners.rb)

#### [Python](https://github.com/jruizgit/rules/blob/master/testpy/manners.py)

#### [Node.js](https://github.com/jruizgit/rules/blob/master/testjs/manners.js)

## Image recognition and Waltzdb

Waltzdb is a constraint propagation problem for image recognition: given a set of lines in a 2D space, the system needs to interpret the 3D depth of the image. The first part of the algorithm consists of identifying four types of junctions, then labeling the junctions following Huffman-Clowes notation. Pairs of adjacent junctions constraint each other’s edge labeling. So, after choosing the labeling for an initial junction, the second part of the algorithm iterates through the graph, propagating the labeling constraints by removing inconsistent labels.  

In this case too, the benchmark flow structure is defined using a statechart to improve code readability. The benchmark requirements are not altered. Execution time is around 3 seconds for the case of 4 regions and around 20 for the case of 50. More details documented in this [blog post](http://jruizblog.com/2015/07/20/miss-manners-and-waltzdb/).  

<div align="center"><img src="https://raw.github.com/jruizgit/rules/master/docs/waltzdb.jpg" width="800px" height="300px" /></div>  

_IMac, 4GHz i7, 32GB 1600MHz DDR3, 1.12 TB Fusion Drive_    

#### [Ruby](https://github.com/jruizgit/rules/blob/master/testrb/waltzdb.rb)

#### [Python](https://github.com/jruizgit/rules/blob/master/testpy/waltzdb.py)

#### [Node.js](https://github.com/jruizgit/rules/blob/master/testjs/waltzdb.js)

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

