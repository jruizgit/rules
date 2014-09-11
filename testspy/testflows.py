import durable

def denied(s):
    print ('denied from: {0}'.format(s.ruleset_name))
    s.state['status'] = 'done'

def approved(s):
    print ('approved from: {0}'.format(s.ruleset_name))
    s.state['status'] = 'done'

def request_approval(s):
    print ('request approval from: {0}'.format(s.ruleset_name))
    print (s.event)
    if 'status' in s.state:
        s.state['status'] = 'approved'
    else:
        s.state['status'] = 'pending'

def start(host):
    def callback(e, result):
        if e:
            print(e)
        else:
            print('ok')

    host.post('a1', {'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100}, callback)
    host.post('a1', {'id': 2, 'sid': 1, 'subject': 'approved'}, callback)
    host.post('a1', {'id': 3, 'sid': 2, 'subject': 'approve', 'amount': 100}, callback)
    host.post('a1', {'id': 4, 'sid': 2, 'subject': 'denied'}, callback)
    host.post('a1', {'id': 5, 'sid': 3, 'subject': 'approve', 'amount': 10000}, callback)
   
    host.post('a2', {'id': '1', 'sid': '1', 'subject': 'approve', 'amount': 100}, callback)
    host.post('a2', {'id': '2', 'sid': '1', 'subject': 'approved'}, callback)
    host.post('a2', {'id': 3, 'sid': 2, 'subject': 'approve', 'amount': 100}, callback)
    host.post('a2', {'id': 4, 'sid': 2, 'subject': 'denied'}, callback)
    host.post('a2', {'id': 5, 'sid': 3, 'subject': 'approve', 'amount': 10000}, callback)
    
durable.run({
    'a1': {
        'r1': {
            'when': {'$and': [{'subject': 'approve'}, {'$gt': {'amount': 1000}}]},
            'run': denied
        },
        'r2': {
            'when': {'$and': [{'subject': 'approve' }, {'$lte': {'amount': 1000 }}]},
            'run': request_approval
        },
        'r3': {
            'whenAll': {
                'm$any': {'a': {'subject': 'approved'}, 'b': {'subject': 'ok'}},
                '$s': {'status': 'pending'}
            },
            'run': request_approval
        },
        'r4': {'when': {'$s': {'status': 'approved'}}, 'run': approved},
        'r5': {'when': {'subject': 'denied'}, 'run': denied}
    },
    'a2$state': {
        'input': {
            'deny': {
                'when': {'$and': [{'subject': 'approve'}, {'$gt': {'amount': 1000}}]},
                'run': denied,
                'to': 'denied'
            },
            'request': {
                'when': {'$and': [{'subject': 'approve'}, {'$lte': {'amount': 1000}}]},
                'run': request_approval,
                'to': 'pending'
            }
        },
        'pending': {
            'request': {
                'whenAny': {'a': {'subject': 'approved'}, 'b': {'subject': 'ok'}},
                'run': request_approval,
                'to': 'pending'
            },
            'approve': {
                'when': {'$s': {'status': 'approved'}},
                'run': approved,
                'to': 'approved'
            },
            'deny': {
                'when': {'subject': 'denied'},
                'run': denied,
                'to': 'denied'
            }
        },
        'denied': {
        },
        'approved': {
        }
    }
}, ['/tmp/redis.sock'], start)