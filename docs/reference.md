Reference Manual
=====
### Table of contents
------
* [Local Setup](reference.md#setup)
* [Cloud Setup](reference.md#setup)
* [Rules](reference.md#rules)
  * [Simple Filter](reference.md#simple-filter)
  * [Correlated Sequence](reference.md#correlated-sequence)
  * [Choice of Sequences](reference.md#choice-of-sequences)
  * [Conflict Resolution](reference.md#conflict-resolution)
  * [Time Window](reference.md#time-window)
* [Data Model](reference.md#data-model)
  * [Events](reference.md#events)
  * [Facts](reference.md#facts)
  * [Context](reference.md#context)
  * [Timers](reference.md#timers) 
* [Flow Structures](reference.md#flow-structures)
  * [Statechart](reference.md#statechart)
  * [Nested States](reference.md#nested-states)
  * [Flowchart](reference.md#flowchart)
  * [Parallel](reference.md#parallel)
* [Extensions](reference.md#extensions)

### Local Setup
------
durable_rules has been tested in MacOS X, Ubuntu Linux and Windows.
#### Redis install
durable.js relies on Redis version 2.8  
 
_Mac_  
1. Download [Redis](http://download.redis.io/releases/redis-2.8.4.tar.gz)   
2. Extract code, compile and start Redis

For more information go to: http://redis.io/download  

_Windows_  
1. Download redis binaries from [MSTechOpen](https://github.com/MSOpenTech/redis/releases)  
2. Extract binaries and start Redis  

For more information go to: https://github.com/MSOpenTech/redis  

#### Node.js install
durable.js uses Node.js version  0.10.15.    

1. Download [Node.js](http://nodejs.org/dist/v0.10.15)  
2. Run the installer and follow the instructions  
3. The installer will set all the necessary environment variables, so you are ready to go  

For more information go to: http://nodejs.org/download  

#### First flight
Now that your cache and web server are ready, let's write a simple rule:  

#####JavaScript
1. Start a terminal  
2. Create a directory for your app: `mkdir firstapp` `cd firstapp`  
3. In the new directory `npm install durable` (this will download durable.js and its dependencies)  
4. In that same directory create a test.js file using your favorite editor  
5. Copy/Paste and save the following code:
  ```javascript
  var d = require('durable');

  with (d.ruleset('a0')) {
      whenAll(m.amount.lt(100), function (c) {
          console.log('a0 approved from ' + c.s.sid);
      });
      whenStart(function (host) {
          host.post('a0', {id: 1, sid: 1, amount: 10});
      });
  } 
  d.runAll();
  ```
7. In the terminal type `node test.js`  
8. You should see the message: `a0 approved from 1`  

Note 1: If you are using [Redis To Go](https://redistogo.com), replace the last line.
  ```javascript
  d.runAll([{host: 'hostName', port: port, password: 'password'}]);
  ```
Note 2: If you are running in Windows, you will need VS2013 express edition and Python 2.7, make sure both the VS build tools and the python directory are in your path.  

[top](reference.md#table-of-contents) 
### Cloud Setup
--------
#### Redis install
Redis To Go has worked well for me and is very fast if you are deploying an app using Heroku or AWS.   
1. Go to: [Redis To Go](https://redistogo.com)  
2. Create an account (the free instance with 5MB has enough space for you to evaluate durable_rules)  
3. Make sure you write down the host, port and password, which represents your new account  
#### Heroku install
Heroku is a good platform to create a cloud application in just a few minutes.  
1. Go to: [Heroku](https://www.heroku.com)  
2. Create an account (the free instance with 1 dyno works well for evaluating durable_rules)  
#### First app
1. Follow the instructions in the [tutorial](https://devcenter.heroku.com/articles/getting-started-with-nodejs#introduction), with the following changes:
  * procfile  
  `web: node test.js`
  * package.json  
  ```javascript
  {
    "name": "test",
    "version": "0.0.6",
    "dependencies": {
      "durable": "0.30.x"
    },
    "engines": {
      "node": "0.10.x",
      "npm": "1.3.x"
    }
  }
  ```
  * test.js
  ```javascript
  var d = require('durable');

  with (d.ruleset('a0')) {
      whenAll(m.amount.lt(100), function (c) {
          console.log('a0 approved from ' + c.s.sid);
      });
      whenStart(function (host) {
          host.post('a0', {id: 1, sid: 1, amount: 10});
      });
  } 
  d.runAll([{host: 'hostName', port: port, password: 'password'}]);
  ```  
2. Deploy and scale the App
3. Run `heroku logs`, you should see the message: `a0 approved from 1`  
[top](reference.md#table-of-contents)  

### Rules
------
Rules are the basic building blocks. All rules have a condition, which defines the events and facts that trigger an action. 
The rule condition is an expression. Its left side represents an event property, followed by a logical operator and its right side defines a pattern to be matched. 
The rule action is a function with a context  


#### Simple Filter

#####Ruby
```ruby
Durable.ruleset :a0 do
  when_all (m.amount < 100) | (m.subject == "approve") | (m.subject == "ok") do
    puts "a0 approved"
  end
  when_start do
    post :a0, {:id => 1, :sid => 1, :amount => 10}
  end
end
```
#####Python
```python
with ruleset('a0'):
    @when_all((m.subject < 100) | (m.subject == 'approve') | (m.subject == 'ok'))
    def approved(c):
        print ('a0 approved ->{0}'.format(c.m.subject))
        
    @when_start
    def start(host):
        host.post('a0', {'id': 1, 'sid': 1, 'amount': 10})
```
#####JavaScript
```javascript
with (d.ruleset('a0')) {
    whenAll(or(m.amount.lt(100), m.subject.eq('approve'), m.subject.eq('ok')), function (c) {
        console.log('a0 approved from ' + c.s.sid);
    });
    whenStart(function (host) {
        host.post('a0', {id: 1, sid: 1, amount: 10});
    });
}
```  
[top](reference.md#table-of-contents) 
#### Correlated Sequence
#####Ruby
```ruby
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
```
#####Python
```python
with ruleset('fraud_detection'):
    @when_all(c.first << m.amount > 100,
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
```
#####JavaScript
```javascript
with (d.ruleset('fraudDetection')) {
    whenAll(c.first = m.amount.gt(100),
            c.second = m.amount.gt(c.first.amount.mul(2)), 
            c.third = m.amount.gt(add(c.first.amount, c.second.amount).div(2)),
        function(c) {
            console.log('fraud detected -> ' + c.first.amount);
            console.log('               -> ' + c.second.amount);
            console.log('               -> ' + c.third.amount);
        }
    );
    whenStart(function (host) {
        host.post('fraudDetection', {id: 1, sid: 1, amount: 200});
        host.post('fraudDetection', {id: 2, sid: 1, amount: 500});
        host.post('fraudDetection', {id: 3, sid: 1, amount: 1000});
    });
}
```
[top](reference.md#table-of-contents)  
#### Choice Of Sequences
#####Ruby
```ruby
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
```
#####Python
```python
with ruleset('a4'):
    @when_any(all(m.subject == 'approve', m.amount == 1000), 
              all(m.subject == 'jumbo', m.amount == 10000))
    def action(c):
        print ('a4 action {0}'.format(c.s.sid))
    
    @when_start
    def start(host):
        host.post('a4', {'id': 1, 'sid': 2, 'subject': 'jumbo'})
        host.post('a4', {'id': 2, 'sid': 2, 'amount': 10000})
```
#####JavaScript
```javascript
with (d.ruleset('a4')) {
    whenAny(all(m.subject.eq('approve'), m.amount.eq(1000)), 
            all(m.subject.eq('jumbo'), m.amount.eq(10000)), function (c) {
        console.log('a4 action from: ' + c.s.sid);
    });
    whenStart(function (host) {
        host.post('a4', {id: 1, sid: 2, subject: 'jumbo'});
        host.post('a4', {id: 2, sid: 2, amount: 10000});
    });
}
```
[top](reference.md#table-of-contents) 
#### Conflict Resolution
#####Ruby
```ruby
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
```
#####Python
```python
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
```
#####JavaScript
```javascript
with (d.ruleset('attributes')) {
    whenAll(pri(3), count(3), m.amount.lt(300),
        function(c) {
            console.log('attributes ->' + c.m[0].amount);
            console.log('           ->' + c.m[1].amount);
            console.log('           ->' + c.m[2].amount);
        }
    );
    whenAll(pri(2), count(2), m.amount.lt(200),
        function(c) {
            console.log('attributes ->' + c.m[0].amount);
            console.log('           ->' + c.m[1].amount);
        }
    );
    whenAll(pri(1), m.amount.lt(100),
        function(c) {
            console.log('attributes ->' + c.m.amount);
        }
    );
    whenStart(function (host) {
        host.assert('attributes', {id: 1, sid: 1, amount: 50});
        host.assert('attributes', {id: 2, sid: 1, amount: 150});
        host.assert('attributes', {id: 3, sid: 1, amount: 250});
    });
}
```
[top](reference.md#table-of-contents) 
#### Time Window
#####Ruby
```ruby
Durable.ruleset :t0 do
  when_all (timeout :my_timer) | (m.count == 0) do
    s.count += 1
    post :t0, {:id => s.count, :sid => 1, :t => "purchase"}
    start_timer(:my_timer, rand(3))
  end
  when_all span(5), m.t == "purchase" do 
    puts("t0 pulse -> #{m.count}")
  end 
  when_start do
    patch_state :sid => 1, :count => 0
  end
end
```
#####Python
```python
with ruleset('t0'):
    @when_all(timeout('my_timer') | (s.count == 0))
    def start_timer(c):
        c.s.count += 1
        c.post('t0', {'id': c.s.count, 'sid': 1, 't': 'purchase'})
        c.start_timer('my_timer', random.randint(1, 3))

    @when_all(span(5), m.t == 'purchase')
    def pulse(c):
        print('t0 pulse -> {0}'.format(len(c.m)))

    @when_start
    def start(host):
        host.patch_state({'sid': 1, 'count': 0})
```
#####JavaScript
```javascript
with (d.ruleset('t0')) {
    whenAll(or(m.count.eq(0), timeout('myTimer')), function (c) {
        c.s.count += 1;
        c.post('t0', {id: c.s.count, sid: 1, t: 'purchase'});
        c.startTimer('myTimer', Math.random() * 3 + 1);
    });
    whenAll(span(5), m.t.eq('purchase'), function (c) {
        console.log('t0 pulse ->' + c.m.length);
    });
    whenStart(function (host) {
        host.patchState({sid: 1, count: 0});
    });
}
```
[top](reference.md#table-of-contents)  
### Data Model
------
#### Events
[top](reference.md#table-of-contents)  

#### Facts
#####Ruby
```ruby
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
```
#####Python
```python
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
```
#####JavaScript
```javascript
with (d.ruleset('fraudDetection')) {
    whenAll(c.first = m.t.eq('purchase'),
            c.second = m.location.neq(c.first.location), 
            count(2), 
        function(c) {
            console.log('fraud detected ->' + c.m[0].first.location + ', ' + c.m[0].second.location);
            console.log('               ->' + c.m[1].first.location + ', ' + c.m[1].second.location);
        }
    );
    whenStart(function (host) {
        host.assert('fraudDetection', {id: 1, sid: 1, t: 'purchase', location: 'US'});
        host.assert('fraudDetection', {id: 2, sid: 1, t: 'purchase', location: 'CA'});
    });
}
```
[top](reference.md#table-of-contents)  

#### Context
#####Ruby
```ruby
Durable.ruleset :a8 do
  when_all m.amount < s.max_amount + s.id(:global).min_amount do
    puts "a8 approved " + m.amount.to_s
  end
  when_start do
    patch_state :a8, {:sid => 1, :max_amount => 500}
    patch_state :a8, {:sid => :global, :min_amount => 100}
    post :a8, {:id => 1, :sid => 1, :amount => 10}
  end
end
```
#####Python
```python
with ruleset('a8'):
    @when_all(m.amount < c.s.max_amount + c.s.id('global').min_amount)
    def approved(c):
        print ('a8 approved {0}'.format(c.m.amount))

    @when_start
    def start(host):
        host.patch_state('a8', {'sid': 1, 'max_amount': 500})
        host.patch_state('a8', {'sid': 'global', 'min_amount': 100})
        host.post('a8', {'id': 1, 'sid': 1, 'amount': 10})
```
#####JavaScript
```javascript
with (d.ruleset('a8')) {
    whenAll(m.amount.lt(add(c.s.maxAmount, c.s.id('global').minAmount)), function (c) {
        console.log('a8 approved ' +  c.m.amount);
    });
    whenStart(function (host) {
        host.patchState('a8', {sid: 1, maxAmount: 500});
        host.patchState('a8', {sid: 'global', minAmount: 100});
        host.post('a8', {id: 1, sid: 1, amount: 10});
    });
}
```
[top](reference.md#table-of-contents)  
#### Timers
#####Ruby
```ruby
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
```
#####Python
```python
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
```
#####JavaScript
```javascript
with (d.ruleset('t1')) {
    whenAll(m.start.eq('yes'), function (c) {
        c.s.start = new Date();
        c.startTimer('myTimer', 5);
    });
    whenAll(timeout('myTimer'), function (c) {
        console.log('t1 end');
        console.log('t1 started ' + c.s.start);
        console.log('t1 ended ' + new Date());
    });
    whenStart(function (host) {
        host.post('t1', {id: 1, sid: 1, start: 'yes'});
    });
}
```
[top](reference.md#table-of-contents)  
### Flow Structures
-------
#### Statechart

#####Ruby
```ruby
Durable.statechart :a2 do
  state :input do
    to :denied, when_all((m.subject == 'approve') & (m.amount > 1000)) do
      puts "a2 state denied: #{s.sid}"
    end
    to :pending, when_all((m.subject == 'approve') & (m.amount <= 1000)) do
      puts "a2 state request approval from: #{s.sid}"
    end
  end  
  state :pending do
    to :pending, when_any(m.subject == 'approved', m.subject == 'ok') do
      puts "a2 state received approval for: #{s.sid}"
      s.status = "approved"
    end
    to :approved, when_all(s.status == 'approved')
    to :denied, when_all(m.subject == 'denied') do
      puts "a2 state denied: #{s.sid}"
    end
  end
  state :approved
  state :denied
end
```
#####Python
```python
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
```
#####JavaScript
```javascript
with (d.statechart('a2')) {
    with (state('input')) {
        to('denied').whenAll(m.subject.eq('approve').and(m.amount.gt(1000)), function (c) {
            console.log('a2 denied from: ' + c.s.sid);
        });
        to('pending').whenAll(m.subject.eq('approve').and(m.amount.lte(1000)), function (c) {
            console.log('a2 request approval from: ' + c.s.sid);
        });
    }
    with (state('pending')) {
        to('pending').whenAll(m.subject.eq('approved'), function (c) {
            console.log('a2 second request approval from: ' + c.s.sid);
            c.s.status = 'approved';
        });
        to('approved').whenAll(s.status.eq('approved'), function (c) {
            console.log('a2 approved from: ' + c.s.sid);
        });
        to('denied').whenAll(m.subject.eq('denied'), function (c) {
            console.log('a2 denied from: ' + c.s.sid);
        });
    }
    state('denied');
    state('approved');
}
```
[top](reference.md#table-of-contents)  
### Nested States
#####Ruby
```ruby
Durable.statechart :a6 do
  state :start do
    to :work
  end
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
    to :work, when_all(m.subject == "reset") do
      puts "a6 resetting"
    end
    to :canceled, when_all(m.subject == "cancel") do
      puts "a6 canceling"
    end
  end
  state :canceled
  when_start do
    post :a6, {:id => 1, :sid => 1, :subject => "enter"}
    post :a6, {:id => 2, :sid => 1, :subject => "continue"}
    post :a6, {:id => 3, :sid => 1, :subject => "continue"}
  end
end
```
#####Python
```python
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
```
#####JavaScript
```javascript
with (d.statechart('a6')) {
    with (state('start')) {
        to('work');
    }
    with (state('work')) {
        with (state('enter')) {
            to('process').whenAll(m.subject.eq('enter'), function (c) {
                console.log('a6 continue process');
            });
        }
        with (state('process')) {
            to('process').whenAll(m.subject.eq('continue'), function (c) {
                console.log('a6 processing');
            });
        }
        to('work').whenAll(m.subject.eq('reset'), function (c) {
            console.log('a6 resetting');
        });
        to('canceled').whenAll(m.subject.eq('cancel'), function (c) {
            console.log('a6 canceling');
        });
    }
    state('canceled');
    whenStart(function (host) {
        host.post('a6', {id: 1, sid: 1, subject: 'enter'});
        host.post('a6', {id: 2, sid: 1, subject: 'continue'});
        host.post('a6', {id: 3, sid: 1, subject: 'continue'});
    });
}
```
[top](reference.md#table-of-contents)
#### Flowchart

#####Ruby
```ruby
Durable.flowchart :a3 do
  stage :input
  to :request, when_all((m.subject == 'approve') & (m.amount <= 1000))
  to :deny, when_all((m.subject == 'approve') & (m.amount > 1000))
  
  stage :request do
    puts "a3 flow requesting approval for: #{s.sid}"
    s.status? ? s.status = "approved": s.status = "pending"
  end
  to :approve, when_all(s.status == 'approved')
  to :deny, when_all(m.subject == 'denied')
  to :request, when_any(m.subject == 'approved', m.subject == 'ok')
  
  stage :approve do
    puts "a3 flow aprroved: #{s.sid}"
  end

  stage :deny do
    puts "a3 flow denied: #{s.sid}"
  end
end
```
#####Python
```python
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
```
#####JavaScript
```javascript
with (d.flowchart('a3')) {
    with (stage('input')) {
        to('request').whenAll(m.subject.eq('approve').and(m.amount.lte(1000)));
        to('deny').whenAll(m.subject.eq('approve').and(m.amount.gt(1000)));
    }
    with (stage('request')) {
        run(function (c) {
            console.log('a3 request approval from: ' + c.s.sid);
            if (c.s.status) 
                c.s.status = 'approved';
            else
                c.s.status = 'pending';
        });
        to('approve').whenAll(s.status.eq('approved'));
        to('deny').whenAll(m.subject.eq('denied'));
        to('request').whenAny(m.subject.eq('approved'), m.subject.eq('ok'));
    }
    with (stage('approve')) {
        run(function (c) {
            console.log('a3 approved from: ' + c.s.sid);
        });
    }
    with (stage('deny')) {
        run(function (c) {
            console.log('a3 denied from: ' + c.s.sid);
        });
    }
}
```
[top](reference.md#table-of-contents)  

#### Parallel

#####Ruby
```ruby
Durable.ruleset :p1 do
  when_all m.start == "yes", paralel do 
    ruleset :one do 
      when_all -s.start do
        s.start = 1
      end
      when_all s.start == 1 do
        puts "p1 finish one"
        signal :id => 1, :end => "one"
        s.start = 2
      end 
    end
    ruleset :two do 
      when_all -s.start do
        s.start = 1
      end
      when_all s.start == 1 do
        puts "p1 finish two"
        signal :id => 1, :end => "two"
        s.start = 2
      end 
    end
  end
  when_all m.end == "one", m.end == "two" do
    puts 'p1 approved'
  end
  when_start do
    post :p1, {:id => 1, :sid => 1, :start => "yes"}
  end
end
```
#####Python
```python
with ruleset('p1'):
    with when_all(m.start == 'yes'): 
        with ruleset('one'):
            @when_all(-s.start)
            def continue_flow(c):
                c.s.start = 1

            @when_all(s.start == 1)
            def finish_one(c):
                print('p1 finish one {0}'.format(c.s.sid))
                c.signal({'id': 1, 'end': 'one'})
                c.s.start = 2

        with ruleset('two'): 
            @when_all(-s.start)
            def continue_flow(c):
                c.s.start = 1

            @when_all(s.start == 1)
            def finish_two(c):
                print('p1 finish two {0}'.format(c.s.sid))
                c.signal({'id': 1, 'end': 'two'})
                c.s.start = 2

    @when_all(m.end == 'one', m.end == 'two')
    def done(c):
        print('p1 done {0}'.format(c.s.sid))

    @when_start
    def start(host):
        host.post('p1', {'id': 1, 'sid': 1, 'start': 'yes'})
```
#####JavaScript
```javascript
with (d.ruleset('p1')) {
    with (whenAll(m.start.eq('yes'))) {
        with (ruleset('one')) {
            whenAll(s.start.nex(), function (c) {
                c.s.start = 1;
            });
            whenAll(s.start.eq(1), function (c) {
                console.log('p1 finish one');
                c.signal({id: 1, end: 'one'});
                c.s.start  = 2;
            });
        }
        with (ruleset('two')) {
            whenAll(s.start.nex(), function (c) {
                c.s.start = 1;
            });
            whenAll(s.start.eq(1), function (c) {
                console.log('p1 finish two');
                c.signal({id: 1, end: 'two'});
                c.s.start  = 2;
            });
        }
    }
    whenAll(m.end.eq('one'), m.end.eq('two'), function (c) {
        console.log('p1 approved');
    });
    whenStart(function (host) {
        host.post('p1', {id: 1, sid: 1, start: 'yes'});
    });
}
```
[top](reference.md#table-of-contents)  
### Extensions
-------
#### Host
[top](reference.md#table-of-contents)  

#### Application
[top](reference.md#table-of-contents)  

