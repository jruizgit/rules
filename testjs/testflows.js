var d = require('../libjs/durable');

d.run({
    approval1: {
        r1: {
            when: { $and: [
                { subject: 'approve' },
                { $gt: { amount: 1000 }}
            ]},
            run: denied
        },
        r2: {
            when: { $and: [
                { subject: 'approve' },
                { $lte: { amount: 1000 }}
            ]},
            run: requestApproval
        },
        r3: {
            whenAll: {
                m$any: {
                    a: { subject: 'approved' },
                    b: { subject: 'ok' }
                },
                $s: { status: 'pending' }
            },
            run: requestApproval
        },
        r4: {
            when: { $s: { status: 'approved' } },
            run: approved
        },
        r5: {
            when: { subject: 'denied' },
            run: denied
        }
    },
    approval2$state: {
        input: {
            deny: {
                when: { $and: [
                    { subject: 'approve' },
                    { $gt: { amount: 1000 }}
                ]},
                run: denied,
                to: 'denied'
            },
            request: {
                when: { $and: [
                    { subject: 'approve' },
                    { $lte: { amount: 1000 }}
                ]},
                run: requestApproval,
                to: 'pending'
            }
        },
        pending: {
            request: {
                whenAny: {
                    a: { subject: 'approved' },
                    b: { subject: 'ok' }
                },
                run: requestApproval,
                to: 'pending'
            },
            approve: {
                when: { $s: { status: 'approved' } },
                run: approved,
                to: 'approved'
            },
            deny: {
                when: { subject: 'denied' },
                run: denied,
                to: 'denied'
            }
        },
        denied: {
        },
        approved: {
        }
    },
    approval3$flow: {
        input: {
            to: {
                request: { $and: [
                    { subject: 'approve' },
                    { $lte: { amount: 1000 }}
                ]},
                deny: { $and: [
                    { subject: 'approve' },
                    { $gt: { amount: 1000 }}
                ]}
            }
        },
        request: {
            run: requestApproval,
            to: {
                approve: { $s: { status: 'approved'} },
                deny: { subject: 'denied'},
                request: {
                    $any: {
                        a: { subject: 'approved' },
                        b: { subject: 'ok'}
                    }
                }
            }
        },
        approve: {
            run: approved
        },
        deny: {
            run: denied
        }
    },
}, '', null, function(host) {
    host.post('approval1', { id: '1', sid: 1, subject: 'approve', amount: 100 }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval1', { id: '2', sid: 1, subject: 'approved' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval2', { id: '1', sid: 1, subject: 'approve', amount: 100 }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval2', { id: '2', sid: 1, subject: 'ok' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval3', { id: '1', sid: 1, subject: 'approve', amount: 100 }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval3', { id: '2', sid: 1, subject: 'denied' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
});

function denied(s) {
    console.log('denied');
    s.status = 'done';
}

function approved(s) {
    console.log('approved');
    s.status = 'done';
}

function requestApproval(s) {
    console.log('requestApproval');
    console.log(s.getOutput());
    if (s.status) {
        s.status = 'approved';
    } else {
        s.status = 'pending';
    }
}
