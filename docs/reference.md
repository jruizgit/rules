Reference Manual
=====
### Table of contents
------
* [Rules](reference.md#rules)
  * [Simple Filter](reference.md#simple-filter)
  * [Correlated Sequence](reference.md#correlated-sequence)
  * [Choice of Sequences](reference.md#choice-of-sequences)
  * [Attributes](reference.md#attributes)
* [Data Model](reference.md#data-model)
  * [Events](reference.md#events)
  * [Facts](reference.md#facts)
  * [Context](reference.md#context)
* [Flow Structures](reference.md#flow-structures)
  * [Statechart](reference.md#statechart)
  * [Nested States](reference.md#nested-states)
  * [Flowchart](reference.md#flowchart)
  * [Parallel](reference.md#parallel)
* [Extensions](reference.md#extensions)

### Rules
------
Rules are the basic building block and consist of antecedent (expression) and consequent (action)

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
#### Attributes
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
[top](reference.md#table-of-contents)  
### Extensions
-------
#### Host
[top](reference.md#table-of-contents)  

#### Application
[top](reference.md#table-of-contents)  

