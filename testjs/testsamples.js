'use strict';
var d = require('../libjs/durable');

d.ruleset('match1', function() {
    whenAll: m.subject.matches('a+(bc|de?)*')
    run: console.log('match 1 ' + m.subject)
});

d.post('match1', {id: 1, sid: 1, subject: 'abcbc'});
d.post('match1', {id: 2, sid: 1, subject: 'addd'});
d.post('match1', {id: 3, sid: 1, subject: 'ddd'}, function(err, state){ console.log('match 1 ' + err.message)});
d.post('match1', {id: 4, sid: 1, subject: 'adee'}, function(err, state){ console.log('match 1 ' + err.message)});

d.ruleset('match2', function() {
    whenAll: m.subject.matches('[0-9]+-[a-z]+')
    run: console.log('match 2 ' + m.subject)
});

d.post('match2', {id: 1, sid: 1, subject: '0-a'});
d.post('match2', {id: 2, sid: 1, subject: '43-bcd'});
d.post('match2', {id: 3, sid: 1, subject: 'a-3'}, function(err, state){ console.log('match 2 ' + err.message)});
d.post('match2', {id: 4, sid: 1, subject: '0a'}, function(err, state){ console.log('match 2 ' + err.message)});

d.ruleset('match3', function() {
    whenAll: m.subject.matches('[adfzx-]+[5678]+')
    run: console.log('match 3 ' + m.subject)
});

d.post('match3', {id: 1, sid: 1, subject: 'ad-567'});
d.post('match3', {id: 2, sid: 1, subject: 'ac-78'}, function(err, state){ console.log('match 3 ' + err.message)});
d.post('match3', {id: 3, sid: 1, subject: 'zx12'}, function(err, state){ console.log('match 3 ' + err.message)});
d.post('match3', {id: 4, sid: 1, subject: 'xxxx-8888'});

d.ruleset('match4', function() {
    whenAll: m.subject.matches('[z-a]+-[9-0]+')
    run: console.log('match 4 ' + m.subject)
});

d.post('match4', {id: 1, sid: 1, subject: 'ed-56'});
d.post('match4', {id: 2, sid: 1, subject: 'ac-ac'}, function(err, state){ console.log('match 4 ' + err.message)});
d.post('match4', {id: 3, sid: 1, subject: 'az-90'});
d.post('match4', {id: 4, sid: 1, subject: 'AZ-90'}, function(err, state){ console.log('match 4 ' + err.message)});

d.ruleset('match5', function() {
    whenAll: m.subject.matches('.+b')
    run: console.log('match 5 ' + m.subject)
});

d.post('match5', {id: 1, sid: 1, subject: 'ab'});
d.post('match5', {id: 2, sid: 1, subject: 'adfb0'}, function(err, state){ console.log('match 5 ' + err.message)});
d.post('match5', {id: 3, sid: 1, subject: 'abbbcd'}, function(err, state){ console.log('match 5 ' + err.message)});
d.post('match5', {id: 4, sid: 1, subject: 'xybz.b'});

d.ruleset('match6', function() {
    whenAll: m.subject.matches('[a-z]{3}')
    run: console.log('match 6 ' + m.subject)
});

d.post('match6', {id: 1, sid: 1, subject: 'ab'}, function(err, state){ console.log('match 6 ' + err.message)});
d.post('match6', {id: 2, sid: 1, subject: 'bbb'});
d.post('match6', {id: 3, sid: 1, subject: 'azc'});
d.post('match6', {id: 4, sid: 1, subject: 'abbbcd'}, function(err, state){ console.log('match 6 ' + err.message)});

d.ruleset('match7', function() {
    whenAll: m.subject.matches('[a-z]{2,4}')
    run: console.log('match 7 ' + m.subject)
});

d.post('match7', {id: 1, sid: 1, subject: 'ar'});
d.post('match7', {id: 2, sid: 1, subject: 'bxbc'});
d.post('match7', {id: 3, sid: 1, subject: 'a'}, function(err, state){ console.log('match 7 ' + err.message)});
d.post('match7', {id: 4, sid: 1, subject: 'abbbc'}, function(err, state){ console.log('match 7 ' + err.message)});

d.ruleset('match8', function() {
    whenAll: m.subject.matches('(a|bc){2,}')
    run: console.log('match 8 ' + m.subject)
});

d.post('match8', {id: 1, sid: 1, subject: 'aa'});
d.post('match8', {id: 2, sid: 1, subject: 'abcbc'});
d.post('match8', {id: 3, sid: 1, subject: 'bc'}, function(err, state){ console.log('match 8 ' + err.message)});
d.post('match8', {id: 4, sid: 1, subject: 'a'}, function(err, state){ console.log('match 8 ' + err.message)});

d.ruleset('match9', function() {
    whenAll: m.subject.matches('a{1,2}')
    run: console.log('match 9 ' + m.subject)
});

d.post('match9', {id: 1, sid: 1, subject: 'a'});
d.post('match9', {id: 2, sid: 1, subject: 'b'}, function(err, state){ console.log('match 9 ' + err.message)});
d.post('match9', {id: 3, sid: 1, subject: 'aa'});
d.post('match9', {id: 4, sid: 1, subject: 'aaa'}, function(err, state){ console.log('match 9 ' + err.message)});

d.ruleset('match10', function() {
    whenAll: m.subject.matches('cba{1,}')
    run: console.log('match 10 ' + m.subject)
});

d.post('match10', {id: 1, sid: 1, subject: 'cba'});
d.post('match10', {id: 2, sid: 1, subject: 'b'}, function(err, state){ console.log('match 10 ' + err.message)});
d.post('match10', {id: 3, sid: 1, subject: 'cb'}, function(err, state){ console.log('match 10 ' + err.message)});
d.post('match10', {id: 4, sid: 1, subject: 'cbaaa'});

d.ruleset('match11', function() {
    whenAll: m.subject.matches('cba{0,}')
    run: console.log('match 11 ' + m.subject)
});

d.post('match11', {id: 1, sid: 1, subject: 'cb'});
d.post('match11', {id: 2, sid: 1, subject: 'b'}, function(err, state){ console.log('match 11 ' + err.message)});
d.post('match11', {id: 3, sid: 1, subject: 'cbab'}, function(err, state){ console.log('match 11 ' + err.message)});
d.post('match11', {id: 4, sid: 1, subject: 'cbaaa'});

d.ruleset('match12', function() {
    whenAll: m.user.matches('[a-z0-9_-]{3,16}')
    run: console.log('match 12 user ' + m.user)
});

d.post('match12', {id: 1, sid: 1, user: 'git-9'});
d.post('match12', {id: 2, sid: 1, user: 'git_9'});
d.post('match12', {id: 3, sid: 1, user: 'GIT-9'}, function(err, state){ console.log('match 12 ' + err.message)});
d.post('match12', {id: 4, sid: 1, user: '01234567890123456789'}, function(err, state){ console.log('match 12 ' + err.message)});
d.post('match12', {id: 5, sid: 1, user: 'gi'}, function(err, state){ console.log('match 12 ' + err.message)});

d.ruleset('match13', function() {
    whenAll: m.number.matches('#?([a-f0-9]{6}|[a-f0-9]{3})')
    run: console.log('match 13 hex ' + m.number)
});

d.post('match13', {id: 1, sid: 1, number: 'fff'});
d.post('match13', {id: 2, sid: 1, number: '#45'}, function(err, state){ console.log('match 13 ' + err.message)});
d.post('match13', {id: 3, sid: 1, number: 'abcdex'}, function(err, state){ console.log('match 13 ' + err.message)});
d.post('match13', {id: 4, sid: 1, number: '#54ba45'});
d.post('match13', {id: 5, sid: 1, number: '#9999'}, function(err, state){ console.log('match 13 ' + err.message)});

d.ruleset('match14', function() {
    whenAll: m.alias.matches('([a-z0-9_.-]+)@([0-9a-z.-]+)%.([a-z]{2,6})')
    run: console.log('match 14 email ' + m.alias)
});

d.post('match14', {id: 1, sid: 1, alias: 'john_59@mail.mx'});
d.post('match14', {id: 2, sid: 1, alias: 'j#5@mail.com'}, function(err, state){ console.log('match 14 ' + err.message)});
d.post('match14', {id: 3, sid: 1, alias: 'abcd.abcd@mail.mx.com'});
d.post('match14', {id: 4, sid: 1, alias: 'dd@dd.'}, function(err, state){ console.log('match 14 ' + err.message)});
d.post('match14', {id: 5, sid: 1, alias: 'dd@dd.c'}, function(err, state){ console.log('match 14 ' + err.message)});

d.ruleset('match15', function() {
    whenAll: m.url.matches('(https?://)?([%da-z.-]+)%.[a-z]{2,6}(/[%w_.-]+/?)*')
    run: console.log('match 15 url ' + m.url)
});

d.post('match15', {id: 1, sid: 1, url: 'https://github.com'});
d.post('match15', {id: 2, sid: 1, url: 'http://github.com/jruizgit/rul!es'}, function(err, state){ console.log('match 15 ' + err.message)});
d.post('match15', {id: 3, sid: 1, url: 'https://github.com/jruizgit/rules/blob/master/docs/rb/reference.md'});
d.post('match15', {id: 4, sid: 1, url: '//rules'}, function(err, state){ console.log('match 15 ' + err.message)});
d.post('match15', {id: 5, sid: 1, url: 'https://github.c/jruizgit/rules'}, function(err, state){ console.log('match 15 ' + err.message)});

d.ruleset('match16', function() {
    whenAll: m.ip.matches('((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)%.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)')
    run: console.log('match 16 ip ' + m.ip)
});

d.post('match16', {id: 1, sid: 1, ip: '73.60.124.136'});
d.post('match16', {id: 2, sid: 1, ip: '256.60.124.136'}, function(err, state){ console.log('match 16 ' + err.message)});
d.post('match16', {id: 3, sid: 1, ip: '250.60.124.256'}, function(err, state){ console.log('match 16 ' + err.message)});
d.post('match16', {id: 4, sid: 1, ip: '73.60.124'}, function(err, state){ console.log('match 16 ' + err.message)});
d.post('match16', {id: 5, sid: 1, ip: '127.0.0.1'});

d.ruleset('match17', function() {
    whenAll: m.subject.matches('([ab]?){30}%w{30}')
    run: console.log('match 17 ' + m.subject)
});

d.post('match17', {id: 1, sid: 1, subject: 'aaaaaaaaab'}, function(err, state){ console.log('match 17 ' + err.message)});
d.post('match17', {id: 2, sid: 1, subject: 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaab'});
d.post('match17', {id: 3, sid: 1, subject: 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab'});
d.post('match17', {id: 4, sid: 1, subject: 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab'}, function(err, state){ console.log('match 17 ' + err.message)});

d.ruleset('match18', function() {
    whenAll: m.subject.matches('%w+')
    run: console.log('match 18 ' + m.subject)
});

d.post('match18', {id: 1, sid: 1, subject: 'aAzZ5678'});
d.post('match18', {id: 2, sid: 1, subject: '789Az'});
d.post('match18', {id: 3, sid: 1, subject: 'aA`89'}, function(err, state){ console.log('match 18 ' + err.message)});

d.ruleset('match19', function() {
    whenAll: m.subject.matches('%u+')
    run: console.log('match 19 upper ' + m.subject)

    whenAll: m.subject.matches('%l+')
    run: console.log('match 19 lower ' + m.subject)
});

d.post('match19', {id: 1, sid: 1, subject: 'ABCDZ'});
d.post('match19', {id: 2, sid: 1, subject: 'W\u00D2\u00D3'});
d.post('match19', {id: 3, sid: 1, subject: 'abcz'});
d.post('match19', {id: 4, sid: 1, subject: 'wßÿ'});
d.post('match19', {id: 4, sid: 1, subject: 'AbcZ'}, function(err, state){ console.log('match 19 ' + err.message)});

d.ruleset('match20', function() {
    whenAll: m.subject.matches('[^ABab]+')
    run: console.log('match 20 ' + m.subject)
});

d.post('match20', {id: 1, sid: 1, subject: 'ABCDZ'}, function(err, state){ console.log('match 20 ' + err.message)});
d.post('match20', {id: 2, sid: 1, subject: '.;:<>'});
d.post('match20', {id: 3, sid: 1, subject: '^&%()'});
d.post('match20', {id: 4, sid: 1, subject: 'abcz'}, function(err, state){ console.log('match 20 ' + err.message)});

d.ruleset('match21', function() {
    whenAll: m.subject.matches('hello.*')
    run: console.log('match 21 starts with hello: ' + m.subject)

    whenAll: m.subject.matches('.*hello')
    run: console.log('match 21 ends with hello: ' + m.subject)

    whenAll: m.subject.matches('.*hello.*')
    run: console.log('match 21 contains hello: ' + m.subject)

    whenAll: m.subject.matches('.*(error while abc).*')
    run: console.log('match 21 error while: ' + m.subject)

    whenAll: m.subject.matches('.*error error.*')
    run: console.log('match 21 error: ' + m.subject)
});

d.assert('match21', {subject: 'hello world'});
d.assert('match21', {subject: 'world hello'});
d.assert('match21', {subject: 'world hello hello'});
d.assert('match21', {subject: 'has hello string'});
d.assert('match21', {subject: 'error while abc'});
d.assert('match21', {subject: 'error error error'});
d.assert('match21', {subject: 'does not match'}, function(err, state){ console.log('match 21 ' + err.message)});

d.ruleset('match22', function() {
    whenAll: m.subject.imatches('hello.*')
    run: console.log('match 22 starts with case insensitive hello: ' + m.subject)

    whenAll: m.subject.imatches('.*hello')
    run: console.log('match 22 ends with case insensitive hello: ' + m.subject)

    whenAll: m.subject.imatches('.*Hello.*')
    run: console.log('match 22 contains case insensitive hello: ' + m.subject)
});

d.assert('match22', {id: 1, sid: 1, subject: 'HELLO world'});
d.assert('match22', {id: 2, sid: 1, subject: 'world hello'});
d.assert('match22', {id: 3, sid: 1, subject: 'world hello hi'});
d.assert('match22', {id: 4, sid: 1, subject: 'has Hello string'});
d.assert('match22', {id: 5, sid: 1, subject: 'does not match'}, function(err, state){ console.log('match 22 ' + err.message)});

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

try {
    d.assert('strings', { subject: 'does not match' });
} catch (err) {
    console.log('strings: ' + err.message);  
}

d.ruleset('risk2_0', function() {
    whenAll: {
        first = m.t == 'deposit'
        none(m.t == 'balance')
        third = m.t == 'withdrawal'
        fourth = m.t == 'chargeback'
    }
    run: console.log('risk2_0 fraud detected sid: ' + s.sid + ' result: ' + first.t + ' ' + third.t + ' ' + fourth.t);
});

d.assert('risk2_0', {t: 'deposit'});
d.assert('risk2_0', {t: 'withdrawal'});
d.assert('risk2_0', {t: 'chargeback'});

d.assert('risk2_0', {sid: 1, t: 'balance'});
d.assert('risk2_0', {sid: 1, t: 'deposit'});
d.assert('risk2_0', {sid: 1, t: 'withdrawal'});
d.assert('risk2_0', {sid: 1, t: 'chargeback'});
d.retract('risk2_0', {sid: 1, t: 'balance'});


d.assert('risk2_0', {sid: 2, t: 'deposit'});
d.assert('risk2_0', {sid: 2, t: 'withdrawal'});
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
        run: s.status = 'expense3: Denied amount: ' + m.amount

        to: 'pending'
        whenAll: m.subject == 'approve' && m.amount <= 1000
        run: s.status = 'expense3: sid ' + m.sid + ' requesting approve amount: ' + m.amount
    }

    pending: {
        to: 'approved'
        whenAll: m.subject == 'approved'
        run: s.status = 'expense3: Expense approved for sid ' + m.sid

        to: 'denied'
        whenAll: m.subject == 'denied'
        run: s.status = 'expense3: Expense denied for sid ' + m.sid
    }
    
    denied: {}
    approved: {}
});

console.log(d.post('expense3', { subject: 'approve', amount: 100 }).status);
console.log(d.post('expense3', { subject: 'approved' }).status);
console.log(d.post('expense3', { sid: 1, subject: 'approve', amount: 100 }).status);
console.log(d.post('expense3', { sid: 1, subject: 'denied' }).status);
console.log(d.post('expense3', { sid: 2, subject: 'approve', amount: 10000 }).status);

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
    pri: 1
    run: console.log('risk6 fraud 4 detected ' + JSON.stringify(m.payments))

    whenAll: {
        m.payments.anyItem(item > 100) && ~m.cash
    }
    run: console.log('risk6 fraud 5 detected ' + JSON.stringify(m))

    whenAll: {
        m.payments.anyItem(item.amount > 100 && ~item.cash)
    }
    run: console.log('risk6 fraud 6 detected ' + JSON.stringify(m))

    whenAll: {
        m.field == 1 && m.payments.allItems(item.allItems(item > 100 && item < 1000))
    }
    run: console.log('risk6 fraud 7 detected ' + JSON.stringify(m.payments))

    whenAll: {
        m.field == 1 && m.payments.allItems(item.anyItem(item > 100 || item < 50))
    }
    run: console.log('risk6 fraud 8 detected ' + JSON.stringify(m.payments))

    whenAll: {
        m.array.anyItem(+item.malformed)
    }
    run: console.log('risk6 fraud 9 detected ' + JSON.stringify(m));

    whenAll: {
        m.array1.anyItem(item.array2.anyItem(item.field21 == 1) && item.field == 8)
    }
    run: console.log('risk6 fraud 10 detected ' + JSON.stringify(m));

    whenAll: {
        m.array1.anyItem(item.field == 8 && item.array2.anyItem(item.field21 == 1))
    }
    run: console.log('risk6 fraud 11 detected ' + JSON.stringify(m));

    whenAll: { 
        m.payments.anyItem((~item.field1 || ~item.field2) && item.field3 == 2) 
    }
    run: console.log('risk6 fraud 12 detected ' + JSON.stringify(m.payments));

    whenAll: { 
        m.a1.anyItem(~item.field1 && item.a2.anyItem(~item.field2)) 
    }
    run: console.log('risk6 fraud 13 detected ' + JSON.stringify(m));

    whenAll: { 
        m.a4.anyItem(~item.field1 || item.field2 == 3)
    }
    run: console.log('risk6 fraud 14 detected ' + JSON.stringify(m))

    whenAll: {
        m.a5.anyItem(item.a6.anyItem(item.field21 == 1) && ~item.field)
    }
    run: console.log('risk6 fraud 15 detected ' + JSON.stringify(m))
});

d.post('risk6', { payments: [ 2500, 150, 450 ]}, function(err, state) {console.log('risk6: ' + err.message)});
d.post('risk6', { payments: [ 1500, 3500, 4500 ]});
d.post('risk6', { payments: [{ amount: 200 }, { amount: 300 }, { amount: 400 }]});
d.post('risk6', { cards: ['one card', 'two cards', 'three cards']});
d.post('risk6', { payments: [[ 10, 20, 30 ], [ 30, 40, 50 ], [ 10, 20 ]]});
d.post('risk6', { payments: [ 150, 50] }); 
d.post('risk6', { payments: [ 150, 50 ],  cash: true }, function(err, state) {console.log('risk6: ' + err.message)});  
d.post('risk6', { payments: [ { amount: 150 } ]}); 
d.post('risk6', { payments: [ { amount: 150, cash: true } ] }, function(err, state) {console.log('risk6: ' + err.message)});    
d.post('risk6', { field: 1, payments: [ [ 200, 300 ], [ 150, 200 ] ]}); 
d.post('risk6', { field: 1, payments: [ [ 20, 80 ], [ 90, 180 ] ]});   
d.post('risk6', { array:[{ tc: 0 }]}, function(err, state) {console.log('risk6: ' + err.message)});
d.post('risk6', { array:[{ malformed: 0 }]});
d.assert('risk6', { array1: [{ field: 8, array2: [{field21: 1}]}]})
d.assert('risk6', { array1: [{ field: 7, array2: [{field21: 1}]}]}, function(err, state) {console.log('risk6: ' + err.message)})
d.assert('risk6', { array1: [{ field: 8, array2: [{field21: 2}]}]}, function(err, state) {console.log('risk6: ' + err.message)})
d.post('risk6', { payments: [{field3: 2}]}); 
d.post('risk6', { payments: [{field2: 1, field3: 2}]}); 
d.post('risk6', { payments: [{field1: 1, field2: 2}]}, function(err, state) {console.log('risk6: ' + err.message)});  
d.post('risk6', { payments: [{field1: 1, field2: 1}]}, function(err, state) {console.log('risk6: ' + err.message)});
d.post('risk6', { a1: [{ field: 8, a2: [{field: 1}]}]});
d.post('risk6', { a1: [{ field1: 8, a2: [{field2: 1}]}]}, function(err, state) {console.log('risk6: ' + err.message)});
d.post('risk6', { a1: [{ field1: 8, a2: [{field: 1}]}]}, function(err, state) {console.log('risk6: ' + err.message)});
d.post('risk6', { a1: [{ field: 8, a2: [{field2: 1}]}]}, function(err, state) {console.log('risk6: ' + err.message)});  
d.post('risk6', { a4: [{field2: 2}]});
d.post('risk6', { a4: [{field2: 3, field1: true}]});
d.post('risk6', { a4: [{field2: 2, field1: true}]}, function(err, state) {console.log('risk6: ' + err.message)});
d.post('risk6', { a5: [{field: 7, a6: [{field21 : 1}]}]}, function(err, state) {console.log('risk6: ' + err.message)}); 
d.post('risk6', { a5: [{a6: [{field21 : 1}]}]}); 

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

d.getHost().setRulesets({ risk7: {
    suspect: {
        run: function(c) { console.log('risk7: fraud detected'); },
        all: [
            {first: {t: 'purchase'}},
            {second: {$neq: {location: {first: 'location'}}}}
        ],
    }
}}, function(err){});

d.post('risk7', {t: 'purchase', location: 'US'});
d.post('risk7', {t: 'purchase', location: 'CA'});

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
