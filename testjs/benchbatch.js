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
        charge1: { whenSome: { $and: [ { country: 'GB' }, { currency: 'EU' }, { store: 'books'} ] }, run: 'charge' },
        charge2: { whenSome: { $and: [ { country: 'DE' }, { currency: 'EU' }, { store: 'books'} ] }, run: 'charge' },
        charge3: { whenSome: { $and: [ { country: 'FR' }, { currency: 'EU' }, { store: 'books'} ] }, run: 'charge' },
        charge4: { whenSome: { $and: [ { country: 'SP' }, { currency: 'EU' }, { store: 'books'} ] }, run: 'charge' },
        charge5: { whenSome: { $and: [ { country: 'IT' }, { currency: 'EU' }, { store: 'books'} ] }, run: 'charge' },
        charge6: { whenSome: { $and: [ { country: 'DM' }, { currency: 'EU' }, { store: 'books'} ] }, run: 'charge' },
        charge7: { whenSome: { $and: [ { country: 'SW' }, { currency: 'EU' }, { store: 'books'} ] }, run: 'charge' },
        charge8: { whenSome: { $and: [ { country: 'NW' }, { currency: 'EU' }, { store: 'books'} ] }, run: 'charge' },
        charge9: { whenSome: { $and: [ { country: 'NL' }, { currency: 'EU' }, { store: 'books'} ] }, run: 'charge' },
        charge10: { whenSome: { $and: [ { country: 'FN' }, { currency: 'EU' }, { store: 'books'} ] }, run: 'charge' },
        charge11: { whenSome: { $and: [ { country: 'GB' }, { currency: 'EU' }, { store: 'cars'} ] }, run: 'charge' },
        charge12: { whenSome: { $and: [ { country: 'DE' }, { currency: 'EU' }, { store: 'cars'} ] }, run: 'charge' },
        charge13: { whenSome: { $and: [ { country: 'FR' }, { currency: 'EU' }, { store: 'cars'} ] }, run: 'charge' },
        charge14: { whenSome: { $and: [ { country: 'SP' }, { currency: 'EU' }, { store: 'cars'} ] }, run: 'charge' },
        charge15: { whenSome: { $and: [ { country: 'IT' }, { currency: 'EU' }, { store: 'cars'} ] }, run: 'charge' },
        charge16: { whenSome: { $and: [ { country: 'DM' }, { currency: 'EU' }, { store: 'cars'} ] }, run: 'charge' },
        charge17: { whenSome: { $and: [ { country: 'SW' }, { currency: 'EU' }, { store: 'cars'} ] }, run: 'charge' },
        charge18: { whenSome: { $and: [ { country: 'NW' }, { currency: 'EU' }, { store: 'cars'} ] }, run: 'charge' },
        charge19: { whenSome: { $and: [ { country: 'NL' }, { currency: 'EU' }, { store: 'cars'} ] }, run: 'charge' },
        charge20: { whenSome: { $and: [ { country: 'FN' }, { currency: 'EU' }, { store: 'cars'} ] }, run: 'charge' },
        charge21: { whenSome: { $and: [ { country: 'GB' }, { currency: 'EU' }, { store: 'planes'} ] }, run: 'charge' },
        charge22: { whenSome: { $and: [ { country: 'DE' }, { currency: 'EU' }, { store: 'planes'} ] }, run: 'charge' },
        charge23: { whenSome: { $and: [ { country: 'FR' }, { currency: 'EU' }, { store: 'planes'} ] }, run: 'charge' },
        charge24: { whenSome: { $and: [ { country: 'SP' }, { currency: 'EU' }, { store: 'planes'} ] }, run: 'charge' },
        charge25: { whenSome: { $and: [ { country: 'IT' }, { currency: 'EU' }, { store: 'planes'} ] }, run: 'charge' },
        charge26: { whenSome: { $and: [ { country: 'DM' }, { currency: 'EU' }, { store: 'planes'} ] }, run: 'charge' },
        charge27: { whenSome: { $and: [ { country: 'SW' }, { currency: 'EU' }, { store: 'planes'} ] }, run: 'charge' },
        charge28: { whenSome: { $and: [ { country: 'NW' }, { currency: 'EU' }, { store: 'planes'} ] }, run: 'charge' },
        charge29: { whenSome: { $and: [ { country: 'NL' }, { currency: 'EU' }, { store: 'planes'} ] }, run: 'charge' },
        charge30: { whenSome: { $and: [ { country: 'FN' }, { currency: 'EU' }, { store: 'planes'} ] }, run: 'charge' },
    };

    handle = getRuleset('books', chargeRules);

    // var events = [];
    // for (var ii = 0; ii < 50; ++ii) {
    //     events.push({
    //         id: 1,
    //         sid: 1,
    //         country: 'GB',
    //     });
    // }

    // console.log('Start somebooks negative 1: ' + new Date());

    // var messages = JSON.stringify(events);
    // for (var m = 0; m < 100000; ++m) {
    //     r.assertEvents(handle, messages);
    // }

    // console.log('End somebooks negative 1: ' + new Date());

    // var events = [];
    // for (var ii = 0; ii < 50; ++ii) {
    //     events.push({
    //         id: 1,
    //         sid: 1,
    //         country: 'GB',
    //         item0: 'a',
    //         item1: 'a',
    //         item2: 'a',
    //         item3: 'a',
    //         item4: 'a',
    //         item5: 'a',
    //         item6: 'a',
    //         item7: 'a',
    //         item8: 'a',
    //         item9: 'a',
    //     });
    // }

    // console.log('Start somebooks negative 10: ' + new Date());

    // var messages = JSON.stringify(events);
    // for (var m = 0; m < 80000; ++m) {
    //     r.assertEvents(handle, messages);
    // }

    // console.log('End somebooks negative 10: ' + new Date());

    // var events = [];
    // for (var ii = 0; ii < 50; ++ii) {
    //     events.push({
    //         id: 1,
    //         sid: 1,
    //         country: 'GB',
    //         item0: 'a',
    //         item1: 'a',
    //         item2: 'a',
    //         item3: 'a',
    //         item4: 'a',
    //         item5: 'a',
    //         item6: 'a',
    //         item7: 'a',
    //         item8: 'a',
    //         item9: 'a',
    //         item10: 'a',
    //         item11: 'a',
    //         item12: 'a',
    //         item13: 'a',
    //         item14: 'a',
    //         item15: 'a',
    //         item16: 'a',
    //         item17: 'a',
    //         item18: 'a',
    //         item19: 'a',
    //         item20: 'a',
    //     });
    // }

    // console.log('Start somebooks negative 20: ' + new Date());

    // var messages = JSON.stringify(events);
    // for (var m = 0; m < 50000; ++m) {
    //     r.assertEvents(handle, messages);
    // }

    // console.log('End somebooks negative 20: ' + new Date());

    // var events = [];
    // for (var ii = 0; ii < 50; ++ii) {
    //     events.push({
    //         id: 1,
    //         sid: 1,
    //         country: 'GB',
    //         item0: 'a',
    //         item1: 'a',
    //         item2: 'a',
    //         item3: 'a',
    //         item4: 'a',
    //         item5: 'a',
    //         item6: 'a',
    //         item7: 'a',
    //         item8: 'a',
    //         item9: 'a',
    //         item10: 'a',
    //         item11: 'a',
    //         item12: 'a',
    //         item13: 'a',
    //         item14: 'a',
    //         item15: 'a',
    //         item16: 'a',
    //         item17: 'a',
    //         item18: 'a',
    //         item19: 'a',
    //         item20: 'a',
    //         item21: 'a',
    //         item22: 'a',
    //         item23: 'a',
    //         item24: 'a',
    //         item25: 'a',
    //         item26: 'a',
    //         item27: 'a',
    //         item28: 'a',
    //         item29: 'a',
    //         item30: 'a',
    //         item31: 'a',
    //         item32: 'a',
    //         item33: 'a',
    //         item34: 'a',
    //         item35: 'a',
    //         item36: 'a',
    //         item37: 'a',
    //         item38: 'a',
    //         item39: 'a',
    //     });
    // }

    // console.log('Start somebooks negative 40: ' + new Date());

    // var messages = JSON.stringify(events);
    // for (var m = 0; m < 40000; ++m) {
    //     r.assertEvents(handle, messages);
    // }

    // console.log('End somebooks negative 40: ' + new Date());

    var events = [];
    for (var ii = 0; ii < 50; ++ii) {
        events.push({
            id: 1,
            sid: 1,
            country: 'GB',
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

    

    var messages = JSON.stringify(events);
    console.log('Start somebooks negative 80: ' + new Date() + ' ' + messages.length);
    for (var m = 0; m < 20000; ++m) {
        r.assertEvents(handle, messages);
    }

    console.log('End somebooks negative 80: ' + new Date());
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