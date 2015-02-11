Concepts
=====
### Table of contents
------
* [Rules](reference.md#rules)
  * [Expressions](reference.md#expressions)
  * [Actions](concepts.md#actions)
  * [Algebra](concepts.md#algebra)
  * [References](concepts.md#references) 
  * [Attributes](concepts.md#references) 
* [Data Model](concepts.md#data-model)
  * [Events](concepts.md#events)
  * [Facts](concepts.md#facts)
  * [Context](concepts.md#context)
  * [Timers](concepts.md#timers)
* [Flow Structures](concepts.md#flow-structures)
  * [Statechart](concepts.md#statechart)
  * [Flowchart](concepts.md#flowchart)
  * [Parallel](concepts.md#parallel)
* [Extensions](concepts.md#extensions)
  * [Host](concepts.md#host)
  * [Application](concepts.md#application)

### Rules
------
Rules are the basic building block and consist of antecedent (expression) and consequent (action)

#####Ruby
```ruby
Durable.ruleset :a0 do
  when_all (m.amount < 100) | (m.subject == "approve") | (m.subject == "ok") do
    puts "a0 approved"
  end
end
```
#####Python
```python
with ruleset('a0'):
    @when_all((m.subject == 'go') | (m.subject == 'approve') | (m.subject == 'ok'))
    def approved(c):
        print ('a0 approved ->{0}'.format(c.m.subject))
```
#####JavaScript
```javascript
with (d.ruleset('a0')) {
    whenAll(or(m.amount.lt(100), m.subject.eq('approve'), m.subject.eq('ok')), function (c) {
        console.log('a0 approved from ' + c.s.sid);
    });
}
```  



[top](reference.md#table-of-contents)  

Logical  
>, <, ==, !=, <=, >=, not, and, or

Arithmetic  
+, -, /, *  

#### Actions

Synchronous  
Asynchronous  
Post  
Assert  
Retract  
Start Timer  

[top](concepts.md#table-of-contents)  

#### Algebra

when all
when any
none

[top](concepts.md#table-of-contents)  

#### References

context  
message  

#### Attributes

pri  
count  
scope  

[top](concepts.md#table-of-contents)  
### Data Model
------
#### Events
[top](concepts.md#table-of-contents)  

#### Facts
[top](concepts.md#table-of-contents)  

#### Context
[top](concepts.md#table-of-contents)  

#### Timers
[top](concepts.md#table-of-contents)  

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

[top](concepts.md#table-of-contents)  

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

[top](concepts.md#table-of-contents)  

#### Parallel
[top](concepts.md#table-of-contents)  
### Extensions
-------
#### Host
[top](concepts.md#table-of-contents)  

#### Application
[top](concepts.md#table-of-contents)  

