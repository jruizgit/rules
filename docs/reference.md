Concepts
=====
### Table of contents
------
* [Rules](concepts.md#rules)
  * [Expressions](concepts.md#expressions)
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



[top](concepts.md#table-of-contents)  

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
[top](concepts.md#table-of-contents)  

#### Flowchart
[top](concepts.md#table-of-contents)  

#### Parallel
[top](concepts.md#table-of-contents)  
### Extensions
-------
#### Host
[top](concepts.md#table-of-contents)  

#### Application
[top](concepts.md#table-of-contents)  

