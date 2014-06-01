r = require('../build/release/rules.node');
var cluster = require('cluster');
if (cluster.isMaster) {
    for (var j = 0; j < 32; j++) {
        cluster.fork();
    }

    cluster.on("exit", function (worker, code, signal) {
        cluster.fork();
    });
} else {
    handle = r.createRuleset('somebooks' + cluster.worker.id,  
        JSON.stringify({
            ship: {
                whenSome: { 
                    $and: [
                        { $lte: { amount: 1000 } },
                        { country: 'US' },
                        { currency: 'US' },
                        { seller: 'bookstore'} 
                    ]
                },
                run: 'ship'
            }
        })
    );

    r.bindRuleset(handle, '/tmp/redis0.sock');
    r.bindRuleset(handle, '/tmp/redis1.sock');
    r.bindRuleset(handle, '/tmp/redis2.sock');
    r.bindRuleset(handle, '/tmp/redis3.sock');
    r.bindRuleset(handle, '/tmp/redis4.sock');
    r.bindRuleset(handle, '/tmp/redis5.sock');
    r.bindRuleset(handle, '/tmp/redis6.sock');
    r.bindRuleset(handle, '/tmp/redis7.sock');
    r.bindRuleset(handle, '/tmp/redis8.sock');
    r.bindRuleset(handle, '/tmp/redis9.sock');
    r.bindRuleset(handle, '/tmp/redis10.sock');
    r.bindRuleset(handle, '/tmp/redis11.sock');
    r.bindRuleset(handle, '/tmp/redis12.sock');
    r.bindRuleset(handle, '/tmp/redis13.sock');
    r.bindRuleset(handle, '/tmp/redis14.sock');
    r.bindRuleset(handle, '/tmp/redis15.sock');

    console.log('Start somebooks negative: ' + new Date());

    for (var m = 0; m < 1000; ++m) {
        for (var i = 0; i < 16; ++i) {
            var events = [];
            for (var ii = 0; ii < 50; ++ii) {
                events.push({
                    id: ii + '_' + m,
                    sid: i,
                    name: 'John Smith',
                    address: '1111 NE 22, Seattle, Wa',
                    phone: '206678787',
                    country: 'US',
                    currency: 'US',
                    seller: 'bookstore',
                    item: 'book',
                    reference: '75323',
                    amount: 5000
                });
            }

            r.assertEvents(handle, JSON.stringify(events));
        }
    }

    console.log('End somebooks negative: ' + new Date());

    console.log('Start somebooks positive: ' + new Date());

    for (var m = 0; m < 125; ++m) {
        for (var i = 0; i < 16; ++i) {
            var events = [];
            for (var ii = 0; ii < 40; ++ii) {
                events.push({
                    id: ii + '_' + m,
                    sid: i,
                    name: 'John Smith',
                    address: '1111 NE 22, Seattle, Wa',
                    phone: '206678787',
                    country: 'US',
                    currency: 'US',
                    seller: 'bookstore',
                    item: 'book',
                    reference: '75323',
                    amount: 500
                });
            }

            r.assertEvents(handle, JSON.stringify(events));
            var result = r.startAction(handle);
            r.completeAction(handle, result[0], result[1]);
        }
    }

    console.log('End somebooks positive: ' + new Date());
}