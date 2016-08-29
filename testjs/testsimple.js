var d = require('../libjs/durable');
var m = d.m, s = d.s, c = d.c; timeout = d.timeout;

d.ruleset('fraud1_1', {
        whenAll: [
            c.first = m.amount.gt(100),
            c.second = m.location.neq(c.first.location)
        ],
        run: function(c) {
            console.log('fraud1_1 detected ' + c.first.location + ' ' + c.second.location);
        }
    },
    function (host) {
        host.post('fraud1_1', {id: 1, sid: 1, amount: 200, location: 'US'});
        host.post('fraud1_1', {id: 2, sid: 1, amount: 200, location: 'CA'});
    }
);

d.ruleset('fraud5_1', {
        whenAll: m.amount.gt(100),
        pri: 3,
        run: function(c) {
            console.log('fraud5_1 first ' + c.m.amount + ' from ' + c.s.sid);
        }
    }, {
        whenAll: m.amount.gt(200),
        pri: 2,
        run: function(c) {
            console.log('fraud5_1 second ' + c.m.amount + ' from ' + c.s.sid);
        }
    }, {
        whenAll: m.amount.gt(300),
        pri: 1,
        run: function(c) {
            console.log('fraud5_1 third ' + c.m.amount + ' from ' + c.s.sid);
        }
    },
    function (host) {
        host.post('fraud5_1', {id: 1, sid: 1, amount: 101, location: 'US'});
        host.post('fraud5_1', {id: 2, sid: 1, amount: 201, location: 'CA'});
        host.post('fraud5_1', {id: 3, sid: 1, amount: 301, location: 'CA'});
        host.assert('fraud5_1', {id: 4, sid: 1, amount: 250, location: 'US'});
        host.assert('fraud5_1', {id: 5, sid: 1, amount: 500, location: 'CA'});
    }
);

d.statechart('fraud0_1', {
    start: {
        to: 'standby',
    },
    standby: {
        to: 'metering',
        whenAll: m.amount.gt(100),
        run: function (c) { c.startTimer('velocity', 30); }
    },
    metering: [{
        to: 'fraud',
        count: 3,
        whenAll: m.amount.gt(100),
        run: function (c) { console.log('fraud0_1 detected'); }
    }, {
        to: 'standby',
        whenAll: timeout('velocity'),
        run: function (c) { console.log('fraud0_1 cleared'); }
    }],
    fraud: {},
    whenStart: function (host) {
        host.post('fraud0_1', {id: 1, sid: 1, amount: 200});
        host.post('fraud0_1', {id: 2, sid: 1, amount: 200});
        host.post('fraud0_1', {id: 3, sid: 1, amount: 200});
        host.post('fraud0_1', {id: 4, sid: 1, amount: 200});
    }
});

d.statechart('a6_0', {
    work: [{
        enter: {
            to: 'process',
            whenAll: m.subject.eq('enter'),
            run: function (c) { console.log('a6_0 continue process'); }
        },
        process: {
            to: 'process',
            whenAll: m.subject.eq('continue'),
            run: function (c) { console.log('a6_0 processing'); }
        }
    }, {
        to: 'canceled',
        pri: 1,
        whenAll: m.subject.eq('cancel'),
        run: function (c) { console.log('a6_0 canceling');}
    }],
    canceled: {},
    whenStart: function (host) {
        host.post('a6_0', {id: 1, sid: 1, subject: 'enter'});
        host.post('a6_0', {id: 2, sid: 1, subject: 'continue'});
        host.post('a6_0', {id: 3, sid: 1, subject: 'continue'});
        host.post('a6_0', {id: 4, sid: 1, subject: 'cancel'});
    }
});

d.flowchart('a3_0', {
    input: {
        request: { whenAll: m.subject.eq('approve').and(m.amount.lte(1000)) }, 
        deny: { whenAll: m.subject.eq('approve').and(m.amount.gt(1000)) }, 
    },
    request: {
        run: function (c) {
            console.log('a3_0 request approval from: ' + c.s.sid);
            if (c.s.status) 
                c.s.status = 'approved';
            else
                c.s.status = 'pending';
        },
        approve: { whenAll: s.status.eq('approved') },
        deny: { whenAll: m.subject.eq('denied') },
        request: { whenAll: m.subject.eq('approved') }
    },
    approve: {
        run: function (c) { console.log('a3_0 approved from: ' + c.s.sid); }
    },
    deny: {
        run: function (c) { console.log('a3_0 denied from: ' + c.s.sid); }
    },
    whenStart: function (host) {
        host.post('a3_0', {id: 1, sid: 1, subject: 'approve', amount: 100});
        host.post('a3_0', {id: 2, sid: 1, subject: 'approved'});
        host.post('a3_0', {id: 3, sid: 2, subject: 'approve', amount: 100});
        host.post('a3_0', {id: 4, sid: 2, subject: 'denied'});
        host.post('a3_0', {id: 5, sid: 3, subject: 'approve', amount: 10000});
    }
});

with (d.statechart('fraud0')) {
    with (state('start')) {
        to('standby');
    }
    with (state('standby')) {
        to('metering').whenAll(m.amount.gt(100), function (c) {
            c.startTimer('velocity', 30);
        });
    }
    with (state('metering')) {
        to('fraud').whenAll(m.amount.gt(100), count(3), function (c) {
            console.log('fraud0 detected');
        });
        to('standby').whenAll(timeout('velocity'), function (c) {
            console.log('fraud0 cleared');  
        });
    }
    state('fraud');
    whenStart(function (host) {
        host.post('fraud0', {id: 1, sid: 1, amount: 200});
        host.post('fraud0', {id: 2, sid: 1, amount: 200});
        host.post('fraud0', {id: 3, sid: 1, amount: 200});
        host.post('fraud0', {id: 4, sid: 1, amount: 200});
    });
}

with (d.ruleset('fraud1')) {
    whenAll(c.first = m.amount.gt(100),
            c.second = m.location.neq(c.first.location), 
        function(c) {
            console.log('fraud1 detected ' + c.first.location + ' ' + c.second.location);
        }
    );
    whenStart(function (host) {
        host.post('fraud1', {id: 1, sid: 1, amount: 200, location: 'US'});
        host.post('fraud1', {id: 2, sid: 1, amount: 200, location: 'CA'});
    });
}

with (d.ruleset('fraud2')) {
    whenAll(c.first = m.amount.gt(100),
            c.second = m.ip.eq(c.first.ip).and(m.cc.neq(c.first.cc)), 
            c.third = m.ip.eq(c.second.ip).and(m.cc.neq(c.first.cc), m.cc.neq(c.second.cc)),
        function(c) {
            console.log('fraud2 detected ' + c.first.cc + ' ' + c.second.cc + ' ' + c.third.cc);
        }
    );
    whenStart(function (host) {
        host.post('fraud2', {id: 1, sid: 1, amount: 200, ip: 'a', cc: 'b'});
        host.post('fraud2', {id: 2, sid: 1, amount: 200, ip: 'a', cc: 'c'});
        host.post('fraud2', {id: 3, sid: 1, amount: 200, ip: 'a', cc: 'd'});
    });
}

with (d.ruleset('fraud3')) {
    whenAll(c.first = m.amount.gt(100),
            c.second = m.amount.gt(c.first.amount.mul(2)), 
            c.third = m.amount.gt(c.second.amount.add(c.first.amount)),
            c.fourth = m.amount.gt(add(c.first.amount, c.second.amount, c.third.amount).div(3)),
        function(c) {
            console.log('fraud3 detected ' + c.first.amount + ' ' + c.second.amount + ' ' + c.third.amount + ' ' + c.fourth.amount);
        }
    );
    whenStart(function (host) {
        host.post('fraud3', {id: 1, sid: 1, amount: 200});
        host.post('fraud3', {id: 2, sid: 1, amount: 500});
        host.post('fraud3', {id: 3, sid: 1, amount: 1000});
        host.post('fraud3', {id: 4, sid: 1, amount: 1000});
    });
}

with (d.ruleset('fraud4')) {
    whenAll(c.first = m.amount.gt(100),
            c.second = m.amount.gt(100).and(m.location.neq(c.first.location)), 
            count(2), 
        function(c) {
            console.log('fraud4 detected ' + c.m[0].first.location + ' ' + c.m[0].second.location);
            console.log('fraud4 detected ' + c.m[1].first.location + ' ' + c.m[1].second.location);
        }
    );
    whenStart(function (host) {
        host.assert('fraud4', {id: 1, sid: 1, amount: 200, location: 'US'});
        host.assert('fraud4', {id: 2, sid: 1, amount: 200, location: 'CA'});
    });
}

with (d.ruleset('fraud5')) {
    whenAll(m.amount.gt(100),
            pri(3),
        function(c) {
            console.log('fraud5 first ' + c.m.amount + ' from ' + c.s.sid);
        }
    );
    whenAll(m.amount.gt(200),
            pri(2),
        function(c) {
            console.log('fraud5 second ' + c.m.amount + ' from ' + c.s.sid);
        }
    );
    whenAll(m.amount.gt(300),
            pri(1),
        function(c) {
            console.log('fraud5 third ' + c.m.amount + ' from ' + c.s.sid);
        }
    );
    whenStart(function (host) {
        host.post('fraud5', {id: 1, sid: 1, amount: 101, location: 'US'});
        host.post('fraud5', {id: 2, sid: 1, amount: 201, location: 'CA'});
        host.post('fraud5', {id: 3, sid: 1, amount: 301, location: 'CA'});
        host.assert('fraud5', {id: 4, sid: 1, amount: 250, location: 'US'});
        host.assert('fraud5', {id: 5, sid: 1, amount: 500, location: 'CA'});
    });
}

q = d.createQueue('fraud5');
q.post({id: 1, sid: 2, amount: 101, location: 'US'});
q.post({id: 2, sid: 2, amount: 201, location: 'CA'});
q.post({id: 3, sid: 2, amount: 301, location: 'CA'});
q.assert({id: 4, sid: 2, amount: 250, location: 'US'});
q.assert({id: 5, sid: 2, amount: 500, location: 'CA'});
q.close();

with (d.ruleset('fraud6')) {
    whenAll(c.first = m.t.eq('deposit'),
            none(m.t.eq('balance')), 
            c.third = m.t.eq('withrawal'),
            c.fourth = m.t.eq('chargeback'),
        function(c) {
            console.log('fraud6 detected ' + c.first.t + ' ' + c.third.t + ' ' + c.fourth.t + ' from ' + c.s.sid);
        }
    );
    whenStart(function (host) {
        host.post('fraud6', {id: 1, sid: 1, t: 'deposit'});
        host.post('fraud6', {id: 2, sid: 1, t: 'withrawal'});
        host.post('fraud6', {id: 3, sid: 1, t: 'chargeback'});
        host.assert('fraud6', {id: 4, sid: 1, t: 'balance'});
        host.post('fraud6', {id: 5, sid: 1, t: 'deposit'});
        host.post('fraud6', {id: 6, sid: 1, t: 'withrawal'});
        host.post('fraud6', {id: 7, sid: 1, t: 'chargeback'});
        host.retract('fraud6', {id: 4, sid: 1, t: 'balance'});
    });
}

q = d.createQueue('fraud6');
q.post({id: 1, sid: 2, t: 'deposit'});
q.post({id: 2, sid: 2, t: 'withrawal'});
q.post({id: 3, sid: 2, t: 'chargeback'});
q.assert({id: 4, sid: 2, t: 'balance'});
q.post({id: 5, sid: 2, t: 'deposit'});
q.post({id: 6, sid: 2, t: 'withrawal'});
q.post({id: 7, sid: 2, t: 'chargeback'});
q.retract({id: 4, sid: 2, t: 'balance'});
q.close();

with (d.statechart('fraud7')) {
    with (state('first')) {
        to('second').whenAll(m.amount.gt(100), function (c, complete) {
            console.log('fraud7 start async 1');
            setTimeout(function() {
                console.log('fraud7 execute 1');
                complete();
            }, 1000);
        });
    }
    with (state('second')) {
        to('fraud', function (c, complete) {
            console.log('fraud7 start async 2');
            setTimeout(function() {
                console.log('fraud7 execute 2');
                complete();
            }, 1000);
        });
    }
    state('fraud');
    whenStart(function (host) {
        host.post('fraud7', {id: 1, sid: 1, amount: 200});
    });
}

with (d.ruleset('a0_0')) {
    whenAll(or(m.subject.eq('go'), m.subject.eq('approve'), m.subject.eq('ok')), function (c) {
        console.log('a0_0 approved ' + c.m.subject);
    });
    whenStart(function (host) {
        host.post('a0_0', {id: 1, sid: 1, subject: 'go'});
        host.post('a0_0', {id: 2, sid: 1, subject: 'approve'});
        host.post('a0_0', {id: 3, sid: 1, subject: 'ok'});
        host.post('a0_0', {id: 4, sid: 1, subject: 'not ok'});
    });
}

with (d.ruleset('a0_1')) {
    whenAll(and(m.subject.eq('go'), or(m.amount.lt(100), m.amount.gt(1000))), function (c) {
        console.log('a0_1 approved  ' + c.m.subject + ' ' + c.m.amount);
    });
    whenStart(function (host) {
        host.post('a0_1', {id: 1, sid: 1, subject: 'go', amount: 50});
        host.post('a0_1', {id: 2, sid: 1, subject: 'go', amount: 500});
        host.post('a0_1', {id: 3, sid: 1, subject: 'go', amount: 5000});
    });
}

with (d.ruleset('a0_2')) {
    whenAll(and(m.subject.eq('go'), or(m.amount.lt(100), and(m.amount.eq(500), m.status.eq('waived')))), function (c) {
        console.log('a0_2 approved  ' + c.m.subject + ' ' + c.m.amount);
    });
    whenStart(function (host) {
        host.post('a0_2', {id: 1, sid: 1, subject: 'go', amount: 50});
        host.post('a0_2', {id: 2, sid: 1, subject: 'go', amount: 500, status: 'waived'});
        host.post('a0_2', {id: 3, sid: 1, subject: 'go', amount: 5000});
    });
}

with (d.ruleset('a1')) {
    whenAll(m.subject.eq('approve').and(m.amount.gt(1000)), function (c) {
        console.log('a1 denied from: ' + c.s.sid);
        c.s.status = 'done';
    });
    whenAll(m.subject.eq('approve').and(m.amount.lte(1000)), function (c) {
        console.log('a1 request approval from: ' + c.s.sid);
        c.s.status = 'pending';
    });
    whenAll(m.subject.eq('approved'), s.status.eq('pending'), function (c) {
        console.log('a1 second request approval from: ' + c.s.sid);
        c.s.status = 'approved';
    });
    whenAll(s.status.eq('approved'), function (c) {
        console.log('a1 approved from: ' + c.s.sid);
        c.s.status = 'done';
    });
    whenAll(m.subject.eq('denied'), function (c) {
        console.log('a1 denied from: ' + c.s.sid);
        c.s.status = 'done';
    });
    whenStart(function (host) {
        host.post('a1', {id: 1, sid: 1, subject: 'approve', amount: 100});
        host.post('a1', {id: 2, sid: 1, subject: 'approved'});
        host.post('a1', {id: 3, sid: 2, subject: 'approve', amount: 100});
        host.post('a1', {id: 4, sid: 2, subject: 'denied'});
        host.post('a1', {id: 5, sid: 3, subject: 'approve', amount: 10000});
    });
}

with (d.statechart('a2')) {
    with (state('input')) {
        to('denied').whenAll(m.subject.eq('approve').and(m.amount.gt(1000)), function (c) {
            console.log('a2 denied from: ' + c.s.sid);
        });
        to('pending').whenAll(m.subject.eq('approve').and(m.amount.lte(1000)), function (c) {
            console.log('a2 request approval from: ' + c.s.sid);
        });
    }
    with (state('pending')) {
        to('pending').whenAll(m.subject.eq('approved'), function (c) {
            console.log('a2 second request approval from: ' + c.s.sid);
            c.s.status = 'approved';
        });
        to('approved').whenAll(s.status.eq('approved'), function (c) {
            console.log('a2 approved from: ' + c.s.sid);
        });
        to('denied').whenAll(m.subject.eq('denied'), function (c) {
            console.log('a2 denied from: ' + c.s.sid);
        });
    }
    state('denied');
    state('approved');
    whenStart(function (host) {
        host.post('a2', {id: 1, sid: 1, subject: 'approve', amount: 100});
        host.post('a2', {id: 2, sid: 1, subject: 'approved'});
        host.post('a2', {id: 3, sid: 2, subject: 'approve', amount: 100});
        host.post('a2', {id: 4, sid: 2, subject: 'denied'});
        host.post('a2', {id: 5, sid: 3, subject: 'approve', amount: 10000});
    });
}

with (d.flowchart('a3')) {
    with (stage('input')) {
        to('request').whenAll(m.subject.eq('approve').and(m.amount.lte(1000)));
        to('deny').whenAll(m.subject.eq('approve').and(m.amount.gt(1000)));
    }
    with (stage('request')) {
        run(function (c) {
            console.log('a3 request approval from: ' + c.s.sid);
            if (c.s.status) 
                c.s.status = 'approved';
            else
                c.s.status = 'pending';
        });
        to('approve').whenAll(s.status.eq('approved'));
        to('deny').whenAll(m.subject.eq('denied'));
        to('request').whenAll(m.subject.eq('approved'));
    }
    with (stage('approve')) {
        run(function (c) {
            console.log('a3 approved from: ' + c.s.sid);
        });
    }
    with (stage('deny')) {
        run(function (c) {
            console.log('a3 denied from: ' + c.s.sid);
        });
    }
    whenStart(function (host) {
        host.post('a3', {id: 1, sid: 1, subject: 'approve', amount: 100});
        host.post('a3', {id: 2, sid: 1, subject: 'approved'});
        host.post('a3', {id: 3, sid: 2, subject: 'approve', amount: 100});
        host.post('a3', {id: 4, sid: 2, subject: 'denied'});
        host.post('a3', {id: 5, sid: 3, subject: 'approve', amount: 10000});
    });
}

with (d.ruleset('a4')) {
    whenAny(all(m.subject.eq('approve'), m.amount.eq(1000)), 
            all(m.subject.eq('jumbo'), m.amount.eq(10000)), function (c) {
        console.log('a4 action from: ' + c.s.sid);
    });
    whenStart(function (host) {
        host.post('a4', {id: 1, sid: 1, subject: 'approve'});
        host.post('a4', {id: 2, sid: 1, amount: 1000});
        host.post('a4', {id: 3, sid: 2, subject: 'jumbo'});
        host.post('a4', {id: 4, sid: 2, amount: 10000});
    });
}

with (d.ruleset('a5')) {
    whenAll(any(m.subject.eq('approve'), m.subject.eq('jumbo')), 
            any(m.amount.eq(100), m.amount.eq(10000)), function (c) {
        console.log('a5 action from: ' + c.s.sid);
    });
    whenStart(function (host) {
        host.post('a5', {id: 1, sid: 1, subject: 'approve'});
        host.post('a5', {id: 2, sid: 1, amount: 100});
        host.post('a5', {id: 3, sid: 2, subject: 'jumbo'});
        host.post('a5', {id: 4, sid: 2, amount: 10000});
    });
}

with (d.statechart('a6')) {
    with (state('work')) {
        with (state('enter')) {
            to('process').whenAll(m.subject.eq('enter'), function (c) {
                console.log('a6 continue process');
            });
        }
        with (state('process')) {
            to('process').whenAll(m.subject.eq('continue'), function (c) {
                console.log('a6 processing');
            });
        }
        to('canceled').whenAll(pri(1), m.subject.eq('cancel'), function (c) {
            console.log('a6 canceling');
        });
    }
    state('canceled');
    whenStart(function (host) {
        host.post('a6', {id: 1, sid: 1, subject: 'enter'});
        host.post('a6', {id: 2, sid: 1, subject: 'continue'});
        host.post('a6', {id: 3, sid: 1, subject: 'continue'});
        host.post('a6', {id: 4, sid: 1, subject: 'cancel'});
    });
}

with (d.ruleset('a7')) {
    whenAll(m.amount.lt(c.s.maxAmount), function (c) {
        console.log('a7 approved ' +  c.m.amount);
    });
    whenStart(function (host) {
        host.patchState('a7', {sid: 1, maxAmount: 100});
        host.post('a7', {id: 1, sid: 1, amount: 10});
        host.post('a7', {id: 2, sid: 1, amount: 1000});
    });
}

with (d.ruleset('a8')) {
    whenAll(m.amount.lt(c.s.maxAmount).and(m.amount.gt(c.s.refId('global').minAmount)), function (c) {
        console.log('a8 approved ' +  c.m.amount);
    });
    whenStart(function (host) {
        host.patchState('a8', {sid: 1, maxAmount: 500});
        host.patchState('a8', {sid: 'global', minAmount: 100});
        host.post('a8', {id: 1, sid: 1, amount: 10});
        host.post('a8', {id: 2, sid: 1, amount: 200});
    });
}

with (d.ruleset('a9')) {
    whenAll(m.amount.gt(c.s.maxAmount.add(c.s.refId('global').minAmount)), function (c) {
        console.log('a9 approved ' +  c.m.amount);
    });
    whenStart(function (host) {
        host.patchState('a9', {sid: 1, maxAmount: 500});
        host.patchState('a9', {sid: 'global', minAmount: 100});
        host.post('a9', {id: 1, sid: 1, amount: 10});
        host.post('a9', {id: 2, sid: 1, amount: 1000});
    });
}

with (d.ruleset('a10')) {
    whenAll(m.amount.lt(100), count(3), function (c) {
        console.log('a10 approved ->' + JSON.stringify(c.m));
    });
    whenStart(function (host) {
        host.postBatch('a10', {id: 1, sid: 1, amount: 10},
                             {id: 2, sid: 1, amount: 10},
                             {id: 3, sid: 1, amount: 10},
                             {id: 4, sid: 1, amount: 10});
        host.postBatch('a10', [{id: 5, sid: 1, amount: 10},
                             {id: 6, sid: 1, amount: 10}]);
    });   
}

with (d.ruleset('a11')) {
    whenAll(m.amount.lt(100), m.subject.eq('approve'), count(3), function (c) {
        console.log('a11 approved ->' + JSON.stringify(c.m));
    });
    whenStart(function (host) {
        host.postBatch('a11', {id: 1, sid: 1, amount: 10},
                              {id: 2, sid: 1, amount: 10},
                              {id: 3, sid: 1, amount: 10},
                              {id: 4, sid: 1, subject: 'approve'});
        host.postBatch('a11', {id: 5, sid: 1, subject: 'approve'},
                              {id: 6, sid: 1, subject: 'approve'});
    }); 
}

with (d.ruleset('a12')) {
    whenAny(m.amount.lt(100), m.subject.eq('please'), count(3), function (c) {
        console.log('a12 approved ->' + JSON.stringify(c.m));
    });
    whenStart(function (host) {
        host.assert('a12', {id: 1, sid: 1, amount: 10});           
        host.postBatch('a12', {id: 2, sid: 1, subject: 'please'},
                              {id: 3, sid: 1, subject: 'please'},
                              {id: 4, sid: 1, subject: 'please'});
    }); 
}

with (d.ruleset('a13')) {
    whenAll(m.invoice.amount.gte(100), function (c) {
        console.log('a13 approved ->' + c.m.invoice.amount);
    });
    whenStart(function (host) {
        host.post('a13', {id: 1, sid: 1, invoice: {amount: 100}});           
    }); 
}

with (d.ruleset('a14')) {
    whenAll(c.first = m.t.eq('bill'),
            c.second = and(m.t.eq('payment'), m.invoice.amount.eq(c.first.invoice.amount)), 
        function(c) {
            console.log('a14 approved ->' + c.first.invoice.amount);
            console.log('a14 approved ->' + c.second.invoice.amount);
        }
    );
    whenStart(function (host) {
        host.post('a14', {id: 1, sid: 1, t: 'bill', invoice: {amount: 100}});  
        host.post('a14', {id: 2, sid: 1, t: 'payment', invoice: {amount: 100}});  
    });
}

with (d.ruleset('a15')) {
    whenAll(m.payment.invoice.amount.gte(100), function (c) {
        console.log('a15 approved ->' + c.m.payment.invoice.amount);
    });
    whenStart(function (host) {
        host.post('a15', {id: 1, sid: 1, payment: {invoice: {amount: 100}}});           
    }); 
}

with (d.ruleset('t0')) {
    whenAll(or(m.count.eq(0), timeout('myTimer')), function (c) {
        c.s.count += 1;
        c.post('t0', {id: c.s.count, sid: 1, t: 'purchase'});
        c.startTimer('myTimer', Math.random() * 3 + 1, 't0_' + c.s.count);
    });
    whenAll(span(5), m.t.eq('purchase'), function (c) {
        console.log('t0 pulse ->' + c.m.length);
    });
    whenStart(function (host) {
        host.patchState('t0', {sid: 1, count: 0});
    });
}

with (d.ruleset('t1')) {
    whenAll(m.start.eq('yes'), function (c) {
        c.s.start = new Date();
        c.startTimer('myFirstTimer', 3);
        c.startTimer('mySecondTimer', 6);
    });
    whenAll(timeout('myFirstTimer'), function (c) {
        console.log('t1 first timer started ' + c.s.start);
        console.log('t1 first timer ended ' + new Date());
        c.cancelTimer('mySecondTimer');
    });
    whenAll(timeout('mySecondTimer'), function (c) {
        console.log('t1 second timer started ' + c.s.start);
        console.log('t1 second timer ended ' + new Date());
    });
    whenStart(function (host) {
        host.post('t1', {id: 1, sid: 1, start: 'yes'});
    });
}

with (d.ruleset('t2')) {
    whenAll(m.start.eq('yes'), function (c, complete) {
        console.log('t2 first started');

        setTimeout(function() {
            console.log('t2 first completed');
            c.post({id: 2, end: 'yes'});
            complete();
        }, 7000);
        return 10;
    });
    whenAll(m.end.eq('yes'), function (c, complete) {
        console.log('t2 second started');
        setTimeout(function() {
            console.log('t2 second completed');
            c.post({id: 3, end: 'yes'});
            complete();
        }, 7000);
        return 4;
    });
    whenAll(s.exception.ex(), function (c) {
        console.log('t2 expected exception ' + c.s.exception);
        delete(c.s.exception); 
    });
    whenStart(function (host) {
        host.post('t2', {id: 1, sid: 1, start: 'yes'});
    });
}

with (d.ruleset('q0')) {
    whenAll(m.start.eq('yes'), function (c) {
        console.log('q0 started');
        c.getQueue('q0').post({sid: 1, id: 2, end: 'yes'});
    });
    whenAll(m.end.eq('yes'), function (c) {
        console.log('q0 ended');
    });
    whenStart(function (host) {
        host.post('q0', {id: 1, sid: 1, start: 'yes'});
    });
}

d.runAll();