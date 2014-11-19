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
    print 'starting a3'
    host.post('a3', {'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100})
    host.post('a3', {'id': 2, 'sid': 1, 'subject': 'approved'})
    host.post('a3', {'id': 3, 'sid': 2, 'subject': 'approve', 'amount': 100})
    host.post('a3', {'id': 4, 'sid': 2, 'subject': 'denied'})
    host.post('a3', {'id': 5, 'sid': 3, 'subject': 'approve', 'amount': 10000})

ruleset('a1',
    when_one((m.subject == 'approve') & (m.amount > 1000), denied),
    when_one((m.subject == 'approve') & (m.amount <= 1000), request_approval),
    when_all([m.subject == 'approved', s.status == 'pending'], request_approval),
    when_one(s.status == 'approved', approved),
    when_one(m.subject == 'denied', denied),
    when_start(start_a1)
)

statechart('a2',
    state('input',
        to('denied').when_one((m.subject == 'approve') & (m.amount > 1000), denied),
        to('pending').when_one((m.subject == 'approve') & (m.amount <= 1000), request_approval)
    ),
    state('pending',
        to('pending').when_one(m.subject == 'approved', request_approval),
        to('approved').when_one(s.status == 'approved', approved),
        to('denied').when_one(m.subject == 'denied', denied)
    ),
    state('denied'),
    state('approved'),
    when_start(start_a2)
)

flowchart('a3',
    stage('input', None,
        to('request').when_one((m.subject == 'approve') & (m.amount <= 1000)),
        to('deny').when_one((m.subject == 'approve') & (m.amount > 1000))
    ),
    stage('request', request_approval, 
        to('approve').when_one(s.status == 'approved'),
        to('deny').when_one(m.subject == 'denied'),
        to('request').when_any([m.subject == 'approved', m.subject == 'ok'])
    ),
    stage('approve', approved),
    stage('deny', denied),
    when_start(start_a3)
)



run_all()
