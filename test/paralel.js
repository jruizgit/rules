var d = require('../lib/durable');

d.run({
    timer: {
        r1: {
            when: { start: 'yes' },
            run: function (s) {
                s.start = new Date().toString();
                s.startTimer('myTimer', 5);
            }
        },
        r2: {
            when: { $t: 'myTimer' },
            run: function (s) {
                console.log('started @' + s.start);
                console.log('timeout @' + new Date());
            }
        }
    },
    parallel: {
        r1: {
            when: { start: 'yes' },
            run: {
                one: {
                    r1: {
                        when: { $s: { $nex: { start: 1 } } },
                        run: function (s) {
                            s.start = new Date().toString();
                            s.startTimer('myTimer', 3);
                        }
                    },
                    r2: {
                        when: { $t: 'myTimer' },
                        run: function (s) {
                            console.log('one @' + s.start);
                            console.log('one timeout @' + new Date().toString());
                            s.signal({ id: 2, end: 'one' });
                        }
                    }
                },
                two: {
                    r1: {
                        when: { $s: { $nex: { start: 1 } } },
                        run: function (s) {
                            s.start = new Date().toString();
                            s.startTimer('myTimer', 6);
                        }
                    },
                    r2: {
                        when: { $t: 'myTimer' },
                        run: function (s) {
                            console.log('two @' + s.start);
                            console.log('two timeout @' + new Date().toString());
                            s.signal({ id: 3, end: 'two' });
                        }
                    }
                }
            }
        },
        r2: {
            whenAll: {
                first: { end: 'one' },
                second: { end: 'two' }
            },
            run: function (s) { console.log('done'); }
        }
    },
    approval$state: {
        input: {
            review: {
                when: { subject: 'approve' },
                run: {
                    first$state: {
                        send: {
                            start: {
                                run: function (s) { s.startTimer('first', Math.floor(Math.random() * 6)); },
                                to: 'evaluate'
                            }
                        },
                        evaluate: {
                            wait: {
                                when: { $t: 'first' },
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
                                run: function (s) { s.startTimer('second', Math.floor(Math.random() * 3)); },
                                to: 'evaluate'
                            }
                        },
                        evaluate: {
                            wait: {
                                when: { $t: 'second' },
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
                when: { subject: 'approved' },
                run: function (s) { console.log('approved'); },
                to: 'approved'
            },
            deny: {
                when: { subject: 'denied' },
                run: function (s) { console.log('denied'); },
                to: 'denied'
            }
        },
        denied: {
        },
        approved: {
        }
    }

}, '', null, function (host) {
    host.post({ id: '1', program: 'timer', sid: 1, start: 'yes' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post({ id: '1', program: 'parallel', sid: 1, start: 'yes' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post({ id: '1', program: 'approval', sid: 1, subject: 'approve' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
});