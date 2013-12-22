var d = require('../lib/durable');

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
    pingpong$state: {
        start: {
            toOn: {
                to: 'on'
            }
        },
        on: {
            $state: {
                start: {
                    toPing: {
                        to: 'ping'
                    }
                },
                ping: {
                    toPong: {
                        when: { content: 'pong' },
                        run: ping,
                        to: 'pong'
                    }
                },
                pong: {
                    toPing: {
                        when: { content: 'ping' },
                        run: pong,
                        to: 'ping'
                    }
                }
            },
            reset: {
                when: { content: 'reset' },
                run: reset,
                to: 'on'
            },
            off: {
                when: { content: 'off' },
                run: off,
                to: 'off'
            }
        },
        off: {
        }
    }
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

function ping(s) {
    console.log('ping');
}

function pong(s) {
    console.log('pong');
}

function reset(s) {
    console.log('reset');
}

function off(s) {
    console.log('off');
}

