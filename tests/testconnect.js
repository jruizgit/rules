var d = require('../lib/durable');

d.run({
    approval1: {
        r1: {
            whenSome: { $and: [
                { subject: 'approve' },
                { $lte: { amount: 1000 }}
            ]},
            run: requestApproval
        },
    }
}, '', [{ host: 'hoki.redistogo.com', port: 9151, password: '81f67f7b3580d6aa4c539a35a9935064'}], function(host) {
    host.postBatch('approval1', [{ id: '0', sid: 1, subject: 'approve', amount: 100 }, 
                                { id: '1', sid: 1, subject: 'approve', amount: 100 },
                                { id: '2', sid: 1, subject: 'approve', amount: 100 },
                                { id: '3', sid: 1, subject: 'approve', amount: 100 }], 
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
}
