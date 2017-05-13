'use strict';
var d = require('../libjs/durable');

d.ruleset('test', function() {
    // antecedent
    whenAll: m.subject == 'World'
    // consequent
    run: console.log('Hello ' + m.subject)
});

d.ruleset('risk0', function() {
    whenAll: {
        first = m.t == 'purchase'
        second = m.location != first.location
    }

    run: console.log('fraud detected ->' + first.location + ', ' + second.location)
   
    whenStart: {
        post('risk0', {t: 'purchase', location: 'US'});
        post('risk0', {t: 'purchase', location: 'CA'});
    }
});

d.ruleset('risk1', function() {
    whenAll: {
        first = m.amount > 100
        second = m.amount > first.amount * 2
        third = m.amount > first.amount + second.amount
    }
    run: {
        console.log('fraud detected -> ' + first.amount);
        console.log('               -> ' + second.amount);
        console.log('               -> ' + third.amount);
    }

    whenStart: {
        host.post('risk1', {amount: 200});
        host.post('risk1', {amount: 500});
        host.post('risk1', {amount: 1000});
    }
});

d.ruleset('risk2', function() {
    whenAll: {
        first = m.t == 'deposit'
        none(m.t == 'balance')
        third = m.t == 'withrawal'
        fourth = m.t == 'chargeback'
    }
    run: console.log('fraud detected ' + first.t + ' ' + third.t + ' ' + fourth.t);

    whenStart: {
        post('risk2', {t: 'deposit'});
        post('risk2', {t: 'withrawal'});
        post('risk2', {t: 'chargeback'});
    }
});


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
            console.log('Approved ' + first.subject + ' ' + second.amount);     
        } else {
            console.log('Approved ' + third.subject + ' ' + fourth.amount);        
        }
    }

    whenStart: {
        post('expense1', {subject: 'approve'});
        post('expense1', {amount: 1000});
        post('expense1', {subject: 'jumbo'});
        post('expense1', {amount: 10000});
    }
});


d.ruleset('expense0', function() {
    whenAll: m.subject == 'approve' || m.subject == 'ok'
    run: console.log('Approved')
    
    whenStart: post('expense0', { subject: 'approve' })
});

d.ruleset('match', function() {
    whenAll: m.url.matches('(https?://)?([%da-z.-]+)%.[a-z]{2,6}(/[%w_.-]+/?)*') 
    run: console.log('match url ' + m.url)
        
    whenStart: {
        post('match', {url: 'https://github.com'});
        post('match', {url: 'http://github.com/jruizgit/rul!es'});
        post('match', {url: 'https://github.com/jruizgit/rules/reference.md'});
        post('match', {url: '//rules'});
        post('match', {url: 'https://github.c/jruizgit/rules'});
    }
});

d.ruleset('strings', function() {
    whenAll: m.subject.matches('hello.*')
    run: console.log('string starts with hello: ' + m.subject)

    whenAll: m.subject.imatches('.*hello')
    run: console.log('string ends with hello: ' + m.subject)

    whenAll: m.subject.imatches('.*hello.*')
    run: console.log('string contains hello (case insensitive): ' + m.subject)

    whenStart: {
        assert('strings', { subject: 'HELLO world' });
        assert('strings', { subject: 'world hello' });
        assert('strings', { subject: 'hello hi' });
        assert('strings', { subject: 'has Hello string' });
        assert('strings', { subject: 'does not match' });
    }
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
       
    whenStart: {
        assert('attributes', {amount: 50});
        assert('attributes', {amount: 150});
        assert('attributes', {amount: 250});
    }
});

d.ruleset('flow0', function() {
    whenAll: s.state == 'start'
    run: {
        s.state = 'next';
        console.log('start');
    }

    whenAll: s.state == 'next'
    run: {
        s.state = 'last';
        console.log('next');
    }

    whenAll: s.state == 'last'
    run: {
        s.state = 'end';
        console.log('last');
        deleteState();
    }

    whenStart: patchState('flow0', {state: 'start'})
});

d.ruleset('expense2', function() {
    whenAll: m.amount < 100
    count: 3
    run: console.log('approved ' + JSON.stringify(m));

    whenAll: {
        expense = m.amount >= 100
        approval = m.review == true
    }
    cap: 2
    run: console.log('rejected ' + JSON.stringify(m));

    whenStart: {
        postBatch('expense2', { amount: 10 },
                             { amount: 20 },
                             { amount: 100 },
                             { amount: 30 },
                             { amount: 200 },
                             { amount: 400 });
        assert('expense2', { review: true })
    }
});
    
d.ruleset('flow1', function() {
    whenAll: s.state == 'first'
    // runAsync labels an async action
    runAsync: {
        setTimeout(function() {
            s.state = 'second';
            console.log('first completed');
            
            // completes the async action
            complete();
        }, 3000);
    }

    whenAll: s.state == 'second'
    runAsync: {
        setTimeout(function() {
            console.log('second completed');
            complete();
        }, 6000);
        
        // overrides the 5 second default abandon timeout
        return 10;
    }

    whenStart: {
        patchState('flow1', { state: 'first' });
    }
});

d.ruleset('flow', function() {
    whenAll: m.action == 'start'
    run: throw 'Unhandled Exception!'

    whenAll: +s.exception
    run: {
        console.log(s.exception);
        delete(s.exception); 
    }

    whenStart: {
        post('flow', { action: 'start' });
    }
});

d.ruleset('timer', function() {
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
        // MyTimer will expire in 5 seconds
        startTimer('MyTimer', 5);
        console.log('Pusle ->' + new Date());
    }

    whenAll: {
        m.cancel == true
    }
    run: {
        cancelTimer('MyTimer');
        console.log('canceled timer');
    }

    whenStart: {
        patchState('timer', { count: 0 }); 
    }
});

// curl -H "content-type: application/json" -X POST -d '{"cancel": true}' http://localhost:5000/timer/events

d.statechart('risk3', function() {
    start: {
        to: 'meter'
        run: startTimer('RiskTimer', 5)
    }

    meter: {
        to: 'fraud'
        whenAll: message = m.amount > 100
        count: 3
        run: m.forEach(function(e, i){ console.log(JSON.stringify(e.message)) });

        to: 'exit'
        whenAll: timeout('RiskTimer')
        run: console.log('exit')    
    }

    fraud: {}
    exit:{}

    whenStart: {
        // three events in a row will trigger the fraud rule
        post('risk3', { amount: 200 }); 
        post('risk3', { amount: 300 }); 
        post('risk3', { amount: 400 }); 

        // two events will exit after 5 seconds
        post('risk3', { sid: 1, amount: 500 }); 
        post('risk3', { sid: 1, amount: 600 }); 
        
    }
});


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
            console.log('velocity: ' + m.length + ' events in 5 seconds');
            // resets and restarts the manual reset timer
            resetTimer('VelocityTimer');
            startTimer('VelocityTimer', 5, true);
        }  

        to: 'meter'
        whenAll: {
            timeout('VelocityTimer')
        }
        run: {
            console.log('velocity: no events in 5 seconds');
            resetTimer('VelocityTimer');
            startTimer('VelocityTimer', 5, true);
        }
    }

    whenStart: {
        // the velocity will 4 events in 5 seconds
        post('risk4', { amount: 200 }); 
        post('risk4', { amount: 300 }); 
        post('risk4', { amount: 50 }); 
        post('risk4', { amount: 500 }); 
        post('risk4', { amount: 600 }); 
    }
});

// curl -H "content-type: application/json" -X POST -d '{"amount": 200}' http://localhost:5000/risk4/events

d.statechart('expense3', function() {
    input: {
        to: 'denied'
        whenAll: m.subject == 'approve' && m.amount > 1000
        run: console.log('Denied amount: ' + m.amount)

        to: 'pending'
        whenAll: m.subject == 'approve' && m.amount <= 1000
        run: console.log('Requesting approve amount: ' + m.amount);
    }

    pending: {
        to: 'approved'
        whenAll: m.subject == 'approved'
        run: console.log('Expense approved')

        to: 'denied'
        whenAll: m.subject == 'denied'
        run: console.log('Expense denied')
    }
    
    denied: {}
    approved: {}
    
    whenStart: {
        post('expense3', { subject: 'approve', amount: 100 });
        post('expense3', { subject: 'approved' });
        post('expense3', { sid: 1, subject: 'approve', amount: 100 });
        post('expense3', { sid: 1, subject: 'denied' });
        post('expense3', { sid: 2, subject: 'approve', amount: 10000 });
    }
});

d.statechart('worker', function() {
    work: {
        enter: {
            to: 'process'
            whenAll: m.subject == 'enter'
            run: console.log('start process')
        }

        process: {
            to: 'process'
            whenAll: m.subject == 'continue'
            run: console.log('continue processing')
        }
    
        to: 'canceled'
        pri: 1
        whenAll: m.subject == 'cancel'
        run: console.log('cancel process')
    }

    canceled: {}
    whenStart: {
        post('worker', { subject: 'enter' });
        post('worker', { subject: 'continue' });
        post('worker', { subject: 'continue' });
        post('worker', { subject: 'cancel' });
    }
});

d.flowchart('expense', function() {
    input: {
        request: m.subject == 'approve' && m.amount <= 1000 
        deny:  m.subject == 'approve' && m.amount > 1000
    }

    request: {
        run: console.log('Requesting approve')
        approve: m.subject == 'approved'
        deny: m.subject == 'denied'
        self: m.subject == 'retry'
    }

    approve: {
        run: console.log('Expense approved')
    }

    deny: {
        run: console.log('Expense denied')
    }

    whenStart: {
        post('expense', { subject: 'approve', amount: 100 });
        post('expense', { subject: 'retry' });
        post('expense', { subject: 'approved' });
        post('expense', {sid: 1, subject: 'approve', amount: 100});
        post('expense', {sid: 1, subject: 'denied'});
        post('expense', {sid: 2, subject: 'approve', amount: 10000});
    }
});

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
    run: console.log('fact: ' + m.subject + ' ' + m.predicate + ' ' + m.object)

    whenStart: {
        assert('animal', { subject: 'Kermit', predicate: 'eats', object: 'flies'});
        assert('animal', { subject: 'Kermit', predicate: 'lives', object: 'water'});
        assert('animal', { subject: 'Greedy', predicate: 'eats', object: 'flies'});
        assert('animal', { subject: 'Greedy', predicate: 'lives', object: 'land'});
        assert('animal', { subject: 'Tweety', predicate: 'eats', object: 'worms'});
    }
});

d.ruleset('expense4', function() {
    // use the '.' notation to match properties in nested objects
    whenAll: {
        bill = m.t == 'bill' && m.invoice.amount > 50
        account = m.t == 'account' && m.payment.invoice.amount == bill.invoice.amount
    }
    run: {
        console.log('bill amount ->' + bill.invoice.amount);
        console.log('account payment amount ->' + account.payment.invoice.amount);
    }

    whenStart: {
        // one level of nesting
        post('expense4', {t: 'bill', invoice: {amount: 100}});  

        // two levels of nesting
        post('expense4', {t: 'account', payment: {invoice: {amount: 100}}}); 
    }
});


d.runAll();
