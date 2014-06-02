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
    handle = r.createRuleset('partbooks' + cluster.worker.id,  
        JSON.stringify({
            ship: {

                whenAll: { 
                    a$some: { $and: [{ $lte: { amount: 1000 }}, { country: 'US' }, { currency: 'US' }, { seller: 'bookstore'}]},
                    $s: { $nex: { done: 1 } }            
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

    console.log('Start partbooks send: ' + new Date());

    for (var m = 0; m < 5000; ++m) {
        for (var i = 0; i < 16; ++i) {
            r.assertEvent(handle, 
                JSON.stringify({
                    id: i + '_' + m,
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
                })
            );
        }
    }

    console.log('Start partbooks actions: ' + new Date());

    try 
    {
        var result = r.startAction(handle);
        while (result) {
            r.completeAction(handle, result[0], result[1]);
            result = r.startAction(handle);
        }
    }
    catch(reason) {
        console.log(reason);
    }

    console.log('End partbooks actions: ' + new Date());
}