import durable
import datetime

def start_timer(s):
    s.state['start'] = datetime.datetime.now().strftime('%I:%M:%S%p')
    s.start_timer('my_timer', 5)

def end_timer(s):
    print('End')
    print('Started @%s' % s.state['start'])
    print('Ended @%s' % datetime.datetime.now().strftime('%I:%M:%S%p'))

def start_first_timer(s):
    s.state['start'] = datetime.datetime.now().strftime('%I:%M:%S%p')
    s.start_timer('first', 4)   

def start_second_timer(s):
    s.state['start'] = datetime.datetime.now().strftime('%I:%M:%S%p')
    s.start_timer('second', 3)

def signal_approved(s):
    s.signal({'id': 2, 'subject': 'approved', 'start': s.state['start']})

def signal_denied(s):
    s.signal({'id': 3, 'subject': 'denied', 'start': s.state['start']})

def report_approved(s):
    print('Approved')
    print('Started @%s' % s.event['start'])
    print('Ended @%s' % datetime.datetime.now().strftime('%I:%M:%S%p'))

def report_denied(s):
    print('Denied')
    print('Started @%s' % s.event['start'])
    print('Ended @%s' % datetime.datetime.now().strftime('%I:%M:%S%p'))

def start(host):
    host.post('t1', {'id': 1, 'sid': 1, 'start': 'yes'})
    host.post('t2', {'id': 1, 'sid': 1, 'subject': 'approve'})
    
durable.run({
    't1': {
        'r1': {'when': {'start': 'yes'}, 'run': start_timer},
        'r2': {'when': {'$t': 'my_timer'}, 'run': end_timer}
    },
    't2$state': {
        'input': {
            'review': {
                'when': {'subject': 'approve'},
                'run': {
                    'first$state': {
                        'send': {'start': {'run': start_first_timer, 'to': 'evaluate'}},
                        'evaluate': {
                            'wait': {'when': {'$t': 'first'}, 'run': signal_approved, 'to': 'end'}
                        },
                        'end': {}
                    },
                    'second$state': {
                        'send': {'start': {'run': start_second_timer, 'to': 'evaluate'}},
                        'evaluate': {
                            'wait': {'when': {'$t': 'second'}, 'run': signal_denied, 'to': 'end'}
                        },
                        'end': {}
                    }
                },
                'to': 'pending'
            }
        },
        'pending': {
            'approve': {'when': {'subject': 'approved'}, 'run': report_approved, 'to': 'approved'},
            'deny': {'when': {'subject': 'denied'}, 'run': report_denied, 'to': 'denied'}
        },
        'denied': {},
        'approved': {}
    }
}, ['/tmp/redis.sock'], start)