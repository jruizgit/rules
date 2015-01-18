import durable

def denied(c):
    print ('denied from: {0}, {1}'.format(c.ruleset_name, c.s['sid']))
    c.s['status'] = 'done'

def approved(c):
    print ('approved from: {0}, {1}'.format(c.ruleset_name, c.s['sid']))
    c.s['status'] = 'done'

def request_approval(c):
    print ('request approval from: {0}, {1}'.format(c.ruleset_name, c.s['sid']))
    print (c.m)
    if 'status' in c.s:
        print ('here')
        c.s['status'] = 'approved'
    else:
        print ('there')
        c.s['status'] = 'pending'

def start(host):
    host.post('a1', {'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100})
    host.post('a1', {'id': 2, 'sid': 1, 'subject': 'approved'})
    host.post('a1', {'id': 3, 'sid': 2, 'subject': 'approve', 'amount': 100})
    host.post('a1', {'id': 4, 'sid': 2, 'subject': 'denied'})
    host.post('a1', {'id': 5, 'sid': 3, 'subject': 'approve', 'amount': 10000})
   
    host.post('a2', {'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100})
    host.post('a2', {'id': 2, 'sid': 1, 'subject': 'approved'})
    host.post('a2', {'id': 3, 'sid': 2, 'subject': 'approve', 'amount': 100})
    host.post('a2', {'id': 4, 'sid': 2, 'subject': 'denied'})
    host.post('a2', {'id': 5, 'sid': 3, 'subject': 'approve', 'amount': 10000})
    
    host.post('a3', {'id': 1, 'sid': 1, 'subject': 'approve', 'amount': 100})
    host.post('a3', {'id': 2, 'sid': 1, 'subject': 'approved'})
    host.post('a3', {'id': 3, 'sid': 2, 'subject': 'approve', 'amount': 100})
    host.post('a3', {'id': 4, 'sid': 2, 'subject': 'denied'})
    host.post('a3', {'id': 5, 'sid': 3, 'subject': 'approve', 'amount': 10000})
    

durable.run({
    'a1': {
        'r1': {
            'all': [{'m': {'$and': [{'subject': 'approve'}, {'$gt': {'amount': 1000}}]}}],
            'run': denied
        },
        'r2': {
            'all': [{'m': {'$and': [{'subject': 'approve' }, {'$lte': {'amount': 1000 }}]}}],
            'run': request_approval
        },
        'r3': {
            'all': [
                {'m$any': [{'a': {'subject': 'approved'}, 'b': {'subject': 'ok'}}]},
                {'s': {'$and': [{'status': 'pending'}, {'$s': 1}]}}
            ],
            'run': request_approval
        },
        'r4': {'all': [{'s': {'$and': [{'status': 'approved'}, {'$s': 1}]}}], 'run': approved},
        'r5': {'all': [{'m': {'subject': 'denied'}}], 'run': denied}
    },
    'a2$state': {
        'input': {
            'deny': {
                'all': [{'m': {'$and': [{'subject': 'approve'}, {'$gt': {'amount': 1000}}]}}],
                'run': denied,
                'to': 'denied'
            },
            'request': {
                'all': [{'m': {'$and': [{'subject': 'approve'}, {'$lte': {'amount': 1000}}]}}],
                'run': request_approval,
                'to': 'pending'
            }
        },
        'pending': {
            'request': {
                'any': [{'a': {'subject': 'approved'}}, {'b': {'subject': 'ok'}}],
                'run': request_approval,
                'to': 'pending'
            },
            'approve': {
                'all': [{'s': {'$and': [{'status': 'approved'}, {'$s': 1}]}}],
                'run': approved,
                'to': 'approved'
            },
            'deny': {
                'all': [{'m': {'subject': 'denied'}}],
                'run': denied,
                'to': 'denied'
            }
        },
        'denied': {
        },
        'approved': {
        }
    },
    'a3$flow': {
        'input': {
            'to': {
                'request': {'all': [{'m': {'$and': [{'subject': 'approve' }, {'$lte': {'amount': 1000}}]}}]},
                'deny': {'all': [{'m': {'$and': [{'subject': 'approve'}, {'$gt': {'amount': 1000}}]}}]}
            }
        },
        'request': {
            'run': request_approval,
            'to': {
                'approve': {'all': [{'s': {'$and': [{'status': 'approved'}, {'$s': 1}]}}]},
                'deny': {'all': [{'m': {'subject': 'denied'}}]},
                'request': {'any': [{'a': {'subject': 'approved'}}, {'b': {'subject': 'ok'}}]}
            }
        },
        'approve': {'run': approved},
        'deny': {'run': denied}
    }
}, ['/tmp/redis.sock'], start)