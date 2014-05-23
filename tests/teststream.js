var d = require('../lib/durable');

d.run({
    approval1: {
        r2: {
            whenSome: { $and: [
                { subject: 'approve' },
                { $lte: { amount: 1000 }}
            ]},
            run: requestApproval
        },
    },
    approval2: {
        r2: {
            whenSome: { 
                a$all: {
                    $m: { $and: [{ subject: 'approve' }, { $lte: { amount: 1000 }}]},
                    $s: { $nex: { done: 1 } }
                }
            },
            run: requestApproval
        },
    }
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
    host.postBatch('approval2', [{ id: '0', sid: 1, subject: 'approve', amount: 100 }, 
                                { id: '1', sid: 1, subject: 'approve', amount: 100 },
                                { id: '2', sid: 1, subject: 'approve', amount: 100 },
                                { id: '3', sid: 1, subject: 'approve', amount: 100 },
                                { id: '4', sid: 1, subject: 'approve', amount: 100 }], 
    function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
    host.postBatch('approval2', [{ id: '5', sid: 1, subject: 'approve', amount: 100 }, 
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