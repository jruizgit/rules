Reference Manual
=====
### Table of contents
------
* [Rules](reference.md#rules)
  * [Simple Filter](reference.md#simple-filter)
  * [Correlated Sequence](reference.md#correlated-sequence)
* [Data Model](reference.md#data-model)
* [Flow Structures](reference.md#flow-structures)
  * [Statechart](reference.md#statechart)
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

### Data Model
------
#### Events
[top](reference.md#table-of-contents)  

#### Facts
[top](reference.md#table-of-contents)  

#### Context
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

