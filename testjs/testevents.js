r = require('../build/release/rules.node');
var cluster = require('cluster');

console.log('fact0');

handle = r.createRuleset('fact0',  
    JSON.stringify({
        suspect: {
            all: [
                {first: {t: 'purchase'}},
                {second: {$neq: {location: {first: 'location'}}}}
            ],
        }
    })
, 100);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

r.assertFact(handle, 
    JSON.stringify({
        id: 1,
        sid: 1,
        t: 'purchase',
        amount: '100',
        location: 'US',
    })
);

r.assertFact(handle, 
    JSON.stringify({
        id: 2,
        sid: 1,
        t: 'purchase',
        amount: '200',
        location: 'CA',
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

console.log('fact1');

handle = r.createRuleset('fact1',  
    JSON.stringify({
        suspect: {
            count: 2,
            all: [
                {first: {t: 'deposit'}},
                {second: {$and: [
                    {t: 'withrawal'},
                    {ip: {first: 'ip'}}, 
                ]}},
                {third: {$and: [
                    {t: 'balance'},
                    {ip: {first: 'ip'}}, 
                    {ip: {second: 'ip' }}, 
                ]}}
            ],
        }
    })
, 100);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

r.assertEvent(handle, 
    JSON.stringify({
        id: 1,
        sid: 1,
        t: 'withrawal',
        ip: '1'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 2,
        sid: 1,
        t: 'balance',
        amount: '100',
        ip: '1'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 3,
        sid: 1,
        t: 'withrawal',
        ip: '1'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 4,
        sid: 1,
        t: 'balance',
        amount: '100',
        ip: '1'
    })
);

r.assertFact(handle, 
    JSON.stringify({
        id: 5,
        sid: 1,
        t: 'deposit',
        ip: '1'
    })
);

result = r.startAction(handle);
console.log(result[1]);
console.log(result[2]);
r.completeAction(handle, result[0], result[1]);
r.deleteRuleset(handle);

console.log('fact2');

handle = r.createRuleset('fact2',  
    JSON.stringify({
        suspect: {
            count: 3,
            all: [
                {first: {t: 'deposit'}},
                {second: {$and: [
                    {t: 'withrawal'},
                    {ip: {first: 'ip'}}, 
                ]}},
                {third: {$and: [
                    {t: 'balance'},
                    {ip: {second: 'ip' }}, 
                ]}},
                {fourth: {$and: [
                    {t: 'chargeback'},
                    {ip: {third: 'ip' }}, 
                ]}}
            ],
        }
    })
, 100);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

r.assertEvent(handle, 
    JSON.stringify({
        id: 1,
        sid: 1,
        t: 'deposit',
        ip: '1'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 2,
        sid: 1,
        t: 'deposit',
        amount: '100',
        ip: '1'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 3,
        sid: 1,
        t: 'deposit',
        amount: '100',
        ip: '1'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 4,
        sid: 1,
        t: 'balance',
        ip: '1'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 5,
        sid: 1,
        t: 'balance',
        amount: '100',
        ip: '1'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 6,
        sid: 1,
        t: 'balance',
        amount: '100',
        ip: '1'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 7,
        sid: 1,
        t: 'chargeback',
        ip: '1'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 8,
        sid: 1,
        t: 'chargeback',
        amount: '100',
        ip: '1'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 9,
        sid: 1,
        t: 'chargeback',
        amount: '100',
        ip: '1'
    })
);

r.assertFact(handle, 
    JSON.stringify({
        id: 10,
        sid: 1,
        t: 'withrawal',
        ip: '1'
    })
);

result = r.startAction(handle);
console.log(result[1]);
console.log(result[2]);
r.completeAction(handle, result[0], result[1]);
r.deleteRuleset(handle);

console.log('fact3');

handle = r.createRuleset('fact3',  
    JSON.stringify({
        suspect: {
            all: [
                {first: {t: 'deposit'}},
                {second$not: {$and: [{t: 'withrawal'}, {ip: {first: 'ip'}}]}},
                {third: {$and: [{t: 'chargeback'}, {ip: {first: 'ip' }}]}}
            ],
        }
    })
, 100);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

r.assertEvent(handle, 
    JSON.stringify({
        id: 1,
        sid: 1,
        t: 'deposit',
        ip: '1'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 2,
        sid: 1,
        t: 'chargeback',
        ip: '1'
    })
);

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);

r.assertFact(handle, 
    JSON.stringify({
        id: 3,
        sid: 1,
        t: 'withrawal',
        ip: 1
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 4,
        sid: 1,
        t: 'deposit',
        ip: '1'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 5,
        sid: 1,
        t: 'chargeback',
        ip: '1'
    })
);

r.retractFact(handle, 
    JSON.stringify({
        id: 3,
        sid: 1,
        t: 'withrawal',
        ip: 1
    })
);

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);

console.log('pri0');

handle = r.createRuleset('pri0',  
    JSON.stringify({
        ship: {
            pri: 2,
            all: [ 
                {m: {$and: [
                    { country: 'US' },
                    { seller: 'bookstore'},
                    { currency: 'US' },
                    { $lte: { amount: 1000 } },
                ]}}
            ],
        },
        order: {
            pri: 1,
            all: [ 
                {m: {$and: [
                    { country: 'US' },
                    { seller: 'bookstore'},
                    { currency: 'US' },
                    { $lte: { amount: 1000 } },
                ]}}
            ],
        },
        submit: {
            pri: 0,
            all: [ 
                {m: {$and: [
                    { country: 'US' },
                    { seller: 'bookstore'},
                    { currency: 'US' },
                    { $lte: { amount: 1000 } },
                ]}}
            ],
        },
    })
, 100);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

r.assertFact(handle, 
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
result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);
r.retractFact(handle, 
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
console.log(result == null);

r.deleteRuleset(handle);

console.log('add0');

handle = r.createRuleset('add0',  
    JSON.stringify({
        suspect: {
            all: [
                {first: {t: 'purchase'}},
                {second: {amount: {$add: {$l: {first: 'amount'}, $r: 1}}}},
                {third: {amount: {$add: {$l: {first: 'amount'}, $r: {second: 'amount'}}}}}
            ],
        }
    })
, 100);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

r.assertEvent(handle, 
    JSON.stringify({
        id: 1,
        sid: 1,
        t: 'purchase',
        amount: 100
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 2,
        sid: 1,
        t: 'purchase',
        amount: 101
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 3,
        sid: 1,
        t: 'purchase',
        amount: 201
    })
);

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);
r.deleteRuleset(handle);

console.log('add1');

handle = r.createRuleset('add1', 
    JSON.stringify({
        r1: {all:[
            {m: 
                {$lte: 
                    {amount: {
                        $add: { 
                            $l: {$s: { name: 'maxAmount', id: 1}}, 
                            $r: {$s: { name: 'minAmount', id: 2}}
                        }  
                    }}
                }
            }
        ]}
    })
, 10);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

console.log(r.assertState(handle,
    JSON.stringify({
        sid: 1,
        maxAmount: 300
    })
));

console.log(r.assertState(handle,
    JSON.stringify({
        sid: 2,
        minAmount: 200
    })
));

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 1,
        sid: 3,
        amount: 600 
    })
));

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 2,
        sid: 3,
        amount: 400 
    })
));

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);

r.deleteRuleset(handle);

console.log('add2');

handle = r.createRuleset('add2', 
    JSON.stringify({
        r1: {all:[
                {m: {name: {$add: {$l: {$s: {name: 'first_name', id: 1}}, $r: {$s: {name: 'last_name', id: 2}}}}}}
            ]}
        })
, 10);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

console.log(r.assertState(handle,
    JSON.stringify({
        sid: 1,
        first_name: "hello"
    })
));

console.log(r.assertState(handle,
    JSON.stringify({
        sid: 2,
        last_name: "world"
    })
));

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 1,
        sid: 3,
        name: "helloworld"
    })
));

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 2,
        sid: 3,
        name: "hellothere" 
    })
));

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);

r.deleteRuleset(handle);

console.log('fraud0');

handle = r.createRuleset('fraud0',  
    JSON.stringify({
        suspect: {
            all: [
                {first: {t: 'purchase'}},
                {second: {$neq: {location: {first: 'location'}}}}
            ],
        }
    })
, 100);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

r.assertEvent(handle, 
    JSON.stringify({
        id: 1,
        sid: 1,
        t: 'purchase',
        amount: '100',
        location: 'US',
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 2,
        sid: 1,
        t: 'purchase',
        amount: '200',
        location: 'CA',
    })
);

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);

r.assertEvent(handle, 
    JSON.stringify({
        id: 3,
        sid: 1,
        t: 'purchase',
        amount: '100',
        location: 'US',
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 4,
        sid: 1,
        t: 'purchase',
        amount: '200',
        location: 'CA',
    })
);

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);
r.deleteRuleset(handle);
                    
console.log('fraud2');

handle = r.createRuleset('fraud2',  
    JSON.stringify({
        suspect: {
            all: [
                {first: {t: 'purchase'}},
                {second: { 
                    $and: [
                        {ip: {first: 'ip'}}, 
                        {$neq: {cc: {first: 'cc'}}} 
                    ]
                }},
                {third: { 
                    $and: [
                        {ip: {first: 'ip'}}, 
                        {$neq: {cc: {first: 'cc'}}}, 
                        {ip: {second: 'ip' }}, 
                        {$neq: {cc: {second: 'cc'}}}
                    ]
                }}
            ],
        }
    })
, 100);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

r.assertEvent(handle, 
    JSON.stringify({
        id: 1,
        sid: 1,
        t: 'purchase',
        amount: '100',
        ip: '1',
        cc: 'a'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 2,
        sid: 1,
        t: 'purchase',
        amount: '100',
        ip: '1',
        cc: 'b'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 3,
        sid: 1,
        t: 'purchase',
        amount: '100',
        ip: '1',
        cc: 'c'
    })
);

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);
r.deleteRuleset(handle);

console.log('fraud3');

handle = r.createRuleset('fraud3',  
    JSON.stringify({
        suspect: {
            all: [
                {first: {t: 'purchase'}},
                {second: {$or: [{$neq: {location: {first: 'location'}}}, {amount: {first: 'amount'}}]}}
            ],
        }
    })
, 100);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

r.assertEvent(handle, 
    JSON.stringify({
        id: 1,
        sid: 1,
        t: 'purchase',
        amount: '100',
        location: 'US',
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 2,
        sid: 1,
        t: 'purchase',
        amount: '100',
        location: 'US',
    })
);

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);
r.deleteRuleset(handle);

console.log('fraud4');

handle = r.createRuleset('fraud4',  
    JSON.stringify({
        suspect: {
            all: [
                {first: {t: 'purchase'}},
                {second: {$or:[ 
                            {$and: [{$neq: {location: {first: 'location'}}}, {amount: {first: 'amount'}}]},
                            {$and: [{$neq: {location: {first: 'location'}}}, {cc: {first: 'cc'}}]},
                        ]}
                }
            ],
        }
    })
, 100);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

r.assertEvent(handle, 
    JSON.stringify({
        id: 1,
        sid: 1,
        t: 'purchase',
        amount: '100',
        location: 'US',
        cc: 'c'
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 2,
        sid: 1,
        t: 'purchase',
        amount: '200',
        location: 'CA',
        cc: 'c'
    })
);

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);
r.deleteRuleset(handle);

console.log('approval0');

handle = r.createRuleset('approval0', 
    JSON.stringify({
        r1: { 
            all: [{m: {$lte: {amount: 100 }}}]
        }
    })
, 4);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 1,
        sid: 1,
        amount: 1000
    })
));

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 2,
        sid: 1,
        amount: 10
    })
));

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);
r.deleteRuleset(handle);

console.log('books');

handle = r.createRuleset('books',  
    JSON.stringify({
        ship: {
            all: [
                {order: { 
                    $and: [
                        { $lte: { amount: 1000 } },
                        { country: 'US' },
                        { currency: 'US' },
                        { seller: 'bookstore'} 
                    ]
                }},
                {available: { 
                    $and: [
                        { item: 'book' },
                        { country: 'US' },
                        { seller: 'bookstore' },
                        { status: 'available'} 
                    ]
                }}
            ],
        }
    })
, 100);

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
            all: [
                {order: { 
                    $and: [
                        { country: 'US' },
                        { seller: 'bookstore'},
                        { currency: 'US' },
                        { $lte: { amount: 1000 } },
                    ]
                }},
                {available:  { 
                    $and: [
                        { country: 'US' },
                        { seller: 'bookstore' },
                        { status: 'available'},
                        { item: 'book' },
                    ]
                }}
            ]
        }
    })
, 100);

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
            all: [ 
                {m: {$and: [
                    { country: 'US' },
                    { seller: 'bookstore'},
                    { currency: 'US' },
                    { $lte: { amount: 1000 } },
                ]}}
            ],
        },
        order: {
            all: [ 
                {m: {$and: [
                    { country: 'US' },
                    { seller: 'bookstore'},
                    { currency: 'US' },
                    { $lte: { amount: 1000 } },
                ]}}
            ],
        },
    })
, 100);

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
            all: [{m: {$nex: {label: 1}}}]
        }
    })
, 100);

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
    JSON.stringify({ r1:{
            all: [
                {a$any: [
                    {b: {subject: 'approve'}},
                    {c: {subject: 'review' }}
                ]},
                {d$any: [
                    {e: { $lt: { total: 1000 }}},
                    {f: { $lt: { amount: 1000 }}}
                ]}
            ]
        }
    })
, 100);

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
            any: [
                {a$all: [
                    {b: {subject: 'approve'}},
                    {c: {subject: 'review'}}
                ]},
                {d$all: [
                    {e: {$lt: {total: 1000}}},
                    {f: {$lt: {amount: 1000}}}
                ]}
            ]
        }
    })
, 100);

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
            all: [{m: {$lte: {amount: {$s: 'maxAmount'}}}}]
        }
    })
, 4);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

console.log(r.assertState(handle,
    JSON.stringify({
        sid: 1,
        maxAmount: 100
    })
));

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 1,
        sid: 1,
        amount: 1000
    })
));

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 2,
        sid: 1,
        amount: 10
    })
));

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);

r.assertState(handle,
    JSON.stringify({
        sid: 1,
        maxAmount: 10000
    })
);

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 3,
        sid: 1,
        amount: 1000
    })
));

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 4,
        sid: 2,
        amount: 1000
    })
));

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 5,
        sid: 3,
        amount: 1000
    })
));

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 6,
        sid: 4,
        amount: 1000
    })
));

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 7,
        sid: 5,
        amount: 1000
    })
));

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 8,
        sid: 1,
        amount: 1000
    })
));

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);

r.deleteRuleset(handle);

console.log('approval4');

handle = r.createRuleset('approval4', 
    JSON.stringify({
        r1: { 
            all:[{m: {$lte: {amount: {$s: { name: 'maxAmount', id: 1}}}}}]
        },
        r2: { 
            all:[{m: {$gte: {amount: {$s: { name: 'minAmount', id: 2}}}}}]
        },
    })
, 4);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

console.log(r.assertState(handle,
    JSON.stringify({
        sid: 1,
        maxAmount: 300
    })
));

console.log(r.assertState(handle,
    JSON.stringify({
        sid: 2,
        minAmount: 200
    })
));

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 1,
        sid: 3,
        amount: 500 
    })
));

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);

console.log(r.assertEvent(handle,
    JSON.stringify({
        id: 2,
        sid: 3,
        amount: 100
    })
));

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);

r.deleteRuleset(handle);