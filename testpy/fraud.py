from durable.lang import *
import time


with ruleset('fraud_detection'):
    @when_all(span(10),
             (m.t == 'debit_cleared') | (m.t == 'credit_cleared'))
    def handle_balance(c):
        debit_total = 0
        credit_total = 0
        if c.s.balance is None:
            c.s.balance = 0
            c.s.avg_balance = 0
            c.s.avg_withdraw = 0

        for tx in c.m:
            if tx.t == 'debit_cleared':
                debit_total += tx.amount
            else:
                credit_total += tx.amount

        c.s.balance = c.s.balance - debit_total + credit_total
        c.s.avg_balance = (c.s.avg_balance * 29 + c.s.balance) / 30
        c.s.avg_withdraw = (c.s.avg_withdraw * 29 + debit_total) / 30
        print('balance {0}, average balance {1}, average withdraw {2}'.format(c.s.balance, c.s.avg_balance, c.s.avg_withdraw))
        
    @when_all(c.first << (m.t == 'debit_request') & 
                         (m.amount > c.s.avg_withdraw * 3),
              c.second << (m.t == 'debit_request') & 
                          (m.amount > c.s.avg_withdraw * 3) & 
                          (m.stamp > c.first.stamp) &
                          (m.stamp < c.first.stamp + 90))
    def first_rule(c):
        print('Medium Risk {0}, {1}'.format(c.first.amount, c.second.amount))

    @when_all(c.first << m.t == 'debit_request',
              c.second << (m.t == 'debit_request') &
                          (m.amount > c.first.amount) & 
                          (m.stamp < c.first.stamp + 180),
              c.third << (m.t == 'debit_request') & 
                         (m.amount > c.second.amount) & 
                         (m.stamp < c.first.stamp + 180),
              s.avg_balance < (c.first.amount + c.second.amount + c.third.amount) / 0.9)
    def second_rule(c):
        print('High Risk {0}, {1}, {2}'.format(c.first.amount, c.second.amount, c.third.amount))

    @when_all(m.t == 'customer' | timeout('customer'))
    def start_timer(c):
        if not c.s.c_count:
            c.s.c_count = 100
        else:
            c.s.c_count += 2

        c.post('fraud_detection', {'id': c.s.c_count, 'sid': 1, 't': 'debit_cleared', 'amount': c.s.c_count})
        c.post('fraud_detection', {'id': c.s.c_count + 1, 'sid': 1, 't': 'credit_cleared', 'amount': (c.s.c_count - 100) * 2 + 100})
        c.start_timer('customer', 1)

    @when_all(m.t == 'fraudster' | timeout('fraudster'))
    def start_timer(c):
        if not c.s.f_count:
            c.s.f_count = 1000
        else:
            c.s.f_count += 1

        c.post('fraud_detection', {'id': c.s.f_count, 'sid': 1, 't': 'debit_request', 'amount': c.s.f_count - 800, 'stamp': time.time()})
        c.start_timer('fraudster', 2)

    @when_start
    def start(host):
        host.post('fraud_detection', {'id': 'c_1' , 'sid': 1, 't': 'customer'})
        host.post('fraud_detection', {'id': 'f_1' , 'sid': 1, 't': 'fraudster'})

run_all()