from durable.lang import *
import datetime

with ruleset('a0'):
    @when((m.amount > 1000) | (m.subject == 'approve') | (m.subject == 'ok'))
    def approved(s):
        print ('a0 approved ->{0}'.format(s.event))

    @when_start
    def start(host):
        host.post('a0', {'id': 1, 'sid': 1, 'subject': 'approve'})
        host.post('a0', {'id': 2, 'sid': 2, 'amount': 2000})
        host.post('a0', {'id': 3, 'sid': 3, 'subject': 'ok'})

with ruleset('a1'):
    @when((m.subject == 'approve') & (m.amount > 1000))
    def denied(s):
        print ('a1 denied from: {0}'.format(s.id))
        s.status = 'done'
    
    @when((m.subject == 'approve') & (m.amount <= 1000))
    def request(s):
        print ('a1 request approval from: {0}'.format(s.id))
        s.status = 'pending'

    @when_all(m.subject == 'approved', s.status == 'pending')
    def second_request(s):
        print ('a1 second request approval from: {0}'.format(s.id))
        s.status = 'approved'

    @when(s.status == 'approved')
    def approved(s):
        print ('a1 approved from: {0}'.format(s.id))
        s.status = 'done'

    @when(m.subject == 'denied')
    def denied(s):
        print ('a1 denied from: {0}'.format(s.id))
        s.status = 'done'

    @when_start
    def start(host):
        host.post('a1', {'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100})
        host.post('a1', {'id': 2, 'sid': 1, 'subject': 'approved'})
        host.post('a1', {'id': 3, 'sid': 2, 'subject': 'approve', 'amount': 100})
        host.post('a1', {'id': 4, 'sid': 2, 'subject': 'denied'})
        host.post('a1', {'id': 5, 'sid': 3, 'subject': 'approve', 'amount': 10000})


with statechart('a2'):
    with state('input'):
        @to('denied')
        @when((m.subject == 'approve') & (m.amount > 1000))
        def denied(s):
            print ('a2 denied from: {0}'.format(s.id))
        
        @to('pending')    
        @when((m.subject == 'approve') & (m.amount <= 1000))
        def request(s):
            print ('a2 request approval from: {0}'.format(s.id))
        
    with state('pending'):
        @to('pending')
        @when(m.subject == 'approved')
        def second_request(s):
            print ('a2 second request approval from: {0}'.format(s.id))
            s.status = 'approved'

        @to('approved')
        @when(s.status == 'approved')
        def approved(s):
            print ('a2 approved from: {0}'.format(s.id))
        
        @to('denied')
        @when(m.subject == 'denied')
        def denied(s):
            print ('a2 denied from: {0}'.format(s.id))
        
    state('denied')
    state('approved')
    @when_start
    def start(host):
        host.post('a2', {'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100})
        host.post('a2', {'id': 2, 'sid': 1, 'subject': 'approved'})
        host.post('a2', {'id': 3, 'sid': 2, 'subject': 'approve', 'amount': 100})
        host.post('a2', {'id': 4, 'sid': 2, 'subject': 'denied'})
        host.post('a2', {'id': 5, 'sid': 3, 'subject': 'approve', 'amount': 10000})


with flowchart('a3'):
    with stage('input'): 
        to('request').when((m.subject == 'approve') & (m.amount <= 1000))
        to('deny').when((m.subject == 'approve') & (m.amount > 1000))
    
    with stage('request'):
        @run
        def request(s):
            print ('a3 request approval from: {0}'.format(s.id))
            if s.status:
                s.status = 'approved'
            else:
                s.status = 'pending'

        to('approve').when(s.status == 'approved')
        to('deny').when(m.subject == 'denied')
        to('request').when_any(m.subject == 'approved', m.subject == 'ok')
    
    with stage('approve'):
        @run 
        def approved(s):
            print ('a3 approved from: {0}'.format(s.id))

    with stage('deny'):
        @run
        def denied(s):
            print ('a3 denied from: {0}'.format(s.id))

    @when_start
    def start(host):
        host.post('a3', {'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100})
        host.post('a3', {'id': 2, 'sid': 1, 'subject': 'approved'})
        host.post('a3', {'id': 3, 'sid': 2, 'subject': 'approve', 'amount': 100})
        host.post('a3', {'id': 4, 'sid': 2, 'subject': 'denied'})
        host.post('a3', {'id': 5, 'sid': 3, 'subject': 'approve', 'amount': 10000})


with ruleset('a4'):
    @when_any(all(m.subject == 'approve', m.amount == 1000), 
              all(m.subject == 'jumbo', m.amount == 10000))
    def action(s):
        print ('a4 action {0}'.format(s.id))
    
    @when_start
    def start(host):
        host.post('a4', {'id': 1, 'sid': 1, 'subject': 'approve'})
        host.post('a4', {'id': 2, 'sid': 1, 'amount': 1000})
        host.post('a4', {'id': 3, 'sid': 2, 'subject': 'jumbo'})
        host.post('a4', {'id': 4, 'sid': 2, 'amount': 10000})


with ruleset('a5'):
    @when_all(any(m.subject == 'approve', m.subject == 'jumbo'), 
              any(m.amount == 100, m.amount == 10000))
    def action(s):
        print ('a5 action {0}'.format(s.id))
    
    @when_start
    def start(host):
        host.post('a5', {'id': 1, 'sid': 1, 'subject': 'approve'})
        host.post('a5', {'id': 2, 'sid': 1, 'amount': 100})
        host.post('a5', {'id': 3, 'sid': 2, 'subject': 'jumbo'})
        host.post('a5', {'id': 4, 'sid': 2, 'amount': 10000})


with statechart('a6'):
    with state('work'):
        with state('enter'):
            @to('process')
            @when(m.subject == 'enter')
            def continue_process(s):
                print('a6 continue_process')
    
        with state('process'):
            @to('process')
            @when(m.subject == 'continue')
            def continue_process(s):
                print('a6 processing')

        @to('work')
        @when(m.subject == 'reset')
        def reset(s):
            print('a6 resetting')

        @to('canceled')
        @when(m.subject == 'cancel')
        def cancel(s):
            print('a6 canceling')

    state('canceled')
    @when_start
    def start(host):
        host.post('a6', {'id': 1, 'sid': 1, 'subject': 'enter'})
        host.post('a6', {'id': 2, 'sid': 1, 'subject': 'continue'})
        host.post('a6', {'id': 3, 'sid': 1, 'subject': 'continue'})
       
        
with ruleset('a7'):
    @when(m.amount < s.max_amount)
    def approved(s):
        print ('a7 approved')

    @when_start
    def start(host):
        host.patch_state('a7', {'id': 1, 'max_amount': 100})
        host.post('a7', {'id': 1, 'sid': 1, 'amount': 10})
        host.post('a7', {'id': 2, 'sid': 1, 'amount': 100})


with ruleset('a8'):
    @when((m.amount < s.max_amount) & (m.amount > s.id('global').min_amount))
    def approved(s):
        print ('a8 approved')

    @when_start
    def start(host):
        host.patch_state('a8', {'id': 1, 'max_amount': 500})
        host.patch_state('a8', {'id': 'global', 'min_amount': 300})
        host.post('a8', {'id': 1, 'sid': 1, 'amount': 400})
        host.post('a8', {'id': 2, 'sid': 1, 'amount': 1600})


with ruleset('a9'):
    @when((m.subject == 'approve') & (m.amount == 100), atLeast = 5, atMost = 10)
    def approved(s):
        print ('a9 approved ->{0}'.format(s.event))

    @when_start
    def start(host):
        host.post_batch('a9', [{'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100},
                               {'id': 2, 'sid': 1, 'subject': 'approve', 'amount': 100},
                               {'id': 3, 'sid': 1, 'subject': 'approve', 'amount': 100},
                               {'id': 4, 'sid': 1, 'subject': 'approve', 'amount': 100},
                               {'id': 5, 'sid': 1, 'subject': 'approve', 'amount': 100},
                               {'id': 6, 'sid': 1, 'subject': 'approve', 'amount': 100}])


with ruleset('a10'):
    @when_all(m.subject == 'approve', m.subject == 'approved', atLeast = 2, atMost = 4)
    def approved(s):
        print ('a10 approved ->{0}'.format(s.event))

    @when_start
    def start(host):
        host.post_batch('a10', [{'id': 1, 'sid': 1, 'subject': 'approve'},
                                {'id': 2, 'sid': 1, 'subject': 'approve'},
                                {'id': 3, 'sid': 1, 'subject': 'approve'}])
        host.post_batch('a10', [{'id': 7, 'sid': 1, 'subject': 'approved'},
                                {'id': 8, 'sid': 1, 'subject': 'approved'},
                                {'id': 9, 'sid': 1, 'subject': 'approved'}])


with ruleset('a11'):
    @when_any(m.subject == 'approve', m.subject == 'approved', atLeast = 3)
    def approved(s):
        print ('a11 approved ->{0}'.format(s.event))

    @when_start
    def start(host):
        host.post_batch('a11', [{'id': 1, 'sid': 1, 'subject': 'approve'},
                                {'id': 2, 'sid': 1, 'subject': 'approve'},
                                {'id': 3, 'sid': 1, 'subject': 'approve'},
                                {'id': 4, 'sid': 1, 'subject': 'approve'}])


with ruleset('a12'):
    @when_any(m.subject == 'approve', exp(m.subject == 'please', atLeast = 3))
    def approved(s):
        print ('a12 approved ->{0}'.format(s.event))

    @when_start
    def start(host):
        host.post('a12', {'id': 1, 'sid': 1, 'subject': 'approve'})
        host.post_batch('a12', [{'id': 2, 'sid': 2, 'subject': 'please'},
                                {'id': 3, 'sid': 2, 'subject': 'please'},
                                {'id': 4, 'sid': 2, 'subject': 'please'}])


with ruleset('a13'):
    @when_all(exp(m.subject == 'approve', atLeast = 2), m.subject == 'please')
    def approved(s):
        print ('a13 approved ->{0}'.format(s.event))

    @when_start
    def start(host):
        host.post('a13', {'id': 1, 'sid': 1, 'subject': 'please'})
        host.post_batch('a13', [{'id': 2, 'sid': 1, 'subject': 'approve'},
                                {'id': 3, 'sid': 1, 'subject': 'approve'}])
                                

with ruleset('p1'):
    with when(m.start == 'yes'): 
        with ruleset('one'):
            @when(-s.start)
            def continue_flow(s):
                s.start = 1

            @when(s.start == 1)
            def finish_one(s):
                print('p1 finish one {0}'.format(s.id))
                s.signal({'id': 1, 'end': 'one'})
                s.start = 2

        with ruleset('two'): 
            @when(-s.start)
            def continue_flow(s):
                s.start = 1

            @when(s.start == 1)
            def finish_two(s):
                print('p1 finish two {0}'.format(s.id))
                s.signal({'id': 1, 'end': 'two'})
                s.start = 2

    @when_all(m.end == 'one', m.end == 'two')
    def done(s):
        print('p1 done {0}'.format(s.id))

    @when_start
    def start(host):
        host.post('p1', {'id': 1, 'sid': 1, 'start': 'yes'})


with statechart('p2'):
    with state('input'):
        @to('process')
        @when(m.subject == 'approve')
        def get_input(s):
            print('p2 input {0} from: {1}'.format(s.event['quantity'], s.id))
            s.quantity = s.event['quantity']

    with state('process'):
        with to('result').when(+s.quantity):
            with statechart('first'):
                with state('evaluate'):
                    @to('end')
                    @when(s.quantity <= 5)
                    def signal_approved(s):
                        print('p2 signaling approved from: {0}'.format(s.id))
                        s.signal({'id': 1, 'subject': 'approved'})

                state('end')
        
            with statechart('second'):
                with state('evaluate'):
                    @to('end')
                    @when(s.quantity > 5)
                    def signal_denied(s):
                        print('p2 signaling denied from: {0}'.format(s.id))
                        s.signal({'id': 1, 'subject': 'denied'})
                
                state('end')
    
    with state('result'):
        @to('approved')
        @when(m.subject == 'approved')
        def report_approved(s):
            print('p2 approved from: {0}'.format(s.id))

        @to('denied')
        @when(m.subject == 'denied')
        def report_denied(s):
            print('p2 denied from: {0}'.format(s.id)) 
    
    state('denied')
    state('approved')

    @when_start
    def start(host):
        host.post('p2', {'id': 1, 'sid': 1, 'subject': 'approve', 'quantity': 3})
        host.post('p2', {'id': 2, 'sid': 2, 'subject': 'approve', 'quantity': 10})

with flowchart('p3'):
    with stage('start'):
        to('input').when(m.subject == 'approve')
    
    with stage('input'):
        @run
        def get_input(s):
            print('p3 input {0} from: {1}'.format(s.event['quantity'], s.id))
            s.quantity = s.event['quantity']

        to('process')
    
    with stage('process'):
        with flowchart('first'):
            with stage('start'):
                to('end').when(s.quantity <= 5)
            
            with stage('end'):
                @run
                def signal_approved(s):
                    print('p3 signaling approved from: {0}'.format(s.id))
                    s.signal({'id': 1, 'subject': 'approved'})

        with flowchart('second'):
            with stage('start'):
                to('end').when(s.quantity > 5)

            with stage('end'):
                @run
                def signal_denied(s):
                    print('p3 signaling denied from: {0}'.format(s.id))
                    s.signal({'id': 1, 'subject': 'denied'})

        to('approve').when(m.subject == 'approved')
        to('deny').when(m.subject == 'denied')

    with stage('approve'):
        @run
        def report_approved(s):
            print('p3 approved from: {0}'.format(s.id))

    with stage('deny'):
        @run
        def report_denied(s):
            print('p3 denied from: {0}'.format(s.id)) 

    @when_start
    def start(host):
        host.post('p3', {'id': 1, 'sid': 1, 'subject': 'approve', 'quantity': 3})
        host.post('p3', {'id': 2, 'sid': 2, 'subject': 'approve', 'quantity': 10})

with ruleset('t1'): 
    @when(m.start == 'yes')
    def start_timer(s):
        s.start = datetime.datetime.now().strftime('%I:%M:%S%p')
        s.start_timer('my_timer', 5)

    @when(timeout('my_timer'))
    def end_timer(s):
        print('t1 started @%s' % s.start)
        print('t1 ended @%s' % datetime.datetime.now().strftime('%I:%M:%S%p'))

    @when_start
    def start(host):
        host.post('t1', {'id': 1, 'sid': 1, 'start': 'yes'})


with statechart('t2'):
    with state('input'):
        with to('pending').when(m.subject == 'approve'):
            with statechart('first'):
                with state('send'):
                    @to('evaluate')
                    def start_timer(s):
                        s.start = datetime.datetime.now().strftime('%I:%M:%S%p')
                        s.start_timer('first', 4)   

                with state('evaluate'):
                    @to('end')
                    @when(timeout('first'))
                    def signal_approved(s):
                        s.signal({'id': 2, 'subject': 'approved', 'start': s.start})

                state('end')

            with statechart('second'):
                with state('send'):
                    @to('evaluate')
                    def start_timer(s):
                        s.start = datetime.datetime.now().strftime('%I:%M:%S%p')
                        s.start_timer('second', 3)

                with state('evaluate'):
                    @to('end')
                    @when(timeout('second'))
                    def signal_denied(s):
                        s.signal({'id': 3, 'subject': 'denied', 'start': s.start})

                state('end')

    with state('pending'):
        @to('approved')
        @when(m.subject == 'approved')
        def report_approved(s):
            print('t2 approved {0}'.format(s.id))
            print('t2 started @%s' % s.event['start'])
            print('t2 ended @%s' % datetime.datetime.now().strftime('%I:%M:%S%p'))

        @to('denied')
        @when(m.subject == 'denied')
        def report_denied(s):
            print('t2 denied {0}'.format(s.id))
            print('t2 started @%s' % s.event['start'])
            print('t2 ended @%s' % datetime.datetime.now().strftime('%I:%M:%S%p'))

    state('approved')
    state('denied')
    
    @when_start
    def start(host):
        host.post('t2', {'id': 1, 'sid': 1, 'subject': 'approve'})

run_all()
