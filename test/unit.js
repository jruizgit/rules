var d = require('../lib/durable');

d.run({
    unitAny: {
        r1: {
            whenAny: {
                a: { $and: [
                    { subject: 'approve' },
                    { $lt: { total: 1000 }}
                ]},
                b: { $and: [
                    { subject: 'approve' },
                    { $lt: { amount: 1000 }}
                ]}
            },
            run: unitTest
        }
    },
    unitAll: {
        r1: {
            whenAll: {
                a: { subject: 'approve' },
                b: { $lt: { amount: 1000 }}
            },
            run: unitTest
        }
    },
    unitAllAny: {
        r1: {
            whenAll: {
                a$any: {
                    a: { subject: 'approve' },
                    b: { subject: 'review' }
                },
                b$any: {
                    a: { $lt: { total: 1000 }},
                    b: { $lt: { amount: 1000 }}
                }
            },
            run: unitTest
        }
    },
    unitAnyAll: {
        r1: {
            whenAny: {
                a$all: {
                    a: { subject: 'approve' },
                    b: { $lt: { total: 1000 }}
                },
                b$all: {
                    a: { subject: 'review' },
                    b: { $lt: { amount: 1000 }}
                }
            },
            run: unitTest
        }
    },
    unitAnyAny: {
        r1: {
            whenAny: {
                a$any: {
                    a: { subject: 'approve' },
                    b: { $lt: { total: 1000 }}
                },
                b$any: {
                    a: { subject: 'review' },
                    b: { $lt: { amount: 1000 }}
                }
            },
            run: unitTest
        }
    },
    unitAllAll: {
        r1: {
            whenAll: {
                a$all: {
                    a: { subject: 'approve' },
                    b: { $lt: { total: 1000 }}
                },
                b$all: {
                    a: { subject: 'review' },
                    b: { $lt: { amount: 1000 }}
                }
            },
            run: unitTest
        }
    }
}, '', null, function (host) {
    host.post({ id: '1', program: 'unitAny', sid: 1, subject: 'approve', amount: 100 }, function (err) {
        if (err) {
            console.log(err);
        }
        else {
            console.log('ok');
        }
    });

    host.post({ id: '1', program: 'unitAll', sid: 1, subject: 'approve' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

//    host.post({ id: '2', program: 'unitAll', sid: 1, amount: '100' }, function (err) {
//        if (err) {
//            console.log(err);
//        } else {
//            console.log('ok');
//        }
//    });

    host.post({ id: '1', program: 'unitAllAny', sid: 1, subject: 'approve' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.post({ id: '2', program: 'unitAllAny', sid: 1, amount: '100' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.post({ id: '1', program: 'unitAnyAll', sid: 1, subject: 'approve' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.post({ id: '2', program: 'unitAnyAll', sid: 1, total: '100' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.post({ id: '1', program: 'unitAnyAny', sid: 1, subject: 'approve' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.post({ id: '1', program: 'unitAllAll', sid: 1, subject: 'approve' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.post({ id: '2', program: 'unitAllAll', sid: 1, total: '100' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.post({ id: '3', program: 'unitAllAll', sid: 1, subject: 'review' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });

    host.post({ id: '4', program: 'unitAllAll', sid: 1, amount: '100' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
});

function unitTest(s) {
    console.log('test');
    console.log(JSON.stringify(s));
    console.log(JSON.stringify(s.getOutput()));
}