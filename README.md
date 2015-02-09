Durable Rules
=====
Durable Rules is a polyglot micro-framework for real-time, consistent and scalable coordination of events. With Durable Rules you can track and analyze information about things that happen (events) by combining data from multiple sources to infer more complicated circumstances.

A full forward chaining implementation (A.K.A. Rete) is used to evaluate facts and massive streams of events in real time. A simple, yet powerful meta-liguistic abstraction lets you define simple and complex rulesets, such as flowcharts, statecharts, nested statecharts, paralel and time driven flows. 

The Durable Rules core engine is implemented in C, which enables ultra fast rule evaluation and inference as well as muti-language support. Durable Rules relies on state of the art technologies:

* [Node.js](http://www.nodejs.org), [Werkzeug](http://werkzeug.pocoo.org/), [Sinatra](http://www.sinatrarb.com/) are used to host rulesets written in JavaScript, Python and Ruby respectively.
* Inference state is cached using [Redis](http://www.redis.io), which lets scaling out without giving up performance.
* A web client based on [D3.js](http://www.d3js.org) provides powerful data visualization and test tools.  

###Example #1  

Let’s consider a couple of fictitious fraud rules used in bank account management. 
Note: I'm paraphrasing the example presented in this [article](https://www.packtpub.com/books/content/drools-jboss-rules-50complex-event-processing).  

1. If there are two debit requests greater than 200% the average monthly withdrawal amount in a span of 2 minutes, flag the account as medium risk.
2. If there are three consecutive increasing debit requests, withdrawing more than 70% the average monthly balance in a span of three minutes, flag the account as high risk.

Remarks: The first rule uses a tumbling time window to calculate averages `span(86400)`. The second rule uses arithmetic and logical event correlation. The third rule leverages an eventually consistent context reference `s.avg_balance < `.

####Ruby
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
    puts "Medium risk #{first.amount}, #{second.amount}"
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
    puts "High risk #{first.amount}, #{second.amount}, #{third.amount}"
  end
end

Durable.run_all
```
####Python
```python
from durable.lang import *
import time

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
        print('Medium Risk {0}, {1}'.format(c.first.amount, c.second.amount))

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
        print('High Risk {0}, {1}, {2}'.format(c.first.amount, c.second.amount, c.third.amount))

run_all()
```
####JavaScript
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
        console.log('Medium risk ' + c.first.amount + ' ,' + c.second.amount);
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
        console.log('High risk ' + c.first.amount + ' ,' + c.second.amount + ' ,' + c.third.amount);
    });
}

d.runAll();
```

### Example #2

Durable Rules supports the expresiveness of traditional production business rules. From conflict resolution to absence rules. The manners benchmark implementation.

```ruby
require "durable"

Durable.statechart :miss_manners do
  state :start do
    to :assign, when_all(m.t == "guest") do
      assert(:t => "seating",
             :id => s.g_count,
             :s_id => s.count,
             :p_id => 0,
             :path => true, 
             :left_seat => 1,
             :left_guest_name => m.name,
             :right_seat => 1,
             :right_guest_name => m.name)
      assert(:t => "path",
             :id => s.g_count + 1,
             :s_id => s.count,
             :seat => 1, 
             :guest_name => m.name)
      s.count += 1
      s.g_count += 2
      puts("assign #{m.name}, #{Time.now}")
    end
  end
  state :assign do
    to :make, when_all(c.seating = (m.t == "seating") & 
                                   (m.path == true), 
                       c.right_guest = (m.t == "guest") & 
                                       (m.name == seating.right_guest_name), 
                       c.left_guest = (m.t == "guest") & 
                                      (m.sex != right_guest.sex) & 
                                      (m.hobby == right_guest.hobby),
                       none((m.t == "path") & 
                            (m.p_id == seating.s_id) & 
                            (m.guest_name == left_guest.name)),
                       none((m.t == "chosen") & 
                            (m.c_id == seating.s_id) & 
                            (m.guest_name == left_guest.name) &
                            (m.hobby == right_guest.hobby))) do
      assert(:t => "seating",
             :id => s.g_count,
             :s_id => s.count,
             :p_id => seating.s_id,
             :path => false,
             :left_seat => seating.right_seat,
             :left_guest_name => seating.right_guest_name,
             :right_seat => seating.right_seat + 1,
             :right_guest_name => left_guest.name)
      assert(:t => "path",
             :id => s.g_count + 1,
             :p_id => s.count,
             :seat => seating.right_seat + 1,
             :guest_name => left_guest.name)
      assert(:t => "chosen",
             :id => s.g_count + 2,
             :c_id => seating.s_id,
             :guest_name => left_guest.name,
             :hobby => right_guest.hobby)
      s.count += 1
      s.g_count += 3
    end
  end
  state :make do
    to :make, when_all(c.seating = (m.t == "seating") &
                                   (m.path == false),
                       c.path = (m.t == "path") &
                                (m.p_id == seating.p_id),
                       none((m.t == "path") &
                            (m.p_id == seating.s_id) &
                            (m.guest_name == path.guest_name))) do
      assert(:t => "path",
             :id => s.g_count,
             :p_id => seating.s_id,
             :seat => path.seat,
             :guest_name => path.guest_name)
      s.g_count += 1
    end
    to :check, when_all(pri(1), (m.t == "seating") & (m.path == false)) do
      retract(m)
      m.id = s.g_count
      m.path = true
      assert(m)
      s.g_count += 1
      puts("path sid: #{m.s_id}, pid: #{m.p_id}, left: #{m.left_guest_name}, right: #{m.right_guest_name}")
    end
  end
  state :check do
    to :end, when_all(c.last_seat = (m.t == "last_seat"),
                     (m.t == "seating") & (m.right_seat == last_seat.seat)) do
      puts("end #{Time.now}")
    end
    to :assign
  end
  state :end
end
Durable.run_all
```
####Python
```python
from durable.lang import *
import datetime

with statechart('miss_manners'):
    with state('start'):
        @to('assign')
        @when_all(m.t == 'guest')
        def assign_first_seating(c):
            c.assert_fact({'t': 'seating',
                           'id': c.s.g_count,
                           's_id': c.s.count, 
                           'p_id': 0, 
                           'path': True, 
                           'left_seat': 1, 
                           'left_guest_name': c.m.name,
                           'right_seat': 1,
                           'right_guest_name': c.m.name})
            c.assert_fact({'t': 'path',
                           'id': c.s.g_count + 1,
                           'p_id': c.s.count, 
                           'seat': 1, 
                           'guest_name': c.m.name})
            c.s.count += 1
            c.s.g_count += 2
            print('assign {0}, {1}'.format(c.m.name, datetime.datetime.now().strftime('%I:%M:%S%p')))

    with state('assign'):
        @to('make')
        @when_all(c.seating << (m.t == 'seating') & 
                               (m.path == True),
                  c.right_guest << (m.t == 'guest') & 
                                   (m.name == c.seating.right_guest_name),
                  c.left_guest << (m.t == 'guest') & 
                                  (m.sex != c.right_guest.sex) & 
                                  (m.hobby == c.right_guest.hobby),
                  none((m.t == 'path') & 
                       (m.p_id == c.seating.s_id) & 
                       (m.guest_name == c.left_guest.name)),
                  none((m.t == 'chosen') & 
                       (m.c_id == c.seating.s_id) & 
                       (m.guest_name == c.left_guest.name) & 
                       (m.hobby == c.right_guest.hobby)))
        def find_seating(c):
            c.assert_fact({'t': 'seating',
                           'id': c.s.g_count,
                           's_id': c.s.count, 
                           'p_id': c.seating.s_id, 
                           'path': False, 
                           'left_seat': c.seating.right_seat, 
                           'left_guest_name': c.seating.right_guest_name,
                           'right_seat': c.seating.right_seat + 1,
                           'right_guest_name': c.left_guest.name})
            c.assert_fact({'t': 'path',
                           'id': c.s.g_count + 1,
                           'p_id': c.s.count, 
                           'seat': c.seating.right_seat + 1, 
                           'guest_name': c.left_guest.name})
            c.assert_fact({'t': 'chosen',
                           'id': c.s.g_count + 2,
                           'c_id': c.seating.s_id,
                           'guest_name': c.left_guest.name,
                           'hobby': c.right_guest.hobby})
            c.s.count += 1
            c.s.g_count += 3

    with state('make'):
        @to('make')
        @when_all(c.seating << (m.t == 'seating') & 
                               (m.path == False),
                  c.path << (m.t == 'path') & 
                            (m.p_id == c.seating.p_id),
                  none((m.t == 'path') & 
                       (m.p_id == c.seating.s_id) & 
                       (m.guest_name == c.path.guest_name)))
        def make_path(c):
            c.assert_fact({'t': 'path',
                           'id': c.s.g_count,
                           'p_id': c.seating.s_id, 
                           'seat': c.path.seat, 
                           'guest_name': c.path.guest_name})
            c.s.g_count += 1

        @to('check')
        @when_all(pri(1), (m.t == 'seating') & (m.path == False))
        def path_done(c):
            c.retract_fact(c.m)
            c.m.id = c.s.g_count
            c.m.path = True
            c.assert_fact(c.m)
            c.s.g_count += 1
            print('path sid: {0}, pid: {1}, left: {2}, right: {3}'.format(c.m.s_id, c.m.p_id, c.m.left_guest_name, c.m.right_guest_name))

    with state('check'):
        @to('end')
        @when_all(c.last_seat << m.t == 'last_seat', 
                 (m.t == 'seating') & (m.right_seat == c.last_seat.seat))
        def done(c):
            print('end {0}'.format(datetime.datetime.now().strftime('%I:%M:%S%p')))
        
        to('assign')
        
    state('end')
    
run_all()
```
####JavaScript
```javascript
var d = require('durable');

with (d.statechart('missManners')) {
    with (state('start')) {
        to('assign').whenAll(m.t.eq('guest'), 
        function(c) {
            c.assert({t: 'seating',
                      id: c.s.gcount,
                      tid: c.s.count,
                      pid: 0,
                      path: true,
                      leftSeat: 1,
                      leftGuestName: c.m.name,
                      rightSeat: 1,
                      rightGuestName: c.m.name});
            c.assert({t: 'path',
                      id: c.s.gcount + 1,
                      pid: c.s.count,
                      seat: 1,
                      guestName: c.m.name});
            c.s.count += 1;
            c.s.gcount += 2;
            console.log('assign ' + c.m.name + ' ' + new Date());
        });
    }
    with (state('assign')) {
        to('make').whenAll(c.seating = and(m.t.eq('seating'), 
                                           m.path.eq(true)),
                           c.rightGuest = and(m.t.eq('guest'), 
                                              m.name.eq(c.seating.rightGuestName)),
                           c.leftGuest = and(m.t.eq('guest'), 
                                             m.sex.neq(c.rightGuest.sex), 
                                             m.hobby.eq(c.rightGuest.hobby)),
                           none(and(m.t.eq('path'),
                                    m.pid.eq(c.seating.tid),
                                    m.guestName.eq(c.leftGuest.name))),
                           none(and(m.t.eq('chosen'),
                                    m.cid.eq(c.seating.tid),
                                    m.guestName.eq(c.leftGuest.name),
                                    m.hobby.eq(c.rightGuest.hobby))), 
        function(c) {
            c.assert({t: 'seating',
                      id: c.s.gcount,
                      tid: c.s.count,
                      pid: c.seating.tid,
                      path: false,
                      leftSeat: c.seating.rightSeat,
                      leftGuestName: c.seating.rightGuestName,
                      rightSeat: c.seating.rightSeat + 1,
                      rightGuestName: c.leftGuest.name});
            c.assert({t: 'path',
                      id: c.s.gcount + 1,
                      pid: c.s.count,
                      seat: c.seating.rightSeat + 1,
                      guestName: c.leftGuest.name});
            c.assert({t: 'chosen',
                      id: c.s.gcount + 2,
                      cid: c.seating.tid,
                      guestName: c.leftGuest.name,
                      hobby: c.rightGuest.hobby});
            c.s.count += 1;
            c.s.gcount += 3;
        }); 
    }
    with (state('make')) {
        to('make').whenAll(c.seating = and(m.t.eq('seating'),
                                           m.path.eq(false)),
                           c.path = and(m.t.eq('path'),
                                        m.pid.eq(c.seating.pid)),
                           none(and(m.t.eq('path'),
                                    m.pid.eq(c.seating.tid),
                                    m.guestName.eq(c.path.guestName))), 
        function(c) {
            c.assert({t: 'path',
                      id: c.s.gcount,
                      pid: c.seating.tid,
                      seat: c.path.seat,
                      guestName: c.path.guestName});
            c.s.gcount += 1;
        });

        to('check').whenAll(pri(1), and(m.t.eq('seating'), m.path.eq(false)), 
        function(c) {
            c.retract(c.m);
            c.m.id = c.s.gcount;
            c.m.path = true;
            c.assert(c.m);
            c.s.gcount += 1;
            console.log('path sid: ' + c.m.tid + ', pid: ' + c.m.pid + ', left: ' + c.m.leftGuestName + ', right: ' + c.m.rightGuestName + ' ' + new Date());
        });
    }
    with (state('check')) {
        to('end').whenAll(c.lastSeat = m.t.eq('lastSeat'),
                          and(m.t.eq('seating'), m.rightSeat.eq(c.lastSeat.seat)), 
        function(c) {
            console.log('end ' + new Date());
        });
        to('assign');
    }
    
    state('end');
  }

d.runAll();
```  

Blog:
* [Boosting Performance with C (08/2014)](http://jruizblog.com/2014/08/19/boosting-performance-with-c/)
* [Rete Meets Redis (02/2014)](http://jruizblog.com/2014/02/02/rete-meets-redis/)
* [Inference: From Expert Systems to Cloud Scale Event Processing (01/2014)](http://jruizblog.com/2014/01/27/event-processing/)



