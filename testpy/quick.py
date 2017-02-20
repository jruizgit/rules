from durable.lang import *

with statechart('coach'):
    with state('work'):
        with state('s1'):
            @to('s2')
            def startup(c):
                print('s1')
                #errorHere()
                print('s1 after')

        with state('s2'):
            @to('s3')
            def s2(c):
                print('s2 before')
                #errorHere()
                print('s2 after')

        with state('s3'):
            @to('s3')
            @when_all(m.completed != True)
            def s2(c):
                print('s3 before')
                errorHere()
                print('s3 after')


        @to('canceled')
        @when_all(+s.exception)
        def exception_handler(c):
            print('Error message: {0}'.format(c.s.exception))
            c.s.exception = None

    state('canceled')

    @when_start
    def start(host):
        host.post('coach', {'id': 1, 'sid': 1, 'completed': False})

run_all()
