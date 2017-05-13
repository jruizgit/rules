=============
durable_rules
=============  
durable_rules is a polyglot micro-framework for real-time, consistent and scalable coordination of events. With durable_rules you can track and analyze information about things that happen (events) by combining data from multiple sources to infer more complicated circumstances.

A full forward chaining implementation (A.K.A. Rete) is used to evaluate facts and massive streams of events in real time. A simple, yet powerful meta-liguistic abstraction lets you define simple and complex rulesets as well as control flow structures such as flowcharts, statecharts, nested statecharts and time driven flows. 

The durable_rules core engine is implemented in C, which enables ultra fast rule evaluation as well as muti-language support. durable_rules relies on state of the art technologies: Werkzeug(http://werkzeug.pocoo.org/ is used to host rulesets). Inference state is cached using Redis(http://www.redis.io), This allows for fault tolerant execution and scale-out without giving up performance.

**Example 1**

durable_rules is simple: to define a rule, all you need to do is describe the event or fact pattern to match (antecedent) and the action to take (consequent). In this example the rule can be triggered by posting `{"subject": "World"}` to url `http://localhost:5000/test/events`

Once the test is running, from a terminal type:   

`curl -H "Content-type: application/json" -X POST -d '{"subject": "World"}' http://localhost:5000/test/events`

::

    from durable.lang import *

    with ruleset('test'):
        # antecedent
        @when_all(m.subject == 'World')
        def say_hello(c):
            # consequent
            print ('Hello {0}'.format(c.m.subject))

    run_all()

**Example 2**

durable_rules super-power is the foward-chaining evaluation of rules. In other words, the repeated application of logical modus ponens(https://en.wikipedia.org/wiki/Modus_ponens) to a set of facts or observed events to derive a conclusion. The example below shows a set of rules applied to a small knowledge base (set of facts).

::

    from durable.lang import *

    with ruleset('animal'):
        @when_all(c.first << (m.predicate == 'eats') & (m.object == 'flies'),
                  (m.predicate == 'lives') & (m.object == 'water') & (m.subject == c.first.subject))
        def frog(c):
            c.assert_fact({ 'subject': c.first.subject, 'predicate': 'is', 'object': 'frog' })

        @when_all(c.first << (m.predicate == 'eats') & (m.object == 'flies'),
                  (m.predicate == 'lives') & (m.object == 'land') & (m.subject == c.first.subject))
        def chameleon(c):
            c.assert_fact({ 'subject': c.first.subject, 'predicate': 'is', 'object': 'chameleon' })

        @when_all((m.predicate == 'eats') & (m.object == 'worms'))
        def bird(c):
            c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'bird' })

        @when_all((m.predicate == 'is') & (m.object == 'frog'))
        def green(c):
            c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'green' })

        @when_all((m.predicate == 'is') & (m.object == 'chameleon'))
        def grey(c):
            c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'grey' })

        @when_all((m.predicate == 'is') & (m.object == 'bird'))
        def black(c):
            c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'black' })

        @when_all(count(11), +m.subject)
        def output(c):
            for f in c.m:
                print ('Fact: {0} {1} {2}'.format(f.subject, f.predicate, f.object))

        @when_start
        def start(host):
            host.assert_fact('animal', { 'subject': 'Kermit', 'predicate': 'eats', 'object': 'flies' })
            host.assert_fact('animal', { 'subject': 'Kermit', 'predicate': 'lives', 'object': 'water' })
            host.assert_fact('animal', { 'subject': 'Greedy', 'predicate': 'eats', 'object': 'flies' })
            host.assert_fact('animal', { 'subject': 'Greedy', 'predicate': 'lives', 'object': 'land' })
            host.assert_fact('animal', { 'subject': 'Tweety', 'predicate': 'eats', 'object': 'worms' })
            
    run_all()

**Example 3**

The combination of forward inference and durable_rules tolerance to failures on rule action dispatch, enables work coordination with data flow structures such as statecharts, nested states and flowcharts. 

Once the test is running, from a terminal type:   

`curl -H "Content-type: application/json" -X POST -d '{"subject": "approve", "amount": 100}' http://localhost:5000/expense/events`  

`curl -H "Content-type: application/json" -X POST -d '{"subject": "approved"}' http://localhost:5000/expense/events`  

`curl -H "Content-type: application/json" -X POST -d '{"subject": "approve", "amount": 100}' http://localhost:5000/expense/events/2`  

`curl -H "Content-type: application/json" -X POST -d '{"subject": "denied"}' http://localhost:5000/expense/events/2`  

::

    from durable.lang import *

    with statechart('expense'):
        with state('input'):
            @to('denied')
            @when_all((m.subject == 'approve') & (m.amount > 1000))
            def denied(c):
                print ('expense denied')
            
            @to('pending')    
            @when_all((m.subject == 'approve') & (m.amount <= 1000))
            def request(c):
                print ('requesting expense approval')
            
        with state('pending'):
            @to('approved')
            @when_all(m.subject == 'approved')
            def approved(c):
                print ('expense approved')
                
            @to('denied')
            @when_all(m.subject == 'denied')
            def denied(c):
                print ('expense denied')
            
        state('denied')
        state('approved')
        
    run_all()

**Example 4**

durable_rules provides string pattern matching. Expressions are compiled down to a DFA, guaranteeing linear execution time in the order of single digit nano seconds per character (note: backtracking expressions are not supported).

Once the test is running, from a terminal type:  

`curl -H "Content-type: application/json" -X POST -d '{"subject": "375678956789765"}' http://localhost:5000/test/events`  

`curl -H "Content-type: application/json" -X POST -d '{"subject": "4345634566789888"}' http://localhost:5000/test/events`  

`curl -H "Content-type: application/json" -X POST -d '{"subject": "2228345634567898"}' http://localhost:5000/test/events` 

::

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

**Example 5**

durable_rules can also be used to solve traditional Production Bussiness Rules problems. The example below is the 'Miss Manners' benchmark. Miss Manners has decided to throw a party. She wants to seat her guests such that adjacent guests are of opposite sex and share at least one hobby. 

Note how the benchmark flow structure is defined using a statechart to improve code readability without sacrificing performance nor altering the combinatorics required by the benchmark. For 128 guests, 438 facts, the execution time is less than 2 seconds. More details documented in this blog post (http://jruizblog.com/2015/07/20/miss-manners-and-waltzdb/).

::

    from durable.lang import *

    with statechart('miss_manners'):
        with state('start'):
            @to('assign')
            @when_all(m.t == 'guest')
            def assign_first_seating(c):
                c.s.count = 0
                c.s.g_count = 1000
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
                print('assign {0}'.format(c.m.name))

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
            @when_all(cap(1000),
                      c.seating << (m.t == 'seating') & 
                                   (m.path == False),
                      c.path << (m.t == 'path') & 
                                (m.p_id == c.seating.p_id),
                      none((m.t == 'path') & 
                           (m.p_id == c.seating.s_id) & 
                           (m.guest_name == c.path.guest_name)))
            def make_path(c):
                for frame in c.m:
                    c.assert_fact({'t': 'path',
                                   'id': c.s.g_count,
                                   'p_id': frame.seating.s_id, 
                                   'seat': frame.path.seat, 
                                   'guest_name': frame.path.guest_name})
                    c.s.g_count += 1
                
            @to('check')
            @when_all(pri(1), (m.t == 'seating') & (m.path == False))
            def path_done(c):
                c.retract_fact(c.m)
                c.m.id = c.s.g_count
                c.m.path = True
                c.assert_fact(c.m)
                c.s.g_count += 1
                print('path sid: {0}, pid: {1}, left guest: {2}, right guest {3}'.format(c.m.s_id, c.m.p_id, c.m.left_guest_name, c.m.right_guest_name))

        with state('check'):
            @to('end')
            @when_all(c.last_seat << m.t == 'last_seat', 
                     (m.t == 'seating') & (m.right_seat == c.last_seat.seat))
            def done(c):
                print('end')
            
            to('assign')

        state('end')

**Reference Manual:**

- Python(https://github.com/jruizgit/rules/blob/master/docs/py/reference.md/)

**Blog:** 

- Miss Manners and Waltzdb (07/2015): http://jruizblog.com/2015/07/20/miss-manners-and-waltzdb/
- Polyglot (03/2015):http://jruizblog.com/2015/03/02/polyglot/
- Rete_D (02/2015):http://jruizblog.com/2015/02/23/rete_d/
- Boosting Performance with C (08/2014): http://jruizblog.com/2014/08/19/boosting-performance-with-c/
- Rete Meets Redis (02/2014):http://jruizblog.com/2014/02/02/rete-meets-redis/
- From Expert Systems to Cloud Scale Event Processing (01/2014):http://jruizblog.com/2014/01/27/event-processing/

