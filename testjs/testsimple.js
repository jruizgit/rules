var d = require('../libjs/durable');

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
            console.log('fraud5 first ' + c.m.amount);
        }
    );
    whenAll(m.amount.gt(200),
            pri(2),
        function(c) {
            console.log('fraud5 second ' + c.m.amount);
        }
    );
    whenAll(m.amount.gt(300),
            pri(1),
        function(c) {
            console.log('fraud5 third ' + c.m.amount);
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

with (d.ruleset('fraud6')) {
    whenAll(c.first = m.t.eq('deposit'),
            none(m.t.eq('withrawal')), 
            c.third = m.t.eq('chargeback'),
            count(2),
        function(c) {
            console.log('fraud6 detected ' + JSON.stringify(c.m));
        }
    );
    whenStart(function (host) {
        host.post('fraud6', {id: 1, sid: 1, t: 'deposit'});
        host.assert('fraud6', {id: 2, sid: 1, t: 'withrawal'});
        host.post('fraud6', {id: 3, sid: 1, t: 'chargeback'});
        host.post('fraud6', {id: 4, sid: 1, t: 'deposit'});
        host.post('fraud6', {id: 5, sid: 1, t: 'chargeback'});
        host.retract('fraud6', {id: 2, sid: 1, t: 'withrawal'});
    });
}

with (d.ruleset('a0')) {
    whenAll(or(m.amount.lt(100), m.subject.eq('approve'), m.subject.eq('ok')), function (c) {
        console.log('a0 approved from ' + c.s.sid);
    });
    whenStart(function (host) {
        host.post('a0', {id: 1, sid: 1, amount: 10});
        host.post('a0', {id: 2, sid: 2, subject: 'approve'});
        host.post('a0', {id: 3, sid: 3, subject: 'ok'});
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
        to('request').whenAny(m.subject.eq('approved'), m.subject.eq('ok'));
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
    with (state('start')) {
        to('work');
    }
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
        to('work').whenAll(m.subject.eq('reset'), function (c) {
            console.log('a6 resetting');
        });
        to('canceled').whenAll(m.subject.eq('cancel'), function (c) {
            console.log('a6 canceling');
        });
    }
    state('canceled');
    whenStart(function (host) {
        host.post('a6', {id: 1, sid: 1, subject: 'enter'});
        host.post('a6', {id: 2, sid: 1, subject: 'enter'});
        host.post('a6', {id: 3, sid: 1, subject: 'continue'});
        host.post('a6', {id: 4, sid: 1, subject: 'continue'});
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
    whenAll(m.amount.lt(c.s.maxAmount).and(m.amount.gt(c.s.id('global').minAmount)), function (c) {
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
    whenAll(m.amount.gt(c.s.maxAmount.add(c.s.id('global').minAmount)), function (c) {
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

with (d.ruleset('p1')) {
    with (whenAll(m.start.eq('yes'))) {
        with (ruleset('one')) {
            whenAll(s.start.nex(), function (c) {
                c.s.start = 1;
            });
            whenAll(s.start.eq(1), function (c) {
                console.log('p1 finish one');
                c.signal({id: 1, end: 'one'});
                c.s.start  = 2;
            });
        }
        with (ruleset('two')) {
            whenAll(s.start.nex(), function (c) {
                c.s.start = 1;
            });
            whenAll(s.start.eq(1), function (c) {
                console.log('p1 finish two');
                c.signal({id: 1, end: 'two'});
                c.s.start  = 2;
            });
        }
    }
    whenAll(m.end.eq('one'), m.end.eq('two'), function (c) {
        console.log('p1 approved');
        s.status = 'approved';
    });
    whenStart(function (host) {
        host.post('p1', {id: 1, sid: 1, start: 'yes'});
    });
}

with (d.statechart('p2')) {
    with (state('input')) {
        to('process').whenAll(m.subject.eq('approve'), function (c) {
            console.log('p2 input ' + c.m.quantity + ' from: ' + c.s.sid);
            c.s.quantity = c.m.quantity;
        });
    }
    with (state('process')) {
        with (to('result').whenAll(s.quantity.ex())) {
            with (statechart('first')) {
                with (state('evaluate')) {
                    to('end').whenAll(s.quantity.lte(5), function (c) {
                        console.log('p2 signaling approved from: ' + c.s.sid);
                        c.signal({id: 1, subject: 'approved'});
                    });
                }
                state('end');
            }
            with (statechart('second')) {
                with (state('evaluate')) {
                    to('end').whenAll(s.quantity.gt(5), function (c) {
                        console.log('p2 signaling denied from: ' + c.s.sid);
                        c.signal({id: 1, subject: 'denied'});
                    });
                }
                state('end');
            }
        }
    }
    with (state('result')) {
        to('approved').whenAll(m.subject.eq('approved'), function (c) {
            console.log('p2 approved from: ' + c.s.sid);
        });
        to('denied').whenAll(m.subject.eq('denied'), function (c) {
            console.log('p2 denied from: ' + c.s.sid);
        });
    }
    state('denied');
    state('approved');
    whenStart(function (host) {
        host.post('p2', {id: 1, sid: 1, subject: 'approve', quantity: 3});
        host.post('p2', {id: 2, sid: 2, subject: 'approve', quantity: 10});
    });
}

with (d.flowchart('p3')) {
    with (stage('start')) {
        to('input').whenAll(m.subject.eq('approve'));
    }
    with (stage('input')) {
        run(function (c) {
            console.log('p3 input ' + c.m.quantity + ' from: ' + c.s.sid);
            c.s.quantity = c.m.quantity;
        });
        to('process');
    }
    with (stage('process')) {
        with (flowchart('first')) {
            with (stage('start')) {
                to('end').whenAll(s.quantity.lte(5));
            }
            with (stage('end')) {
                run(function (c) {
                    console.log('p3 signaling approved from: ' + c.s.sid);
                    c.signal({id: 1, subject: 'approved'});
                });
            }
        }
        with (flowchart('second')) {
            with (stage('start')) {
                to('end').whenAll(s.quantity.gt(5));
            }
            with (stage('end')) {
                run(function (c) {
                    console.log('p3 signaling denied from: ' + c.s.sid);
                    c.signal({id: 1, subject: 'denied'});
                });
            }
        }
        to('approve').whenAll(m.subject.eq('approved'));
        to('deny').whenAll(m.subject.eq('denied'));
    }
    with (stage('approve')) {
        run(function (c) {
            console.log('p3 approved from: ' + c.s.sid);
        });
    }
    with (stage('deny')) {
        run(function (c) {
            console.log('p3 denied from: ' + c.s.sid);
        });
    }
    whenStart(function (host) {
        host.post('p3', {id: 1, sid: 1, subject: 'approve', quantity: 3});
        host.post('p3', {id: 2, sid: 2, subject: 'approve', quantity: 10});
    });
}

with (d.ruleset('t0')) {
    whenAll(or(m.start.eq('yes'), timeout('myTimer')), function (c) {
        c.s.count = !c.s.count ? 1 : c.s.count + 1;
        c.post('t0', {id: c.s.count, sid: 1, t: 'purchase'});
        c.startTimer('myTimer', Math.random() * 3 + 1);
    });
    whenAll(span(5), m.t.eq('purchase'), function (c) {
        console.log('t0 pulse ->' + c.m.length);
    });
    whenStart(function (host) {
        host.post('t0', {id: 1, sid: 1, start: 'yes'});
    });
}

with (d.ruleset('t1')) {
    whenAll(m.start.eq('yes'), function (c) {
        c.s.start = new Date();
        c.startTimer('myTimer', 5);
    });
    whenAll(timeout('myTimer'), function (c) {
        console.log('t1 end');
        console.log('t1 started ' + c.s.start);
        console.log('t1 ended ' + new Date());
    });
    whenStart(function (host) {
        host.post('t1', {id: 1, sid: 1, start: 'yes'});
    });
}

with (d.flowchart('t2')) {
    with (stage('start')) {
        to('input').whenAll(m.subject.eq('approve'));
    }
    with (stage('input')) {
        with (statechart('first')) {
            with (state('send')) {
                to('evaluate', function(c) {
                    c.s.start = new Date();
                    c.startTimer('first', 4);
                });
            }
            with (state('evaluate')) {
                to('end').whenAll(timeout('first'), function(c) {
                    c.signal({id: 2, subject: 'approved', start: c.s.start});
                });
            }
        }
        with (statechart('second')) {
            with (state('send')) {
                to('evaluate', function(c) {
                    c.s.start = new Date();
                    c.startTimer('first', 3);
                });
            }
            with (state('evaluate')) {
                to('end').whenAll(timeout('first'), function(c) {
                    c.signal({id: 3, subject: 'denied', start: c.s.start});
                });
            }
        }
        to('approve').whenAll(m.subject.eq('approved'));
        to('deny').whenAll(m.subject.eq('denied'));
    }
    with (stage('approve')) {
        run(function(c) {
            console.log('t2 approved');
            console.log('t2 started ' + c.m.start);
            console.log('t2 ended ' + new Date());
        });
    }
    with (stage('deny')) {
        run(function(c) {
            console.log('t2 denied');
            console.log('t2 started ' + c.m.start);
            console.log('t2 ended ' + new Date());
        });
    }
    whenStart(function (host) {
        host.post('t2', {id: 1, sid: 1, subject: 'approve'});
    });
}

d.runAll();