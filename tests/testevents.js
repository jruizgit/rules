r = require('../build/release/rules.node');
var cluster = require('cluster');


handle = r.createRuleset('books',  
    JSON.stringify({
        ship: {
            whenAll: {
                order: { 
                    $and: [
                        { $lte: { amount: 1000 } },
                        { country: 'US' },
                        { currency: 'US' },
                        { seller: 'bookstore'} 
                    ]
                },
                available:  { 
                    $and: [
                        { item: 'book' },
                        { country: 'US' },
                        { seller: 'bookstore' },
                        { status: 'available'} 
                    ]
                }
            },
            run: 'ship'
        }
    })
);

r.bindRuleset(handle, '/tmp/redis.sock');

r.assertEvent(handle, 
    JSON.stringify({
        id: 1,
        sid: 'first',
        name: 'John Smith',
        address: '1111 NE 22, Seattle, Wa',
        phone: '206678787',
        country: 'US',
        currency: 'US',
        seller: 'bookstore',
        item: 'book',
        reference: '75323',
        amount: 500
    })
);

r.assertEvent(handle,
    JSON.stringify({
        id: 2,
        sid: 'first',
        item: 'book',
        status: 'available',
        country: 'US',
        seller: 'bookstore'
    })
);

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);
r.deleteRuleset(handle);

handle = r.createRuleset('approval1', 
    JSON.stringify({ r1: {
            whenAll: {
                a$any: {
                    b: { subject: 'approve' },
                    c: { subject: 'review' }
                },
                d$any: {
                    e: { $lt: { total: 1000 }},
                    f: { $lt: { amount: 1000 }}
                }
            },
            run: 'unitTest'
        }
    })
);

r.bindRuleset(handle, '/tmp/redis.sock');

r.assertEvent(handle,
    JSON.stringify({
        id: 3,
        sid: 'second',
        subject: 'approve'
    })
);

r.assertEvent(handle,
    JSON.stringify({
        id: 4,
        sid: 'second',
        amount: 100
    })
);

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);

r.deleteRuleset(handle);

handle = r.createRuleset('approval2', 
    JSON.stringify({ r2: {
            whenAny: {
                a$all: {
                    b: { subject: 'approve' },
                    c: { subject: 'review' }
                },
                d$all: {
                    e: { $lt: { total: 1000 }},
                    f: { $lt: { amount: 1000 }}
                }
            },
            run: 'unitTest'
        }
    })
);

r.bindRuleset(handle, '/tmp/redis.sock');

r.assertEvent(handle,
    JSON.stringify({
        id: 1,
        sid: 'third',
        subject: 'approve'
    })
);

r.assertEvent(handle,
    JSON.stringify({
        id: 2,
        sid: 'third',
        subject: 'review'
    })
);

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);

r.deleteRuleset(handle);
