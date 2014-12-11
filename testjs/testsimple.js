var d = require('../libjs/durable');

with (d.ruleset('a1')) {
    when(m.subject.eq('approve').and(m.amount.gt(1000)), function (s) {
        console.log('a1 denied from: ' + s.id);
        s.status = 'done';
    });
    when(m.subject.eq('approve').and(m.amount.lte(1000)), function (s) {
        console.log('a1 request approval from: ' + s.id);
        s.status = 'pending';
    });
    whenAll(m.subject.eq('approved'), s.status.eq('pending'), function (s) {
        console.log('a1 second request approval from: ' + s.id);
        s.status = 'approved';
    });
    when(s.status.eq('approved'), function (s) {
        console.log('a1 approved from: ' + s.id);
        s.status = 'done';
    });
    when(m.subject.eq('denied'), function (s) {
        console.log('a1 denied from: ' + s.id);
        s.status = 'done';
    });
    whenStart(function (host) {
        host.post('a1', {id: 1, sid: 1, subject: 'approve', amount: 100});
        host.post('a1', {id: 2, sid: 1, subject: 'approved'});
        host.post('a1', {id: 3, sid: 2, subject: 'approve', amount: 100});
        host.post('a1', {id: 4, sid: 2, subject: 'denied'});
        host.post('a1', {id: 5, sid: 3, subject: 'approve', amount: 10000});
    });
}

with (d.statechart('a2')) {
    with (state('input')) {
        to('denied').when(m.subject.eq('approve').and(m.amount.gt(1000)), function (s) {
            console.log('a2 denied from: ' + s.id);
        });
        to('pending').when(m.subject.eq('approve').and(m.amount.lte(1000)), function (s) {
            console.log('a2 request approval from: ' + s.id);
        });
    }
    with (state('pending')) {
        to('pending').when(m.subject.eq('approved'), function (s) {
            console.log('a2 second request approval from: ' + s.id);
            s.status = 'approved';
        });
        to('approved').when(s.status.eq('approved'), function (s) {
            console.log('a2 approved from: ' + s.id);
        });
        to('denied').when(m.subject.eq('denied'), function (s) {
            console.log('a2 denied from: ' + s.id);
        });
    }
    state('denied');
    state('approved');
    whenStart(function (host) {
        host.post('a2', {id: 1, sid: 1, subject: 'approve', amount: 100});
        host.post('a2', {id: 2, sid: 1, subject: 'approved'});
        host.post('a2', {id: 3, sid: 2, subject: 'approve', amount: 100});
        host.post('a2', {id: 4, sid: 2, subject: 'denied'});
        host.post('a2', {id: 5, sid: 3, subject: 'approve', amount: 10000});
    });
}

with (d.flowchart('a3')) {
    with (stage('input')) {
        to('request').when(m.subject.eq('approve').and(m.amount.lte(1000)));
        to('deny').when(m.subject.eq('approve').and(m.amount.gt(1000)));
    }
    with (stage('request')) {
        run(function (s) {
            console.log('a3 request approval from: ' + s.id);
            if (s.status) 
                s.status = 'approved';
            else
                s.status = 'pending';
        });
        to('approve').when(s.status.eq('approved'));
        to('deny').when(m.subject.eq('denied'));
        to('request').whenAny(m.subject.eq('approved'), m.subject.eq('ok'));
    }
    with (stage('approve')) {
        run(function (s) {
            console.log('a3 approved from: ' + s.id);
        });
    }
    with (stage('deny')) {
        run(function (s) {
            console.log('a3 denied from: ' + s.id);
        });
    }
    whenStart(function (host) {
        host.post('a3', {id: 1, sid: 1, subject: 'approve', amount: 100});
        host.post('a3', {id: 2, sid: 1, subject: 'approved'});
        host.post('a3', {id: 3, sid: 2, subject: 'approve', amount: 100});
        host.post('a3', {id: 4, sid: 2, subject: 'denied'});
        host.post('a3', {id: 5, sid: 3, subject: 'approve', amount: 10000});
    });
}

d.runAll();