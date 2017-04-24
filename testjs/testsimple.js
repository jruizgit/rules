'use strict';
var d = require('../libjs/durable');

d.ruleset('match1', function() {
    whenAll: m.subject.matches('a+(bc|de?)*')
    run: console.log('match 1 ' + m.subject)
    
    whenStart: {
        post('match1', {id: 1, sid: 1, subject: 'abcbc'});
        post('match1', {id: 2, sid: 1, subject: 'addd'});
        post('match1', {id: 3, sid: 1, subject: 'ddd'});
        post('match1', {id: 4, sid: 1, subject: 'adee'});
    }
});

d.ruleset('match2', function() {
    whenAll: m.subject.matches('[0-9]+-[a-z]+')
    run: console.log('match 2 ' + m.subject)

    whenStart: {
        post('match2', {id: 1, sid: 1, subject: '0-a'});
        post('match2', {id: 2, sid: 1, subject: '43-bcd'});
        post('match2', {id: 3, sid: 1, subject: 'a-3'});
        post('match2', {id: 4, sid: 1, subject: '0a'});
    }
});

d.ruleset('match3', function() {
    whenAll: m.subject.matches('[adfzx-]+[5678]+')
    run: console.log('match 3 ' + m.subject)

    whenStart: {
        post('match3', {id: 1, sid: 1, subject: 'ad-567'});
        post('match3', {id: 2, sid: 1, subject: 'ac-78'});
        post('match3', {id: 3, sid: 1, subject: 'zx12'});
        post('match3', {id: 4, sid: 1, subject: 'xxxx-8888'});
    }
});

d.ruleset('match4', function() {
    whenAll: m.subject.matches('[z-a]+-[9-0]+')
    run: console.log('match 4 ' + m.subject)

    whenStart: {
        post('match4', {id: 1, sid: 1, subject: 'ed-56'});
        post('match4', {id: 2, sid: 1, subject: 'ac-ac'});
        post('match4', {id: 3, sid: 1, subject: 'az-90'});
        post('match4', {id: 4, sid: 1, subject: 'AZ-90'});
    }
});

d.ruleset('match5', function() {
    whenAll: m.subject.matches('.+b')
    run: console.log('match 5 ' + m.subject)

    whenStart: {
        post('match5', {id: 1, sid: 1, subject: 'ab'});
        post('match5', {id: 2, sid: 1, subject: 'adfb0'});
        post('match5', {id: 3, sid: 1, subject: 'abbbcd'});
        post('match5', {id: 4, sid: 1, subject: 'xybz.b'});
    }
});

d.ruleset('match6', function() {
    whenAll: m.subject.matches('[a-z]{3}')
    run: console.log('match 6 ' + m.subject)

    whenStart: {
        post('match6', {id: 1, sid: 1, subject: 'ab'});
        post('match6', {id: 2, sid: 1, subject: 'bbb'});
        post('match6', {id: 3, sid: 1, subject: 'azc'});
        post('match6', {id: 4, sid: 1, subject: 'abbbcd'});
    }
});

d.ruleset('match7', function() {
    whenAll: m.subject.matches('[a-z]{2,4}')
    run: console.log('match 7 ' + m.subject)

    whenStart: {
        post('match7', {id: 1, sid: 1, subject: 'ar'});
        post('match7', {id: 2, sid: 1, subject: 'bxbc'});
        post('match7', {id: 3, sid: 1, subject: 'a'});
        post('match7', {id: 4, sid: 1, subject: 'abbbc'});
    }
});

d.ruleset('match8', function() {
    whenAll: m.subject.matches('(a|bc){2,}')
    run: console.log('match 8 ' + m.subject)

    whenStart: {
        post('match8', {id: 1, sid: 1, subject: 'aa'});
        post('match8', {id: 2, sid: 1, subject: 'abcbc'});
        post('match8', {id: 3, sid: 1, subject: 'bc'});
        post('match8', {id: 4, sid: 1, subject: 'a'});
    }
});


d.ruleset('match9', function() {
    whenAll: m.subject.matches('a{1,2}')
    run: console.log('match 9 ' + m.subject)

    whenStart: {
        post('match9', {id: 1, sid: 1, subject: 'a'});
        post('match9', {id: 2, sid: 1, subject: 'b'});
        post('match9', {id: 3, sid: 1, subject: 'aa'});
        post('match9', {id: 4, sid: 1, subject: 'aaa'});
    }
});

d.ruleset('match10', function() {
    whenAll: m.subject.matches('cba{1,}')
    run: console.log('match 10 ' + m.subject)

    whenStart: {
        post('match10', {id: 1, sid: 1, subject: 'cba'});
        post('match10', {id: 2, sid: 1, subject: 'b'});
        post('match10', {id: 3, sid: 1, subject: 'cb'});
        post('match10', {id: 4, sid: 1, subject: 'cbaaa'});
    }
});

d.ruleset('match11', function() {
    whenAll: m.subject.matches('cba{0,}')
    run: console.log('match 11 ' + m.subject)

    whenStart: {
        post('match11', {id: 1, sid: 1, subject: 'cb'});
        post('match11', {id: 2, sid: 1, subject: 'b'});
        post('match11', {id: 3, sid: 1, subject: 'cbab'});
        post('match11', {id: 4, sid: 1, subject: 'cbaaa'});
    }
});

d.ruleset('match12', function() {
    whenAll: m.user.matches('[a-z0-9_-]{3,16}')
    run: console.log('match 12 user ' + m.user)
    
    whenStart: {
        post('match12', {id: 1, sid: 1, user: 'git-9'});
        post('match12', {id: 2, sid: 1, user: 'git_9'});
        post('match12', {id: 3, sid: 1, user: 'GIT-9'});
        post('match12', {id: 4, sid: 1, user: '01234567890123456789'});
        post('match12', {id: 5, sid: 1, user: 'gi'});
    }
});

d.ruleset('match13', function() {
    whenAll: m.number.matches('#?([a-f0-9]{6}|[a-f0-9]{3})')
    run: console.log('match 13 hex ' + m.number)

    whenStart: {
        post('match13', {id: 1, sid: 1, number: 'fff'});
        post('match13', {id: 2, sid: 1, number: '#45'});
        post('match13', {id: 3, sid: 1, number: 'abcdex'});
        post('match13', {id: 4, sid: 1, number: '#54ba45'});
        post('match13', {id: 5, sid: 1, number: '#9999'});
    }
});

d.ruleset('match14', function() {
    whenAll: m.alias.matches('([a-z0-9_.-]+)@([0-9a-z.-]+)%.([a-z]{2,6})')
    run: console.log('match 14 email ' + m.alias)

    whenStart: {
        post('match14', {id: 1, sid: 1, alias: 'john_59@mail.mx'});
        post('match14', {id: 2, sid: 1, alias: 'j#5@mail.com'});
        post('match14', {id: 3, sid: 1, alias: 'abcd.abcd@mail.mx.com'});
        post('match14', {id: 4, sid: 1, alias: 'dd@dd.'});
        post('match14', {id: 5, sid: 1, alias: 'dd@dd.c'});
    }
});

d.ruleset('match15', function() {
    whenAll: m.url.matches('(https?://)?([%da-z.-]+)%.[a-z]{2,6}(/[%w_.-]+/?)*')
    run: console.log('match 15 url ' + m.url)

    whenStart: {
        post('match15', {id: 1, sid: 1, url: 'https://github.com'});
        post('match15', {id: 2, sid: 1, url: 'http://github.com/jruizgit/rul!es'});
        post('match15', {id: 3, sid: 1, url: 'https://github.com/jruizgit/rules/blob/master/docs/rb/reference.md'});
        post('match15', {id: 4, sid: 1, url: '//rules'});
        post('match15', {id: 5, sid: 1, url: 'https://github.c/jruizgit/rules'});
    }
});

d.ruleset('match16', function() {
    whenAll: m.ip.matches('((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)%.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)')
    run: console.log('match 16 ip ' + m.ip)

    whenStart: {
        post('match16', {id: 1, sid: 1, ip: '73.60.124.136'});
        post('match16', {id: 2, sid: 1, ip: '256.60.124.136'});
        post('match16', {id: 3, sid: 1, ip: '250.60.124.256'});
        post('match16', {id: 4, sid: 1, ip: '73.60.124'});
        post('match16', {id: 5, sid: 1, ip: '127.0.0.1'});
    }
});

d.ruleset('match17', function() {
    whenAll: m.subject.matches('([ab]?){30}%w{30}')
    run: console.log('match 17 ' + m.subject)

    whenStart: {
        post('match17', {id: 1, sid: 1, subject: 'aaaaaaaaab'});
        post('match17', {id: 2, sid: 1, subject: 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaab'});
        post('match17', {id: 3, sid: 1, subject: 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab'});
        post('match17', {id: 4, sid: 1, subject: 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab'});
    }
});

d.ruleset('match18', function() {
    whenAll: m.subject.matches('%w+')
    run: console.log('match 18 ' + m.subject)

    whenStart: {
        post('match18', {id: 1, sid: 1, subject: 'aAzZ5678'});
        post('match18', {id: 2, sid: 1, subject: '789Az'});
        post('match18', {id: 3, sid: 1, subject: 'aA`89'});
    }
});

d.ruleset('match19', function() {
    whenAll: m.subject.matches('%u+')
    run: console.log('match 19 upper ' + m.subject)

    whenAll: m.subject.matches('%l+')
    run: console.log('match 19 lower ' + m.subject)

    whenStart: {
        post('match19', {id: 1, sid: 1, subject: 'ABCDZ'});
        post('match19', {id: 2, sid: 1, subject: 'W\u00D2\u00D3'});
        post('match19', {id: 3, sid: 1, subject: 'abcz'});
        post('match19', {id: 4, sid: 1, subject: 'wßÿ'});
        post('match19', {id: 4, sid: 1, subject: 'AbcZ'});
    }
});

d.ruleset('match20', function() {
    whenAll: m.subject.matches('[^ABab]+')
    run: console.log('match 20 ' + m.subject)

    whenStart: {
        post('match20', {id: 1, sid: 1, subject: 'ABCDZ'});
        post('match20', {id: 2, sid: 1, subject: '.;:<>'});
        post('match20', {id: 3, sid: 1, subject: '^&%()'});
        post('match20', {id: 4, sid: 1, subject: 'abcz'});
    }
});

d.ruleset('match21', function() {
    whenAll: m.subject.matches('hello.*')
    run: console.log('match 21 starts with hello: ' + m.subject)

    whenAll: m.subject.matches('.*hello')
    run: console.log('match 21 ends with hello: ' + m.subject)

    whenAll: m.subject.matches('.*hello.*')
    run: console.log('match 21 contains hello: ' + m.subject)

    whenStart: {
        assert('match21', {id: 1, sid: 1, subject: 'hello world'});
        assert('match21', {id: 2, sid: 1, subject: 'world hello'});
        assert('match21', {id: 3, sid: 1, subject: 'world hello hello'});
        assert('match21', {id: 4, sid: 1, subject: 'has hello string'});
        assert('match21', {id: 5, sid: 1, subject: 'does not match'});
    }
});

d.statechart('fraud0', function() {
    start: {
        to: 'standby'
    }

    standby: {
        to: 'metering'
        whenAll: m.amount > 100
        run: startTimer('velocity', 30)
    }

    metering: {
        to: 'fraud'
        whenAll: m.amount > 100
        count: 3
        run: console.log('fraud0 detected')
    
        to: 'standby'
        whenAll: timeout('velocity')
        run: console.log('fraud0 cleared')
    }
    
    fraud: {}
    
    whenStart: {
        post('fraud0', {id: 1, sid: 1, amount: 200});
        post('fraud0', {id: 2, sid: 1, amount: 200});
        post('fraud0', {id: 3, sid: 1, amount: 200});
        post('fraud0', {id: 4, sid: 1, amount: 200});
    }
});

d.ruleset('fraud1', function() {
    whenAll: {
        first = m.t == 'purchase'
        second = m.location != first.location
    }  
    count: 2
    run: {
        console.log('fraud1 detected ->' + m[0].first.location + ', ' + m[0].second.location);
        console.log('               ->' + m[1].first.location + ', ' + m[1].second.location);
    }

    whenStart: {
        assert('fraud1', {id: 1, sid: 1, t: 'purchase', location: 'US'});
        assert('fraud1', {id: 2, sid: 1, t: 'purchase', location: 'CA'});
    }
});

d.ruleset('fraud2', function() {
    whenAll: {
        first = m.amount > 100
        second = (m.ip == first.ip) && (m.cc != first.cc) 
        third = (m.ip == second.ip) && (m.cc != first.cc) && (m.cc != second.cc)
    }
    run: console.log('fraud2 detected ' + first.cc + ' ' + second.cc + ' ' + third.cc)

    whenStart: {
        post('fraud2', {id: 1, sid: 1, amount: 200, ip: 'a', cc: 'b'});
        post('fraud2', {id: 2, sid: 1, amount: 200, ip: 'a', cc: 'c'});
        post('fraud2', {id: 3, sid: 1, amount: 200, ip: 'a', cc: 'd'});
    }
});

d.ruleset('fraud3', function() {
    whenAll: {
        first = m.amount > 100
        second = m.amount > first.amount * 2
        third = m.amount > second.amount + first.amount
        fourth = m.amount > (first.amount + second.amount + third.amount) / 3
    }
    run: console.log('fraud3 detected ' + first.amount + ' ' + second.amount + ' ' + third.amount + ' ' + fourth.amount)

    whenStart: {
        post('fraud3', {id: 1, sid: 1, amount: 200});
        post('fraud3', {id: 2, sid: 1, amount: 500});
        post('fraud3', {id: 3, sid: 1, amount: 1000});
        post('fraud3', {id: 4, sid: 1, amount: 1000});
    }
});

d.ruleset('fraud5', function() {
    whenAll: m.amount > 100
    pri: 3
    run: console.log('fraud5 first ' + m.amount + ' from ' + s.sid)

    whenAll: m.amount > 200
    pri: 2
    run: console.log('fraud5 second ' + m.amount + ' from ' + s.sid)

    whenAll: m.amount > 300
    pri: 1
    run: console.log('fraud5 third ' + m.amount + ' from ' + s.sid)

    whenStart: {
        post('fraud5', {id: 1, sid: 1, amount: 101, location: 'US'});
        post('fraud5', {id: 2, sid: 1, amount: 201, location: 'CA'});
        post('fraud5', {id: 3, sid: 1, amount: 301, location: 'CA'});
        assert('fraud5', {id: 4, sid: 1, amount: 250, location: 'US'});
        assert('fraud5', {id: 5, sid: 1, amount: 500, location: 'CA'});
    }
});

var q = d.createQueue('fraud5');
q.post({id: 1, sid: 2, amount: 101, location: 'US'});
q.post({id: 2, sid: 2, amount: 201, location: 'CA'});
q.post({id: 3, sid: 2, amount: 301, location: 'CA'});
q.assert({id: 4, sid: 2, amount: 250, location: 'US'});
q.assert({id: 5, sid: 2, amount: 500, location: 'CA'});
q.close();

d.ruleset('fraud6', function() {
    whenAll: {
        first = m.t == 'deposit'
        none(m.t == 'balance')
        third = m.t == 'withrawal'
        fourth = m.t == 'chargeback'
    }
    run: console.log('fraud6 detected ' + first.t + ' ' + third.t + ' ' + fourth.t + ' from ' + s.sid);

    whenStart: {
        post('fraud6', {id: 1, sid: 1, t: 'deposit'});
        post('fraud6', {id: 2, sid: 1, t: 'withrawal'});
        post('fraud6', {id: 3, sid: 1, t: 'chargeback'});
    }
});

q = d.createQueue('fraud6');
q.post({id: 1, sid: 2, t: 'deposit'});
q.post({id: 2, sid: 2, t: 'withrawal'});
q.post({id: 3, sid: 2, t: 'chargeback'});
q.assert({id: 4, sid: 2, t: 'balance'});
q.post({id: 5, sid: 2, t: 'deposit'});
q.post({id: 6, sid: 2, t: 'withrawal'});
q.post({id: 7, sid: 2, t: 'chargeback'});
q.retract({id: 4, sid: 2, t: 'balance'});
q.close();

d.statechart('fraud7', function() {
    first: {
        to: 'second'
        whenAll: m.amount > 100
        runAsync: {
            console.log('fraud7 start async 1');
            setTimeout(function() {
                console.log('fraud7 execute 1');
                complete();
            }, 1000);
        }
    }

    second: {
        to: 'fraud'
        whenAll: m.amount > 100
        runAsync: {
            console.log('fraud7 start async 2');
            setTimeout(function() {
                console.log('fraud7 execute 2');
                complete();
            }, 1000);   
        }
    }
    
    fraud: {}
    
    whenStart: {
        assert('fraud7', {id: 1, sid: 1, amount: 200});
    }
});

d.ruleset('a0', function() {
    whenAll: m.subject == 'go' || m.subject == 'approve' || m.subject == 'ok'
    run: console.log('a0 approved ' + m.subject)

    whenStart: {
        post('a0', {id: 1, sid: 1, subject: 'go'});
        post('a0', {id: 2, sid: 1, subject: 'approve'});
        post('a0', {id: 3, sid: 1, subject: 'ok'});
        post('a0', {id: 4, sid: 1, subject: 'not ok'});
    }
});

d.ruleset('a1', function() {
    whenAll:  m.subject == 'go' && (m.amount < 100 || m.amount > 1000)
    run: console.log('a1 approved ' + m.subject + ' ' + m.amount)

    whenStart: {
        post('a1', {id: 1, sid: 1, subject: 'go', amount: 50});
        post('a1', {id: 2, sid: 1, subject: 'go', amount: 500});
        post('a1', {id: 3, sid: 1, subject: 'go', amount: 5000});
    }
});

d.ruleset('a2', function() {
    whenAll:  m.subject == 'go' && (m.amount < 100 || (m.amount == 500 && m.status == 'waived'))
    run: console.log('a2 approved ' + m.subject + ' ' + m.amount)

    whenStart: {
        post('a2', {id: 1, sid: 1, subject: 'go', amount: 50});
        post('a2', {id: 2, sid: 1, subject: 'go', amount: 500, status: 'waived'});
        post('a2', {id: 3, sid: 1, subject: 'go', amount: 5000});
    }
});

d.flowchart('a3', function() {
    input: {
        request: { 
            whenAll: m.subject == 'approve' && m.amount <= 1000 
            pri: 1
        }
        deny:  m.subject == 'approve' && m.amount > 1000
    }

    request: {
        run: {
            console.log('a3 request approval from: ' + c.s.sid);
            if (c.s.status) 
                c.s.status = 'approved';
            else
                c.s.status = 'pending';
        }

        approve: s.status == 'approved'
        deny: m.subject == 'denied'
        self: m.subject == 'approved'
    }

    approve: {
        run: console.log('a3 approved from: ' + c.s.sid)
    }

    deny: {
        run: console.log('a3 denied from: ' + c.s.sid)
    }

    whenStart: {
        post('a3', {id: 1, sid: 1, subject: 'approve', amount: 100});
        post('a3', {id: 2, sid: 1, subject: 'approved'});
        post('a3', {id: 3, sid: 2, subject: 'approve', amount: 100});
        post('a3', {id: 4, sid: 2, subject: 'denied'});
        post('a3', {id: 5, sid: 3, subject: 'approve', amount: 10000});
    }
});

d.ruleset('a4', function() {
    whenAll: m.subject == 'approve' && m.amount > 1000
    run: {
        console.log('a4 denied from: ' + s.sid);
        s.status = 'done';
    }

    whenAll:  m.subject == 'approve' && m.amount <= 1000
    run: {
        console.log('a4 request approval from: ' + s.sid);
        s.status = 'pending';
    }

    whenAll:  {
        m.subject == 'approved' 
        s.status == 'pending'
    }
    run: {
        console.log('a4 second request approval from: ' + s.sid);
        s.status = 'approved';
    }

    whenAll:  s.status == 'approved'
    run: {
        console.log('a4 approved from: ' + s.sid);
        s.status = 'done';
    }

    whenAll:  m.subject == 'denied'
    run: {
        console.log('a4 denied from: ' + s.sid);
        s.status = 'done';
    }

    whenStart: {
        post('a4', {id: 1, sid: 1, subject: 'approve', amount: 100});
        post('a4', {id: 2, sid: 1, subject: 'approved'});
        post('a4', {id: 3, sid: 2, subject: 'approve', amount: 100});
        post('a4', {id: 4, sid: 2, subject: 'denied'});
        post('a4', {id: 5, sid: 3, subject: 'approve', amount: 10000});
    }
});

d.statechart('a5', function() {
    input: {
        to: 'denied'
        whenAll: m.subject == 'approve' && m.amount > 1000
        run: console.log('a5 denied from: ' + s.sid)

        to: 'pending'
        whenAll: m.subject == 'approve' && m.amount <= 1000
        run: console.log('a5 request approval from: ' + c.s.sid);
    }

    pending: {
        to: 'pending'
        whenAll: m.subject == 'approved'
        run: {
            console.log('a5 second request approval from: ' + s.sid);
            s.status = 'approved';
        }

        to: 'approved'
        whenAll: s.status == 'approved'
        run: console.log('a5 approved from: ' + s.sid)

        to: 'denied'
        whenAll: m.subject == 'denied'
        run: console.log('a5 denied from: ' + s.sid)
    }
    
    denied: {}
    approved: {}
    
    whenStart: {
        post('a5', {id: 1, sid: 1, subject: 'approve', amount: 100});
        post('a5', {id: 2, sid: 1, subject: 'approved'});
        post('a5', {id: 3, sid: 2, subject: 'approve', amount: 100});
        post('a5', {id: 4, sid: 2, subject: 'denied'});
        post('a5', {id: 5, sid: 3, subject: 'approve', amount: 10000});
    }
});

d.statechart('a6', function() {
    work: {
        enter: {
            to: 'process'
            whenAll: m.subject == 'enter'
            run: console.log('a6 continue process')
        }

        process: {
            to: 'process'
            whenAll: m.subject == 'continue'
            run: console.log('a6 processing')
        }
    
        to: 'canceled'
        pri: 1
        whenAll: m.subject == 'cancel'
        run: console.log('a6 canceling')
    }

    canceled: {}
    whenStart: {
        post('a6', {id: 1, sid: 1, subject: 'enter'});
        post('a6', {id: 2, sid: 1, subject: 'continue'});
        post('a6', {id: 3, sid: 1, subject: 'continue'});
        post('a6', {id: 4, sid: 1, subject: 'cancel'});
    }
});

d.ruleset('a7', function() {
    whenAny: {
        whenAll: m.subject == 'approve', m.amount == 1000
        whenAll: m.subject == 'jumbo', m.amount == 10000
    }
    run: {
        if (c.m_0) {
            console.log('a7 action from: ' + s.sid + ' ' + c.m_0.m_0.subject + ' ' + c.m_0.m_1.amount);     
        } else {
            console.log('a7 action from: ' + s.sid + ' ' + c.m_1.m_0.subject + ' ' + c.m_1.m_1.amount);        
        }
    }

    whenStart: {
        post('a7', {id: 1, sid: 1, subject: 'approve'});
        post('a7', {id: 2, sid: 1, amount: 1000});
        post('a7', {id: 3, sid: 2, subject: 'jumbo'});
        post('a7', {id: 4, sid: 2, amount: 10000});
    }
});


d.ruleset('a8', function() {
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
            console.log('a8 action from: ' + s.sid + ' ' + first.subject + ' ' + second.amount);     
        } else {
            console.log('a8 action from: ' + s.sid + ' ' + third.subject + ' ' + fourth.amount);        
        }
    }

    whenStart: {
        post('a8', {id: 1, sid: 1, subject: 'approve'});
        post('a8', {id: 2, sid: 1, amount: 1000});
        post('a8', {id: 3, sid: 2, subject: 'jumbo'});
        post('a8', {id: 4, sid: 2, amount: 10000});
    }
});


d.ruleset('a9', function() {
    whenAll: {
        whenAny: m.subject == 'approve', m.subject == 'jumbo'
        whenAny: m.amount == 1000, m.amount == 10000
    }
    run: {
        if (c.m_0.m_0) {
            if (c.m_1.m_0) {
                console.log('a9 action from: ' + s.sid + ' ' + c.m_0.m_0.subject + ' ' + c.m_1.m_0.amount);        
            } else {
                console.log('a9 action from: ' + s.sid + ' ' + c.m_0.m_0.subject + ' ' + c.m_1.m_1.amount);
            }
        } else {
            if (c.m_1.m_0) {
                console.log('a9 action from: ' + s.sid + ' ' + c.m_0.m_1.subject + ' ' + c.m_1.m_0.amount);        
            } else {
                console.log('a9 action from: ' + s.sid + ' ' + c.m_0.m_1.subject + ' ' + c.m_1.m_1.amount);
            }
        }
    }

    whenStart: {
        post('a9', {id: 1, sid: 1, subject: 'approve'});
        post('a9', {id: 2, sid: 1, amount: 1000});
        post('a9', {id: 3, sid: 2, subject: 'jumbo'});
        post('a9', {id: 4, sid: 2, amount: 10000});
    }
});


d.ruleset('a10', function() {
    whenAll: {
        whenAny: {
            first = m.subject == 'approve'
            second = m.subject == 'jumbo'
        }
        whenAny: {
            third = m.amount == 1000
            fourth = m.amount == 10000
        }
    }
    run: {
        if (first) {
            if (third) {
                console.log('a10 action from: ' + s.sid + ' ' + first.subject + ' ' + third.amount);        
            } else {
                console.log('a10 action from: ' + s.sid + ' ' + first.subject + ' ' + fourth.amount);
            }
        } else {
            if (third) {
                console.log('a10 action from: ' + s.sid + ' ' + second.subject + ' ' + third.amount);        
            } else {
                console.log('a10 action from: ' + s.sid + ' ' + second.subject + ' ' + fourth.amount);
            }
        }
    }

    whenStart: {
        post('a10', {id: 1, sid: 1, subject: 'approve'});
        post('a10', {id: 2, sid: 1, amount: 1000});
        post('a10', {id: 3, sid: 2, subject: 'jumbo'});
        post('a10', {id: 4, sid: 2, amount: 10000});
    }
});

d.ruleset('a11', function() {
    whenAll: m.amount < s.maxAmount
    run: console.log('a11 approved ' +  m.amount);

    whenStart: {
        patchState('a11', {sid: 1, maxAmount: 100});
        post('a11', {id: 1, sid: 1, amount: 10});
        post('a11', {id: 2, sid: 1, amount: 1000});
    }
});

d.ruleset('a12', function() {
    whenAll: m.amount < s.maxAmount && m.amount > s.refId('global').minAmount
    run: console.log('a12 approved ' +  m.amount);

    whenStart: {
        patchState('a12', {sid: 1, maxAmount: 500});
        patchState('a12', {sid: 'global', minAmount: 100});
        post('a12', {id: 1, sid: 1, amount: 10});
        post('a12', {id: 2, sid: 1, amount: 200});
    }
});

d.ruleset('a13', function() {
    whenAll: m.amount > c.s.maxAmount + s.refId('global').minAmount
    run: console.log('a13 approved ' +  m.amount);

    whenStart: {
        patchState('a13', {sid: 1, maxAmount: 500});
        patchState('a13', {sid: 'global', minAmount: 100});
        post('a13', {id: 1, sid: 1, amount: 10});
        post('a13', {id: 2, sid: 1, amount: 1000});
    }
});

d.ruleset('a14', function() {
    whenAll: m.amount < 100 
    count: 3
    run: console.log('a14 approved ->' + JSON.stringify(m));

    whenStart: {
        postBatch('a14', {id: 1, sid: 1, amount: 10},
                         {id: 2, sid: 1, amount: 10},
                         {id: 3, sid: 1, amount: 10},
                         {id: 4, sid: 1, amount: 10});
        postBatch('a14', [{id: 5, sid: 1, amount: 10},
                         {id: 6, sid: 1, amount: 10}]); 
    }
});

d.ruleset('a15', function() {
    whenAll: m.amount < 100, m.subject == 'approve'
    count: 3
    run: console.log('a15 approved ->' + JSON.stringify(m));

    whenStart: {
        postBatch('a15', {id: 1, sid: 1, amount: 10},
                         {id: 2, sid: 1, amount: 10},
                         {id: 3, sid: 1, amount: 10},
                         {id: 4, sid: 1, subject: 'approve'});
        postBatch('a15', {id: 5, sid: 1, subject: 'approve'},
                         {id: 6, sid: 1, subject: 'approve'});
    }
});

d.ruleset('a16', function() {
    whenAll: m.amount < 100, m.subject == 'please'
    count: 3
    run: console.log('a16 approved ->' + JSON.stringify(m));

    whenStart: {
        assert('a16', {id: 1, sid: 1, amount: 10});           
        assert('a16', {id: 2, sid: 1, subject: 'please'});
        assert('a16', {id: 3, sid: 1, subject: 'please'});
        assert('a16', {id: 4, sid: 1, subject: 'please'});
    }
});

d.ruleset('a17', function() {
    whenAll: m.invoice.amount >= 100
    run: console.log('a17 approved ->' + m.invoice.amount);

    whenStart: {
        post('a17', {id: 1, sid: 1, invoice: {amount: 100}});   
    }
});

d.ruleset('a18', function() {
    whenAll: {
        first = m.t == 'bill'
        second = m.t == 'payment' && m.invoice.amount == first.invoice.amount
    }
    run: {
        console.log('a18 approved ->' + first.invoice.amount);
        console.log('a18 approved ->' + second.invoice.amount);
    }

    whenStart: {
        post('a18', {id: 1, sid: 1, t: 'bill', invoice: {amount: 100}});  
        post('a18', {id: 2, sid: 1, t: 'payment', invoice: {amount: 100}}); 
    }
});

d.ruleset('a19', function() {
    whenAll: m.payment.invoice.amount >= 100
    run: console.log('a19 approved ->' + m.payment.invoice.amount);

    whenStart: {
        post('a19', {id: 1, sid: 1, payment: {invoice: {amount: 100}}});  
    }
});

d.ruleset('t0', function() {
    whenAll: m.count == 0 || timeout('myTimer')
    run: {
        s.count += 1;
        post('t0', {id: s.count, sid: 1, t: 'purchase'});
        startTimer('myTimer', Math.random() * 3 + 1, 't0_' + s.count);
    }

    whenAll: m.t == 'purchase'
    span: 5
    run: console.log('t0 pulse ->' + s.count);

    whenStart: {
        patchState('t0', {sid: 1, count: 0}); 
    }
});

d.ruleset('t1', function() {
    whenAll: m.start == 'yes'
    run: {
        s.start = new Date();
        startTimer('myFirstTimer', 3);
        startTimer('mySecondTimer', 6, 'mySecondTimer');
    }

    whenAll: timeout('myFirstTimer')
    run: {
        console.log('t1 first timer started ' + s.start);
        console.log('t1 first timer ended ' + new Date());
        cancelTimer('mySecondTimer', 'mySecondTimer');
    }

    whenAll: timeout('mySecondTimer')
    run: {
        console.log('t1 second timer started ' + s.start);
        console.log('t1 second timer ended ' + new Date());
    }

    whenStart: {
        post('t1', {id: 1, sid: 1, start: 'yes'}); 
    }
});

d.ruleset('t2', function() {
    whenAll: m.start == 'yes'
    runAsync: {
        console.log('t2 first started');
        setTimeout(function() {
            console.log('t2 first completed');
            post({id: 2, end: 'yes'});
            complete();
        }, 6000);
        return 10;
    }

    whenAll: m.end == 'yes'
    runAsync: {
        console.log('t2 second started');
        setTimeout(function() {
            console.log('t2 second completed');
            post({id: 3, end: 'yes'});
            complete();
        }, 7000);
        return 3;
    }

    whenAll: +s.exception
    run: {
        console.log('t2 expected exception ' + s.exception);
        delete(s.exception); 
    }

    whenStart: {
        post('t2', {id: 1, sid: 1, start: 'yes'});
    }
});

d.ruleset('a23', function() {
    whenAll: m.start == 'yes'
    run: {
        console.log('a23 started');
        getQueue('a23').post({sid: 1, id: 2, end: 'yes'});
    }

    whenAll: m.end == 'yes'
    run: console.log('a23 ended')

    whenStart: post('a23', {id: 1, sid: 1, start: 'yes'});
});


d.runAll();


