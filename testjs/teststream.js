var d = require('../libjs/durable');

d.run({
    a1: {
        r1: {
            $count: 5,
            all: [{m: {$and: [{subject: 'approve'}, {$lte: {amount: 1000}}]}}],
            run: requestApproval
        },
    },
    a2: {
        r1: {
            any: [
                {$count: 3, a$all: [{b: {$and: [{subject: 'approve' }, {$lte: {amount: 1000 }}]}}]},
                {s: {$and: [{$nex: {done: 1}}, {$s:1}]}}
            ],
            run: requestApproval
        },
    },
    a3: {
        r1: {
            $count: 3,
            all: [ 
                {a: {$and: [{ subject: 'approve' }, { $lte: { amount: 1000 }}]}},
                {s: {$and: [{$nex: {done: 1}}, {$s:1}]}}          
            ],
            run: requestApproval
        },
    },
    a4$state: {
        input: {
            request: {
                $count: 5,
                all: [{m: {$and: [{subject: 'approve'}, {$lte: {amount: 1000}}]}}],
                run: requestApproval,
                to: 'pending'
            }
        },
        pending: {
            request: {
                $count: 5,
                any: [
                    {a: {subject: 'approved'}},
                    {b: {subject: 'ok'}},
                ],
                run: requestApproval,
                to: 'pending'
            },
            approve: {
                all: [{s: {$and: [{done: 1}, {$s:1}]}}],
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
                request: {$count: 5, all:[{m: {$and: [{subject: 'approve'}, {$lte: {amount: 1000}}]}}]},
            }
        },
        request: {
            run: requestApproval,
            to: {
                approve: {all: [{s: {$and: [{done: 1}, {$s: 1}]}}]},
                request: {
                    $count: 5,
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

}, '', null, function(host) {
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
});

function requestApproval(s) {
    console.log(s.getRulesetName() + ' requestApproval');
    console.log(s.getOutput());
    if (!s.count) {
        s.count = 1;
    } else {
        ++s.count;
    }

    if (s.count == 2) {
        s.done = 1;
    }
}

function approved(s) {
    console.log(s.getRulesetName() + ' approved');
    console.log(s.getOutput());
}
