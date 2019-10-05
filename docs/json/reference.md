Reference Manual
=====
## Table of contents
* [Basics](reference.md#basics)
  * [Rules](reference.md#rules)
  * [Facts](reference.md#facts)
  * [Events](reference.md#events)
  * [State](reference.md#state)
  * [Identity](reference.md#identity)
* [Antecedents](reference.md#antecedents)
  * [Simple Filter](reference.md#simple-filter)
  * [Pattern Matching](reference.md#pattern-matching)
  * [String Operations](reference.md#string-operations)
  * [Correlated Sequence](reference.md#correlated-sequence)
  * [Choice of Sequences](reference.md#choice-of-sequences)
  * [Lack of Information](reference.md#lack-of-information)
  * [Nested Objects](reference.md#nested-objects)
  * [Arrays](reference.md#arrays)
  * [Facts and Events as rvalues](reference.md#facts-and-events-as-rvalues)
* [Consequents](reference.md#consequents)  
  * [Conflict Resolution](reference.md#conflict-resolution)
  * [Unhandled Exceptions](reference.md#unhandled-exceptions)
* [Flow Structures](reference.md#flow-structures) 
  * [Statechart](reference.md#statechart)
  * [Nested States](reference.md#nested-states)
  * [Flowchart](reference.md#flowchart)
  * [Timers](reference.md#timers)


## Basics
### Rules

* `all` and `any` label the antecendent definition of a rule
* `run` label the consequent definition of a rule 
  
```javascript
{
  "test": {
    "r_0": {
      "all": [
        {
          "m": {
            "subject": "World"
          }
        }
      ],
      "run": "approved"
    }
  }
}
```
### Facts

```javascript
{
  "animal": {
    "r_0": {
      "all": [
        {
          "first": {
            "$and": [
              {
                "predicate": "eats"
              },
              {
                "object": "flies"
              }
            ]
          }
        }
      ],
      "run": "frog"
    },
    "r_2": {
      "all": [
        {
          "m": {
            "$and": [
              {
                "predicate": "eats"
              },
              {
                "object": "worms"
              }
            ]
          }
        }
      ],
      "run": "bird"
    },
    "r_3": {
      "all": [
        {
          "m": {
            "$and": [
              {
                "predicate": "is"
              },
              {
                "object": "frog"
              }
            ]
          }
        }
      ],
      "run": "green"
    },
    "r_5": {
      "all": [
        {
          "m": {
            "$and": [
              {
                "predicate": "is"
              },
              {
                "object": "bird"
              }
            ]
          }
        }
      ],
      "run": "black"
    },
    "r_6": {
      "all": [
        {
          "m": {
            "$ex": {
              "subject": 1
            }
          }
        }
      ],
      "run": "log"
    }
  }
}
```

Facts can  be asserted using the http API. For the example above, run the following command:  

<sub>`curl -H "content-type: application/json" -X POST -d '{"subject": "Tweety", "predicate": "eats", "object": "worms"}' http://localhost:5000/animal/facts`</sub>

[top](reference.md#table-of-contents)  

### Events

```javascript
{
  "risk": {
    "r_0": {
      "all": [
        {
          "first": {
            "t": "purchase"
          }
        },
        {
          "second": {
            "$neq": {
              "location": {
                "first": "location"
              }
            }
          }
        }
      ],
      "run": "fraud"
    }
  }
}
```

Events can be posted using the http API. When the example above is listening, run the following commands:  

<sub>`curl -H "content-type: application/json" -X POST -d '{"t": "purchase", "location": "BR"}' http://localhost:5000/risk/events`</sub>  
<sub>`curl -H "content-type: application/json" -X POST -d '{"t": "purchase", "location": "JP"}' http://localhost:5000/risk/events`</sub>  

**Note from the autor:**  

*Using facts in the example above will produce the following output:*   

<sub>`Fraud detected -> US, CA`</sub>  
<sub>`Fraud detected -> CA, US`</sub>  

[top](reference.md#table-of-contents)  
### State
Context state changes can be evaluated by rules. By convention, context state change events have the `"$s": 1` attribute.

```javascript
{
  "flow": {
    "r_0": {
      "all": [
        {
          "m": {
            "$and": [
              {
                "state": "start"
              },
              {
                "$s": 1
              }
            ]
          }
        }
      ],
      "run": "next"
    },
    "r_1": {
      "all": [
        {
          "m": {
            "$and": [
              {
                "state": "next"
              },
              {
                "$s": 1
              }
            ]
          }
        }
      ],
      "run": "last"
    },
    "r_2": {
      "all": [
        {
          "m": {
            "$and": [
              {
                "state": "last"
              },
              {
                "$s": 1
              }
            ]
          }
        }
      ],
      "run": "end"
    }
  }
}
```
State can also be retrieved and modified using the http API. When the example above is running, try the following commands:  
<sub>`curl -H "content-type: application/json" -X POST -d '{"status": "next"}' http://localhost:5000/flow/state`</sub>  

[top](reference.md#table-of-contents)  

### Identity

```javascript
{
  "bookstore": {
    "r_0": {
      "all": [
        {
          "m": {
            "$ex": {
              "status": 1
            }
          }
        }
      ],
      "run": "log"
    },
    "r_1": {
      "all": [
        {
          "m": {
            "$ex": {
              "name": 1
            }
          }
        }
      ],
      "run": "retract"
    },
    "r_2": {
      "all": [
        {
          "m$not": {
            "$ex": {
              "name": 1
            }
          }
        }
      ],
      "run": "log"
    }
  }
}
```

## Antecendents
### Simple Filter
A rule antecedent is an expression. The left side of the expression represents an event or fact property. The right side defines a pattern to be matched.   

Logical operators:  
* Unary: "$nex" (does not exist), "$ex" (exists)  
* Logical operators: "$and", "$or"  
* Relational operators: "$lt", "$gt", "$lte", "$gte", "$neq", attribute relation tests equality  

```javascript
{
  "expense": {
    "r_0": {
      "all": [
        {
          "m": {
            "$or": [
              {
                "subject": "approve"
              },
              {
                "subject": "ok"
              }
            ]
          }
        }
      ],
      "run": "approve"
    }
  }
}
```  
[top](reference.md#table-of-contents)
### Pattern Matching
durable_rules implements a simple pattern matching dialect. Use `"$mt"` to define the rule match pattern. 

**Repetition**  
\+ 1 or more repetitions  
\* 0 or more repetitions  
? optional (0 or 1 occurrence)  

**Special**  
() group  
| disjunct  
[] range  
{} repeat  

**Character classes**  
.	all characters  
%a	letters  
%c	control characters  
%d	digits  
%l	lower case letters  
%p	punctuation characters  
%s	space characters  
%u	upper case letters  
%w	alphanumeric characters  
%x	hexadecimal digits  

```javascript
{
  "match": {
    "r_0": {
      "all": [
        {
          "m": {
            "$mt": {
              "url": "(https?://)?([%da-z.-]+)%.[a-z]{2,6}(/[%w_.-]+/?)*"
            }
          }
        }
      ],
      "run": "log"
    }
  }
}
```  
[top](reference.md#table-of-contents) 
### String Operations  
The pattern matching dialect can be used for common string operations. Use `"$imt"` fto define case insensitive rule match pattern.

```javascript
{
  "strings": {
    "r_0": {
      "all": [
        {
          "m": {
            "$mt": {
              "subject": "hello.*"
            }
          }
        }
      ],
      "run": "logStarts"
    },
    "r_1": {
      "all": [
        {
          "m": {
            "$imt": {
              "subject": ".*hello"
            }
          }
        }
      ],
      "run": "logEnds"
    },
    "r_2": {
      "all": [
        {
          "m": {
            "$imt": {
              "subject": ".*hello.*"
            }
          }
        }
      ],
      "run": "logContains"
    }
  }
}
```  
[top](reference.md#table-of-contents)

### Correlated Sequence
The `"all"` object expresses a sequence of events or facts. The object names are used to name events or facts, which can be referenced in subsequent expressions. When referencing events or facts, all properties are available. Complex patterns can be expressed using arithmetic operators.  Arithmetic operators have left `"$l"` and right `"$r"` subexpressions.

Arithmetic operators: `"$add"`, `"$sub"`, `"$mul"`, `"$div"`  
```javascript
{
  "risk": {
    "r_0": {
      "all": [
        {
          "first": {
            "$gt": {
              "amount": 100
            }
          }
        },
        {
          "second": {
            "$gt": {
              "amount": {
                "$mul": {
                  "$l": {
                    "first": "amount"
                  },
                  "$r": 2
                }
              }
            }
          }
        },
        {
          "third": {
            "$gt": {
              "amount": {
                "$add": {
                  "$l": {
                    "first": "amount"
                  },
                  "$r": {
                    "second": "amount"
                  }
                }
              }
            }
          }
        }
      ],
      "run": "fraud"
    }
  }
}
```
[top](reference.md#table-of-contents)  

### Choice of Sequences
The following two labels can be used and combined to define richer event sequences:  
* `"all"`: a set of event or fact patterns. All of them are required to match to trigger an action.  
* `"any"`: a set of event or fact patterns. Any one match will trigger an action.  

```javascript
{
  "expense": {
    "r_0": {
      "any": [
        {
          "m_0$all": [
            {
              "first": {
                "subject": "approve"
              }
            },
            {
              "second": {
                "amount": 1000
              }
            }
          ]
        },
        {
          "m_1$all": [
            {
              "third": {
                "subject": "jumbo"
              }
            },
            {
              "fourth": {
                "amount": 10000
              }
            }
          ]
        }
      ],
      "run": "log"
    }
  }
}
```
[top](reference.md#table-of-contents) 

### Lack of Information
The `$not` modifier can be used in rules with correlated sequences to evaluate the lack of information.

```javascript
{
  "risk": {
    "r_0": {
      "all": [
        {
          "first": {
            "t": "deposit"
          }
        },
        {
          "m_1$not": {
            "t": "balance"
          }
        },
        {
          "third": {
            "t": "withdrawal"
          }
        },
        {
          "fourth": {
            "t": "chargeback"
          }
        }
      ],
      "run": "fraud"
    }
  }
}
```

[top](reference.md#table-of-contents)  

### Nested Objects
Queries on nested events or facts are also supported. The `.` notation is used for defining conditions on properties in nested objects.  

```javascript
{
  "expense": {
    "r_0": {
      "all": [
        {
          "bill": {
            "$and": [
              {
                "t": "bill"
              },
              {
                "$gt": {
                  "invoice.amount": 50
                }
              }
            ]
          }
        },
        {
          "account": {
            "$and": [
              {
                "t": "account"
              },
              {
                "payment.invoice.amount": {
                  "bill": "invoice.amount"
                }
              }
            ]
          }
        }
      ],
      "run": "log"
    }
  }
}
```
[top](reference.md#table-of-contents)  

### Arrays

Use `"$iall"` to match all items in an array, `"$iany"` to match any element in an array, `"$i"` to specify an item in the array.  

```javascript
{
  "risk": {
    "r_0": {
      "all": [
        {
          "m": {
            "$iall": {
              "payments": {
                "$gt": {
                  "$i": 2000
                }
              }
            }
          }
        }
      ],
      "run": "fraud1"
    },
    "r_1": {
      "all": [
        {
          "m": {
            "$iall": {
              "payments": {
                "$gt": {
                  "$i": 1000
                }
              }
            }
          }
        }
      ],
      "run": "fraud2"
    },
    "r_2": {
      "all": [
        {
          "m": {
            "$iall": {
              "payments": {
                "$or": [
                  {
                    "$lt": {
                      "$i.amount": 250
                    }
                  },
                  {
                    "$gte": {
                      "$i.amount": 300
                    }
                  }
                ]
              }
            }
          }
        }
      ],
      "run": "fraud3"
    },
    "r_3": {
      "all": [
        {
          "m": {
            "$iany": {
              "cards": {
                "$mt": {
                  "$i": "three.*"
                }
              }
            }
          }
        }
      ],
      "run": "fraud4"
    },
    "r_4": {
      "all": [
        {
          "m": {
            "$iany": {
              "payments": {
                "$iall": {
                  "$i": {
                    "$lt": {
                      "$i": 100
                    }
                  }
                }
              }
            }
          }
        }
      ],
      "run": "fraud5"
    },
    "r_5": {
      "all": [
        {
          "m": {
            "$and": [
              {
                "$iall": {
                  "payments": {
                    "$gt": {
                      "$i": 100
                    }
                  }
                }
              },
              {
                "cash": true
              }
            ]
          }
        }
      ],
      "run": "fraud6"
    }
  }
}
```
[top](reference.md#table-of-contents) 
### Facts and Events as rvalues

Aside from scalars (strings, number and boolean values), it is possible to use the fact or event observed on the right side of an expression. Use `"$m"` to reference the event or fact. 

```javascript
{
  "risk": {
    "r_0": {
      "all": [
        {
          "m": {
            "$gt": {
              "debit": {
                "$mul": {
                  "$l": {
                    "$m": "credit"
                  },
                  "$r": 2
                }
              }
            }
          }
        }
      ],
      "run": "log"
    },
    "r_1": {
      "all": [
        {
          "first": {
            "$gt": {
              "amount": 100
            }
          }
        },
        {
          "second": {
            "$gt": {
              "amount": {
                "$add": {
                  "$l": {
                    "first": "amount"
                  },
                  "$r": {
                    "$div": {
                      "$l": {
                        "$m": "amount"
                      },
                      "$r": 2
                    }
                  }
                }
              }
            }
          }
        }
      ],
      "run": "log"
    }
  }
}
```

[top](reference.md#table-of-contents) 

## Consequents
### Conflict Resolution
Event and fact evaluation can lead to multiple consequents. The triggering order can be controlled by using the `pri` (salience) attribute. 
```javascript
{
  "attributes": {
    "r_0": {
      "pri": 3,
      "all": [
        {
          "m": {
            "$lt": {
              "amount": 300
            }
          }
        }
      ],
      "run": "logP3"
    },
    "r_1": {
      "pri": 2,
      "all": [
        {
          "m": {
            "$lt": {
              "amount": 200
            }
          }
        }
      ],
      "run": "logP2"
    },
    "r_2": {
      "pri": 1,
      "all": [
        {
          "m": {
            "$lt": {
              "amount": 100
            }
          }
        }
      ],
      "run": "logP1"
    }
  }
}
```
[top](reference.md#table-of-contents) 

### Unhandled Exceptions  
When exceptions are not handled by actions, they are stored in the context state. Exceptions are stored as an object with `"$ex"` name. 

```javascript
{
  "flow": {
    "r_0": {
      "all": [
        {
          "m": {
            "action": "start"
          }
        }
      ],
      "run": "start"
    },
    "r_1": {
      "all": [
        {
          "m": {
            "$and": [
              {
                "$ex": {
                  "exception": 1
                }
              },
              {
                "$s": 1
              }
            ]
          }
        }
      ],
      "run": "exceptionHandler"
    }
  }
}
```
[top](reference.md#table-of-contents)  

## Flow Structures

### Statechart
Rules can be organized using statecharts. A statechart is a deterministic finite automaton (DFA). The state context is in one of a number of possible states with conditional transitions between these states. 

Statechart rules:  
* A statechart can have one or more states.  
* A statechart requires an initial state.  
* An initial state is defined as a vertex without incoming edges.  
* A state can have zero or more triggers.  
* A state can have zero or more states (see [nested states](reference.md#nested-states)).  
* A trigger has a destination state.  
* A trigger can have a rule (absence means state enter).  
* A trigger can have an action.  

```javascript
{
  "expense$state": {
    "input": {
      "t_0": {
        "all": [
          {
            "m": {
              "$and": [
                {
                  "subject": "approve"
                },
                {
                  "$gt": {
                    "amount": 1000
                  }
                }
              ]
            }
          }
        ],
        "to": "denied",
        "run": "logDenied"
      },
      "t_1": {
        "all": [
          {
            "m": {
              "$and": [
                {
                  "subject": "approve"
                },
                {
                  "$lte": {
                    "amount": 1000
                  }
                }
              ]
            }
          }
        ],
        "to": "pending",
        "run": "logPending"
      }
    },
    "pending": {
      "t_0": {
        "all": [
          {
            "m": {
              "subject": "approved"
            }
          }
        ],
        "to": "approved",
        "run": "logApproved"
      },
      "t_1": {
        "all": [
          {
            "m": {
              "subject": "denied"
            }
          }
        ],
        "to": "denied",
        "run": "logDenied"
      }
    },
    "denied": {},
    "approved": {}
  }
}

```
[top](reference.md#table-of-contents)  
### Nested States
Nested states allow for writing compact statecharts. If a context is in the nested state, it also (implicitly) is in the surrounding state. The statechart will attempt to handle any event in the context of the sub-state. If the sub-state does not handle an event, the event is automatically handled at the context of the super-state. Nested states are defined as an object with the name `"$chart"`.

```javascript
{
   "worker$state": {
    "work": {
      "t_0": {
        "pri": 1,
        "all": [
          {
            "m": {
              "subject": "cancel"
            }
          }
        ],
        "to": "canceled",
        "run": "logCanceled"
      },
      "$chart": {
        "enter": {
          "t_0": {
            "all": [
              {
                "m": {
                  "subject": "enter"
                }
              }
            ],
            "to": "process",
            "run": "logProcess"
          }
        },
        "process": {
          "t_0": {
            "all": [
              {
                "m": {
                  "subject": "continue"
                }
              }
            ],
            "to": "process",
            "run": "logProcess"
          }
        }
      }
    },
    "canceled": {}
  }
}

```
[top](reference.md#table-of-contents)
### Flowchart
A flowchart is another way of organizing a ruleset flow. In a flowchart each stage represents an action to be executed. So (unlike the statechart state), when applied to the context state, it results in a transition to another stage.  

Flowchart rules:  
* A flowchart can have one or more stages.  
* A flowchart requires an initial stage.  
* An initial stage is defined as a vertex without incoming edges.  
* A stage can have an action.  
* A stage can have zero or more conditions.  
* A condition has a rule and a destination stage.  

```javascript
{
  "expense$flow": {
    "input": {
      "to": {
        "request": {
          "all": [
            {
              "m": {
                "$and": [
                  {
                    "subject": "approve"
                  },
                  {
                    "$lte": {
                      "amount": 1000
                    }
                  }
                ]
              }
            }
          ]
        },
        "deny": {
          "all": [
            {
              "m": {
                "$and": [
                  {
                    "subject": "approve"
                  },
                  {
                    "$gt": {
                      "amount": 1000
                    }
                  }
                ]
              }
            }
          ]
        }
      }
    },
    "request": {
      "run": "logRequest",
      "to": {
        "approve": {
          "all": [
            {
              "m": {
                "subject": "approved"
              }
            }
          ]
        },
        "deny": {
          "all": [
            {
              "m": {
                "subject": "denied"
              }
            }
          ]
        },
        "request": {
          "all": [
            {
              "m": {
                "subject": "retry"
              }
            }
          ]
        }
      }
    },
    "approve": {
      "run": "logApproved",
      "to": {}
    },
    "deny": {
      "run": "logDenied",
      "to": {}
    }
  }
}
```
[top](reference.md#table-of-contents)  
### Timers
Events can be scheduled with timers. A timeout condition can be included in the rule antecedent. Use `"$t"` to specify the timer name to be observed.


```javascript
{
   "timer": {
    "r_0": {
      "any": [
        {
          "m_0$all": [
            {
              "m_0.m": {
                "$and": [
                  {
                    "count": 0
                  },
                  {
                    "$s": 1
                  }
                ]
              }
            }
          ]
        },
        {
          "m_1$all": [
            {
              "m_1.m_0": {
                "$and": [
                  {
                    "$lt": {
                      "count": 5
                    }
                  },
                  {
                    "$s": 1
                  }
                ]
              }
            },
            {
              "m_1.m_1": {
                "$t": "MyTimer"
              }
            }
          ]
        }
      ],
      "run": "restartTimer"
    },
    "r_1": {
      "all": [
        {
          "m": {
            "cancel": true
          }
        }
      ],
      "run": "cancelTimer"
    }
  }
}
```

The example below uses a timer to detect higher event rate (use `"count"` to match a specific number of events):

```javascript
{
  "risk$state": {
    "start": {
      "t_0": {
        "to": "meter"
      }
    },
    "meter": {
      "t_0": {
        "count": 3,
        "all": [
          {
            "message": {
              "$gt": {
                "amount": 100
              }
            }
          }
        ],
        "to": "fraud",
        "run": "logFraud"
      },
      "t_1": {
        "all": [
          {
            "m": {
              "$t": "RiskTimer"
            }
          }
        ],
        "to": "exit",
        "run": "logExit"
      }
    },
    "fraud": {},
    "exit": {}
  }
}
```

In this example a manual reset timer is used for measuring velocity (use  `"cap"` to limit the number of events matched). 

Try issuing the command below multiple times.

<sub>`curl -H "content-type: application/json" -X POST -d '{"amount": 200}' http://localhost:5000/risk/events`</sub>  

```javascript
{
  "risk4$state": {
    "start": {
      "t_0": {
        "to": "meter",
        "run": "startTimer"
      }
    },
    "meter": {
      "t_0": {
        "cap": 100,
        "all": [
          {
            "message": {
              "$gt": {
                "amount": 100
              }
            }
          },
          {
            "m_1": {
              "$t": "VelocityTimer"
            }
          }
        ],
        "to": "meter",
        "run": "logVelocityAndResetTimers"
      },
      "t_1": {
        "all": [
          {
            "m": {
              "$t": "VelocityTimer"
            }
          }
        ],
        "to": "meter",
        "run": "logVelocityAndResetTimers"
      }
    }
  }
}

```

[top](reference.md#table-of-contents)  
 

