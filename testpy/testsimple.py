from durable.lang import *
import datetime

with statechart('fraud0'):
    with state('start'):
        to('standby')

    with state('standby'):
        @to('metering')
        @when_all(m.amount > 100)
        def start_metering(c):
            c.start_timer('velocity', 30)

    with state('metering'):
        @to('fraud')
        @when_all(m.amount > 100, count(2))
        def report_fraud(c):
            print('fraud0 detected')

        @to('standby')
        @when_all(timeout('velocity'))
        def clear_fraud(c):
            print('fraud0 cleared')

    state('fraud')

    @when_start
    def start(host):
        host.post('fraud0', {'id': 1, 'sid': 1, 'amount': 200})
        host.post('fraud0', {'id': 2, 'sid': 1, 'amount': 200})
        host.post('fraud0', {'id': 3, 'sid': 1, 'amount': 200})


with ruleset('fraud1'):
    @when_all(c.first << m.t == 'purchase',
              c.second << m.location != c.first.location)
    def detected(c):
        print ('fraud1 detected {0}, {1}'.format(c.first.location, c.second.location))

    @when_start
    def start(host):
        host.post('fraud1', {'id': 1, 'sid': 1, 't': 'purchase', 'location': 'US'})
        host.post('fraud1', {'id': 2, 'sid': 1, 't': 'purchase', 'location': 'CA'})


with ruleset('fraud2'):
    @when_all(c.first << m.amount > 10,
              c.second << m.amount > c.first.amount * 2)
    def detected(c):
        print('fraud2 detected {0}, {1}'.format(c.first.amount, c.second.amount))

    @when_start
    def start(host):
        host.post('fraud2', {'id': 1, 'sid': 1, 'amount': 50})
        host.post('fraud2', {'id': 2, 'sid': 1, 'amount': 150})


with ruleset('fraud3'):
    @when_all(c.first << m.amount > 100,
              c.second << m.amount > c.first.amount,
              c.third << m.amount > c.second.amount,
              c.fourth << m.amount > (c.first.amount + c.second.amount + c.third.amount) / 3)
    def detected(c):
        print('fraud3 detected -> {0}'.format(c.first.amount))
        print('                -> {0}'.format(c.second.amount))
        print('                -> {0}'.format(c.third.amount))
        print('                -> {0}'.format(c.fourth.amount))

    @when_start
    def start(host):
        host.post('fraud3', {'id': 1, 'sid': 1, 'amount': 101})
        host.post('fraud3', {'id': 2, 'sid': 1, 'amount': 102})
        host.post('fraud3', {'id': 3, 'sid': 1, 'amount': 103})
        host.post('fraud3', {'id': 4, 'sid': 1, 'amount': 104})


with ruleset('fraud4'):
    @when_all(pri(3), m.amount < 300)
    def first_detect(c):
        print('fraud4 first detect {0}'.format(c.m.amount))

    @when_all(pri(2), m.amount < 200)
    def second_detect(c):
        print('fraud4 second detect {0}'.format(c.m.amount))

    @when_all(pri(1), m.amount < 100)
    def third_detect(c):
        print('fraud4 third detect {0}'.format(c.m.amount))

    @when_start
    def start(host):
        host.post('fraud4', {'id': 1, 'sid': 1, 'amount': 50})
        host.post('fraud4', {'id': 2, 'sid': 1, 'amount': 150})
        host.post('fraud4', {'id': 3, 'sid': 1, 'amount': 250})


with ruleset('fraud5'):
    @when_all(c.first << m.t == 'purchase',
              c.second << m.location != c.first.location,
              count(2))
    def detected(c):
        print ('fraud5 detected {0}, {1}'.format(c.m[0].first.location, c.m[0].second.location))
        print ('               {0}, {1}'.format(c.m[1].first.location, c.m[1].second.location))

    @when_start
    def start(host):
        host.assert_fact('fraud5', {'id': 1, 'sid': 1, 't': 'purchase', 'location': 'US'})
        host.assert_fact('fraud5', {'id': 2, 'sid': 1, 't': 'purchase', 'location': 'CA'})


with ruleset('fraud6'):
    @when_all(pri(3), count(3), m.amount < 300)
    def first_detect(c):
        print('fraud6 first detect ->{0}'.format(c.m[0].amount))
        print('                    ->{0}'.format(c.m[1].amount))
        print('                    ->{0}'.format(c.m[2].amount))

    @when_all(pri(2), count(2), m.amount < 200)
    def second_detect(c):
        print('fraud6 second detect ->{0}'.format(c.m[0].amount))
        print('                     ->{0}'.format(c.m[1].amount))

    @when_all(pri(1), m.amount < 100)
    def third_detect(c):
        print('fraud6 third detect ->{0}'.format(c.m.amount))
        
    @when_start
    def start(host):
        host.assert_fact('fraud6', {'id': 1, 'sid': 1, 'amount': 50})
        host.assert_fact('fraud6', {'id': 2, 'sid': 1, 'amount': 150})
        host.assert_fact('fraud6', {'id': 3, 'sid': 1, 'amount': 250})


with ruleset('fraud7'):
    @when_all(c.first << m.t == 'deposit',
              none(m.t == 'balance'),
              c.third << m.t == 'withrawal',
              c.fourth << m.t == 'chargeback')
    def detected(c):
        print('fraud7 detected {0}, {1}, {2}'.format(c.first.t, c.third.t, c.fourth.t))

    @when_start
    def start(host):
        host.post('fraud7', {'id': 1, 'sid': 1, 't': 'deposit'})
        host.post('fraud7', {'id': 2, 'sid': 1, 't': 'withrawal'})
        host.post('fraud7', {'id': 3, 'sid': 1, 't': 'chargeback'})
        host.assert_fact('fraud7', {'id': 4, 'sid': 1, 't': 'balance'})
        host.post('fraud7', {'id': 5, 'sid': 1, 't': 'deposit'})
        host.post('fraud7', {'id': 6, 'sid': 1, 't': 'withrawal'})
        host.post('fraud7', {'id': 7, 'sid': 1, 't': 'chargeback'})
        host.retract_fact('fraud7', {'id': 4, 'sid': 1, 't': 'balance'})


with ruleset('a0'):
    @when_all((m.subject == 'go') | (m.subject == 'approve') | (m.subject == 'ok'))
    def approved(c):
        print ('a0 approved ->{0}'.format(c.m.subject))

    @when_start
    def start(host):
        host.post('a0', {'id': 1, 'sid': 1, 'subject': 'approve'})
        host.post('a0', {'id': 2, 'sid': 2, 'subject': 'go'})
        host.post('a0', {'id': 3, 'sid': 3, 'subject': 'ok'})


with ruleset('a1'):
    @when_all((m.subject == 'approve') & (m.amount > 1000))
    def denied(c):
        print ('a1 denied from: {0}'.format(c.s.sid))
        c.s.status = 'done'
    
    @when_all((m.subject == 'approve') & (m.amount <= 1000))
    def request(c):
        print ('a1 request approval from: {0}'.format(c.s.sid))
        c.s.status = 'pending'

    @when_all(m.subject == 'approved', s.status == 'pending')
    def second_request(c):
        print ('a1 second request approval from: {0}'.format(c.s.sid))
        c.s.status = 'approved'

    @when_all(s.status == 'approved')
    def approved(c):
        print ('a1 approved from: {0}'.format(c.s.sid))
        c.s.status = 'done'

    @when_all(m.subject == 'denied')
    def denied(c):
        print ('a1 denied from: {0}'.format(c.s.sid))
        c.s.status = 'done'

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
    @when_start
    def start(host):
        host.post('a2', {'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100})
        host.post('a2', {'id': 2, 'sid': 1, 'subject': 'approved'})
        host.post('a2', {'id': 3, 'sid': 2, 'subject': 'approve', 'amount': 100})
        host.post('a2', {'id': 4, 'sid': 2, 'subject': 'denied'})
        host.post('a2', {'id': 5, 'sid': 3, 'subject': 'approve', 'amount': 10000})


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
    def action(c):
        print ('a4 action {0}'.format(c.s.sid))
    
    @when_start
    def start(host):
        host.post('a4', {'id': 1, 'sid': 1, 'subject': 'approve'})
        host.post('a4', {'id': 2, 'sid': 1, 'amount': 1000})
        host.post('a4', {'id': 3, 'sid': 2, 'subject': 'jumbo'})
        host.post('a4', {'id': 4, 'sid': 2, 'amount': 10000})


with ruleset('a5'):
    @when_all(any(m.subject == 'approve', m.subject == 'jumbo'), 
              any(m.amount == 100, m.amount == 10000))
    def action(c):
        print ('a5 action {0}'.format(c.s.sid))
    
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
       

with ruleset('a7'):
    @when_all(m.amount < c.s.max_amount)
    def approved(c):
        print ('a7 approved {0}'.format(c.m.amount))

    @when_start
    def start(host):
        host.patch_state('a7', {'sid': 1, 'max_amount': 100})
        host.post('a7', {'id': 1, 'sid': 1, 'amount': 10})
        host.post('a7', {'id': 2, 'sid': 1, 'amount': 100})


with ruleset('a8'):
    @when_all((m.amount < c.s.max_amount) & (m.amount > c.s.id('global').min_amount))
    def approved(c):
        print ('a8 approved {0}'.format(c.m.amount))

    @when_start
    def start(host):
        host.patch_state('a8', {'sid': 1, 'max_amount': 500})
        host.patch_state('a8', {'sid': 'global', 'min_amount': 300})
        host.post('a8', {'id': 1, 'sid': 1, 'amount': 400})
        host.post('a8', {'id': 2, 'sid': 1, 'amount': 1600})


with ruleset('a9'):
    @when_all(m.amount < c.s.max_amount * 2)
    def approved(c):
        print ('a9 approved {0}'.format(c.m.amount))

    @when_start
    def start(host):
        host.patch_state('a9', {'sid': 1, 'max_amount': 100})
        host.post('a9', {'id': 1, 'sid': 1, 'amount': 100})
        host.post('a9', {'id': 2, 'sid': 1, 'amount': 300})


with ruleset('a10'):
    @when_all(m.amount < c.s.max_amount + c.s.id('global').max_amount)
    def approved(c):
        print ('a10 approved {0}'.format(c.m.amount))

    @when_start
    def start(host):
        host.patch_state('a10', {'sid': 1, 'max_amount': 500})
        host.patch_state('a10', {'sid': 'global', 'max_amount': 300})
        host.post('a10', {'id': 1, 'sid': 1, 'amount': 400})
        host.post('a10', {'id': 2, 'sid': 1, 'amount': 900})


with ruleset('a11'):
    @when_all((m.subject == 'approve') & (m.amount == 100), count(3))
    def approved(c):
        print ('a11 approved ->{0}'.format(c.m[0].amount))
        print ('             ->{0}'.format(c.m[1].amount))
        print ('             ->{0}'.format(c.m[2].amount))

    @when_start
    def start(host):
        host.post_batch('a11', [{'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100},
                               {'id': 2, 'sid': 1, 'subject': 'approve', 'amount': 100},
                               {'id': 3, 'sid': 1, 'subject': 'approve', 'amount': 100},
                               {'id': 4, 'sid': 1, 'subject': 'approve', 'amount': 100},
                               {'id': 5, 'sid': 1, 'subject': 'approve', 'amount': 100},
                               {'id': 6, 'sid': 1, 'subject': 'approve', 'amount': 100}])


with ruleset('a12'):
    @when_any(m.subject == 'approve', m.subject == 'approved', count(2))
    def approved(c):
        print ('a12 approved ->{0}'.format(c.m[0].m_0.subject))
        print ('             ->{0}'.format(c.m[1].m_0.subject))
    
    @when_start
    def start(host):
        host.post_batch('a12', [{'id': 1, 'sid': 1, 'subject': 'approve'},
                                {'id': 2, 'sid': 1, 'subject': 'approve'},
                                {'id': 3, 'sid': 1, 'subject': 'approve'},
                                {'id': 4, 'sid': 1, 'subject': 'approve'}])


with ruleset('p1'):
    with when_all(m.start == 'yes'): 
        with ruleset('one'):
            @when_all(-s.start)
            def continue_flow(c):
                c.s.start = 1

            @when_all(s.start == 1)
            def finish_one(c):
                print('p1 finish one {0}'.format(c.s.sid))
                c.signal({'id': 1, 'end': 'one'})
                c.s.start = 2

        with ruleset('two'): 
            @when_all(-s.start)
            def continue_flow(c):
                c.s.start = 1

            @when_all(s.start == 1)
            def finish_two(c):
                print('p1 finish two {0}'.format(c.s.sid))
                c.signal({'id': 1, 'end': 'two'})
                c.s.start = 2

    @when_all(m.end == 'one', m.end == 'two')
    def done(c):
        print('p1 done {0}'.format(c.s.sid))

    @when_start
    def start(host):
        host.post('p1', {'id': 1, 'sid': 1, 'start': 'yes'})


with statechart('p2'):
    with state('input'):
        @to('process')
        @when_all(m.subject == 'approve')
        def get_input(c):
            print('p2 input {0} from: {1}'.format(c.m.quantity, c.s.sid))
            c.s.quantity = c.m.quantity

    with state('process'):
        with to('result').when_all(+s.quantity):
            with statechart('first'):
                with state('evaluate'):
                    @to('end')
                    @when_all(s.quantity <= 5)
                    def signal_approved(c):
                        print('p2 signaling approved from: {0}'.format(c.s.sid))
                        c.signal({'id': 1, 'subject': 'approved'})

                state('end')
        
            with statechart('second'):
                with state('evaluate'):
                    @to('end')
                    @when_all(s.quantity > 5)
                    def signal_denied(c):
                        print('p2 signaling denied from: {0}'.format(c.s.sid))
                        c.signal({'id': 1, 'subject': 'denied'})
                
                state('end')
    
    with state('result'):
        @to('approved')
        @when_all(m.subject == 'approved')
        def report_approved(c):
            print('p2 approved from: {0}'.format(c.s.sid))

        @to('denied')
        @when_all(m.subject == 'denied')
        def report_denied(c):
            print('p2 denied from: {0}'.format(c.s.sid)) 
    
    state('denied')
    state('approved')

    @when_start
    def start(host):
        host.post('p2', {'id': 1, 'sid': 1, 'subject': 'approve', 'quantity': 3})
        host.post('p2', {'id': 2, 'sid': 2, 'subject': 'approve', 'quantity': 10})


with flowchart('p3'):
    with stage('start'):
        to('input').when_all(m.subject == 'approve')
    
    with stage('input'):
        @run
        def get_input(c):
            print('p3 input {0} from: {1}'.format(c.m.quantity, c.s.sid))
            c.s.quantity = c.m.quantity

        to('process')
    
    with stage('process'):
        with flowchart('first'):
            with stage('start'):
                to('end').when_all(s.quantity <= 5)
            
            with stage('end'):
                @run
                def signal_approved(c):
                    print('p3 signaling approved from: {0}'.format(c.s.sid))
                    c.signal({'id': 1, 'subject': 'approved'})

        with flowchart('second'):
            with stage('start'):
                to('end').when_all(s.quantity > 5)

            with stage('end'):
                @run
                def signal_denied(c):
                    print('p3 signaling denied from: {0}'.format(c.s.sid))
                    c.signal({'id': 1, 'subject': 'denied'})

        to('approve').when_all(m.subject == 'approved')
        to('deny').when_all(m.subject == 'denied')

    with stage('approve'):
        @run
        def report_approved(c):
            print('p3 approved from: {0}'.format(c.s.sid))

    with stage('deny'):
        @run
        def report_denied(c):
            print('p3 denied from: {0}'.format(c.s.sid)) 

    @when_start
    def start(host):
        host.post('p3', {'id': 1, 'sid': 1, 'subject': 'approve', 'quantity': 3})
        host.post('p3', {'id': 2, 'sid': 2, 'subject': 'approve', 'quantity': 10})


with ruleset('t1'): 
    @when_all(m.start == 'yes')
    def start_timer(c):
        c.s.start = datetime.datetime.now().strftime('%I:%M:%S%p')
        c.start_timer('my_timer', 5)

    @when_all(timeout('my_timer'))
    def end_timer(c):
        print('t1 started @%s' % c.s.start)
        print('t1 ended @%s' % datetime.datetime.now().strftime('%I:%M:%S%p'))

    @when_start
    def start(host):
        host.post('t1', {'id': 1, 'sid': 1, 'start': 'yes'})


with statechart('t2'):
    with state('input'):
        with to('pending').when_all(m.subject == 'approve'):
            with statechart('first'):
                with state('send'):
                    @to('evaluate')
                    def start_timer(c):
                        c.s.start = datetime.datetime.now().strftime('%I:%M:%S%p')
                        c.start_timer('first', 4)   

                with state('evaluate'):
                    @to('end')
                    @when_all(timeout('first'))
                    def signal_approved(c):
                        c.signal({'id': 2, 'subject': 'approved', 'start': c.s.start})

                state('end')

            with statechart('second'):
                with state('send'):
                    @to('evaluate')
                    def start_timer(c):
                        c.s.start = datetime.datetime.now().strftime('%I:%M:%S%p')
                        c.start_timer('second', 3)

                with state('evaluate'):
                    @to('end')
                    @when_all(timeout('second'))
                    def signal_denied(c):
                        c.signal({'id': 3, 'subject': 'denied', 'start': c.s.start})

                state('end')

    with state('pending'):
        @to('approved')
        @when_all(m.subject == 'approved')
        def report_approved(c):
            print('t2 approved {0}'.format(c.s.sid))
            print('t2 started @%s' % c.m.start)
            print('t2 ended @%s' % datetime.datetime.now().strftime('%I:%M:%S%p'))

        @to('denied')
        @when_all(m.subject == 'denied')
        def report_denied(c):
            print('t2 denied {0}'.format(c.s.sid))
            print('t2 started @%s' % c.m.start)
            print('t2 ended @%s' % datetime.datetime.now().strftime('%I:%M:%S%p'))

    state('approved')
    state('denied')
    
    @when_start
    def start(host):
        host.post('t2', {'id': 1, 'sid': 1, 'subject': 'approve'})

run_all()
