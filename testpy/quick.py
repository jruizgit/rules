from durable.lang import *

with ruleset('coach'):
    @when_all(m.completed != True)
    def trigger_reminders(c):
        print('An error message should follow this line')
        # this function is not defined and should crash
        errorOutHere()
        # this line is never reached
        print('This part is never reached and no error is thrown')

    @when_all(+s.exception)
    def exception_handler(c):
        print('Error message: {0}'.format(c.s.exception))
        c.s.exception = None

    @when_start
    def start(host):
        host.assert_fact('coach', {'id': 1, 'sid': 1, 'completed': False})


with ruleset('test'):
    @when_all(m.subject == 'World')
    def say_hello(c):
        print ('Hello {0}'.format(len(c.m.mydata)))

    @when_start
    def start(host):
        host.post('test', {'id': 1, 'sid': 11221, 'subject': 'World', 'mydata': ['World','games']})

run_all()
