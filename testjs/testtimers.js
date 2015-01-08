var d = require('../libjs/durable');

d.run({
    timer1: {
        r1: {
            all: [{m: {start: 'yes'}}],
            run: function (s) {
                s.start = new Date().toString();
                s.startTimer('myTimer', 5);
            }
        },
        r2: {
            all: [{m: {$t: 'myTimer'}}],
            run: function (s) {
                console.log('started @' + s.start);
                console.log('timeout @' + new Date());
            }
        }
    },
    timer2$state: {
        input: {
            review: {
                all: [{m: {subject: 'approve'}}],
                run: {
                    first$state: {
                        send: {
                            start: {
                                run: function (s) { s.startTimer('first', 4); },
                                to: 'evaluate'
                            }
                        },
                        evaluate: {
                            wait: {
                                all: [{m: {$t: 'first'}}],
                                run: function (s) { s.signal({ id: 2, subject: 'approved' }); },
                                to: 'end'
                            }
                        },
                        end: {
                        }

                    },
                    second$state: {
                        send: {
                            start: {
                                run: function (s) { s.startTimer('second', 3); },
                                to: 'evaluate'
                            }
                        },
                        evaluate: {
                            wait: {
                                all: [{m: {$t: 'second'}}],
                                run: function (s) { s.signal({ id: 3, subject: 'denied' }); },
                                to: 'end'
                            }
                        },
                        end: {
                        }
                    }
                },
                to: 'pending'
            }
        },
        pending: {
            approve: {
                all: [{m: {subject: 'approved'}}],
                run: function (s) { console.log('approved'); },
                to: 'approved'
            },
            deny: {
                all: [{m: {subject: 'denied'}}],
                run: function (s) { console.log('denied'); },
                to: 'denied'
            }
        },
        denied: {
        },
        approved: {
        }
    }
}, '', null, function(host) {
    host.post('timer1', { id: 1, sid: 1, start: 'yes' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('timer2', { id: '1', sid: 1, subject: 'approve' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
});