r = require('../build/release/rules.node');
var cluster = require('cluster');

console.log('books');

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
        }
    })
);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

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

console.log('books2');

handle = r.createRuleset('books2',  
    JSON.stringify({
        ship: {
            whenAll: {
                order: { 
                    $and: [
                        { country: 'US' },
                        { seller: 'bookstore'},
                        { currency: 'US' },
                        { $lte: { amount: 1000 } },
                    ]
                },
                available:  { 
                    $and: [
                        { country: 'US' },
                        { seller: 'bookstore' },
                        { status: 'available'},
                        { item: 'book' },
                    ]
                }
            }
        }
    })
);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

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

console.log('books3');

handle = r.createRuleset('books3',  
    JSON.stringify({
        ship: {
            when: { 
                $and: [
                    { country: 'US' },
                    { seller: 'bookstore'},
                    { currency: 'US' },
                    { $lte: { amount: 1000 } },
                ]
            },
        },
        order: {
            when: { 
                $and: [
                    { country: 'US' },
                    { seller: 'bookstore'},
                    { currency: 'US' },
                    { $lte: { amount: 1000 } },
                ]
            },
        },
    })
);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

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

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);
r.deleteRuleset(handle);

console.log('books4');

handle = r.createRuleset('books4',  
    JSON.stringify({
        ship: {
            when: { $nex: { label: 1 }}
        }
    })
);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

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

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);
r.deleteRuleset(handle);


console.log('approval1');

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
            }
        }
    })
);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

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

console.log('approval2');

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
            }
        }
    })
);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

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

console.log('approval3');

handle = r.createRuleset('approval3', 
    JSON.stringify({
        r1: { 
            when: {$lte: {amount: {$s: 'maxAmount'}}}
        }
    })
);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

r.assertState(handle,
    JSON.stringify({
        id: 'fourth',
        maxAmount: 100
    })
);

console.log(JSON.parse(r.getState(handle, 'fourth')));

r.assertEvent(handle,
    JSON.stringify({
        id: 1,
        sid: 'fourth',
        amount: 1000
    })
);

r.assertEvent(handle,
    JSON.stringify({
        id: 2,
        sid: 'fourth',
        amount: 10
    })
);

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);

console.log('approval4');

handle = r.createRuleset('approval4', 
    JSON.stringify({
        r1: { 
            when: {$lte: {amount: {$r: 'maxAmount'}}}
        }
    })
);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

r.setRulesetState(handle,
    JSON.stringify({maxAmount: 100})
);

console.log(JSON.parse(r.getRulesetState(handle)));

r.assertEvent(handle,
    JSON.stringify({
        id: 1,
        sid: 'fifth',
        amount: 5
    })
);

r.assertEvent(handle,
    JSON.stringify({
        id: 2,
        sid: 'sixth',
        amount: 6
    })
);

r.assertEvent(handle,
    JSON.stringify({
        id: 3,
        sid: 'sixth',
        amount: 100
    })
);

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);

r.deleteRuleset(handle);
