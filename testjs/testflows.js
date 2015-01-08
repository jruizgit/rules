var d = require('../libjs/durable');

d.run({
    approval1: {
        r1: {
            all: [
                {m: {$and: [{subject: 'approve'}, {$gt: {amount: 1000}}]}}
            ],
            run: denied
        },
        r2: {
            all: [
                {m: {$and: [{subject: 'approve'}, {$lte: {amount: 1000}}]}}
            ],
            run: requestApproval
        },
        r3: {
            all: [
                {m$any: [
                    {a: {subject: 'approved'}},
                    {b: {subject: 'ok'}}
                ]},
                {s: {$and: [{status: 'pending'}, {$s: 1}]}}
            ],
            run: requestApproval
        },
        r4: {
            all: [
                {s: {$and: [{status: 'approved'}, {$s: 1}]}}
            ],
            run: approved
        },
        r5: {
            all: [
                {m: {subject: 'denied'}}
            ],
            run: denied
        }
    },
    approval2$state: {
        input: {
            deny: {
                all: [
                    {m: {$and: [{subject: 'approve'}, {$gt: {amount: 1000}}]}}
                ],
                run: denied,
                to: 'denied'
            },
            request: {
                all: [
                    {m: {$and: [{subject: 'approve'}, {$lte: {amount: 1000}}]}}
                ],
                run: requestApproval,
                to: 'pending'
            }
        },
        pending: {
            request: {
                any: [
                    {a: {subject: 'approved'}},
                    {b: {subject: 'ok'}}
                ],
                run: requestApproval,
                to: 'pending'
            },
            approve: {
                all: [
                    {s: {$and: [{status: 'approved'}, {$s: 1}]}}
                ],
                run: approved,
                to: 'approved'
            },
            deny: {
                all: [
                    {m: {subject: 'denied'}}
                ],
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
                request: {all: [{m: {$and: [{subject: 'approve'}, {$lte: {amount: 1000}}]}}]},
                deny: {all: [{m: {$and: [{subject: 'approve'}, {$gt: {amount: 1000}}]}}]}
            }
        },
        request: {
            run: requestApproval,
            to: {
                approve: {all: [{s: {$and: [{status: 'approved'}, {$s: 1}]}}]},
                deny: {all: [{m: {subject: 'denied'}}]},
                request: {any: [
                    {a: {subject: 'approved'}},
                    {b: {subject: 'ok'}}
                ]}
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
    host.post('approval1', { id: '1', sid: 2, subject: 'approve', amount: 100 }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval1', { id: '2', sid: 2, subject: 'denied' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval1', { id: '1', sid: 3, subject: 'approve', amount: 10000 }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval2', { id: '1', sid: 4, subject: 'approve', amount: 100 }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval2', { id: '2', sid: 4, subject: 'ok' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval2', { id: '1', sid: 5, subject: 'approve', amount: 10000 }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval2', { id: '1', sid: 6, subject: 'approve', amount: 100 }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval2', { id: '2', sid: 6, subject: 'denied' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval3', { id: '1', sid: 7, subject: 'approve', amount: 100 }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval3', { id: '2', sid: 7, subject: 'denied' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval3', { id: '1', sid: 8, subject: 'approve', amount: 100 }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval3', { id: '2', sid: 8, subject: 'approved' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('approval3', { id: '1', sid: 9, subject: 'approve', amount: 10000 }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
});

function denied(s) {
    console.log(s.sid + ' ' + s.getRulesetName() + ' denied');
    console.log(s.getOutput());
    s.status = 'done';
}

function approved(s) {
    console.log(s.sid + ' ' + s.getRulesetName() + ' approved');
    console.log(s.getOutput());
    s.status = 'done';
}

function requestApproval(s) {
    console.log(s.sid + ' ' + s.getRulesetName() + ' requestApproval');
    console.log(s.getOutput());
    if (s.status) {
        s.status = 'approved';
    } else {
        s.status = 'pending';
    }
}
