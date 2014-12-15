Durable Rules
=====
Durable Rules is a polyglot micro-framework for real-time, consistent and scalable coordination of events. With Durable Rules you can track and analyze information about things that happen (events) by combining data from multiple sources to infer more complicated circumstances.

A forward chaining algorithm (A.K.A. Rete) is used to evaluate massive streams of data. A simple, yet powerful meta-liguistic abstraction lets you define simple and complex rulesets, such as flowcharts, statecharts, nested statecharts, paralel and time driven flows. 

The Durable Rules core engine is implemented in C, which enables ultra fast rule evaluation and inference as well as muti-language support. Durable Rules relies on state of the art technologies:

* [Node.js](http://www.nodejs.org), [Werkzeug](http://werkzeug.pocoo.org/), [Sinatra](http://www.sinatrarb.com/) are used to host rulesets written in JavaScript, Python and Ruby respectively.
* Inference state is cached using [Redis](http://www.redis.io), which lets scaling out without giving up performance.
* A web client based on [D3.js](http://www.d3js.org) provides powerful data visualization and test tools.

Below is an example on how easy it is to define a real-time fraud detection rule (three purchases over $100 in a span of 30 seconds).

####Ruby
```ruby
require 'durable'

Durable.statechart :fraud do
  state :start do
    to :standby
  end
  state :standby do
    to :metering, when_(m.amount > 100) do
      start_timer :velocity, 30
    end
  end
  state :metering do
    to :fraud, when_(m.amount > 100, at_least(2)) do
      puts "fraud detected"
    end
    to :standby, when_(timeout :velocity) do
      puts "fraud cleared"
    end
  end
  state :fraud
end

Durable.run_all
```
####Python
```python
from durable.lang import *

with statechart('fraud'):
    with state('start'):
        to('standby')

    with state('standby'):
        @to('metering')
        @when(m.amount > 100)
        def start_metering(s):
            s.start_timer('velocity', 30)

    with state('metering'):
        @to('fraud')
        @when((m.amount > 100).at_least(2))
        def report_fraud(s):
            print('fraud detected')

        @to('standby')
        @when(timeout('velocity'))
        def clear_fraud(s):
            print('fraud cleared')

    state('fraud')

run_all()
```
####JavaScript
```javascript
var d = require('durable');

with (d.statechart('fraud')) {
    with (state('start')) {
        to('standby');
    }
    with (state('standby')) {
        to('metering').when(m.amount.gt(100), function (s) {
            s.startTimer('velocity', 30);
        });
    }
    with (state('metering')) {
        to('fraud').when(m.amount.gt(100).atLeast(2), function (s) {
            console.log('fraud detected');
        });
        to('standby').when(timeout('velocity'), function (s) {
            console.log('fraud cleared');  
        });
    }
    state('fraud');
}

d.runAll();
```
####Visual
![Fraud Statechart](https://raw.github.com/jruizgit/rules/master/statechart.png =300x300)

#### Resources
To learn more:
* [Setup](https://github.com/jruizgit/rules/blob/master/setup.md)
* [Tutorial](https://github.com/jruizgit/rules/blob/master/tutorial.md)
* [Concepts](https://github.com/jruizgit/rules/blob/master/concepts.md)  
 
Blog:
* [Boosting Performance with C (08/2014)](http://jruizblog.com/2014/08/19/boosting-performance-with-c/)
* [Rete Meets Redis (02/2014)](http://jruizblog.com/2014/02/02/rete-meets-redis/)
* [Inference: From Expert Systems to Cloud Scale Event Processing (01/2014)](http://jruizblog.com/2014/01/27/event-processing/)



