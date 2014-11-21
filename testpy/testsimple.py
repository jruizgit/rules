from durable.lang import *

def denied(s):
    print ('denied from: {0}, {1}'.format(s.ruleset_name, s.id))
    s.status = 'done'

def approved(s):
    print ('approved from: {0}, {1}'.format(s.ruleset_name, s.id))
    s.status = 'done'

def request_approval(s):
    print ('request approval from: {0}, {1}'.format(s.ruleset_name, s.id))
    print (s.event)
    if s.status:
        s.status = 'approved'
    else:
        s.status = 'pending'

def finish_one(s):
    print('finish one {0}, {1}'.format(s.ruleset_name, s.id))
    s.signal({'id': 1, 'end': 'one'})
    s.start = 2

def finish_two(s):
    print('finish two {0}, {1}'.format(s.ruleset_name, s.id))
    s.signal({'id': 1, 'end': 'two'})
    s.start = 2

def continue_flow(s):
    s.start = 1

def done(s):
    print('done {0}, {1}'.format(s.ruleset_name, s.id))

def get_input(s):
    print('input {0} from: {1}, {2}'.format(s.event['quantity'], s.ruleset_name, s.id))
    s.state['quantity'] = s.event['quantity']

def signal_approved(s):
    print('signaling approved from: {0}, {1}'.format(s.ruleset_name, s.id))
    s.signal({'id': 1, 'subject': 'approved'})   

def signal_denied(s):
    print('signaling denied from: {0}, {1}'.format(s.ruleset_name, s.id))
    s.signal({'id': 1, 'subject': 'denied'})

def report_approved(s):
    print('approved from: {0}, {1}'.format(s.ruleset_name, s.id))

def report_denied(s):
    print('denied from: {0}, {1}'.format(s.ruleset_name, s.id))   

def start_a1(host):
    host.post('a1', {'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100})
    host.post('a1', {'id': 2, 'sid': 1, 'subject': 'approved'})
    host.post('a1', {'id': 3, 'sid': 2, 'subject': 'approve', 'amount': 100})
    host.post('a1', {'id': 4, 'sid': 2, 'subject': 'denied'})
    host.post('a1', {'id': 5, 'sid': 3, 'subject': 'approve', 'amount': 10000})

def start_a2(host):
    host.post('a2', {'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100})
    host.post('a2', {'id': 2, 'sid': 1, 'subject': 'approved'})
    host.post('a2', {'id': 3, 'sid': 2, 'subject': 'approve', 'amount': 100})
    host.post('a2', {'id': 4, 'sid': 2, 'subject': 'denied'})
    host.post('a2', {'id': 5, 'sid': 3, 'subject': 'approve', 'amount': 10000})
    
def start_a3(host):
    host.post('a3', {'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100})
    host.post('a3', {'id': 2, 'sid': 1, 'subject': 'approved'})
    host.post('a3', {'id': 3, 'sid': 2, 'subject': 'approve', 'amount': 100})
    host.post('a3', {'id': 4, 'sid': 2, 'subject': 'denied'})
    host.post('a3', {'id': 5, 'sid': 3, 'subject': 'approve', 'amount': 10000})

def start_p1(host):
    host.post('p1', {'id': 1, 'sid': 1, 'start': 'yes'})

def start_p2(host):
    host.post('p2', {'id': 1, 'sid': 1, 'subject': 'approve', 'quantity': 3})
    host.post('p2', {'id': 2, 'sid': 2, 'subject': 'approve', 'quantity': 10})

def start_p3(host):
    host.post('p3', {'id': 1, 'sid': 1, 'subject': 'approve', 'quantity': 3})
    host.post('p3', {'id': 2, 'sid': 2, 'subject': 'approve', 'quantity': 10})

with ruleset('a1'):
    when((m.subject == 'approve') & (m.amount > 1000), denied)
    when((m.subject == 'approve') & (m.amount <= 1000), request_approval)
    when_all(m.subject == 'approved', s.status == 'pending', request_approval)
    when(s.status == 'approved', approved)
    when(m.subject == 'denied', denied)
    when_start(start_a1)

with statechart('a2'):
    with state('input'):
        to('denied').when((m.subject == 'approve') & (m.amount > 1000), denied)
        to('pending').when((m.subject == 'approve') & (m.amount <= 1000), request_approval)

    with state('pending'):
        to('pending').when(m.subject == 'approved', request_approval)
        to('approved').when(s.status == 'approved', approved)
        to('denied').when(m.subject == 'denied', denied)

    state('denied')
    state('approved')
    when_start(start_a2)

with flowchart('a3'):
    with stage('input'): 
        to('request').when((m.subject == 'approve') & (m.amount <= 1000))
        to('deny').when((m.subject == 'approve') & (m.amount > 1000))
    
    with stage('request', request_approval):
        to('approve').when(s.status == 'approved')
        to('deny').when(m.subject == 'denied')
        to('request').when_any(m.subject == 'approved', m.subject == 'ok')
    
    stage('approve', approved)
    stage('deny', denied)
    when_start(start_a3)

with ruleset('p1'):
    with when(m.start == 'yes'): 
        with ruleset('one'):
            when(-s.start, continue_flow)
            when(s.start == 1, finish_one)

        with ruleset('two'): 
            when(-s.start, continue_flow)
            when(s.start == 1, finish_two)

    when_all(m.end == 'one', m.end == 'two', done)
    when_start(start_p1)


with statechart('p2'):
    with state('input'):
        to('process').when(m.subject == 'approve', get_input)
    
    with state('process'):
        with to('result').when(+s.quantity):
            with statechart('first'):
                with state('evaluate'):
                    to('end').when(s.quantity <= 5, signal_approved)
                state('end')
        
            with statechart('second'):
                with state('evaluate'):
                    to('end').when(s.quantity > 5, signal_denied)
                state('end')
    
    with state('result'):
        to('approved').when(m.subject == 'approved', report_approved)
        to('denied').when(m.subject == 'denied', report_denied)
    
    state('denied')
    state('approved')
    when_start(start_p2)

with flowchart('p3'):
    with stage('start'):
        to('input').when(m.subject == 'approve')
    
    with stage('input', get_input):
        to('process')
    
    with stage('process'):
        with flowchart('first'):
            with stage('start'):
                to('end').when(s.quantity <= 5)
            stage('end', signal_approved)

        with flowchart('second'):
            with stage('start'):
                to('end').when(s.quantity > 5)
            stage('end', signal_denied)

        to('approve').when(m.subject == 'approved')
        to('deny').when(m.subject == 'denied')

    stage('approve', report_approved)
    stage('deny', report_denied)
    when_start(start_p3)

run_all()
