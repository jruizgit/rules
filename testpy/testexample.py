from durable.lang import *
import pandas as pd
import random
import time

with ruleset('fraud_detection'):
    @when_all((m.t == 'debit_cleared') | (m.t == 'credit_cleared'))
    def handle_balance(c):
        balance_sheet = pd.DataFrame(c.s.balance_sheet, columns=['stamp', 'debit', 'credit', 'balance'])
        if c.s.balance_sheet:
            balance = balance_sheet.iloc[-1, -1]
        else:
            balance = 0

        new_row = None
        if c.m.t == 'debit_cleared':
            new_row = {'stamp': c.m.stamp, 'debit': c.m.amount, 'balance': balance - c.m.amount}
        else:
            new_row = {'stamp': c.m.stamp, 'credit': c.m.amount, 'balance': balance + c.m.amount}

        balance_sheet = balance_sheet.append(new_row, ignore_index=True)
        balance_sheet = balance_sheet[balance_sheet.stamp > c.m.stamp - 30 * 24 * 3600]
        c.s.balance_sheet = balance_sheet.values.tolist()
        c.s.avg_withdraw = balance_sheet.debit.mean()
        c.s.avg_deposit = balance_sheet.credit.mean()
        c.s.avg_balance = balance_sheet.balance.mean()
        
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

    @when_all(timeout('my_timer'))
    def start_timer(c):
        if not c.s.count:
            c.s.count = 1000
        else:
            c.s.count += 1

        c.post('fraud_detection', {'id': c.s.count, 'sid': 1, 't': 'debit_request', 'amount': c.s.count - 850, 'stamp': time.time()})
        c.start_timer('my_timer', 3)

    @when_start
    def start(host):
        host.post('fraud_detection', {'id': 1, 'sid': 1, 't': 'credit_cleared', 'amount': 500, 'stamp': time.time()})
        host.post('fraud_detection', {'id': 2, 'sid': 1, 't': 'debit_cleared', 'amount': 100, 'stamp': time.time()})
        host.post('fraud_detection', {'id': 3, 'sid': 1, 't': 'debit_cleared', 'amount': 100, 'stamp': time.time()})
        host.post('fraud_detection', {'id': 4, 'sid': 1, 't': 'debit_cleared', 'amount': 100, 'stamp': time.time()})
        host.start_timer('fraud_detection', 1, 'my_timer', 3)

run_all()
                          



            
