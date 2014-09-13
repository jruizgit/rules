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

def get_input(s):
    print('input {0}'.format(s.event['quantity']))
    s.state['quantity'] = s.event['quantity']

def signal_approved(s):
    print('signaling approved')
    s.signal({'id': 1, 'subject': 'approved'})   

def signal_denied(s):
    print('signaling denied')
    s.signal({'id': 1, 'subject': 'denied'})

def report_approved(s):
    print('approved')

def report_denied(s):
    print('denied')    

def start(host):
    def callback(e, result):
        if e:
            print(e)
        else:
            print('ok')

    host.post('p1', {'id': 1, 'sid': 1, 'start': 'yes'}, callback)

    host.post('p2', {'id': 1, 'sid': 1, 'subject': 'approve', 'quantity': 3}, callback)
    host.post('p2', {'id': 2, 'sid': 2, 'subject': 'approve', 'quantity': 10}, callback)

durable.run({
    'p1': {
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
            'whenAll': {'first': {'end': 'one'}, 'second': {'end': 'two'}},
            'run': done
        }
    },
    'p2$state': {
        'input': {
            'get': {
                'when': {'subject': 'approve'},
                'run': get_input,
                'to': 'process'
            },
        },
        'process': {
            'review': {
                'when': {'$s': {'$ex': {'quantity': 1}}},
                'run': {
                    'first$state': {
                        'evaluate': {
                            'wait': {
                                'when': {'$s': {'$lte': {'quantity': 5}}}, 
                                'run': signal_approved,
                                'to': 'end'
                            }
                        },
                        'end': {
                        }

                    },
                    'second$state': {
                        'evaluate': {
                            'wait': {
                                'when': {'$s': {'$gt': {'quantity': 5}}},
                                'run': signal_denied,
                                'to': 'end'
                            }
                        },
                        'end': {
                        }
                    }
                },
                'to': 'result'
            }
        },
        'result': {
            'approve': {
                'when': {'subject': 'approved'},
                'run': report_approved,
                'to': 'approved'
            },
            'deny': {
                'when': {'subject': 'denied'},
                'run': report_denied,
                'to': 'denied'
            }
        },
        'denied': {
        },
        'approved': {
        }
    },
}, ['/tmp/redis.sock'], start)