var d = require('../libjs/durable');

with (d.ruleset('fraudDetection')) {
    whenAll(span(10), 
            or(m.t.eq('debitCleared'), m.t.eq('creditCleared')), 
    function(c) {
        var debitTotal = 0;
        var creditTotal = 0;
        if (c.s.balance == null) {
            c.s.balance = 0;
            c.s.avgBalance = 0;
            c.s.avgWithdraw = 0;
        } 

        for (var i = 0; i < c.m.length; ++i) {
            if (c.m[i].t === 'debitCleared') {
                debitTotal += c.m[i].amount;
            } else {
                creditTotal += c.m[i].amount;
            }
        }

        c.s.balance = c.s.balance - debitTotal + creditTotal;
        c.s.avgBalance = (c.s.avgBalance * 29 + c.s.balance) / 30;
        c.s.avgWithdraw = (c.s.avgWithdraw * 29 + debitTotal) / 30;
    });

    whenAll(c.first = and(m.t.eq('debitRequest'), 
                          m.amount.gt(c.s.avgWithdraw.mul(3))),
            c.second = and(m.t.eq('debitRequest'),
                           m.amount.gt(c.s.avgWithdraw.mul(3)),
                           m.stamp.gt(c.first.stamp),
                           m.stamp.lt(c.first.stamp.add(90))),
    function(c) {
        console.log('Medium risk ' + c.first.amount + ' ,' + c.second.amount);
    });

    whenAll(c.first = m.t.eq('debitRequest'),
            c.second = and(m.t.eq('debitRequest'),
                           m.amount.gt(c.first.amount),
                           m.stamp.lt(c.first.stamp.add(180))),
            c.third = and(m.t.eq('debitRequest'),
                          m.amount.gt(c.second.amount),
                          m.stamp.lt(c.first.stamp.add(180))),
            s.avgBalance.lt(add(c.first.amount, c.second.amount, c.third.amount).div(0.9)),
    function(c) {
        console.log('High risk ' + c.first.amount + ' ,' + c.second.amount + ' ,' + c.third.amount);
    });

    whenAll(or(m.t.eq('customer'), timeout('customer')), 
    function(c) {
        if (!c.s.cCount) {
            c.s.cCount = 100;
        } else {
            c.s.cCount += 2;
        }

        c.post('fraudDetection', {id: c.s.cCount, sid: 1, t: 'debitCleared', amount: c.s.cCount});
        c.post('fraudDetection', {id: c.s.cCount + 1, sid: 1, t: 'creditCleared', amount: (c.s.cCount - 100) * 2 + 100});
        c.startTimer('customer', 1, 'c' + c.s.cCount);
    });

    whenAll(or(m.t.eq('fraudster'), timeout('fraudster')), 
    function(c) {
        if (!c.s.fCount) {
            c.s.fCount = 1000;
        } else {
            c.s.fCount += 1;
        }

        c.post('fraudDetection', {id: c.s.fCount, sid: 1, t: 'debitRequest', amount: c.s.fCount - 800, 'stamp': Date.now() / 1000});
        c.startTimer('fraudster', 2, 'f' + c.s.cCount);
    });

    whenStart(function (host) {
        host.post('fraudDetection', {id: 'c_1', sid: 1, t: 'customer'});
        host.post('fraudDetection', {id: 'f_1', sid: 1, t: 'fraudster'});
    });
}

d.runAll();