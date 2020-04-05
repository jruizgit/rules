from durable.lang import *
import threading
import time
import datetime

with ruleset('test'):
    # antecedent
    @when_all(m.subject == 'World')
    def say_hello(c):
        # consequent
        print('test-> Hello {0}'.format(c.m.subject))

post('test', { 'subject': 'World' })

with ruleset('none_test'):
    @when_all(m.v == None)
    def is_none(c):
        print('none_test passed')

post('none_test', {'v': None})
try:
    post('none_test', {'v': 'something'})
except BaseException as e:
    print('none_test -> expected {0}'.format(e.message))


with ruleset('animal'):
    @when_all(c.first << (m.predicate == 'eats') & (m.object == 'flies'),
              (m.predicate == 'lives') & (m.object == 'water') & (m.subject == c.first.subject))
    def frog(c):
        c.assert_fact({ 'subject': c.first.subject, 'predicate': 'is', 'object': 'frog' })

    @when_all(c.first << (m.predicate == 'eats') & (m.object == 'flies'),
              (m.predicate == 'lives') & (m.object == 'land') & (m.subject == c.first.subject))
    def chameleon(c):
        c.assert_fact({ 'subject': c.first.subject, 'predicate': 'is', 'object': 'chameleon' })

    @when_all((m.predicate == 'eats') & (m.object == 'worms'))
    def bird(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'bird' })

    @when_all((m.predicate == 'is') & (m.object == 'frog'))
    def green(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'green' })

    @when_all((m.predicate == 'is') & (m.object == 'chameleon'))
    def grey(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'grey' })

    @when_all((m.predicate == 'is') & (m.object == 'bird'))
    def black(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'black' })

    @when_all(+m.subject)
    def output(c):
        print ('animal-> Fact: {0} {1} {2}'.format(c.m.subject, c.m.predicate, c.m.object))

assert_fact('animal', { 'subject': 'Kermit', 'predicate': 'eats', 'object': 'flies' })
assert_fact('animal', { 'subject': 'Kermit', 'predicate': 'lives', 'object': 'water' })
assert_fact('animal', { 'subject': 'Greedy', 'predicate': 'eats', 'object': 'flies' })
assert_fact('animal', { 'subject': 'Greedy', 'predicate': 'lives', 'object': 'land' })
assert_fact('animal', { 'subject': 'Tweety', 'predicate': 'eats', 'object': 'worms' })
print('animal -> {0}'.format(get_facts('animal')))

with ruleset('animal0'):
    # will be triggered by 'Kermit eats flies'
    @when_all((m.predicate == 'eats') & (m.object == 'flies'))
    def frog(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'frog' })

    @when_all((m.predicate == 'eats') & (m.object == 'worms'))
    def bird(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'bird' })

    # will be chained after asserting 'Kermit is frog'
    @when_all((m.predicate == 'is') & (m.object == 'frog'))
    def green(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'green' })

    @when_all((m.predicate == 'is') & (m.object == 'bird'))
    def black(c):
        c.assert_fact({ 'subject': c.m.subject, 'predicate': 'is', 'object': 'black' })

    @when_all(+m.subject)
    def output(c):
        print('animal0-> Fact: {0} {1} {2}'.format(c.m.subject, c.m.predicate, c.m.object))

assert_fact('animal0', { 'subject': 'Kermit', 'predicate': 'eats', 'object': 'flies' })


with ruleset('risk'):
    @when_all(c.first << m.t == 'purchase',
              c.second << m.location != c.first.location)
    # the event pair will only be observed once
    def fraud(c):
        print('risk-> Fraud detected -> {0}, {1}'.format(c.first.location, c.second.location))

    
# 'post' submits events, try 'assert' instead and to see differt behavior
post('risk', {'t': 'purchase', 'location': 'US'})
post('risk', {'t': 'purchase', 'location': 'CA'})

with ruleset('counter_or'): 
    @when_all(c.first << (+m.counter), 
              c.second<< ((m.countersValid == True) & ((m.primeCount == c.first.counter) | (m.secondCount == c.first.counter))))  
    def match(c): 
        print ('counter_or match') 

assert_fact('counter_or', { 'counter': 1}) 
assert_fact('counter_or', { 'primeCount': 2,'secondCount':1,'countersValid':True})
assert_fact('counter_or', { 'primeCount': 2,'secondCount':2,'countersValid':True})

with ruleset('flow'):
    # state condition uses 's'
    @when_all(s.status == 'start')
    def start(c):
        # state update on 's'
        c.s.status = 'next' 
        print('flow-> start')

    @when_all(s.status == 'next')
    def next(c):
        c.s.status = 'last' 
        print('flow-> next')

    @when_all(s.status == 'last')
    def last(c):
        c.s.status = 'end' 
        print('flow-> last')
        # deletes state at the end
        c.delete_state()

# modifies default context state
update_state('flow', { 'status': 'start' })


with ruleset('expense0'):
    @when_all((m.subject == 'approve') | (m.subject == 'ok'))
    def approved(c):
        print ('expense0-> Approved subject: {0}'.format(c.m.subject))
        
post('expense0', { 'subject': 'approve'})

with ruleset('match'):
    @when_all(m.url.matches('(https?://)?([0-9a-z.-]+)%.[a-z]{2,6}(/[A-z0-9_.-]+/?)*'))
    def approved(c):
        print ('match-> url {0}'.format(c.m.url))

post('match', { 'url': 'https://github.com' })
try:
    post('match', { 'url': 'http://github.com/jruizgit/rul!es' })
except BaseException as e:
    print('match -> expected {0}'.format(e.message))
post('match', { 'url': 'https://github.com/jruizgit/rules/reference.md' })
def match_callback(e, state):
    print('match -> expected {0}'.format(e.message))
post('match', { 'url': '//rules'}, match_callback)
post('match', { 'url': 'https://github.c/jruizgit/rules' }, match_callback)


with ruleset('strings'):
    @when_all(m.subject.matches('hello.*'))
    def starts_with(c):
        print ('string-> starts with hello: {0}'.format(c.m.subject))

    @when_all(m.subject.matches('.*hello'))
    def ends_with(c):
        print ('string-> ends with hello: {0}'.format(c.m.subject))

    @when_all(m.subject.imatches('.*hello.*'))
    def contains(c):
        print ('string-> contains hello (case insensitive): {0}'.format(c.m.subject))

assert_fact('strings', { 'subject': 'HELLO world' })
assert_fact('strings', { 'subject': 'world hello' })
assert_fact('strings', { 'subject': 'hello hi' })
assert_fact('strings', { 'subject': 'has Hello string' })
try:
    assert_fact('strings', { 'subject': 'does not match' })
except BaseException as e:
    print('strings -> expected {0}'.format(e.message))

try:
    assert_fact('strings', { 'subject': 5 })
except BaseException as e:
    print('strings -> expected {0}'.format(e.message))


with ruleset('coerce'):
    @when_all(m.subject.matches('10.500000'))
    def starts_with(c):
        print ('coerce-> matches 10.500000: {0}'.format(c.m.subject))

    @when_all(m.subject.matches('5'))
    def starts_with(c):
        print ('coerce-> matches 5: {0}'.format(c.m.subject))

    @when_all(m.subject.matches('false'))
    def starts_with(c):
        print ('coerce-> matches false: {0}'.format(c.m.subject))
   
    @when_all(m.subject.matches('nil'))
    def starts_with(c):
        print ('coerce-> matches nil')


assert_fact('coerce', { 'subject': 10.5 })
assert_fact('coerce', { 'subject': 5 })
assert_fact('coerce', { 'subject': False })
assert_fact('coerce', { 'subject': None })


try:
    with ruleset('invalid1'):
        @when_all(m.subject.matches('*'))
        def should_not_work(c):
            print ('should not work: {0}'.format(c.m.subject))

    assert_fact('invalid1', { 'subject': 'hello world' })
except BaseException as e:
    print('invalid1 -> expected {0}'.format(e))


try:
    with ruleset('invalid2'):
        @when_all(m.subject.matches('.**'))
        def should_not_work(c):
            print ('should not work: {0}'.format(c.m.subject))

    assert_fact('invalid2', { 'subject': 'hello world' })
except BaseException as e:
    print('invalid2 -> expected {0}'.format(e))


try:
    with ruleset('invalid3'):
        @when_all(m.subject.matches('.?*'))
        def should_not_work(c):
            print ('should not work: {0}'.format(c.m.subject))

    assert_fact('invalid3', { 'subject': 'hello world' })
except BaseException as e:
    print('invalid3 -> expected {0}'.format(e))


with ruleset('indistinct'):
    @when_all(distinct(False),
              c.first << m.amount > 10,
              c.second << m.amount > c.first.amount * 2,
              c.third << m.amount > (c.first.amount + c.second.amount) / 2)
    def detected(c):
        print('indistinct -> {0}'.format(c.first.amount))
        print('           -> {0}'.format(c.second.amount))
        print('           -> {0}'.format(c.third.amount))
        
post('indistinct', { 'amount': 50 })
post('indistinct', { 'amount': 200 })
post('indistinct', { 'amount': 251 })

        
with ruleset('distinct'):
    @when_all(c.first << m.amount > 10,
              c.second << m.amount > c.first.amount * 2,
              c.third << m.amount > (c.first.amount + c.second.amount) / 2)
    def detected(c):
        print('distinct -> {0}'.format(c.first.amount))
        print('         -> {0}'.format(c.second.amount))
        print('         -> {0}'.format(c.third.amount))
        
post('distinct', { 'amount': 50 })
post('distinct', { 'amount': 200 })
post('distinct', { 'amount': 251 })


with ruleset('expense1'):
    @when_any(all(c.first << m.subject == 'approve', 
                  c.second << m.amount == 1000), 
              all(c.third << m.subject == 'jumbo', 
                  c.fourth << m.amount == 10000))
    def action(c):
        if c.first:
            print ('expense1-> Approved {0} {1}'.format(c.first.subject, c.second.amount))
        else:
            print ('expense1-> Approved {0} {1}'.format(c.third.subject, c.fourth.amount))
    
post('expense1', { 'subject': 'approve' })
post('expense1', { 'amount': 1000 })
post('expense1', { 'subject': 'jumbo' })
post('expense1', { 'amount': 10000 })


with ruleset('risk1'):
    @when_all(c.first << m.t == 'deposit',
              none(m.t == 'balance'),
              c.third << m.t == 'withdrawal',
              c.fourth << m.t == 'chargeback')
    def detected(c):
        print('risk1-> fraud detected {0} {1} {2}'.format(c.first.t, c.third.t, c.fourth.t))
        
post('risk1', { 't': 'deposit' })
post('risk1', { 't': 'withdrawal' })
post('risk1', { 't': 'chargeback' })


with ruleset('attributes'):
    @when_all(pri(3), m.amount < 300)
    def first_detect(c):
        print('attributes-> P3: {0}'.format(c.m.amount))
        print('attributes-> {0}'.format(c.get_facts()))
        
    @when_all(pri(2), m.amount < 200)
    def second_detect(c):
        print('attributes-> P2: {0}'.format(c.m.amount))
        
    @when_all(pri(1), m.amount < 100)
    def third_detect(c):
        print('attributes-> P1: {0}'.format(c.m.amount))
        
assert_fact('attributes', { 'amount': 50 })
assert_fact('attributes', { 'amount': 150 })
assert_fact('attributes', { 'amount': 250 })


with ruleset('expense2'):
    # this rule will trigger as soon as three events match the condition
    @when_all(count(3), m.amount < 100)
    def approve(c):
        print('expense2-> approved {0}'.format(c.m))

    # this rule will be triggered when 'expense' is asserted batching at most two results       
    @when_all(cap(2),
              c.expense << m.amount >= 100,
              c.approval << m.review == True)
    def reject(c):
        print('expense2-> rejected {0}'.format(c.m))

post_batch('expense2', [{ 'amount': 10 },
                        { 'amount': 20 },
                        { 'amount': 100 },
                        { 'amount': 30 },
                        { 'amount': 200 },
                        { 'amount': 400 }])
assert_fact('expense2', { 'review': True })


with ruleset('flow1'):
    
    @when_all(m.action == 'start')
    def first(c):
        raise Exception('Unhandled Exception!')

    # when the exception property exists
    @when_all(+s.exception)
    def second(c):
        print('flow1-> expected {0}'.format(c.s.exception))
        c.s.exception = None
        
post('flow1', { 'action': 'start' })


with statechart('expense3'):
    
    # initial state 'input' with two triggers
    with state('input'):
        # trigger to move to 'denied' given a condition
        @to('denied')
        @when_all((m.subject == 'approve') & (m.amount > 1000))
        # action executed before state change
        def denied(c):
            c.s.status = 'expense3-> denied amount {0}'.format(c.m.amount)
        
        @to('pending')    
        @when_all((m.subject == 'approve') & (m.amount <= 1000))
        def request(c):
            c.s.status = 'expense3-> requesting approve amount {0}'.format(c.m.amount)
    
    # intermediate state 'pending' with two triggers
    with state('pending'):
        @to('approved')
        @when_all(m.subject == 'approved')
        def approved(c):
            c.s.status = 'expense3-> expense approved'
            
        @to('denied')
        @when_all(m.subject == 'denied')
        def denied(c):
            c.s.status = 'expense3-> expense denied'
    
    # 'denied' and 'approved' are final states    
    state('denied')
    state('approved')

# events directed to default statechart instance
print(post('expense3', { 'subject': 'approve', 'amount': 100 })['status'])
print(post('expense3', { 'subject': 'approved' })['status'])

# events directed to statechart instance with id '1'
print(post('expense3', { 'sid': 1, 'subject': 'approve', 'amount': 100 })['status'])
print(post('expense3', { 'sid': 1, 'subject': 'denied' })['status'])

# events directed to statechart instance with id '2'
print(post('expense3', { 'sid': 2, 'subject': 'approve', 'amount': 10000 })['status'])


with flowchart('expense4'):
    # initial stage 'input' has two conditions
    with stage('input'): 
        to('request').when_all((m.subject == 'approve') & (m.amount <= 1000))
        to('deny').when_all((m.subject == 'approve') & (m.amount > 1000))
    
    # intermediate stage 'request' has an action and three conditions
    with stage('request'):
        @run
        def request(c):
            print('expense4-> requesting approve')
            
        to('approve').when_all(m.subject == 'approved')
        to('deny').when_all(m.subject == 'denied')
        # reflexive condition: if met, returns to the same stage
        to('request').when_all(m.subject == 'retry')
    
    with stage('approve'):
        @run 
        def approved(c):
            print('expense4-> expense approved')

    with stage('deny'):
        @run
        def denied(c):
            print('expense4-> expense denied')
    
# events for the default flowchart instance, approved after retry
post('expense4', { 'subject': 'approve', 'amount': 100 })
post('expense4', { 'subject': 'retry' })
post('expense4', { 'subject': 'approved' })

# events for the flowchart instance '1', denied after first try
post('expense4', { 'sid': 1, 'subject': 'approve', 'amount': 100})
post('expense4', { 'sid': 1, 'subject': 'denied'})

# event for the flowchart instance '2' immediately denied
post('expense4', { 'sid': 2, 'subject': 'approve', 'amount': 10000})


with statechart('worker'):
    # super-state 'work' has two states and one trigger
    with state('work'):
        # sub-state 'enter' has only one trigger
        with state('enter'):
            @to('process')
            @when_all(m.subject == 'enter')
            def continue_process(c):
                print('worker-> start process')
                print('worker-> {0}'.format(c.get_pending_events()))
    
        with state('process'):
            @to('process')
            @when_all(m.subject == 'continue')
            def continue_process(c):
                print('worker-> continue processing')

        # the super-state trigger will be evaluated for all sub-state triggers
        @to('canceled')
        @when_all(m.subject == 'cancel')
        def cancel(c):
            print('worker-> cancel process')

    state('canceled')

# will move the statechart to the 'work.process' sub-state
post('worker', { 'subject': 'enter' })

# will keep the statechart to the 'work.process' sub-state
post('worker', { 'subject': 'continue' })
post('worker', { 'subject': 'continue' })

# will move the statechart out of the work state
post('worker', { 'subject': 'cancel' })


with ruleset('expense5'):
    @when_all(c.bill << (m.t == 'bill') & (m.invoice.amount > 50),
              c.account << (m.t == 'account') & (m.payment.invoice.amount == c.bill.invoice.amount))
    def approved(c):
        print ('expense5-> bill amount: {0}'.format(c.bill.invoice.amount))
        print ('expense5-> account payment amount: {0}'.format(c.account.payment.invoice.amount))
        
post('expense5', {'t': 'bill', 'invoice': {'amount': 100}})
post('expense5', {'t': 'account', 'payment': {'invoice': {'amount': 100}}})


with ruleset('expense6'):
    @when_any((m.subject == 'approve') & (m.amount == 1000), 
              (m.subject == 'jumbo') & (m.amount == 10000))
    def action(c):
        print('expense6 matched')
    
post('expense6', { 'subject': 'approve', 'amount': 1000 })
post('expense6', { 'subject': 'jumbo', 'amount': 10000 })


with ruleset("expense7"):
    @when_any(m.observations.anyItem(item.matches("a")) & m.observations.anyItem(item.matches("b")),
              m.observations.anyItem(item.matches("c")) & m.observations.anyItem(item.matches("d")))
    def match(c):
        print("expense7 matched")

post("expense7", {"observations": ["a", "b"]})
post("expense7", {"observations": ["c", "d"]})


with ruleset('nested'):
    @when_all(+m.item)
    def test(c):
        print('nested ->{0}'.format(c.m.item))

post('nested', {'item': 'not_nested'})
post('nested', {'item': {'nested': 'true'}})
post('nested', {'item': {'nested': {'nested': 'true'}}})


with ruleset('bookstore'):
    # this rule will trigger for events with status
    @when_all(+m.status)
    def event(c):
        print('bookstore-> Reference {0} status {1}'.format(c.m.reference, c.m.status))

    @when_all(+m.name)
    def fact(c):
        print('bookstore-> Added {0}'.format(c.m.name))
        
    # this rule will be triggered when the fact is retracted
    @when_all(none(+m.name))
    def empty(c):
        print('bookstore-> No books')

assert_fact('bookstore', {
    'name': 'The new book',
    'seller': 'bookstore',
    'reference': '75323',
    'price': 500
})
# will throw MessageObservedError because the fact has already been asserted 
try:
    assert_fact('bookstore', {
        'reference': '75323',
        'name': 'The new book',
        'price': 500,
        'seller': 'bookstore'
    })
except BaseException as e:
    print('bookstore expected {0}'.format(e.message))
post('bookstore', {
    'reference': '75323',
    'status': 'Active'
})
post('bookstore', {
    'reference': '75323',
    'status': 'Active'
})
retract_fact('bookstore', {
    'reference': '75323',
    'name': 'The new book',
    'price': 500,
    'seller': 'bookstore'
})


with ruleset('risk5'):
    # compares properties in the same event, this expression is evaluated in the client 
    @when_all(m.debit > m.credit * 2)
    def fraud_1(c):
        print('risk5-> debit {0} more than twice the credit {1}'.format(c.m.debit, c.m.credit))

    # compares two correlated events, this expression is evaluated in the backend
    @when_all(c.first << m.amount > 100,
              c.second << m.amount > c.first.amount + m.amount / 2)
    def fraud_2(c):
        print('risk5-> fraud detected ->{0}'.format(c.first.amount))
        print('risk5-> fraud detected ->{0}'.format(c.second.amount))
        
post('risk5', { 'debit': 220, 'credit': 100 })

try:
    post('risk5', { 'debit': 150, 'credit': 100 })
except BaseException as e:
    print('risk5 expected {0}'.format(e.message))

post('risk5', { 'amount': 200 })
post('risk5', { 'amount': 500 })


with ruleset('risk6'):
    # matching primitive array
    @when_all(m.payments.allItems((item > 100) & (item < 400)))
    def rule1(c):
        print('risk6-> should not match {0}'.format(c.m.payments))

    # matching primitive array
    @when_all(m.payments.allItems((item > 100) & (item < 500)))
    def rule1(c):
        print('risk6-> fraud 1 detected {0}'.format(c.m.payments))

    # matching object array
    @when_all(m.payments.allItems((item.amount < 250) | (item.amount >= 300)))
    def rule2(c):
        print('risk6-> fraud 2 detected {0}'.format(c.m.payments))

    # pattern matching string array
    @when_all(m.cards.anyItem(item.matches('three.*')))
    def rule3(c):
        print('risk6-> fraud 3 detected {0}'.format(c.m.cards))

    # matching nested arrays
    @when_all(m.payments.anyItem(item.allItems(item < 100)))
    def rule4(c):
        print('risk6-> fraud 4 detected {0}'.format(c.m.payments))


    @when_all(m.payments.allItems(item > 100) & (m.cash == True))
    def rule5(c):
        print('risk6-> fraud 5 detected {0}'.format(c.m.cash))

    @when_all((m.field == 1) & m.payments.allItems(item.allItems((item > 100) & (item < 1000))))
    def rule6(c):
        print('risk6-> fraud 6 detected {0}'.format(c.m.payments))


    @when_all((m.field == 1) & m.payments.allItems(item.anyItem((item > 100) | (item < 50))))
    def rule7(c):
        print('risk6-> fraud 7 detected {0}'.format(c.m.payments))


    @when_all(m.payments.anyItem(-item.field1 & (item.field2 == 2)))
    def rule8(c):
        print('risk6-> fraud 8 detected {0}'.format(c.m.payments))
 
    @when_all(m.features.feature1.anyItem(item == 'a') | (m.features.feature2 == 'c'))
    def rule9(c):
        print('risk6-> fraud 9 detected {0}'.format(c.m.features))

def risk6_callback(e, state):
    print('risk6 -> expected {0}'.format(e.message))

post('risk6', {'payments': [ 150, 300, 450 ]})
post('risk6', {'payments': [ { 'amount' : 200 }, { 'amount' : 300 }, { 'amount' : 450 } ]})
post('risk6', {'cards': [ 'one card', 'two cards', 'three cards' ]})
post('risk6', {'payments': [ [ 10, 20, 30 ], [ 30, 40, 50 ], [ 10, 20 ] ]})  
post('risk6', {'payments': [ 150, 350, 600 ], 'cash': True })  
post('risk6', {'field': 1, 'payments': [ [ 200, 300 ], [ 150, 200 ] ]})  
post('risk6', {'field': 1, 'payments': [ [ 20, 180 ], [ 90, 190 ] ]})
post('risk6', {'payments': [ {'field2': 2 } ]})
post('risk6', {'payments': [ {'field2': 1 } ]}, risk6_callback)
post('risk6', {'payments': [ {'field1': 1, 'field2': 2} ]}, risk6_callback)
post('risk6', {'payments': [ {'field1': 1, 'field2': 1} ]}, risk6_callback)
post('risk6', {'features': {'feature1': ['a'],'feature2': 'c'}})
post('risk6', {'features': {'feature1': [],'feature2': 'c'}})

def risk7_callback(c):
    print('risk7 fraud detected')

get_host().set_rulesets({ 'risk7': {
    'suspect': {
        'run': risk7_callback,
        'all': [
            {'first': {'t': 'purchase'}},
            {'second': {'$neq': {'location': {'first': 'location'}}}}
        ],
    }
}})

post('risk7', {'t': 'purchase', 'location': 'US'})
post('risk7', {'t': 'purchase', 'location': 'CA'})

with ruleset('timer1'):
    
    @when_all(m.subject == 'start')
    def start(c):
        c.start_timer('MyTimer', 5)
        
    @when_all(timeout('MyTimer'))
    def timer(c):
        print('timer1 timeout')

post('timer1', { 'subject': 'start' })


with ruleset('timer2'):
    # will trigger when MyTimer expires
    @when_any(all(s.count == 0),
              all(s.count < 5,
                  timeout('MyTimer')))
    def pulse(c):
        c.s.count += 1
        # MyTimer will expire in 5 seconds
        c.start_timer('MyTimer', 1)
        print('timer2 pulse ->{0}'.format(datetime.datetime.now().strftime('%I:%M:%S%p')))
        
    @when_all(m.cancel == True)
    def cancel(c):
        c.cancel_timer('MyTimer')
        print('timer2 canceled timer')

update_state('timer2', { 'count': 0 })


with statechart('risk3'):
    with state('start'):
        @to('meter')
        def start(c):
            c.start_timer('RiskTimer', 5)

    with state('meter'):
        @to('fraud')
        @when_all(count(3), c.message << m.amount > 100)
        def fraud(c):
            for e in c.m:
                print('risk3-> {0}'.format(e.message)) 

        @to('exit')
        @when_all(timeout('RiskTimer'))
        def exit(c):
            print('risk3-> exit')

    state('fraud')
    state('exit')

# three events in a row will trigger the fraud rule
post('risk3', { 'amount': 200 })
post('risk3', { 'amount': 300 })
post('risk3', { 'amount': 400 })

# two events will exit after 5 seconds
post('risk3', { 'sid': 1, 'amount': 500 })
post('risk3', { 'sid': 1, 'amount': 600 })


with statechart('risk4'):
    with state('start'):
        @to('meter')
        def start(c):
            c.start_timer('VelocityTimer', 5, True)

    with state('meter'):
        @to('meter')
        @when_all(cap(5), 
                  m.amount > 100,
                  timeout('VelocityTimer'))
        def some_events(c):
            print('velocity: {0} in 5 seconds'.format(len(c.m)))
            # resets and restarts the manual reset timer
            c.start_timer('VelocityTimer', 5, True)

        @to('meter')
        @when_all(timeout('VelocityTimer'))
        def no_events(c):
            print('velocity: no events in 5 seconds')
            c.cancel_timer('VelocityTimer')
            
post('risk4', { 'amount': 200 })
post('risk4', { 'amount': 300 })
post('risk4', { 'amount': 500 })
post('risk4', { 'amount': 600 })


with ruleset('flow0'):
    timer = None

    def start_timer(time, callback):
        timer = threading.Timer(time, callback)
        timer.daemon = True    
        timer.start()

    @when_all(s.state == 'first')
    # async actions take a callback argument to signal completion
    def first(c, complete):
        def end_first():
            c.s.state = 'second'     
            print('flow0-> first completed')

            # completes the action after 3 seconds
            complete(None)
        
        start_timer(3, end_first)

    @when_all(s.state == 'second')
    def second(c, complete):
        def end_second():
            c.s.state = 'third'
            print('flow0-> second completed')

            # completes the action after 6 seconds
            # use the first argument to signal an error
            complete(Exception('error detected'))

        start_timer(10, end_second)

        # overrides the 5 second default abandon timeout
        return 15

update_state('flow0', { 'state': 'first' })


with ruleset('multi_thread_test'):
    
    @when_all(m.subject == 'World')
    def say_hello(c):
        if not c.s.count:
            c.s.count = 1
        else:
            c.s.count += 1

        print('multi_thread_test -> Hello {0}, {1}, {2}'.format(c.m.subject, c.m.id, c.s.count))

post('multi_thread_test', { 'subject': 'World' })

def try_multi_thread_test(thread_number):
    for ii in range(5):
        post('multi_thread_test', { 'id': thread_number * 10000 + ii, 'subject': 'World' })

threads = list()

for i in range(5):
    t = threading.Thread(target=try_multi_thread_test, args=(i,), daemon=True)
    threads.append(t)
    t.start()


time.sleep(30)

