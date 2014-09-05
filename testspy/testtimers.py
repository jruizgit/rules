import durable
import datetime

def start_timer(s):
	s.state['start'] = datetime.datetime.now().strftime('%I:%M:%S%p')
	s.start_timer('my_timer', 5)

def end_timer(s):
	print('Started @%s' % s.state['start'])
	print('Ended @%s' % datetime.datetime.now().strftime('%I:%M:%S%p'))

def start(host):
    def callback(e, result):
        if e:
            print(e)
        else:
            print('ok')

    host.post('timer1', {'id': 1, 'sid': 1, 'start': 'yes'}, callback)
    
durable.run({
    'timer1': {
        'r1': {'when': {'start': 'yes'}, 'run': start_timer},
        'r2': {'when': {'$t': 'my_timer'}, 'run': end_timer}
    }
}, ['/tmp/redis.sock'], start)