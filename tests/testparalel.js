var d = require('../lib/durable');

d.run({
    paralel1: {
        r1: {
            when: { start: 'yes' },
            run: {
                one: {
                    r1: {
                        when: { $s: { $nex: { start: 1 }}},
                        run: function (s) { s.start = 1; }
                    },
                    r2: {
                        when: { $s: { start: 1 } },
                        run: function (s) { s.signal({ id: 1, end: 'one' }); s.start = 2; }
                    }
                },
                two: {
                    r1: {
                        when: { $s: { $nex: { start: 1 }}},
                        run: function (s) { s.start = 1; }
                    },
                    r2: {
                        when: { $s: { start: 1 }},
                        run: function (s) { s.signal({ id: 1, end: 'two' }); s.start = 2;  }
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
    paralel2$state: {
        input: {
            get: {
                when: { subject: 'approve' },
                run: function(s) { s.quantity = s.getOutput().quantity; },
                to: 'process'
            },
        },
        process: {
            review: {
                when: { $s: { $ex: {quantity: 1}}},
                run: {
                    first$state: {
                        evaluate: {
                            wait: {
                                when: { $s: { $lte: { quantity: 5 }}}, 
                                run: function (s) { s.signal({ id: 1, subject: 'approved' }); },
                                to: 'end'
                            }
                        },
                        end: {
                        }

                    },
                    second$state: {
                        evaluate: {
                            wait: {
                                when: { $s: { $gt: { quantity: 5 }}},
                                run: function (s) { s.signal({ id: 1, subject: 'denied' }); },
                                to: 'end'
                            }
                        },
                        end: {
                        }
                    }
                },
                to: 'result'
            }
        },
        result: {
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
    },
    paralel3$flow: {
        start: {
            to: { input: { subject: 'approve' }}
        },
        input: {
            run: function(s) { s.quantity = s.getOutput().quantity; },
            to: 'process'
        },
        process: {
            run: {
                first$flow: {
                    start: {
                        to: { end: { $s: { $lte: { quantity: 5 }}}}
                    },
                    end: {
                        run: function (s) { s.signal({ id: 1, subject: 'approved' }); }
                    }

                },
                second$flow: {
                    start: {
                        to: { end: { $s: { $gt: { quantity: 5 }}}}
                    },
                    end: {
                        run: function (s) { s.signal({ id: 1, subject: 'denied' }); }
                    }
                }
            },
            to: {
                approve: { subject: 'approved' },
                deny: { subject: 'denied'},
            }
        },
        approve: {
            run: function (s) { console.log('approved'); }
        },
        deny: {
            run: function (s) { console.log('denied'); }
        }
    },
}, '', null, function(host) {
    host.post('paralel1', { id: 1, sid: 1, start: 'yes' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('paralel2', { id: 1, sid: 1, subject: 'approve', quantity: 4 }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('paralel2', { id: 2, sid: 2, subject: 'approve', quantity: 10 }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('paralel3', { id: 1, sid: 1, subject: 'approve', quantity: 4 }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.post('paralel3', { id: 2, sid: 2, subject: 'approve', quantity: 10 }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
});