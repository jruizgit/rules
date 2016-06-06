import * as d from 'durable';
let m = d.m, s = d.s, c = d.c;

d.ruleset('a0_0', {
    whenAll: m['amount'].lt(100),
    run: (c) => { console.log('a0 approved from ' + c.s['sid']); }
},
(host) => {
    host.post('a0_0', { id: 1, sid: 1, amount: 10 });
});


d.ruleset('a1_1', {
    whenAll: [c['first'] = m['amount'].gt(100),
        c['second'] = m['location'].neq(c['first']['location'])],
    run: (c) => {
        console.log('fraud1 detected ' + c['first']['location'] + ' ' + c['second']['location']);
    }
},
(host) => {
    host.post('a1_1', { id: 1, sid: 1, amount: 200, location: 'US' });
    host.post('a1_1', { id: 2, sid: 1, amount: 200, location: 'CA' });
});

d.statechart('a2_2', {
    input: [{
        to: 'denied',
        whenAll: m['subject'].eq('approve').and(m['amount'].gt(1000)),
        run: (c) => {
            console.log('a2_2 denied from: ' + c.s['sid']);
        }
    }, {
        to: 'pending',
        whenAll: m['subject'].eq('approve').and(m['amount'].lte(1000)),
        run: (c) => {
            console.log('a2_2 request approval from: ' + c.s['sid']);
        }
    }],
    pending: [{
        to: 'pending',
        whenAll: m['subject'].eq('approved'),
        run: (c) => {
            console.log('a2_2 second request approval from: ' + c.s['sid']);
            c.s['status'] = 'approved';
        }
    }, {
        to: 'approved',
        whenAll: s['status'].eq('approved'),
        run: (c) => {
            console.log('a2_2 approved from: ' + c.s['sid']);
        }
    }, {
        to: 'denied',
        whenAll: m['subject'].eq('denied'),
        run: (c) => {
            console.log('a2_2 denied from: ' + c.s['sid']);
        }
    }],
    denied: {},
    approved: {},
    whenStart: (host) => {
        host.post('a2_2', { id: 1, sid: 1, subject: 'approve', amount: 100 });
        host.post('a2_2', { id: 2, sid: 1, subject: 'approved' });
        host.post('a2_2', { id: 3, sid: 2, subject: 'approve', amount: 100 });
        host.post('a2_2', { id: 4, sid: 2, subject: 'denied' });
        host.post('a2_2', { id: 5, sid: 3, subject: 'approve', amount: 10000 });
    }
});

d.flowchart('a3_1', {
    input: {
        request: { whenAll: m['subject'].eq('approve').and(m['amount'].lte(1000)) },
        deny: { whenAll: m['subject'].eq('approve').and(m['amount'].gt(1000)) }
    },
    request: {
        run: (c) => {
            console.log('a3_1 request approval from: ' + c.s['sid']);
            if (c.s['status'])
                c.s['status'] = 'approved';
            else
                c.s['status'] = 'pending';
        },
        approve: { whenAll: s['status'].eq('approved') },
        deny: { whenAll: m['subject'].eq('denied') },
        request: { whenAll: m['subject'].eq('approved') }
    },
    approve: {
        run: (c) => {
            console.log('a3_1 approved from: ' + c.s['sid']);
        }
    },
    deny: {
        run: (c) => {
            console.log('a3_1 denied from: ' + c.s['sid']);
        }
    },
    whenStart: (host) => {
        host.post('a3_1', { id: 1, sid: 1, subject: 'approve', amount: 100 });
        host.post('a3_1', { id: 2, sid: 1, subject: 'approved' });
        host.post('a3_1', { id: 3, sid: 2, subject: 'approve', amount: 100 });
        host.post('a3_1', { id: 4, sid: 2, subject: 'denied' });
        host.post('a3_1', { id: 5, sid: 3, subject: 'approve', amount: 10000 });
    }
});

d.runAll();