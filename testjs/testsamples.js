'use strict';
var d = require('../libjs/durable');

d.ruleset('test', function() {
    // antecedent
    whenAll: m.subject == 'World'
    // consequent
    run: console.log('Hello ' + m.subject)
});

d.post('test', {subject: 'World'});

d.ruleset('risk0', function() {
    whenAll: {
        first = m.t == 'purchase'
        second = m.location != first.location
    }
    run: console.log('risk0 fraud detected ->' + first.location + ', ' + second.location)
});

d.post('risk0', {t: 'purchase', location: 'US'});
d.post('risk0', {t: 'purchase', location: 'CA'});

d.ruleset('indistinct', function() {
    whenAll: {
        first = m.amount > 10
        second = m.amount > first.amount * 2
        third = m.amount > (first.amount + second.amount) / 2
    }
    distinct: false
    run: {
        console.log('indistinct detected -> ' + first.amount);
        console.log('               -> ' + second.amount);
        console.log('               -> ' + third.amount);
    }
});

d.post('indistinct', {amount: 200});
d.post('indistinct', {amount: 500});
d.post('indistinct', {amount: 1000});

d.ruleset('distinct', function() {
    whenAll: {
        first = m.amount > 10
        second = m.amount > first.amount * 2
        third = m.amount > (first.amount + second.amount) / 2
    }
    run: {
        console.log('distinct detected -> ' + first.amount);
        console.log('               -> ' + second.amount);
        console.log('               -> ' + third.amount);
    }
});

d.post('distinct', {amount: 50});
d.post('distinct', {amount: 200});
d.post('distinct', {amount: 251});

d.ruleset('expense0', function() {
    whenAll: m.subject == 'approve' || m.subject == 'ok'
    run: console.log('expense0 Approved');
});

d.post('expense0', { subject: 'approve' }, function(err, state){});

d.ruleset('match', function() {
    whenAll: m.url.matches('(https?://)?([%da-z.-]+)%.[a-z]{2,6}(/[%w_.-]+/?)*') 
    run: console.log('match url ' + m.url)
});

d.post('match', {url: 'https://github.com'});
d.post('match', {url: 'http://github.com/jruizgit/rul!es'}, function(err, state){ console.log('match: ' + err.message)});
d.post('match', {url: 'https://github.com/jruizgit/rules/reference.md'});
d.post('match', {url: '//rules'}, function(err, state){ console.log('match: ' + err.message)});
d.post('match', {url: 'https://github.c/jruizgit/rules'}, function(err, state){ console.log('match: ' + err.message)});

d.ruleset('strings', function() {
    whenAll: m.subject.matches('hello.*')
    run: console.log('string starts with hello: ' + m.subject)

    whenAll: m.subject.imatches('.*hello')
    run: console.log('string ends with hello: ' + m.subject)

    whenAll: m.subject.imatches('.*hello.*')
    run: console.log('string contains hello (case insensitive): ' + m.subject)
});

d.assert('strings', { subject: 'HELLO world' });
d.assert('strings', { subject: 'world hello' });
d.assert('strings', { subject: 'hello hi' });
d.assert('strings', { subject: 'has Hello string' });
d.assert('strings', { subject: 'does not match' }, function(err, state){ console.log('string: ' + err.message)});

d.ruleset('risk2_0', function() {
    whenAll: {
        first = m.t == 'deposit'
        none(m.t == 'balance')
        third = m.t == 'withrawal'
        fourth = m.t == 'chargeback'
    }
    run: console.log('risk2_0 fraud detected sid: ' + s.sid + ' result: ' + first.t + ' ' + third.t + ' ' + fourth.t);
});

d.assert('risk2_0', {t: 'deposit'});
d.assert('risk2_0', {t: 'withrawal'});
d.assert('risk2_0', {t: 'chargeback'});

d.assert('risk2_0', {sid: 1, t: 'balance'});
d.assert('risk2_0', {sid: 1, t: 'deposit'});
d.assert('risk2_0', {sid: 1, t: 'withrawal'});
d.assert('risk2_0', {sid: 1, t: 'chargeback'});
d.retract('risk2_0', {sid: 1, t: 'balance'});


d.assert('risk2_0', {sid: 2, t: 'deposit'});
d.assert('risk2_0', {sid: 2, t: 'withrawal'});
d.assert('risk2_0', {sid: 2, t: 'chargeback'});
d.assert('risk2_0', {sid: 2, t: 'balance'});

d.ruleset('expense2', function() {
    whenAll: m.amount < 100
    count: 3
    run: console.log('expense2 approved ' + JSON.stringify(m));

    whenAll: {
        expense = m.amount >= 100
        approval = m.review == true
    }
    cap: 2
    run: console.log('expense2 rejected ' + JSON.stringify(m));

});

d.postEvents('expense2', [{ amount: 10 },
                          { amount: 20 },
                          { amount: 100 },
                          { amount: 30 },
                          { amount: 200 },
                          { amount: 400 }]);

d.assert('expense2', { review: true })

d.ruleset('flow0', function() {
    whenAll: s.state == 'start'
    run: {
        s.state = 'next';
        console.log('flow0 start');
    }

    whenAll: s.state == 'next'
    run: {
        s.state = 'last';
        console.log('flow0 next');
    }

    whenAll: s.state == 'last'
    run: {
        s.state = 'end';
        console.log('flow0 last');
        deleteState();
    } 
});

d.updateState('flow0', {state: 'start'});

d.ruleset('flow', function() {
    whenAll: m.action == 'start'
    run: throw 'Unhandled Exception!'

    whenAll: +s.exception
    run: {
        console.log('flow expected ' + s.exception);
        delete(s.exception); 
    }

    whenStart: {
        
    }
});

d.post('flow', { action: 'start' });

d.statechart('expense3', function() {
    input: {
        to: 'denied'
        whenAll: m.subject == 'approve' && m.amount > 1000
        run: console.log('expense3: Denied amount: ' + m.amount)

        to: 'pending'
        whenAll: m.subject == 'approve' && m.amount <= 1000
        run: console.log('expense3: sid ' + m.sid + ' requesting approve amount: ' + m.amount);
    }

    pending: {
        to: 'approved'
        whenAll: m.subject == 'approved'
        run: console.log('expense3: Expense approved for sid ' + m.sid)

        to: 'denied'
        whenAll: m.subject == 'denied'
        run: console.log('expense3: Expense denied for sid ' + m.sid)
    }
    
    denied: {}
    approved: {}
});

d.post('expense3', { subject: 'approve', amount: 100 });
d.post('expense3', { subject: 'approved' });
d.post('expense3', { sid: 1, subject: 'approve', amount: 100 });
d.post('expense3', { sid: 1, subject: 'denied' });
d.post('expense3', { sid: 2, subject: 'approve', amount: 10000 });

d.statechart('worker', function() {
    work: {
        enter: {
            to: 'process'
            whenAll: m.subject == 'enter'
            run: console.log('worker start process')
        }

        process: {
            to: 'process'
            whenAll: m.subject == 'continue'
            run: console.log('worker continue processing')
        }
    
        to: 'canceled'
        pri: 1
        whenAll: m.subject == 'cancel'
        run: console.log('worker cancel process')
    }

    canceled: {}
});

d.post('worker', { subject: 'enter' });
d.post('worker', { id: 1, subject: 'continue' });
d.post('worker', { id: 2, subject: 'continue' });
d.post('worker', { subject: 'cancel' });

d.flowchart('expense4', function() {
    input: {
        request: m.subject == 'approve' && m.amount <= 1000 
        deny:  m.subject == 'approve' && m.amount > 1000
    }

    request: {
        run: console.log('expense4: Requesting approve for ' + m.sid + ' for ' + m.amount)
        approve: m.subject == 'approved'
        deny: m.subject == 'denied'
        self: m.subject == 'retry'
    }

    approve: {
        run: console.log('expense4: Expense approved for ' + m.sid)
    }

    deny: {
        run: console.log('expense4: Expense denied for ' + m.sid)
    }
});


d.post('expense4', { subject: 'approve', amount: 100 });
d.post('expense4', { subject: 'retry' });
d.post('expense4', { subject: 'approved' });
d.post('expense4', {sid: 1, subject: 'approve', amount: 100});
d.post('expense4', {sid: 1, subject: 'denied'});
d.post('expense4', {sid: 2, subject: 'approve', amount: 10000});

d.ruleset('bookstore', function() {
    // this rule will trigger for events with status
    whenAll: +m.status
    run: console.log('bookstore reference ' + m.reference + ' status ' + m.status)

    whenAll: +m.name
    run: { 
        console.log('bookstore added: ' + m.name);
    }

    // this rule will be triggered when the fact is retracted
    whenAll: none(+m.name)
    run: console.log('bookstore no books');
});

// will not throw because the fact assert was successful 
d.assert('bookstore', {
    name: 'The new book',
    seller: 'bookstore',
    reference: '75323',
    price: 500
});


// will throw MessageObservedError because the fact has already been asserted 
try {
    d.assert('bookstore', {
        reference: '75323',
        name: 'The new book',
        price: 500,
        seller: 'bookstore'
    });
} catch (err) {
    console.log('bookstore: ' + err.message);   
}

// will not throw because a new event is being posted
d.post('bookstore', {
    reference: '75323',
    status: 'Active'
});

// will not throw because a new event is being posted
d.post('bookstore', {
    reference: '75323',
    status: 'Active'
});


d.ruleset('attributes', function() {
    whenAll: m.amount < 300
    pri: 3 
    run: console.log('attributes P3 ->' + m.amount);
        
    whenAll: m.amount < 200
    pri: 2
    run: console.log('attributes P2 ->' + m.amount);     
            
    whenAll: m.amount < 100
    pri: 1
    run: console.log('attributes P1 ->' + m.amount);
});

d.assert('attributes', {amount: 50});
d.assert('attributes', {amount: 150});
d.assert('attributes', {amount: 250});

d.ruleset('expense5', function() {
    // use the '.' notation to match properties in nested objects
    whenAll: {
        bill = m.t == 'bill' && m.invoice.amount > 50
        account = m.t == 'account' && m.payment.invoice.amount == bill.invoice.amount
    }
    run: {
        console.log('expense5 bill amount ->' + bill.invoice.amount);
        console.log('expense5 account payment amount ->' + account.payment.invoice.amount);
    }
});

// one level of nesting
d.post('expense5', {t: 'bill', invoice: {amount: 100}});  

// two levels of nesting
d.post('expense5', {t: 'account', payment: {invoice: {amount: 100}}}); 

d.ruleset('animal', function() {
    whenAll: {
        first = m.predicate == 'eats' && m.object == 'flies' 
        m.predicate == 'lives' && m.object == 'water' && m.subject == first.subject
    }
    run: assert({ subject: first.subject, predicate: 'is', object: 'frog' })

    whenAll: {
        first = m.predicate == 'eats' && m.object == 'flies' 
        m.predicate == 'lives' && m.object == 'land' && m.subject == first.subject
    }
    run: assert({ subject: first.subject, predicate: 'is', object: 'chameleon' })

    whenAll: m.predicate == 'eats' && m.object == 'worms' 
    run: assert({ subject: m.subject, predicate: 'is', object: 'bird' })

    whenAll: m.predicate == 'is' && m.object == 'frog'
    run: assert({ subject: m.subject, predicate: 'is', object: 'green'})

    whenAll: m.predicate == 'is' && m.object == 'chameleon'
    run: assert({ subject: m.subject, predicate: 'is', object: 'green'})

    whenAll: m.predicate == 'is' && m.object == 'bird' 
    run: assert({ subject: m.subject, predicate: 'is', object: 'black'})

    whenAll: +m.subject
    run: console.log('animal fact: ' + m.subject + ' ' + m.predicate + ' ' + m.object)
});

d.assert('animal', { subject: 'Kermit', predicate: 'eats', object: 'flies'});
d.assert('animal', { subject: 'Kermit', predicate: 'lives', object: 'water'});
d.assert('animal', { subject: 'Greedy', predicate: 'eats', object: 'flies'});
d.assert('animal', { subject: 'Greedy', predicate: 'lives', object: 'land'});
d.assert('animal', { subject: 'Tweety', predicate: 'eats', object: 'worms'});

d.ruleset('risk5', function() {
    
    // compares properties in the same event, evaluated in alpha tree (node.js)
    whenAll: {
        m.debit > 2 * m.credit
    }
    run: console.log('risk5 debit ' + m.debit + ' more than twice the credit ' + m.credit)
   
    // correlates two events, evaluated in the beta tree (redis)
    whenAll: {
        first = m.amount > 100
        second = m.amount > first.amount + m.amount / 2
    }
    run: {
        console.log('risk5 fraud detected -> ' + first.amount);
        console.log('risk5 fraud detected -> ' + second.amount);
    }
});

d.post('risk5', { debit: 220, credit: 100 });
d.post('risk5', { debit: 150, credit: 100 });
d.post('risk5', { amount: 200 });
d.post('risk5', { amount: 500 });

d.ruleset('risk6', function() {
    
    // matching primitive array
    whenAll: {
        m.payments.allItems(item > 2000)
    }
    run: console.log('risk6 should not match ' + m.payments)

    // matching primitive array
    whenAll: {
        m.payments.allItems(item > 1000)
    }
    run: console.log('risk6 fraud 1 detected ' + m.payments)

    // matching object array
    whenAll: {
        m.payments.allItems(item.amount < 250 || item.amount >= 300)
    }
    run: console.log('risk6 fraud 2 detected ' + JSON.stringify(m.payments))
   
    // pattern matching string array
    whenAll: {
        m.cards.anyItem(item.matches('three.*'))
    }
    run: console.log('risk6 fraud 3 detected ' + m.cards)

    // matching nested arrays
    whenAll: {
        m.payments.anyItem(item.allItems(item < 100))
    }
    run: console.log('risk6 fraud 4 detected ' + JSON.stringify(m.payments))

    // matching array and value
    whenAll: {
        m.payments.allItems(item > 100) && m.cash == true
    }
    run: console.log('risk6 fraud 5 detected ' + JSON.stringify(m))

    whenAll: {
        m.field == 1 && m.payments.allItems(item.allItems(item > 100 && item < 1000))
    }
    run: console.log('risk6 fraud 6 detected ' + JSON.stringify(m.payments))

    whenAll: {
        m.field == 1 && m.payments.allItems(item.anyItem(item > 100 || item < 50))
    }
    run: console.log('risk6 fraud 7 detected ' + JSON.stringify(m.payments))

    whenAll: { 
        m.payments.anyItem(~item.field1 && item.field2 == 2) 
    }
    run: console.log('risk6 fraud 8 detected ' + JSON.stringify(m.payments));

});

d.post('risk6', { payments: [ 2500, 150, 450 ]}, function(err, state) {console.log('risk6: ' + err.message)});
d.post('risk6', { payments: [ 1500, 3500, 4500 ]});
d.post('risk6', { payments: [{ amount: 200 }, { amount: 300 }, { amount: 400 }]});
d.post('risk6', { cards: ['one card', 'two cards', 'three cards']});
d.post('risk6', { payments: [[ 10, 20, 30 ], [ 30, 40, 50 ], [ 10, 20 ]]});
d.post('risk6', { payments: [ 150, 350, 450 ], cash : true});    
d.post('risk6', { field: 1, payments: [ [ 200, 300 ], [ 150, 200 ] ]}); 
d.post('risk6', { field: 1, payments: [ [ 20, 180 ], [ 90, 190 ] ]}); 
d.post('risk6', { payments: [{field2: 2}]}); 
d.post('risk6', { payments: [{field2: 1}]}, function(err, state) {console.log('risk6: ' + err.message)}); 
d.post('risk6', { payments: [{field1: 1, field2: 2}]}, function(err, state) {console.log('risk6: ' + err.message)});  
d.post('risk6', { payments: [{field1: 1, field2: 1}]}, function(err, state) {console.log('risk6: ' + err.message)});  

d.ruleset('expense1', function() {
    whenAny: {
        whenAll: {
            first = m.subject == 'approve'
            second = m.amount == 1000
        }
        whenAll: { 
            third = m.subject == 'jumbo'
            fourth = m.amount == 10000
        }
    }
    run: {
        if (first) {
            console.log('expense1 Approved ' + first.subject + ' ' + second.amount);     
        } else {
            console.log('expense1 Approved ' + third.subject + ' ' + fourth.amount);        
        }
    }
});

d.post('expense1', {subject: 'approve'});
d.post('expense1', {amount: 1000});
d.post('expense1', {subject: 'jumbo'});
d.post('expense1', {amount: 10000});

d.ruleset('expense6', function() {
    whenAny: {
        first = m.subject == 'approve' || m.subject == 'jumbo'
        second = m.amount <= 1000
    }   
    run: {
        console.log('expense 6 Approved')
        if (first) {
            console.log('expense6 ' + first.subject);     
        } 

        if (second) {
            console.log('expense6 ' + second.amount);     
        }
    }
});

d.post('expense6', {subject: 'approve'});
d.assert('expense6', {amount: 1000});
d.assert('expense6', {subject: 'jumbo'});
d.post('expense6', {amount: 100});

d.ruleset('expense7', function() {
    whenAll: {
        whenAny: {
            first = m.subject == 'approve'
            second = m.amount == 1000
        }
        whenAny: { 
            third = m.subject == 'jumbo'
            fourth = m.amount == 10000
        }
    }
    run: {
        console.log('expense 7 Approved')
        if (first) {
            console.log('expense7 ' + first.subject);     
        } 

        if (second) {
            console.log('expense7 ' + second.amount);     
        }

        if (third) {
            console.log('expense7 ' + third.subject);     
        } 

        if (fourth) {
            console.log('expense7 ' + fourth.amount);     
        } 
    }

});

d.post('expense7', {subject: 'approve'});
d.assert('expense7', {amount: 1000});
d.assert('expense7', {subject: 'jumbo'});
d.post('expense7', {amount: 10000});

d.ruleset('timer1', function() {
    
    whenAll: m.subject == 'start'
    run: startTimer('MyTimer', 5);

    whenAll: {
        timeout('MyTimer')    
    }
    run: {
        console.log('timer1 timeout'); 
    }
});


d.post('timer1', {subject: 'start'});

d.ruleset('timer2', function() {
    whenAny: {
        whenAll: s.count == 0
        // will trigger when MyTimer expires
        whenAll: {
            s.count < 5 
            timeout('MyTimer')
        }
    }
    run: {
        s.count += 1;
        // MyTimer will expire in 1 second
        startTimer('MyTimer', 1);
        console.log('timer2 Pusle ->' + new Date());
    }

    whenAll: {
        m.cancel == true
    }
    run: {
        cancelTimer('MyTimer');
        console.log('timer2 canceled timer');
    }
});

d.updateState('timer2', { count: 0 }); 

d.statechart('risk3', function() {
    start: {
        to: 'meter'
        run: startTimer('RiskTimer', 5)
    }

    meter: {
        to: 'fraud'
        whenAll: message = m.amount > 100
        count: 3
        run: m.forEach(function(e, i){ console.log('risk3 ' + JSON.stringify(e.message)) })

        to: 'exit'
        whenAll: timeout('RiskTimer')
        run: console.log('risk3 exit for ' + c.s.sid)
    }

    fraud: {}
    exit:{}
});

// three events in a row will trigger the fraud rule
d.post('risk3', { amount: 200 }); 
d.post('risk3', { amount: 300 }); 
d.post('risk3', { amount: 400 }); 

// two events will exit after 5 seconds
d.post('risk3', { sid: 1, amount: 500 }); 
d.post('risk3', { sid: 1, amount: 600 }); 

// // curl -H "content-type: application/json" -X POST -d '{"cancel": true}' http://localhost:5000/timer2/events

d.statechart('risk4', function() {
    start: {
        to: 'meter'
        // will start a manual reset timer
        run: startTimer('VelocityTimer', 5, true)
    }

    meter: {
        to: 'meter'
        whenAll: { 
            message = m.amount > 100
            timeout('VelocityTimer')
        }
        cap: 100
        run: {
            console.log('risk4 velocity: ' + m.length + ' events in 5 seconds');
            // resets and restarts the manual reset timer
            startTimer('VelocityTimer', 5, true);
        }  

        to: 'meter'
        whenAll: {
            timeout('VelocityTimer')
        }
        run: {
            console.log('risk4 velocity: no events in 5 seconds');
            cancelTimer('VelocityTimer');
        }
    }
});

d.post('risk4', { amount: 200 }); 
d.post('risk4', { amount: 300 }); 
d.post('risk4', { amount: 500 }); 
d.post('risk4', { amount: 600 }); 


// // curl -H "content-type: application/json" -X POST -d '{"amount": 200}' http://localhost:5000/risk4/events

d.ruleset('flow1', function() {
    whenAll: s.state == 'first'
    // runAsync labels an async action
    runAsync: {
        setTimeout(function() {
            s.state = 'second';
            console.log('flow1 first completed');
            
            // completes the async action
            complete();
        }, 3000);
    }

    whenAll: s.state == 'second'
    runAsync: {
        setTimeout(function() {
            console.log('flow1 second completed');
            s.state = 'last'
            complete();
        }, 10000);
        
        // overrides the 5 second default abandon timeout
        return 15;
    }
});

d.updateState('flow1', { state: 'first' });

//d.runAll();
