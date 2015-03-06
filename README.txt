=============
durable_rules
=============  
for real time analytics
-----------------------
durable_rules is a polyglot micro-framework for real-time, consistent and scalable coordination of events. With durable_rules you can track and analyze information about things that happen (events) by combining data from multiple sources to infer more complicated circumstances.

A full forward chaining implementation (A.K.A. Rete) is used to evaluate facts and massive streams of events in real time. A simple, yet powerful meta-liguistic abstraction lets you define simple and complex rulesets, such as flowcharts, statecharts, nested statecharts, paralel and time driven flows. 

The Durable Rules core engine is implemented in C, which enables ultra fast rule evaluation and inference as well as muti-language support. Durable Rules relies on state of the art technologies: Werkzeug(http://werkzeug.pocoo.org/ is used to host rulesets). Inference state is cached using Redis(http://www.redis.io), which lets scaling out without giving up performance.

Let’s consider a couple of fictitious fraud rules used in bank account management.  
Note: I'm paraphrasing the example presented in this article(https://www.packtpub.com/books/content/drools-jboss-rules-50complex-event-processing).  

1. If there are two debit requests greater than 200% the average monthly withdrawal amount in a span of 2 minutes, flag the account as medium risk.
2. If there are three consecutive increasing debit requests, withdrawing more than 70% the average monthly balance in a span of three minutes, flag the account as high risk

**Example**::

    from durable.lang import *
    
    with ruleset('fraud_detection'):
        # compute monthly averages
        @when_all(span(86400), (m.t == 'debit_cleared') | (m.t == 'credit_cleared'))
        def handle_balance(c):
            debit_total = 0
            credit_total = 0
            for tx in c.m:
                if tx.t == 'debit_cleared':
                    debit_total += tx.amount
                else:
                    credit_total += tx.amount

            c.s.balance = c.s.balance - debit_total + credit_total
            c.s.avg_balance = (c.s.avg_balance * 29 + c.s.balance) / 30
            c.s.avg_withdraw = (c.s.avg_withdraw * 29 + debit_total) / 30
        
        # medium risk rule
        @when_all(c.first << (m.t == 'debit_request') & 
                             (m.amount > c.s.avg_withdraw * 2),
                  c.second << (m.t == 'debit_request') & 
                              (m.amount > c.s.avg_withdraw * 2) & 
                              (m.stamp > c.first.stamp) &
                              (m.stamp < c.first.stamp + 120))
        def first_rule(c):
            print('Medium Risk')

        # high risk rule
        @when_all(c.first << m.t == 'debit_request',
                  c.second << (m.t == 'debit_request') &
                              (m.amount > c.first.amount) & 
                              (m.stamp < c.first.stamp + 180),
                  c.third << (m.t == 'debit_request') & 
                             (m.amount > c.second.amount) & 
                             (m.stamp < c.first.stamp + 180),
                  s.avg_balance < (c.first.amount + c.second.amount + c.third.amount) / 0.7)
        def second_rule(c):
            print('High Risk')

    run_all()

**Reference Manual:**

- Python(https://github.com/jruizgit/rules/blob/master/docs/py/reference.md/)

**Blog:** 

- Polyglot (03/2015):http://jruizblog.com/2015/03/02/polyglot/
- Rete_D (02/2015):http://jruizblog.com/2015/02/23/rete_d/
- Boosting Performance with C (08/2014): http://jruizblog.com/2014/08/19/boosting-performance-with-c/
- Rete Meets Redis (02/2014):http://jruizblog.com/2014/02/02/rete-meets-redis/
- From Expert Systems to Cloud Scale Event Processing (01/2014):http://jruizblog.com/2014/01/27/event-processing/

