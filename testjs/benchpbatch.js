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
    var chargeRules = {
        charge1: { when: { $and: [ { country: 'GB' }, { store: 'books'} ] }, run: 'charge' },
        charge2: { when: { $and: [ { country: 'DE' }, { store: 'books'} ] }, run: 'charge' },
        charge3: { when: { $and: [ { country: 'FR' }, { store: 'books'} ] }, run: 'charge' },
        charge4: { when: { $and: [ { country: 'SP' }, { store: 'books'} ] }, run: 'charge' },
        charge5: { when: { $and: [ { country: 'IT' }, { store: 'books'} ] }, run: 'charge' },
        charge6: { when: { $and: [ { country: 'DM' }, { store: 'books'} ] }, run: 'charge' },
        charge7: { when: { $and: [ { country: 'SW' }, { store: 'books'} ] }, run: 'charge' },
        charge8: { when: { $and: [ { country: 'NW' }, { store: 'books'} ] }, run: 'charge' },
        charge9: { when: { $and: [ { country: 'NL' }, { store: 'books'} ] }, run: 'charge' },
        charge10: { when: { $and: [ { country: 'FN' }, { store: 'books'} ] }, run: 'charge' },
        charge11: { when: { $and: [ { country: 'GB' }, { store: 'cars'} ] }, run: 'charge' },
        charge12: { when: { $and: [ { country: 'DE' }, { store: 'cars'} ] }, run: 'charge' },
        charge13: { when: { $and: [ { country: 'FR' }, { store: 'cars'} ] }, run: 'charge' },
        charge14: { when: { $and: [ { country: 'SP' }, { store: 'cars'} ] }, run: 'charge' },
        charge15: { when: { $and: [ { country: 'IT' }, { store: 'cars'} ] }, run: 'charge' },
        charge16: { when: { $and: [ { country: 'DM' }, { store: 'cars'} ] }, run: 'charge' },
        charge17: { when: { $and: [ { country: 'SW' }, { store: 'cars'} ] }, run: 'charge' },
        charge18: { when: { $and: [ { country: 'NW' }, { store: 'cars'} ] }, run: 'charge' },
        charge19: { when: { $and: [ { country: 'NL' }, { store: 'cars'} ] }, run: 'charge' },
        charge20: { when: { $and: [ { country: 'FN' }, { store: 'cars'} ] }, run: 'charge' },
        charge21: { when: { $and: [ { country: 'GB' }, { store: 'planes'} ] }, run: 'charge' },
        charge22: { when: { $and: [ { country: 'DE' }, { store: 'planes'} ] }, run: 'charge' },
        charge23: { when: { $and: [ { country: 'FR' }, { store: 'planes'} ] }, run: 'charge' },
        charge24: { when: { $and: [ { country: 'SP' }, { store: 'planes'} ] }, run: 'charge' },
        charge25: { when: { $and: [ { country: 'IT' }, { store: 'planes'} ] }, run: 'charge' },
        charge26: { when: { $and: [ { country: 'DM' }, { store: 'planes'} ] }, run: 'charge' },
        charge27: { when: { $and: [ { country: 'SW' }, { store: 'planes'} ] }, run: 'charge' },
        charge28: { when: { $and: [ { country: 'NW' }, { store: 'planes'} ] }, run: 'charge' },
        charge29: { when: { $and: [ { country: 'NL' }, { store: 'planes'} ] }, run: 'charge' },
        charge30: { when: { $and: [ { country: 'FN' }, { store: 'planes'} ] }, run: 'charge' },
    };

    // handle = getRuleset('books', chargeRules);

    // var sample = JSON.stringify({
    //     id: 1,
    //     sid: 1,
    //     country: 'GB',
    //     store: 'books'
    // });

    // console.log('Start books send 1: ' + new Date() + ' size: ' + sample.length);

    // for (var m = 0; m < 125; ++m) {
    //     for (var i = 0; i < 16; ++i) {
    //         var events = [];
    //         for (var ii = 0; ii < 40; ++ii) {
    //             events.push({
    //                 id: i + '_' + ii + '_' + m,
    //                 sid: i,
    //                 country: 'GB',
    //                 store: 'books'
    //             });
    //         }

    //         r.assertEvents(handle, JSON.stringify(events));
    //     }
    // }

    // console.log('End books send 1: ' + new Date());

    // handle = getRuleset('books', chargeRules);

    // var sample = JSON.stringify({
    //     id: 1,
    //     sid: 1,
    //     country: 'GB',
    //     store: 'books',
    //     item0: 'a',
    //     item1: 'a',
    //     item2: 'a',
    //     item3: 'a',
    //     item4: 'a',
    //     item5: 'a',
    //     item6: 'a',
    //     item7: 'a',
    //     item8: 'a',
    //     item9: 'a',
    // });

    // console.log('Start books send 10: ' + new Date() + ' size: ' + sample.length);

    // for (var m = 0; m < 125; ++m) {
    //     for (var i = 0; i < 16; ++i) {
    //         var events = [];
    //         for (var ii = 0; ii < 40; ++ii) {
    //             events.push({
    //                 id: i + '_' + ii + '_' + m,
    //                 sid: i,
    //                 country: 'GB',
    //                 store: 'books',
    //                 item0: 'a',
    //                 item1: 'a',
    //                 item2: 'a',
    //                 item3: 'a',
    //                 item4: 'a',
    //                 item5: 'a',
    //                 item6: 'a',
    //                 item7: 'a',
    //                 item8: 'a',
    //                 item9: 'a',
    //             });
    //         }

    //         r.assertEvents(handle, JSON.stringify(events));
    //     }
    // }

    // console.log('End books send 10: ' + new Date());

    // handle = getRuleset('books', chargeRules);

    // var sample = JSON.stringify({
    //     id: 1,
    //     sid: 1,
    //     country: 'GB',
    //     store: 'books',
    //     item0: 'a',
    //     item1: 'a',
    //     item2: 'a',
    //     item3: 'a',
    //     item4: 'a',
    //     item5: 'a',
    //     item6: 'a',
    //     item7: 'a',
    //     item8: 'a',
    //     item9: 'a',
    //     item10: 'a',
    //     item11: 'a',
    //     item12: 'a',
    //     item13: 'a',
    //     item14: 'a',
    //     item15: 'a',
    //     item16: 'a',
    //     item17: 'a',
    //     item18: 'a',
    //     item19: 'a',
    //     item20: 'a',
    // });

    // console.log('Start books send 20: ' + new Date() + ' size: ' + sample.length);

    // for (var m = 0; m < 125; ++m) {
    //     for (var i = 0; i < 16; ++i) {
    //         var events = [];
    //         for (var ii = 0; ii < 40; ++ii) {
    //             events.push({
    //                 id: i + '_' + ii + '_' + m,
    //                 sid: i,
    //                 country: 'GB',
    //                 store: 'books',
    //                 item0: 'a',
    //                 item1: 'a',
    //                 item2: 'a',
    //                 item3: 'a',
    //                 item4: 'a',
    //                 item5: 'a',
    //                 item6: 'a',
    //                 item7: 'a',
    //                 item8: 'a',
    //                 item9: 'a',
    //                 item10: 'a',
    //                 item11: 'a',
    //                 item12: 'a',
    //                 item13: 'a',
    //                 item14: 'a',
    //                 item15: 'a',
    //                 item16: 'a',
    //                 item17: 'a',
    //                 item18: 'a',
    //                 item19: 'a',
    //                 item20: 'a',
    //             });
    //         }

    //         r.assertEvents(handle, JSON.stringify(events));
    //     }
    // }

    // console.log('End books send 20: ' + new Date());

    // handle = getRuleset('books', chargeRules);

    // var sample = JSON.stringify({
    //     id: 1,
    //     sid: 1,
    //     country: 'GB',
    //     store: 'books',
    //     item0: 'a',
    //     item1: 'a',
    //     item2: 'a',
    //     item3: 'a',
    //     item4: 'a',
    //     item5: 'a',
    //     item6: 'a',
    //     item7: 'a',
    //     item8: 'a',
    //     item9: 'a',
    //     item10: 'a',
    //     item11: 'a',
    //     item12: 'a',
    //     item13: 'a',
    //     item14: 'a',
    //     item15: 'a',
    //     item16: 'a',
    //     item17: 'a',
    //     item18: 'a',
    //     item19: 'a',
    //     item20: 'a',
    //     item21: 'a',
    //     item22: 'a',
    //     item23: 'a',
    //     item24: 'a',
    //     item25: 'a',
    //     item26: 'a',
    //     item27: 'a',
    //     item28: 'a',
    //     item29: 'a',
    //     item30: 'a',
    //     item31: 'a',
    //     item32: 'a',
    //     item33: 'a',
    //     item34: 'a',
    //     item35: 'a',
    //     item36: 'a',
    //     item37: 'a',
    //     item38: 'a',
    //     item39: 'a',
    // });

    // console.log('Start books send 40: ' + new Date() + ' size: ' + sample.length);

    // for (var m = 0; m < 125; ++m) {
    //     for (var i = 0; i < 16; ++i) {
    //         var events = [];
    //         for (var ii = 0; ii < 40; ++ii) {
    //             events.push({
    //                 id: i + '_' + ii + '_' + m,
    //                 sid: i,
    //                 country: 'GB',
    //                 store: 'books',
    //                 item0: 'a',
    //                 item1: 'a',
    //                 item2: 'a',
    //                 item3: 'a',
    //                 item4: 'a',
    //                 item5: 'a',
    //                 item6: 'a',
    //                 item7: 'a',
    //                 item8: 'a',
    //                 item9: 'a',
    //                 item10: 'a',
    //                 item11: 'a',
    //                 item12: 'a',
    //                 item13: 'a',
    //                 item14: 'a',
    //                 item15: 'a',
    //                 item16: 'a',
    //                 item17: 'a',
    //                 item18: 'a',
    //                 item19: 'a',
    //                 item20: 'a',
    //                 item21: 'a',
    //                 item22: 'a',
    //                 item23: 'a',
    //                 item24: 'a',
    //                 item25: 'a',
    //                 item26: 'a',
    //                 item27: 'a',
    //                 item28: 'a',
    //                 item29: 'a',
    //                 item30: 'a',
    //                 item31: 'a',
    //                 item32: 'a',
    //                 item33: 'a',
    //                 item34: 'a',
    //                 item35: 'a',
    //                 item36: 'a',
    //                 item37: 'a',
    //                 item38: 'a',
    //                 item39: 'a',
    //             });
    //         }

    //         r.assertEvents(handle, JSON.stringify(events));
    //     }
    // }

    // console.log('End books send 40: ' + new Date());

    handle = getRuleset('books', chargeRules);

    var sample = JSON.stringify({
        id: 1,
        sid: 1,
        country: 'GB',
        store: 'books',
        item0: 'a',
        item1: 'a',
        item2: 'a',
        item3: 'a',
        item4: 'a',
        item5: 'a',
        item6: 'a',
        item7: 'a',
        item8: 'a',
        item9: 'a',
        item10: 'a',
        item11: 'a',
        item12: 'a',
        item13: 'a',
        item14: 'a',
        item15: 'a',
        item16: 'a',
        item17: 'a',
        item18: 'a',
        item19: 'a',
        item20: 'a',
        item21: 'a',
        item22: 'a',
        item23: 'a',
        item24: 'a',
        item25: 'a',
        item26: 'a',
        item27: 'a',
        item28: 'a',
        item29: 'a',
        item30: 'a',
        item31: 'a',
        item32: 'a',
        item33: 'a',
        item34: 'a',
        item35: 'a',
        item36: 'a',
        item37: 'a',
        item38: 'a',
        item39: 'a',
        item40: 'a',
        item41: 'a',
        item42: 'a',
        item43: 'a',
        item44: 'a',
        item45: 'a',
        item46: 'a',
        item47: 'a',
        item48: 'a',
        item49: 'a',
        item50: 'a',
        item51: 'a',
        item52: 'a',
        item53: 'a',
        item54: 'a',
        item55: 'a',
        item56: 'a',
        item57: 'a',
        item58: 'a',
        item59: 'a',
        item60: 'a',
        item61: 'a',
        item62: 'a',
        item63: 'a',
        item64: 'a',
        item65: 'a',
        item66: 'a',
        item67: 'a',
        item68: 'a',
        item69: 'a',
        item70: 'a',
        item71: 'a',
        item72: 'a',
        item73: 'a',
        item74: 'a',
        item75: 'a',
        item76: 'a',
        item77: 'a',
        item78: 'a',
        item79: 'a',
        item80: 'a',
    });

    console.log('Start books send 80: ' + new Date() + ' size: ' + sample.length);

    for (var m = 0; m < 125; ++m) {
        for (var i = 0; i < 16; ++i) {
            var events = [];
            for (var ii = 0; ii < 40; ++ii) {
                events.push({
                    id: i + '_' + ii + '_' + m,
                    sid: i,
                    country: 'GB',
                    store: 'books',
                    item0: 'a',
                    item1: 'a',
                    item2: 'a',
                    item3: 'a',
                    item4: 'a',
                    item5: 'a',
                    item6: 'a',
                    item7: 'a',
                    item8: 'a',
                    item9: 'a',
                    item10: 'a',
                    item11: 'a',
                    item12: 'a',
                    item13: 'a',
                    item14: 'a',
                    item15: 'a',
                    item16: 'a',
                    item17: 'a',
                    item18: 'a',
                    item19: 'a',
                    item20: 'a',
                    item21: 'a',
                    item22: 'a',
                    item23: 'a',
                    item24: 'a',
                    item25: 'a',
                    item26: 'a',
                    item27: 'a',
                    item28: 'a',
                    item29: 'a',
                    item30: 'a',
                    item31: 'a',
                    item32: 'a',
                    item33: 'a',
                    item34: 'a',
                    item35: 'a',
                    item36: 'a',
                    item37: 'a',
                    item38: 'a',
                    item39: 'a',
                    item40: 'a',
                    item41: 'a',
                    item42: 'a',
                    item43: 'a',
                    item44: 'a',
                    item45: 'a',
                    item46: 'a',
                    item47: 'a',
                    item48: 'a',
                    item49: 'a',
                    item50: 'a',
                    item51: 'a',
                    item52: 'a',
                    item53: 'a',
                    item54: 'a',
                    item55: 'a',
                    item56: 'a',
                    item57: 'a',
                    item58: 'a',
                    item59: 'a',
                    item60: 'a',
                    item61: 'a',
                    item62: 'a',
                    item63: 'a',
                    item64: 'a',
                    item65: 'a',
                    item66: 'a',
                    item67: 'a',
                    item68: 'a',
                    item69: 'a',
                    item70: 'a',
                    item71: 'a',
                    item72: 'a',
                    item73: 'a',
                    item74: 'a',
                    item75: 'a',
                    item76: 'a',
                    item77: 'a',
                    item78: 'a',
                    item79: 'a',
                    item80: 'a',
                });
            }

            r.assertEvents(handle, JSON.stringify(events));
        }
    }

    console.log('End books send 80: ' + new Date());
}

function getRuleset(name, rules) {
    var handle = r.createRuleset(name + cluster.worker.id, JSON.stringify(rules));
    r.bindRuleset(handle, '/tmp/redis0.sock', 0, null);
    r.bindRuleset(handle, '/tmp/redis1.sock', 0, null);
    r.bindRuleset(handle, '/tmp/redis2.sock', 0, null);
    r.bindRuleset(handle, '/tmp/redis3.sock', 0, null);
    r.bindRuleset(handle, '/tmp/redis4.sock', 0, null);
    r.bindRuleset(handle, '/tmp/redis5.sock', 0, null);
    r.bindRuleset(handle, '/tmp/redis6.sock', 0, null);
    r.bindRuleset(handle, '/tmp/redis7.sock', 0, null);
    r.bindRuleset(handle, '/tmp/redis8.sock', 0, null);
    r.bindRuleset(handle, '/tmp/redis9.sock', 0, null);
    r.bindRuleset(handle, '/tmp/redis10.sock', 0, null);
    r.bindRuleset(handle, '/tmp/redis11.sock', 0, null);
    r.bindRuleset(handle, '/tmp/redis12.sock', 0, null);
    r.bindRuleset(handle, '/tmp/redis13.sock', 0, null);
    r.bindRuleset(handle, '/tmp/redis14.sock', 0, null);
    r.bindRuleset(handle, '/tmp/redis15.sock', 0, null);    
    return handle;
}