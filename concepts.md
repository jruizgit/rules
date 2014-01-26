Concepts
=====
### Table of contents
------
* [Introduction](concepts.md#introduction)
* [Abstraction](concepts.md#abstraction)
  * [Rules](concepts.md#rules)
  * [Statechart](concepts.md#statechart)
  * [Flowchart](concepts.md#flowchart) 
  * [Parallel](concepts.md#parallel)
  * [Expressions](concepts.md#expressions)
  * [Event Algebra](concepts.md#event-algebra)
  * [Actions](concepts.md#actions)
* [Event Sources](concepts.md#event-sources)
  * [Web Messages](concepts.md#web-messages)
  * [Internal Messages](concepts.md#internal-messages)
  * [Signals](concepts.md#signals)
  * [Timers](concepts.md#timers)
* [Advanced Topics](concepts.md#advanced-topics)
  * [Inference](concepts.md#inference)
  * [Scale](concepts.md#scale)
  * [Consistency](concepts.md#consistency)

### Introduction
------
Durable rules is designed for coordinating real time event streams with long running program state. This allows for tracking and analyzing information about things that happen (events) by combining data from multiple sources and inferring more complicated circumstances. In other words, identifying meaningful events and acting on them as quickly as possible. In fact this is a very useful pattern to model applications that coordinate interactions with external services or require human intervention (business and system process management, financial applications...).

[Rules](concepts.md#rules) are the basic building blocks of the system. Rules define actions for event patterns. Event patterns can be defined with a rich [expression](concepts.md#expressions) abstraction. An [inference](concepts.md#inference) algorithm enables ultra fast event pattern evaluation. More complex coordinations such as [Statechart](concepts.md#statechart) and [Flowchart](concepts.md#flowchart) are just different ways of organizing a rule set.

Durable rules is based on the JSON type system through and through: a REST interface is exposed for posting event messages as JSON objects, program state is stored in Redis as a JSON object and rulesets are defined as a JSON objects. The result is a simple, intuitive and consistent meta-linguistic abstraction as well as optimal system performance because resources are not wasted in type system mappings. 

Node.js is the program hosting environment and Redis is used to store the user as well as inference state. The single thread execution model of both systems allows optimal resource utilization. [Scale out](concepts.md#scale) is achieved by simply adding Node processes and Redis servers.

Durable Rules provides a management web interface based on svg and driven by d3.js. The user interface provides visualization of program structure as well as state overlays. This enables the analysis of code execution sequences, provides an intuitive set of actions for testing and repairing program instances.  

_Note: This work is the result of my own research and experimentation and does not represent the views nor opinions of my employer._
  
[top](concepts.md#table-of-contents)  

### Abstraction
-----
#### Rules
Rules are the basic building blocks of the system. Simply put, a ruleset is a set of conditions followed by actions. Syntactic definition:  
* A ruleset has a name.  
* A ruleset contains a set of named rules.  
* Each rule consists of  
  * Antecedent (when): which is an [expression](concepts.md#expressions).  
  * Consequent (run): which is typically an [action](concepts.md#actions), but can also be a set of child rulesets.  

```javascript
var d = require('durable');
d.run({
    aruleset: {
        rule1: {
            when: { $and: [
                { subject: 'approve' },
                { $gt: { amount: 1000 }}
            ]},
            run: function(s) { }
        },
        rule2: {
            when: { $and: [
                { subject: 'approve' },
                { $lte: { amount: 1000 }}
            ]},
            run: function(s) { }
        }
    }
});
```
[top](concepts.md#table-of-contents)  

#### Statechart
A statechart is a finite state machine. A statecahrt is a model of computation used to design sequential logic. The machine can be in only one of a finite number of states and can change from one state to another when initiated by a triggering event or condition. Syntactic definition:  
* A statechart has a name postfixed by `$state`.  
* A statechart contains a set of named states.  
* Each state contains a set of named transitions.  
* A state can also contain a child statechart, which name is postfixed by `$state`.  
* Each transition consists of:  
  * Trigger: which is an [expression](concepts.md#expressions).  
  * Action: which is typically an [action](concepts.md#actions), but can also be a set of child rulesets.  
  * Destination: which the state to transition to.  

```javascript
var d = require('durable');
d.run({
    achart$state: {
        input: {
            deny: {
                when: { $gt: { amount: 1000 }},
                run: function(s) { },
                to: 'denied'
            },
            request: {
                when: { $lte: { amount: 1000 }},
                run: function(s) { },
                to: 'pending'
            }
        },
        pending: {
            approve: {
                when: { subject: 'approved' },
                run: function(s) { },
                to: 'approved'
            },
            deny: {
                when: { subject: 'denied' },
                run: function(s) { },
                to: 'denied'
            }
        },
        denied: {
        },
        approved: {
        }
    }
});
```
[top](concepts.md#table-of-contents)  

#### Flowchart
A flowchart is typically used to represent complex processes and programs. Flowcharts consist of processing steps and decisions, which are ordered by connecting them with arrows. Syntactic definition:
* A flowchart has a name postfixed by `$flow`.  
* A flowchart contains a set of named stages.  
* Each stage consists of:  
  * Action: which is typically an [action](concepts.md#actions), but can also be a set of child rulesets.  
  * A set of conditions arranged as a set of:  
    * Condition Name: is the stage to go to if the condition is met.  
    * Condition: which is an [expression](concepts.md#expressions).  

```javascript
var d = require('durable');
d.run({
    achart$flow: {
        input: {
            to: {
                request: { $lte: { amount: 1000 } },
                deny: { $gt: { amount: 1000 } }
            }
        },
        request: {
            run: function(s) { },
            to: {
                approve: { subject: 'approved' },
                deny: { subject: 'denied'}
            }
        },
        approve: {
            run: function(s) { }
        },
        deny: {
            run: function(s) { }
        }
    },
});
```
[top](concepts.md#table-of-contents)  
#### Parallel
Parallels are expressed by defining children rulesets, statecharts or flowcharts as action objects (the property named 'run'). When parallel execution is started the user state is forked. The child user state is identified with the convention '[parentSid].[childRulesetName]'. Parallels can be synchronized by using [signals](concepts.md#signals). The ruleset below, when r1 antecedent is met, will start two parallel rulesets named 'one' and 'two'. Rulesets 'one' and 'two' will synchronize by signaling completion to its parent ruleset when the [timers](concepts.md#timers) complete.
```javascript
var d = require('durable');
d.run({
    parallel: {
        r1: {
            when: { start: 'yes' },
            run: {
                one: {
                    r1: {
                        when: { $s: { $nex: { start: 1 } } },
                        run: function (s) { s.startTimer('myTimer', 3); }
                    },
                    r2: {
                        when: { $t: 'myTimer' },
                        run: function (s) { s.signal({ id: 2, end: 'one' }); }
                    }
                },
                two: {
                    r1: {
                        when: { $s: { $nex: { start: 1 } } },
                        run: function (s) { s.startTimer('myTimer', 6); }
                    },
                    r2: {
                        when: { $t: 'myTimer' },
                        run: function (s) { s.signal({ id: 3, end: 'two' }); }
                    }
                }
            }
        },
        r2: {
            whenAll: {
                first: { end: 'one' },
                second: { end: 'two' }
            },
            run: function (s) { console.log('done'); }
        }
    }
});
```

[top](concepts.md#table-of-contents)  

#### Expressions 
Expressions are used to define the rules antecedent. Each expression is a Json object. An expression is a set of name value pairs defining the names and values properties to match on the target object. The list of supported expression operators:

* The equality operator is a simple name value. For example: The expression `{ subject:'approval' }`, read as 'subject equal to "approval"', matches the target object `{ id:5, amount:100, subject:'approval' }`. 

* Logical operators (the position in the array is the execution order):  
  $and: []  
  $or: []  
  For example: `{ $or: [{ subject: 'approval' }, $gt: { amount: 100 }] }` is read as 'subject equal to "approval" or amount greater than 100. 

* Equality operators:  
  $gt: { name: number }  
  $lt: { name: number }    
  $neq: { name: number }  
  $lte:  { name: number }  
  $gte: { name: number }    
  For example: `{ $lte: { amount: 10 } }` is read as 'amount less than or equal to 10'. 

* Existential operators:  
  $ex: { name: 1 }  
  $nex: { name: 1 }  
  For example: `{ $nex: { subject: 1 } }` is read as 'subject field does not exist'.

[top](concepts.md#table-of-contents)  
#### Event Algebra  
There are two interesting operators, which compose well with expressions and allow for sophisticated coordination of events: `$all` wait for all matching events to happen. `$any` wait for any matching event to occur.  
* $all: { name: expression, name: expression...}  
* $any: { name: expression, name: expression...}  

The example below uses both operators to wait for the first of two cases each of which will wait for two events:  

```javascript
var d = require('durable');
d.run({
    aruleset: {
        rule1: {
            whenAny: {
                a$all: {
                    a: { request: 'approve' },
                    b: { response: 'yes' }
                },
                b$all: {
                    a: { request: 'approve' },
                    b: { response: 'no' }
                }
            },
            run: function(s) { }
        }
    }
});
```
[top](concepts.md#table-of-contents)  

#### Actions

Actions are the rules consequent (they compose with statecharts and flowcharts as well). Actions are javascript functions, which react to events. Actions can be synchronous (very useful for calculations) and asynchronous (typically used for async I/O).  

Synchronous actions take the user state as a parameter. Errors are returned by throwing exceptions.  

```javascript
function repeat(s) {
    s.attempt = (s.attempt ? s.attempt + 1: 1);
}
```

Asynchronous actions take a completion callback as the second parameter, which should be called when the action is done. If there is an error, by convention is has to be passed in the first argument of the callback.   

```javascript
function deny (s, complete) {
    var mailOptions = {
        from: 'approval',
        to: s.from,
        subject: 'Request for ' + s.amount + ' has been denied.'
    };
    transport.sendMail(mailOptions, complete);
}
```

Remarks:
* When there is an error, Durable Rules runtime will reclaim control and the action will be retried later (see [consistency](concepts.md#consistency) section for more details).
* The event that originates the action is included in the user state object 's' and can be retrieved by calling the function `s.getOutput()`. 
* If the action is executed when coordinating several events (using event algebra), the output is a graph that follows the structure provided in the originating expression.

```javascript
var d = require('durable');
d.run({
    aruleset: {
        rule1: {
            whenAll: { 
                request: { subject: 'approve' },
                reply: { status: 'approved' }
            },
            run: function(s) { 
                console.log(s.getOutput().request.subject); // approve
                console.log(s.getOutput().reply.status); // approved
            }
        }
    }
});
```
[top](concepts.md#table-of-contents)  

### Event Sources
------
#### Web Messages
To integrate the system with the external world, events can be raised by posting JSON messages to a REST API. Messages have a unique identifier, which is used to ensure the post method consistency contract. Messages are addressed to a session (user state) of a ruleset. If the addressed session doesn't exist, a new session is created.  
```javascript
POST: http://example.com/ping/1
{ 
    id: 1,
    content: 'a'
}
```
[top](concepts.md#table-of-contents)  

#### Internal Messages
Events can also be raised between rulesets hosted in the same system, which enables faster event coordination between application components (provided the application has been component-ized using rulesets, statecharts and flowcharts). Messages are JSON objects and have a unique identifier named 'id'. Contrasting with the REST API, internal messages have a property named 'program', which defines the ruleset the message is addressed to and a a property named 'sid', which identifies the user state the message is addressed to. Internal messages are sent using the `post` function in the user state passed to an action. The example below shows a two statecharts, one named 'ping' and a second one named 'pong' sending internal messages to each other.

```javascript
var d = require('durable');
d.run({
    ping$state: {
        s: {
            tr: {
                run: function (s) { s.i = 0; },
                to: 'r'
            }
        },            
        r: {
            ft: {
                when: { content: 'ping' },
                run: function (s) {
                    ++s.i;
                    s.post({ program: 'pong', sid: s.id, id: s.id + '.' + s.i, content: 'pong' });
                },
                to: 'r'
            }
        }
    },
    pong$state: {
        s: {
            tr: {
                run: function (s) { s.i = 0; },
                to: 'r'
            }
        },
        r: {
            ft: {
                when: { content: 'pong' },
                run: function (s) {
                    ++s.i;
                    s.post({ program: 'ping', sid: s.id, id: s.id + '.' + s.i, content: 'ping' });
                },
                to: 'r'
            }
        }
    }
));
```
[top](concepts.md#table-of-contents)  

#### Signals
When rulesets, statecharts or flowcharts initiate parallel execution, the children rulesets (statecharts or flowcharts) can coordinate and synchronize with the parent by sending signals. A signal is a Message, a JSON object with a unique identifier named 'id'. As opposed to internal messages signals don't have 'program' nor 'sid' property. Signals are sent using the `signal` function in the user state passed to an action. The example below illustrates how a parent statechart coordinates with two children statecharts, which send a signal when transitioning to the 'end' state.

```javascript
    approval$state: {
        input: {
            review: {
                when: { subject: 'approve' },
                run: {
                    first$state: {
                        send: {
                            start: {
                                run: function (s) { s.startTimer('first', Math.floor(Math.random() * 6)); },
                                to: 'evaluate'
                            }
                        },
                        evaluate: {
                            wait: {
                                when: { $t: 'first' },
                                run: function (s) { s.signal({ id: 2, subject: 'approved' }); },
                                to: 'end'
                            }
                        },
                        end: {
                        }

                    },
                    second$state: {
                        send: {
                            start: {
                                run: function (s) { s.startTimer('second', Math.floor(Math.random() * 3)); },
                                to: 'evaluate'
                            }
                        },
                        evaluate: {
                            wait: {
                                when: { $t: 'second' },
                                run: function (s) { s.signal({ id: 3, subject: 'denied' }); },
                                to: 'end'
                            }
                        },
                        end: {
                        }
                    }
                },
                to: 'pending'
            }
        },
        pending: {
            approve: {
                when: { subject: 'approved' },
                run: function (s) { console.log('approved'); },
                to: 'approved'
            },
            deny: {
                when: { subject: 'denied' },
                run: function (s) { console.log('denied'); },
                to: 'denied'
            }
        },
        denied: {
        },
        approved: {
        }
    }
```
[top](concepts.md#table-of-contents)  

#### Timers
Timers are critical for coordinating events (an application needs limit the time for waiting an event to happen, an application needs to count how many events happen over a period of time...). When a timer is due, an event (message) is posted to the originating ruleset and user state (sid). The example below shows how a timer is started and how the ruleset reacts to the event raised when the timer is due.
```javascript
var d = require('durable');
d.run({
    timer: {
        r1: {
            when: { start: 'yes' },
            run: function (s) {
                s.start = new Date().toString();
                s.startTimer('myTimer', 5);
            }
        },
        r2: {
            when: { $t: 'myTimer' },
            run: function (s) {
                console.log('started @' + s.start);
                console.log('timeout @' + new Date());
            }
        }
    }
});
```
[top](concepts.md#table-of-contents)  

### Advanced Topics
-------
#### Inference  
A common pattern in traditional middle tier applications is storing messages in a message queue, while background processes dequeue the messages, deserialize correlated user state and evaluate rulesets. Inference allows for a much more efficient approach, where messages are evaluated upfront, user sate is not deserialized for rule evaluation and the entire ruleset is not re-evaluated for every message. Consider the following ruleset:
```javascript
var d = require('durable');
d.run({
    approve: {
        r1: {
            when: { $lt: { amount: 1000 } },
            run: function(s) { s.status = 'pending' }
        },
        r2: {
            whenAll: {
                $m: { subject: 'approved' },
                $s: { status: 'pending' } 
            },
            run: function(s) { console.log('approved'); }
        }
    }
});
```
Below is the tree constructed by Durable Rules when running the ruleset above. 
* The solid circles are called alpha nodes. Alpha nodes have one parent and multiple child nodes. They represent filters or conditions.
* The dashed circle is called beta node. All beta nodes have multiple parents and one child node. They represent joins of conditions.
* The squares are called action nodes. They are the tree leafs and can have multiple parents.

![Rete tree](https://raw.github.com/jruizgit/rules/master/rete.jpg)  

1. Let's start by posting the following message:
```javascript
http://www.durablejs.org/examples/simple/mySession
{
    id: 1,
    amount: 500
}
```
 1. The request comes through the Node.js post handler and the message is run through the ruleset's Rete tree. 
 2. The message is first pushed to the alpha node `type = '$m'`, which pushes the message to the next alpha node.
 3. The message meets the condition `$m.amount < 1000`, so it is pushed to the action node. 
 4. Now an action is added to the action queue of the ruleset. 
 5. The message is stored in the ruleset message hashset. The response is sent to back to the user at this point.
2. The action is picked up by the background dispatch loop:  
 1. The action is dequeued and executed. During the action execution the state will be changed `s.status = 'pending'`.
 2. The new state is pushed to the alpha node `type = $s`, which pushes the state the the next alpha node.  
 3. The state meets the condition `s.status = 'pending'`, so it is pushed to the beta node.
 4. The 'all' join in the beta node is not complete yet. The left side is remembered.
 5. The message that triggered the action is removed from the ruleset message hashset.
 6. The new state is stored in the ruleset state hashset.
3. Now let's post the message:
```javascript
http://www.durablejs.org/examples/simple/1
{
    id: 2,
    sid: 'mySession',
    subject: 'approved'
}
```
 1. The request comes through the Node.js post handler and the message is run through the ruleset's Rete tree.
 2. The message is pushed to the alpha node `type = '$m'`, which pushes the message to the next alpha node.
 3. The message meets the condition `$m.subject = 'approved'` and is pushed to the beta node.
 4. The 'all' join is now satisfied so the message is pushed to the action node.
 5. Now an action is added to the action queue of the ruleset. 
 6. The message is stored in the ruleset message hashset. The response is sent to back to the user at this point.
4. The action is picked up by the background dispatch loop.
 1. The all join state is deleted.
 2. The action is dequeued and dispatched. 
 3. The message that triggered the action is removed from the ruleset hashset. 

[top](concepts.md#table-of-contents)  

#### Scale

The goal of scaling out is to increase the system throughput when adding more hardware. Durable Rules has two interesting scale challenges (the second one is a consequence of how the first is addressed): 1. Adding more processes to compute rules 2. Data partitioning to increase the cache throughput.  

![Scale](https://raw.github.com/jruizgit/rules/master/scale.jpg)  

First let's consider the problem of adding more processes. In the [inference](concepts.md#inference) section, I glossed over the details of steps 2.iv and 3.iv. The forward chaining algorithms described in published white-papers require heavy use of memory. In order to scale out, and be able to use several cores to evaluate the same ruleset, it is necessary to store the state in a cache external to the processes. There are three important challenges with this approach:  

1. Concurrent evaluation of join
2. Reducing the number of cache access
3. Fault tolerance 

With Redis features such as sorted sets, single threaded multi command execution and Lua scripting as well as a slight variation of the forward chaining algorithm on memory management and join evaluation, it is possible to efficiently address the three problems above. The key variation in the algorithm: the join state is now pushed all the way down to the action nodes. The action nodes now are in charge of storing the information in the cache as well as evaluating upstream joins. To illustrate the point, in the example above, the following sorted sets are created to store the join information:

* approve.r1.$m.mySession
* approve.r2.$s.$s.mySession
* approve.r2.$m.$m.mySession 

The join evaluation works as follows:   

1. In step 2.iv an entry with 'mySession' (the session id) is added to approve.r2.$s.$s.mySession with Date().getTime() score.
2. The action node evaluates the join by testing if both approve.r2.$s.$s.mySession and approve.r2.$m.$m.mySession have an entry. The test will be false at this point.
3. In step 3.iv an entry with '2' is added to approve.r2.$m.$m.mySession.
4. The action node evaluates the join by testing if both approve.r2.$s.$s.mySession and approve.r2.$m.$m.mySession have an entry. The test will be true at this point.

Storing the event changes and evaluating joins can be done in a single redis access, ensuring consistency and reducing the number of cache accesses.  

As more processes are added, the redis server eventually limits the system throughput. To overcome this problem, Durable Rules implements a simple data partitioning scheme: Given an array of redis server addresses, each will be considered a different partition, the first one will be the primary. The primary partition stores a lookup table user state id (sid) -> redis server address. When a 'sid' is first seen in a process the primary partition is asked to assign a partition (round robin selection) or provide one if already assigned.  

The code below illustrates how to setup a server that uses 12 processes and 3 partitions, where redis servers are listening to 'tmp/redis0.sock', 'tmp/redis1.sock' and 'tmp/redis2.sock' (as an aside I found this configuration provides the highest throughput in my imac quad core i5-3.7, 16GB).

```javascript
var d = require('durable');
var cluster = require('cluster');

if (cluster.isMaster) {
    for (var i = 0; i < 12; i++) {
        cluster.fork();
    }

    cluster.on("exit", function (worker, code, signal) {
        cluster.fork();
    });
}
else {
    d.run({
        // rulesets definitions go here
    }, '', ['/tmp/redis0.sock', '/tmp/redis1.sock', '/tmp/redis2.sock']);
}
```
[top](concepts.md#table-of-contents)  

#### Consistency

Durable rules guarantees events are never lost and dispatched at least once. To illustrate how this is achieved it is best to describe the internal event evaluation mechanism as a consistency cycle.

![Consistency Statechart](https://raw.github.com/jruizgit/rules/master/state.jpg)

1. Peek Action  
  1. Read the first score lower than `new Date().getTime()` from ruleset action hashset. 
  2. Include corresponding input messages and state.
  3. Increase the action score by 120.
2. Execute Action  
  1. Create input parameters.
  2. Run action.
  3. Post output messages and\or fork state.  
3. Assert State  
  1. Negate input messages.  
  2. Negate initial state.
  3. Assert new state.  
4. Commit State  
  1. Store new state. 
  2. Remove action from ruleset action hashset.  

Note: If an error happens at any point after #1 (which runs as a single redis script), the action will be picked up again when its score is less than `new Date().getTime()`. The implication is that all operations,  including the action function, will be executed at least once, therefore they have to be idempotent.
  
[top](concepts.md#table-of-contents)  

