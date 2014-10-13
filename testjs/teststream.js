var d = require('../libjs/durable');

d.run({
    approval1: {
        r1: {
            whenSome: { $and: [
                { subject: 'approve' },
                { $lte: { amount: 1000 }}
            ]},
            run: requestApproval
        },
    },
    approval2: {
        r1: {
            whenSome: { 
                a$any: {
                    a: { $and: [{ subject: 'approve' }, { $lte: { amount: 1000 }}]},
                    $s: { $nex: { done: 1 } }
                }
            },
            run: requestApproval
        },
    },
    approval3: {
        r1: {
            whenAll: { 
                a$some: { $and: [{ subject: 'approve' }, { $lte: { amount: 1000 }}]},
                $s: { $nex: { done: 1 } }            
            },
            run: requestApproval
        },
    },
    approval4$state: {
        input: {
            request: {
                whenSome: { $and: [{ subject: 'approve' }, { $lte: { amount: 1000 }}]},
                run: requestApproval,
                to: 'pending'
            }
        },
        pending: {
            request: {
                whenAny: {
                    a$some: { subject: 'approved' },
                    b$some: { subject: 'ok' }
                },
                run: requestApproval,
                to: 'pending'
            },
            approve: {
                when: { $s: { done: 1 } },
                run: approved,
                to: 'done'
            }
        },
        done: {
        }
    },
    approval5$flow: {
        input: {
            to: {
                request: { $some:{ $and: [{ subject: 'approve' }, { $lte: { amount: 1000 }}]}},
            }
        },
        request: {
            run: requestApproval,
            to: {
                approve: { $s: { done: 1} },
                request: {
                    $any: {
                        a$some: { subject: 'approved' },
                        b$some: { subject: 'ok'}
                    }
                }
            }
        },
        approve: {
            run: approved
        }
    },

}, '', null, function(host) {
    host.postBatch('approval1', [{ id: '0', sid: 1, subject: 'approve', amount: 100 }, 
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

    host.postBatch('approval2', [{ id: '0', sid: 2, subject: 'approve', amount: 100 }, 
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

    host.postBatch('approval2', [{ id: '5', sid: 2, subject: 'approve', amount: 100 }, 
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

    host.postBatch('approval3', [{ id: '0', sid: 3, subject: 'approve', amount: 100 }, 
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

    host.postBatch('approval3', [{ id: '5', sid: 3, subject: 'approve', amount: 100 }, 
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

    host.postBatch('approval4', [{ id: '0', sid: 4, subject: 'approve', amount: 100 }, 
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

    host.postBatch('approval4', [{ id: '5', sid: 4, subject: 'ok', amount: 100 }, 
                                { id: '6', sid: 4, subject: 'ok', amount: 100 },
                                { id: '7', sid: 4, subject: 'ok', amount: 100 },
                                { id: '8', sid: 4, subject: 'ok', amount: 100 },
                                { id: '9', sid: 4, subject: 'ok', amount: 100 }], 
    function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.postBatch('approval5', [{ id: '0', sid: 5, subject: 'approve', amount: 100 }, 
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

    host.postBatch('approval5', [{ id: '5', sid: 5, subject: 'approved', amount: 100 }, 
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
    console.log('requestApproval');
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
    console.log('approved');
    console.log(s.getOutput());
}
