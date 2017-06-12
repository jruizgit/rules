from durable.lang import *
import threading
import datetime


with statechart('expense'):
    with state('input'):
        @to('denied')
        @when_all((m.subject == 'approve') & (m.amount > 1000))
        def denied(c):
            print ('expense denied')
        
        @to('pending')    
        @when_all((m.subject == 'approve') & (m.amount <= 1000))
        def request(c):
            print ('requesting expense approva')
        
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

    @when_all(+m.subject)
    def output(c):
        print ('Fact: {0} {1} {2}'.format(c.m.subject, c.m.predicate, c.m.object))

    @when_start
    def start(host):
        host.assert_fact('animal', { 'subject': 'Kermit', 'predicate': 'eats', 'object': 'flies' })
        host.assert_fact('animal', { 'subject': 'Kermit', 'predicate': 'lives', 'object': 'water' })
        host.assert_fact('animal', { 'subject': 'Greedy', 'predicate': 'eats', 'object': 'flies' })
        host.assert_fact('animal', { 'subject': 'Greedy', 'predicate': 'lives', 'object': 'land' })
        host.assert_fact('animal', { 'subject': 'Tweety', 'predicate': 'eats', 'object': 'worms' })


with ruleset('test'):
    # antecedent
    @when_all(m.subject == 'World')
    def say_hello(c):
        # consequent
        print('Hello {0}'.format(c.m.subject))

    # on ruleset start
    @when_start
    def start(host):    
        host.post('test', { 'subject': 'World' })


with ruleset('animal0'):
    # will be triggered by 'Kermit eats flies'
    @when_all((m.predicate == 'eats') & (m.object == 'flies'))
    def frog(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'frog' })

    @when_all((m.predicate == 'eats') & (m.object == 'worms'))
    def bird(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'bird' })

    # will be chained after asserting 'Kermit is frog'
    @when_all((m.predicate == 'is') & (m.object == 'frog'))
    def green(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'green' })

    @when_all((m.predicate == 'is') & (m.object == 'bird'))
    def black(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'black' })

    @when_all(+m.subject)
    def output(c):
        print('Fact: {0} {1} {2}'.format(c.m.subject, c.m.predicate, c.m.object))

    @when_start
    def start(host):
        host.assert_fact('animal0', { 'subject': 'Kermit', 'predicate': 'eats', 'object': 'flies' })


with ruleset('risk'):
    @when_all(c.first << m.t == 'purchase',
              c.second << m.location != c.first.location)
    # the event pair will only be observed once
    def fraud(c):
        print('Fraud detected -> {0}, {1}'.format(c.first.location, c.second.location))

    @when_start
    def start(host):
        # 'post' submits events, try 'assert' instead and to see differt behavior
        host.post('risk', {'t': 'purchase', 'location': 'US'});
        host.post('risk', {'t': 'purchase', 'location': 'CA'});


with ruleset('flow'):
    # state condition uses 's'
    @when_all(s.status == 'start')
    def start(c):
        # state update on 's'
        c.s.status = 'next' 
        print('start')

    @when_all(s.status == 'next')
    def next(c):
        c.s.status = 'last' 
        print('next')

    @when_all(s.status == 'last')
    def last(c):
        c.s.status = 'end' 
        print('last')
        # deletes state at the end
        c.delete_state()

    @when_start
    def on_start(host):
        # modifies default context state
        host.patch_state('flow', { 'status': 'start' })



with ruleset('expense0'):
    @when_all((m.subject == 'approve') | (m.subject == 'ok'))
    def approved(c):
        print ('Approved subject: {0}'.format(c.m.subject))
        
    @when_start
    def start(host):
        host.post('expense0', { 'subject': 'approve'})


with ruleset('match'):
    @when_all(m.url.matches('(https?://)?([0-9a-z.-]+)%.[a-z]{2,6}(/[A-z0-9_.-]+/?)*'))
    def approved(c):
        print ('match url ->{0}'.format(c.m.url))

    @when_start
    def start(host):
        host.post('match', { 'url': 'https://github.com' })
        host.post('match', { 'url': 'http://github.com/jruizgit/rul!es' })
        host.post('match', { 'url': 'https://github.com/jruizgit/rules/reference.md' })
        host.post('match', { 'url': '//rules'})
        host.post('match', { 'url': 'https://github.c/jruizgit/rules' })


with ruleset('strings'):
    @when_all(m.subject.matches('hello.*'))
    def starts_with(c):
        print ('string starts with hello -> {0}'.format(c.m.subject))

    @when_all(m.subject.matches('.*hello'))
    def ends_with(c):
        print ('string ends with hello -> {0}'.format(c.m.subject))

    @when_all(m.subject.imatches('.*hello.*'))
    def contains(c):
        print ('string contains hello (case insensitive) -> {0}'.format(c.m.subject))

    @when_start
    def start(host):
        host.assert_fact('strings', { 'subject': 'HELLO world' })
        host.assert_fact('strings', { 'subject': 'world hello' })
        host.assert_fact('strings', { 'subject': 'hello hi' })
        host.assert_fact('strings', { 'subject': 'has Hello string' })
        host.assert_fact('strings', { 'subject': 'does not match' })
        

with ruleset('risk0'):
    @when_all(c.first << m.amount > 10,
              c.second << m.amount > c.first.amount * 2,
              c.third << m.amount > c.first.amount + c.second.amount)
    def detected(c):
        print('fraud detected -> {0}'.format(c.first.amount))
        print('               -> {0}'.format(c.second.amount))
        print('               -> {0}'.format(c.third.amount))
        
    @when_start
    def start(host):
        host.post('risk0', { 'amount': 50 })
        host.post('risk0', { 'amount': 200 })
        host.post('risk0', { 'amount': 300 })


with ruleset('expense1'):
    @when_any(all(c.first << m.subject == 'approve', 
                  c.second << m.amount == 1000), 
              all(c.third << m.subject == 'jumbo', 
                  c.fourth << m.amount == 10000))
    def action(c):
        if c.first:
            print ('Approved {0} {1}'.format(c.first.subject, c.second.amount))
        else:
            print ('Approved {0} {1}'.format(c.third.subject, c.fourth.amount))
    
    @when_start
    def start(host):
        host.post('expense1', { 'subject': 'approve' })
        host.post('expense1', { 'amount': 1000 })
        host.post('expense1', { 'subject': 'jumbo' })
        host.post('expense1', { 'amount': 10000 })


with ruleset('risk1'):
    @when_all(c.first << m.t == 'deposit',
              none(m.t == 'balance'),
              c.third << m.t == 'withrawal',
              c.fourth << m.t == 'chargeback')
    def detected(c):
        print('fraud detected {0} {1} {2}'.format(c.first.t, c.third.t, c.fourth.t))
        
    @when_start
    def start(host):
        host.post('risk1', { 't': 'deposit' })
        host.post('risk1', { 't': 'withrawal' })
        host.post('risk1', { 't': 'chargeback' })


with ruleset('attributes'):
    @when_all(pri(3), m.amount < 300)
    def first_detect(c):
        print('attributes P3 ->{0}'.format(c.m.amount))
        
    @when_all(pri(2), m.amount < 200)
    def second_detect(c):
        print('attributes P2 ->{0}'.format(c.m.amount))
        
    @when_all(pri(1), m.amount < 100)
    def third_detect(c):
        print('attributes P1 ->{0}'.format(c.m.amount))
        
    @when_start
    def start(host):
        host.assert_fact('attributes', { 'amount': 50 })
        host.assert_fact('attributes', { 'amount': 150 })
        host.assert_fact('attributes', { 'amount': 250 })


with ruleset('expense2'):
    # this rule will trigger as soon as three events match the condition
    @when_all(count(3), m.amount < 100)
    def approve(c):
        print('approved {0}'.format(c.m))

    # this rule will be triggered when 'expense' is asserted batching at most two results       
    @when_all(cap(2),
              c.expense << m.amount >= 100,
              c.approval << m.review == True)
    def reject(c):
        print('rejected {0}'.format(c.m))

    @when_start
    def start(host):
        host.post_batch('expense2', [{ 'amount': 10 },
                                    { 'amount': 20 },
                                    { 'amount': 100 },
                                    { 'amount': 30 },
                                    { 'amount': 200 },
                                    { 'amount': 400 }])
        host.assert_fact('expense2', { 'review': True })


with ruleset('flow0'):
    timer = None

    def start_timer(time, callback):
        timer = threading.Timer(time, callback)
        timer.daemon = True    
        timer.start()

    @when_all(s.state == 'first')
    # async actions take a callback argument to signal completion
    def first(c, complete):
        def end_first():
            c.s.state = 'second'     
            print('first completed')

            # completes the action after 3 seconds
            complete(None)
        
        start_timer(3, end_first)

    @when_all(s.state == 'second')
    def second(c, complete):
        def end_second():
            c.s.state = 'third'
            print('second completed')

            # completes the action after 6 seconds
            # use the first argument to signal an error
            complete(Exception('error detected'))

        start_timer(6, end_second)

        # overrides the 5 second default abandon timeout
        return 10


    @when_start
    def on_start(host):
        host.patch_state('flow0', { 'state': 'first' })


with ruleset('flow1'):
    
    @when_all(m.action == 'start')
    def first(c):
        raise Exception('Unhandled Exception!')

    # when the exception property exists
    @when_all(+s.exception)
    def second(c):
        print(c.s.exception)
        c.s.exception = None
        
    @when_start
    def on_start(host):
        host.post('flow1', { 'action': 'start' })


with ruleset('timer'):
    # will trigger when MyTimer expires
    @when_any(all(s.count == 0),
              all(s.count < 5,
                  timeout('MyTimer')))
    def pulse(c):
        c.s.count += 1
        # MyTimer will expire in 5 seconds
        c.start_timer('MyTimer', 5)
        print('pulse ->{0}'.format(datetime.datetime.now().strftime('%I:%M:%S%p')))
        
    @when_all(m.cancel == True)
    def cancel(c):
        c.cancel_timer('MyTimer')
        print('canceled timer')

    @when_start
    def on_start(host):
        host.patch_state('timer', { 'count': 0 })

# curl -H "content-type: application/json" -X POST -d '{"cancel": true}' http://localhost:5000/timer/events

with statechart('risk3'):
    with state('start'):
        @to('meter')
        def start(c):
            c.start_timer('RiskTimer', 5)

    with state('meter'):
        @to('fraud')
        @when_all(count(3), c.message << m.amount > 100)
        def fraud(c):
            for e in c.m:
                print(e.message) 

        @to('exit')
        @when_all(timeout('RiskTimer'))
        def exit(c):
            print('exit')

    state('fraud')
    state('exit')

    @when_start
    def on_start(host):
        # three events in a row will trigger the fraud rule
        host.post('risk3', { 'amount': 200 })
        host.post('risk3', { 'amount': 300 })
        host.post('risk3', { 'amount': 400 })

        # two events will exit after 5 seconds
        host.post('risk3', { 'sid': 1, 'amount': 500 })
        host.post('risk3', { 'sid': 1, 'amount': 600 })


with statechart('risk4'):
    with state('start'):
        @to('meter')
        def start(c):
            c.start_timer('VelocityTimer', 5, True)

    with state('meter'):
        @to('meter')
        @when_all(cap(5), 
                  m.amount > 100,
                  timeout('VelocityTimer'))
        def some_events(c):
            print('velocity: {0} in 5 seconds'.format(len(c.m)))
            # resets and restarts the manual reset timer
            c.reset_timer('VelocityTimer')
            c.start_timer('VelocityTimer', 5, True)

        @to('meter')
        @when_all(pri(1), timeout('VelocityTimer'))
        def no_events(c):
            print('velocity: no events in 5 seconds')
            c.reset_timer('VelocityTimer')
            c.start_timer('VelocityTimer', 5, True)

    @when_start
    def on_start(host):
        # the velocity will be 4 events in 5 seconds
        host.post('risk4', { 'amount': 200 })
        host.post('risk4', { 'amount': 300 })
        host.post('risk4', { 'amount': 50 })
        host.post('risk4', { 'amount': 500 })
        host.post('risk4', { 'amount': 600 })

# curl -H "content-type: application/json" -X POST -d '{"amount": 200}' http://localhost:5000/risk4/events

with statechart('expense3'):
    # initial state 'input' with two triggers
    with state('input'):
        # trigger to move to 'denied' given a condition
        @to('denied')
        @when_all((m.subject == 'approve') & (m.amount > 1000))
        # action executed before state change
        def denied(c):
            print ('denied amount {0}'.format(c.m.amount))
        
        @to('pending')    
        @when_all((m.subject == 'approve') & (m.amount <= 1000))
        def request(c):
            print ('requesting approve amount {0}'.format(c.m.amount))
    
    # intermediate state 'pending' with two triggers
    with state('pending'):
        @to('approved')
        @when_all(m.subject == 'approved')
        def approved(c):
            print ('expense approved')
            
        @to('denied')
        @when_all(m.subject == 'denied')
        def denied(c):
            print ('expense denied')
    
    # 'denied' and 'approved' are final states    
    state('denied')
    state('approved')

    @when_start
    def on_start(host):
        # events directed to default statechart instance
        host.post('expense3', { 'subject': 'approve', 'amount': 100 });
        host.post('expense3', { 'subject': 'approved' });
        
        # events directed to statechart instance with id '1'
        host.post('expense3', { 'sid': 1, 'subject': 'approve', 'amount': 100 });
        host.post('expense3', { 'sid': 1, 'subject': 'denied' });
        
        # events directed to statechart instance with id '2'
        host.post('expense3', { 'sid': 2, 'subject': 'approve', 'amount': 10000 });


with flowchart('expense4'):
    # initial stage 'input' has two conditions
    with stage('input'): 
        to('request').when_all((m.subject == 'approve') & (m.amount <= 1000))
        to('deny').when_all((m.subject == 'approve') & (m.amount > 1000))
    
    # intermediate stage 'request' has an action and three conditions
    with stage('request'):
        @run
        def request(c):
            print('requesting approve')
            
        to('approve').when_all(m.subject == 'approved')
        to('deny').when_all(m.subject == 'denied')
        # reflexive condition: if met, returns to the same stage
        to('request').when_all(m.subject == 'retry')
    
    with stage('approve'):
        @run 
        def approved(c):
            print('expense approved')

    with stage('deny'):
        @run
        def denied(c):
            print('expense denied')

    @when_start
    def start(host):
        # events for the default flowchart instance, approved after retry
        host.post('expense4', { 'subject': 'approve', 'amount': 100 })
        host.post('expense4', { 'subject': 'retry' })
        host.post('expense4', { 'subject': 'approved' })

        # events for the flowchart instance '1', denied after first try
        host.post('expense4', { 'sid': 1, 'subject': 'approve', 'amount': 100})
        host.post('expense4', { 'sid': 1, 'subject': 'denied'})

        # event for the flowchart instance '2' immediately denied
        host.post('expense4', { 'sid': 2, 'subject': 'approve', 'amount': 10000})


with statechart('worker'):
    # super-state 'work' has two states and one trigger
    with state('work'):
        # sub-sate 'enter' has only one trigger
        with state('enter'):
            @to('process')
            @when_all(m.subject == 'enter')
            def continue_process(c):
                print('start process')
    
        with state('process'):
            @to('process')
            @when_all(m.subject == 'continue')
            def continue_process(c):
                print('continue processing')

        # the super-state trigger will be evaluated for all sub-state triggers
        @to('canceled')
        @when_all(m.subject == 'cancel')
        def cancel(c):
            print('cancel process')

    state('canceled')

    @when_start
    def start(host):
        # will move the statechart to the 'work.process' sub-state
        host.post('worker', { 'subject': 'enter' })

        # will keep the statechart to the 'work.process' sub-state
        host.post('worker', { 'subject': 'continue' })
        host.post('worker', { 'subject': 'continue' })

        # will move the statechart out of the work state
        host.post('worker', { 'subject': 'cancel' })


with ruleset('expense5'):
    @when_all(c.bill << (m.t == 'bill') & (m.invoice.amount > 50),
              c.account << (m.t == 'account') & (m.payment.invoice.amount == c.bill.invoice.amount))
    def approved(c):
        print ('bill amount  ->{0}'.format(c.bill.invoice.amount))
        print ('account payment amount ->{0}'.format(c.account.payment.invoice.amount))
        
    @when_start
    def start(host):
        host.post('expense5', {'t': 'bill', 'invoice': {'amount': 100}})
        host.post('expense5', {'t': 'account', 'payment': {'invoice': {'amount': 100}}})


with ruleset('bookstore'):
    # this rule will trigger for events with status
    @when_all(+m.status)
    def event(c):
        print('Reference {0} status {1}'.format(c.m.reference, c.m.status))

    @when_all(+m.name)
    def fact(c):
        print('Added {0}'.format(c.m.name))
        c.retract_fact({
            'name': 'The new book',
            'reference': '75323',
            'price': 500,
            'seller': 'bookstore'
        })
    # this rule will be triggered when the fact is retracted
    @when_all(none(+m.name))
    def empty(c):
        print('No books')

    @when_start
    def start(host):    
        # will return 0 because the fact assert was successful 
        print(host.assert_fact('bookstore', {
            'name': 'The new book',
            'seller': 'bookstore',
            'reference': '75323',
            'price': 500
        }))

        # will return 212 because the fact has already been asserted
        print(host.assert_fact('bookstore', {
            'reference': '75323',
            'name': 'The new book',
            'price': 500,
            'seller': 'bookstore'
        }))

        # will return 0 because a new event is being posted
        print(host.post('bookstore', {
            'reference': '75323',
            'status': 'Active'
        }))

        # will return 0 because a new event is being posted
        print(host.post('bookstore', {
            'reference': '75323',
            'status': 'Active'
        }))

with ruleset('risk5'):
    # compares properties in the same event, this expression is evaluated in the client 
    @when_all(m.debit > m.credit * 2)
    def fraud_1(c):
        print('debit {0} more than twice the credit {1}'.format(c.m.debit, c.m.credit))

    # compares two correlated events, this expression is evaluated in the backend
    @when_all(c.first << m.amount > 100,
              c.second << m.amount > c.first.amount + m.amount / 2)
    def fraud_2(c):
        print('fraud detected ->{0}'.format(c.first.amount))
        print('fraud detected ->{0}'.format(c.second.amount))
        
    @when_start
    def start(host):    
        host.post('risk5', { 'debit': 220, 'credit': 100 })
        host.post('risk5', { 'debit': 150, 'credit': 100 })
        host.post('risk5', { 'amount': 200 })
        host.post('risk5', { 'amount': 500 })

# with ruleset('flow'):
#     @when_all(m.status == 'start')
#     def start(c):
#         c.post({ 'status': 'next' })
#         print('start')
#     # the process will always exit here every time the action is run
#     # when restarting the process this action will be retried after a few seconds
#     @when_all(m.status == 'next')
#     def next(c):
#         c.post({ 'status': 'last' })
#         print('next')
#         os._exit(1)

#     @when_all(m.status == 'last')
#     def last(c):
#         print('last')
        
#     @when_start
#     def on_start(host):
#         host.post('flow', { 'status': 'start' })

run_all()

