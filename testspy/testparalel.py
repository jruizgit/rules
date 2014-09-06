import durable

def finish_one(s):
    print('finish one')
    s.signal({'id': 1, 'end': 'one'})
    s.state['start'] = 2

def finish_two(s):
    print('finish two')
    s.signal({'id': 1, 'end': 'two'})
    s.state['start'] = 2

def continue_flow(s):
    s.state['start'] = 1

def done(s):
    print('done')

def start(host):
    def callback(e, result):
        if e:
            print(e)
        else:
            print('ok')

    host.post('paralel1', {'id': 1, 'sid': 1, 'start': 'yes'}, callback)

durable.run({
    'paralel1': {
        'r1': {
            'when': {'start': 'yes'},
            'run': {
                'one': {
                    'r1': {
                        'when': {'$s': {'$nex': {'start': 1}}},
                        'run': continue_flow
                    },
                    'r2': {
                        'when': {'$s': {'start': 1}},
                        'run': finish_one
                    }
                },
                'two': {
                    'r1': {
                        'when': {'$s': {'$nex': {'start': 1}}},
                        'run': continue_flow
                    },
                    'r2': {
                        'when': {'$s': {'start': 1}},
                        'run': finish_two
                    }
                }
            }
        },
        'r2': {
            'whenAll': {'first': {'end': 'one' }, 'second': {'end': 'two' }},
            'run': done
        }
    }
}, ['/tmp/redis.sock'], start)