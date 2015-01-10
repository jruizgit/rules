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
    a1: {
        r1: {
            count: 5,
            all: [{m: {$and: [{subject: 'approve'}, {$lte: {amount: 1000}}]}}],
            run: requestApproval
        },
    },
    a2: {
        r1: {
            count: 3,
            any: [
                {a$all: [{b: {$and: [{subject: 'approve' }, {$lte: {amount: 1000 }}]}}]},
                {s: {$and: [{$nex: {status: 1}}, {$s:1}]}}
            ],
            run: requestApproval
        },
    },
    a3: {
        r1: {
            count: 3,
            all: [ 
                {a: {$and: [{ subject: 'approve' }, { $lte: { amount: 1000 }}]}},
                {s: {$and: [{$nex: {status: 1}}, {$s:1}]}}          
            ],
            run: requestApproval
        },
    },
    a4$state: {
        input: {
            request: {
                count: 5,
                all: [{m: {$and: [{subject: 'approve'}, {$lte: {amount: 1000}}]}}],
                run: requestApproval,
                to: 'pending'
            }
        },
        pending: {
            request: {
                count: 5,
                any: [
                    {a: {subject: 'approved'}},
                    {b: {subject: 'ok'}},
                ],
                run: requestApproval,
                to: 'pending'
            },
            approve: {
                all: [{s: {$and: [{status: 'approved'}, {$s:1}]}}],
                run: approved,
                to: 'done'
            }
        },
        done: {
        }
    },
    a5$flow: {
        input: {
            to: {
                request: {count: 5, all:[{m: {$and: [{subject: 'approve'}, {$lte: {amount: 1000}}]}}]},
            }
        },
        request: {
            run: requestApproval,
            to: {
                approve: {all: [{s: {$and: [{status: 'approved'}, {$s: 1}]}}]},
                request: {
                    count: 5,
                    any: [
                        {a: {subject: 'approved'}},
                        {b: {subject: 'ok'}}
                    ]
                }
            }
        },
        approve: {
            run: approved
        }
    },
    timer1: {
        r1: {
            all: [{m: {start: 'yes'}}],
            run: function (c) {
                c.s.start = new Date().toString();
                c.startTimer('myTimer', 5);
            }
        },
        r2: {
            all: [{m: {$t: 'myTimer'}}],
            run: function (c) {
                console.log('started @' + c.s.start);
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
                                run: function (c) { c.startTimer('first', 4); },
                                to: 'evaluate'
                            }
                        },
                        evaluate: {
                            wait: {
                                all: [{m: {$t: 'first'}}],
                                run: function (c) { c.signal({ id: 2, subject: 'approved' }); },
                                to: 'end'
                            }
                        },
                        end: {
                        }

                    },
                    second$state: {
                        send: {
                            start: {
                                run: function (c) { c.startTimer('second', 3); },
                                to: 'evaluate'
                            }
                        },
                        evaluate: {
                            wait: {
                                all: [{m: {$t: 'second'}}],
                                run: function (c) { c.signal({ id: 3, subject: 'denied' }); },
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
                run: function (c) { console.log('approved'); },
                to: 'approved'
            },
            deny: {
                all: [{m: {subject: 'denied'}}],
                run: function (c) { console.log('denied'); },
                to: 'denied'
            }
        },
        denied: {
        },
        approved: {
        }
    }
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
    host.postBatch('a1', [{ id: '0', sid: 1, subject: 'approve', amount: 100 }, 
                          { id: '1', sid: 1, subject: 'approve', amount: 100 },
                          { id: '2', sid: 1, subject: 'approve', amount: 100 },
                          { id: '3', sid: 1, subject: 'approve', amount: 100 },
                          { id: '4', sid: 1, subject: 'approve', amount: 100 }, 
                          { id: '5', sid: 1, subject: 'approve', amount: 100 }, 
                          { id: '6', sid: 1, subject: 'approve', amount: 100 },
                          { id: '7', sid: 1, subject: 'approve', amount: 100 },
                          { id: '8', sid: 1, subject: 'approve', amount: 100 },
                          { id: '9', sid: 1, subject: 'approve', amount: 100 }], 
    function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.postBatch('a2', [{ id: '0', sid: 2, subject: 'approve', amount: 100 }, 
                          { id: '1', sid: 2, subject: 'approve', amount: 100 },
                          { id: '2', sid: 2, subject: 'approve', amount: 100 },
                          { id: '3', sid: 2, subject: 'approve', amount: 100 },
                          { id: '4', sid: 2, subject: 'approve', amount: 100 }], 
    function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.postBatch('a2', [{ id: '5', sid: 2, subject: 'approve', amount: 100 }, 
                          { id: '6', sid: 2, subject: 'approve', amount: 100 },
                          { id: '7', sid: 2, subject: 'approve', amount: 100 },
                          { id: '8', sid: 2, subject: 'approve', amount: 100 },
                          { id: '9', sid: 2, subject: 'approve', amount: 100 }], 
    function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.postBatch('a3', [{ id: '0', sid: 3, subject: 'approve', amount: 100 }, 
                          { id: '1', sid: 3, subject: 'approve', amount: 100 },
                          { id: '2', sid: 3, subject: 'approve', amount: 100 },
                          { id: '3', sid: 3, subject: 'approve', amount: 100 },
                          { id: '4', sid: 3, subject: 'approve', amount: 100 }], 
    function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.postBatch('a3', [{ id: '5', sid: 3, subject: 'approve', amount: 100 }, 
                          { id: '6', sid: 3, subject: 'approve', amount: 100 },
                          { id: '7', sid: 3, subject: 'approve', amount: 100 },
                          { id: '8', sid: 3, subject: 'approve', amount: 100 },
                          { id: '9', sid: 3, subject: 'approve', amount: 100 }], 
    function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.postBatch('a4', [{ id: '0', sid: 4, subject: 'approve', amount: 100 }, 
                          { id: '1', sid: 4, subject: 'approve', amount: 100 },
                          { id: '2', sid: 4, subject: 'approve', amount: 100 },
                          { id: '3', sid: 4, subject: 'approve', amount: 100 },
                          { id: '4', sid: 4, subject: 'approve', amount: 100 }], 
    function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.postBatch('a4', [{ id: '5', sid: 4, subject: 'approved' }, 
                          { id: '6', sid: 4, subject: 'approved' },
                          { id: '7', sid: 4, subject: 'approved' },
                          { id: '8', sid: 4, subject: 'approved' },
                          { id: '9', sid: 4, subject: 'approved' }], 
    function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.postBatch('a5', [{ id: '0', sid: 5, subject: 'approve', amount: 100 }, 
                                { id: '1', sid: 5, subject: 'approve', amount: 100 },
                                { id: '2', sid: 5, subject: 'approve', amount: 100 },
                                { id: '3', sid: 5, subject: 'approve', amount: 100 },
                                { id: '4', sid: 5, subject: 'approve', amount: 100 }], 
    function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.postBatch('a5', [{ id: '5', sid: 5, subject: 'approved', amount: 100 }, 
                                { id: '6', sid: 5, subject: 'approved', amount: 100 },
                                { id: '7', sid: 5, subject: 'approved', amount: 100 },
                                { id: '8', sid: 5, subject: 'approved', amount: 100 },
                                { id: '9', sid: 5, subject: 'approved', amount: 100 }], 
    function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
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

function denied(c) {
    console.log(c.s.sid + ' ' + c.getRulesetName() + ' denied');
    console.log(c.m);
    c.s.status = 'done';
}

function approved(c) {
    console.log(c.s.sid + ' ' + c.getRulesetName() + ' approved');
    console.log(c.m);
    c.s.status = 'done';
}

function requestApproval(c) {
    console.log(c.s.sid + ' ' + c.getRulesetName() + ' requestApproval');
    console.log(c.m);
    if (c.s.status) {
        c.s.status = 'approved';
    } else {
        c.s.status = 'pending';
    }
}


