from durable.lang import *
import datetime
import random
import sys
import threading
import time

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
        print('fraud7 detected {0}, {1}, {2} from {3}'.format(c.first.t, c.third.t, c.fourth.t, c.s.sid))

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

q = create_queue('fraud7')
q.post({'id': 1, 'sid': 2, 't': 'deposit'})
q.post({'id': 2, 'sid': 2, 't': 'withrawal'})
q.post({'id': 3, 'sid': 2, 't': 'chargeback'})
q.assert_fact({'id': 4, 'sid': 2, 't': 'balance'})
q.post({'id': 5, 'sid': 2, 't': 'deposit'})
q.post({'id': 6, 'sid': 2, 't': 'withrawal'})
q.post({'id': 7, 'sid': 2, 't': 'chargeback'})
q.retract_fact({'id': 4, 'sid': 2, 't': 'balance'})
q.close()

t = None
with statechart('fraud8'):
    with state('first'):
        @to('second')
        @when_all(m.amount > 100)
        def start_first(c, complete):
            def execute_scripts():
                print('fraud8 execute 1')
                complete(None)
               
            print('fraud8 start async 1') 
            t = threading.Timer(1, execute_scripts)
            t.start()

    with state('second'):
        @to('fraud')
        def start_second(c, complete):
            def execute_scripts():
                print('fraud8 execute 2')
                complete(None)
               
            print('fraud8 start async 2') 
            t = threading.Timer(1, execute_scripts)
            t.start()

        
    state('fraud')

    @when_start
    def start(host):
        host.post('fraud8', {'id': 1, 'sid': 1, 'amount': 200})


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
        to('request').when_all(m.subject == 'approved')
    
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
        host.post('a6', {'id': 4, 'sid': 1, 'subject': 'cancel'})
       

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


with ruleset('t0'):
    @when_all(timeout('my_timer') | (s.count == 0))
    def start_timer(c):
        c.s.count += 1
        c.post('t0', {'id': c.s.count, 'sid': 1, 't': 'purchase'})
        c.start_timer('my_timer', random.randint(1, 3), 't_{0}'.format(c.s.count))

    @when_all(span(5), m.t == 'purchase')
    def pulse(c):
        print('t0 pulse -> {0}'.format(len(c.m)))

    @when_start
    def start(host):
        host.patch_state('t0', {'sid': 1, 'count': 0})


with ruleset('t1'): 
    @when_all(m.start == 'yes')
    def start_timer(c):
        c.s.start = datetime.datetime.now().strftime('%I:%M:%S%p')
        c.start_timer('my_first_timer', 3)
        c.start_timer('my_second_timer', 6)

    @when_all(timeout('my_first_timer'))
    def end_first_timer(c):
        print('t1 started @%s' % c.s.start)
        print('t1 first_timer ended @%s' % datetime.datetime.now().strftime('%I:%M:%S%p'))
        c.cancel_timer('my_second_timer')

    @when_all(timeout('my_second_timer'))
    def end_first_timer(c):
        print('t1 started @%s' % c.s.start)
        print('t1 second_timer ended @%s' % datetime.datetime.now().strftime('%I:%M:%S%p'))

    @when_start
    def start(host):
        host.post('t1', {'id': 1, 'sid': 1, 'start': 'yes'})


with ruleset('t2'): 
    @when_all(m.start == 'yes')
    def first_time(c):
        print('t2 first started')
        time.sleep(7)
        print('renewing lease')
        c.renew_action_lease()
        time.sleep(7)
        c.post({'id': 2, 'end': 'yes'})

    @when_all(m.end == 'yes')
    def second_time(c):
        print('t2 second completed')

    @when_start
    def start(host):
        host.post('t2', {'id': 1, 'sid': 1, 'start': 'yes'})


t3 = None
with ruleset('t3'):
    @when_all(m.start == 'yes')
    def first_time3(c, complete):
        def first_timeout3():
            print('t3 first completed')
            c.post({'id': 2, 'end': 'yes'})
            complete(None)

        print('t3 first started {0}'.format(c.m.id))
        t3 = threading.Timer(5, first_timeout3)
        t3.daemon = True      
        t3.start()
        return 10
        
    @when_all(m.end == 'yes')
    def second_time3(c, complete):
        def second_timeout3():
            print('t3 second completed')
            c.post({'id': 3, 'end': 'yes'})
            complete(None)

        print('t3 second started')
        t3 = threading.Timer(7, second_timeout3) 
        t3.daemon = True     
        t3.start()
        return 5
        
    @when_all(+s.exception)
    def timeout(c):
        print('t3 expected exception: {0}'.format(c.s.exception))
        c.s.exception = None

    @when_start
    def start(host):
        host.post('t3', {'id': 1, 'sid': 1, 'start': 'yes'})


with ruleset('q0'): 
    @when_all(m.start == 'yes')
    def start_queue(c):
        print('q0 started')
        c.get_queue('q0').post({'id': 2, 'sid': 1, 'end': 'yes'})

    @when_all(m.end == 'yes')
    def end_timer(c):
        print('q0 ended')

    @when_start
    def start(host):
        host.post('q0', {'id': 1, 'sid': 1, 'start': 'yes'})


run_all()
