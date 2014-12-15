var d = require('../libjs/durable');


with (d.statechart('fraud2')) {
    with (state('start')) {
        to('standby');
    }
    with (state('standby')) {
        to('metering').when(m.amount.gt(100), function (s) {
            s.startTimer('velocity', 30);
        });
    }
    with (state('metering')) {
        to('fraud').when(m.amount.gt(100).atLeast(2), function (s) {
            console.log('fraud2 detected');
        });
        to('standby').when(timeout('velocity'), function (s) {
            console.log('fraud2 cleared');  
        });
    }
    state('fraud');
    whenStart(function (host) {
        host.post('fraud2', {id: 1, sid: 1, amount: 200});
        host.post('fraud2', {id: 2, sid: 1, amount: 200});
        host.post('fraud2', {id: 3, sid: 1, amount: 200});
    });
}

with (d.ruleset('a0')) {
    when(m.amount.lt(100).or(m.subject.eq('approve')).or(m.subject.eq('ok')), function (s) {
        console.log('a0 approved from ' + s.id);
    });
    whenStart(function (host) {
        host.post('a0', {id: 1, sid: 1, amount: 10});
        host.post('a0', {id: 2, sid: 2, subject: 'approve'});
        host.post('a0', {id: 3, sid: 3, subject: 'ok'});
    });
}

with (d.ruleset('a1')) {
    when(m.subject.eq('approve').and(m.amount.gt(1000)), function (s) {
        console.log('a1 denied from: ' + s.id);
        s.status = 'done';
    });
    when(m.subject.eq('approve').and(m.amount.lte(1000)), function (s) {
        console.log('a1 request approval from: ' + s.id);
        s.status = 'pending';
    });
    whenAll(m.subject.eq('approved'), s.status.eq('pending'), function (s) {
        console.log('a1 second request approval from: ' + s.id);
        s.status = 'approved';
    });
    when(s.status.eq('approved'), function (s) {
        console.log('a1 approved from: ' + s.id);
        s.status = 'done';
    });
    when(m.subject.eq('denied'), function (s) {
        console.log('a1 denied from: ' + s.id);
        s.status = 'done';
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
        to('denied').when(m.subject.eq('approve').and(m.amount.gt(1000)), function (s) {
            console.log('a2 denied from: ' + s.id);
        });
        to('pending').when(m.subject.eq('approve').and(m.amount.lte(1000)), function (s) {
            console.log('a2 request approval from: ' + s.id);
        });
    }
    with (state('pending')) {
        to('pending').when(m.subject.eq('approved'), function (s) {
            console.log('a2 second request approval from: ' + s.id);
            s.status = 'approved';
        });
        to('approved').when(s.status.eq('approved'), function (s) {
            console.log('a2 approved from: ' + s.id);
        });
        to('denied').when(m.subject.eq('denied'), function (s) {
            console.log('a2 denied from: ' + s.id);
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
        to('request').when(m.subject.eq('approve').and(m.amount.lte(1000)));
        to('deny').when(m.subject.eq('approve').and(m.amount.gt(1000)));
    }
    with (stage('request')) {
        run(function (s) {
            console.log('a3 request approval from: ' + s.id);
            if (s.status) 
                s.status = 'approved';
            else
                s.status = 'pending';
        });
        to('approve').when(s.status.eq('approved'));
        to('deny').when(m.subject.eq('denied'));
        to('request').whenAny(m.subject.eq('approved'), m.subject.eq('ok'));
    }
    with (stage('approve')) {
        run(function (s) {
            console.log('a3 approved from: ' + s.id);
        });
    }
    with (stage('deny')) {
        run(function (s) {
            console.log('a3 denied from: ' + s.id);
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
            all(m.subject.eq('jumbo'), m.amount.eq(10000)), function (s) {
        console.log('a4 action from: ' + s.id);
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
            any(m.amount.eq(100), m.amount.eq(10000)), function (s) {
        console.log('a5 action from: ' + s.id);
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
            to('process').when(m.subject.eq('enter'), function (s) {
                console.log('a6 continue process');
            });
        }
        with (state('process')) {
            to('process').when(m.subject.eq('continue'), function (s) {
                console.log('a6 processing');
            });
        }
        to('work').when(m.subject.eq('reset'), function (s) {
            console.log('a6 resetting');
        });
        to('canceled').when(m.subject.eq('cancel'), function (s) {
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
    when(m.amount.lt(s.maxAmount), function (s, m) {
        console.log('a7 approved ' +  m.amount);
    });
    whenStart(function (host) {
        host.patchState('a7', {id: 1, maxAmount: 100});
        host.post('a7', {id: 1, sid: 1, amount: 10});
        host.post('a7', {id: 2, sid: 1, amount: 1000});
    });
}

with (d.ruleset('a8')) {
    when(m.amount.lt(s.maxAmount).and(m.amount.gt(s.id('global').minAmount)), function (s, m) {
        console.log('a8 approved ' +  m.amount);
    });
    whenStart(function (host) {
        host.patchState('a8', {id: 1, maxAmount: 500});
        host.patchState('a8', {id: 'global', minAmount: 100});
        host.post('a8', {id: 1, sid: 1, amount: 10});
        host.post('a8', {id: 2, sid: 1, amount: 200});
    });
}

with (d.ruleset('a9')) {
    when(m.amount.lt(100), atLeast(5), atMost(6), function (s, m) {
        console.log('a9 approved ->' + JSON.stringify(m));
    });
    whenStart(function (host) {
        host.postBatch('a9', {id: 1, sid: 1, amount: 10},
                             {id: 2, sid: 1, amount: 10},
                             {id: 3, sid: 1, amount: 10},
                             {id: 4, sid: 1, amount: 10});
        host.postBatch('a9', [{id: 5, sid: 1, amount: 10},
                             {id: 6, sid: 1, amount: 10}]);
    });   
}

with (d.ruleset('a10')) {
    whenAll(m.amount.lt(100), m.subject.eq('approve'), atLeast(3), atMost(6), function (s, m) {
        console.log('a10 approved ->' + JSON.stringify(m));
    });
    whenStart(function (host) {
        host.postBatch('a10', {id: 1, sid: 1, amount: 10},
                              {id: 2, sid: 1, amount: 10},
                              {id: 3, sid: 1, amount: 10},
                              {id: 4, sid: 1, subject: 'approve'});
        host.postBatch('a10', {id: 5, sid: 1, subject: 'approve'},
                              {id: 6, sid: 1, subject: 'approve'});
    }); 
}

with (d.ruleset('a11')) {
    whenAll(m.amount.lt(100), m.subject.eq('please').atLeast(3).atMost(6), function (s, m) {
        console.log('a11 approved ->' + JSON.stringify(m));
    });
    whenStart(function (host) {
        host.postBatch('a11', {id: 1, sid: 1, amount: 10},
                              {id: 2, sid: 1, subject: 'please'},
                              {id: 3, sid: 1, subject: 'please'},
                              {id: 4, sid: 1, subject: 'please'});
    }); 
}

with (d.ruleset('p1')) {
    with (when(m.start.eq('yes'))) {
        with (ruleset('one')) {
            when(s.start.nex(), function (s) {
                s.start = 1;
            });
            when(s.start.eq(1), function (s) {
                console.log('p1 finish one');
                s.signal({id: 1, end: 'one'});
                s.start  = 2;
            });
        }
        with (ruleset('two')) {
            when(s.start.nex(), function (s) {
                s.start = 1;
            });
            when(s.start.eq(1), function (s) {
                console.log('p1 finish two');
                s.signal({id: 1, end: 'two'});
                s.start  = 2;
            });
        }
    }
    whenAll(m.end.eq('one'), m.end.eq('two'), function (s) {
        console.log('p1 approved');
        s.status = 'approved';
    });
    whenStart(function (host) {
        host.post('p1', {id: 1, sid: 1, start: 'yes'});
    });
}

with (d.statechart('p2')) {
    with (state('input')) {
        to('process').when(m.subject.eq('approve'), function (s, m) {
            console.log('p2 input ' + m.quantity + ' from: ' + s.id);
            s.quantity = m.quantity;
        });
    }
    with (state('process')) {
        with (to('result').when(s.quantity.ex())) {
            with (statechart('first')) {
                with (state('evaluate')) {
                    to('end').when(s.quantity.lte(5), function (s) {
                        console.log('p2 signaling approved from: ' + s.id);
                        s.signal({id: 1, subject: 'approved'});
                    });
                }
                state('end');
            }
            with (statechart('second')) {
                with (state('evaluate')) {
                    to('end').when(s.quantity.gt(5), function (s) {
                        console.log('p2 signaling denied from: ' + s.id);
                        s.signal({id: 1, subject: 'denied'});
                    });
                }
                state('end');
            }
        }
    }
    with (state('result')) {
        to('approved').when(m.subject.eq('approved'), function (s) {
            console.log('p2 approved from: ' + s.id);
        });
        to('denied').when(m.subject.eq('denied'), function (s) {
            console.log('p2 denied from: ' + s.id);
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
        to('input').when(m.subject.eq('approve'));
    }
    with (stage('input')) {
        run(function (s, m) {
            console.log('p3 input ' + m.quantity + ' from: ' + s.id);
            s.quantity = m.quantity;
        });
        to('process');
    }
    with (stage('process')) {
        with (flowchart('first')) {
            with (stage('start')) {
                to('end').when(s.quantity.lte(5));
            }
            with (stage('end')) {
                run(function (s) {
                    console.log('p3 signaling approved from: ' + s.id);
                    s.signal({id: 1, subject: 'approved'});
                });
            }
        }
        with (flowchart('second')) {
            with (stage('start')) {
                to('end').when(s.quantity.gt(5));
            }
            with (stage('end')) {
                run(function (s) {
                    console.log('p3 signaling denied from: ' + s.id);
                    s.signal({id: 1, subject: 'denied'});
                });
            }
        }
        to('approve').when(m.subject.eq('approved'));
        to('deny').when(m.subject.eq('denied'));
    }
    with (stage('approve')) {
        run(function (s) {
            console.log('p3 approved from: ' + s.id);
        });
    }
    with (stage('deny')) {
        run(function (s) {
            console.log('p3 denied from: ' + s.id);
        });
    }
    whenStart(function (host) {
        host.post('p3', {id: 1, sid: 1, subject: 'approve', quantity: 3});
        host.post('p3', {id: 2, sid: 2, subject: 'approve', quantity: 10});
    });
}

with (d.ruleset('t1')) {
    when(m.start.eq('yes'), function (s) {
        s.start = new Date();
        s.startTimer('myTimer', 5);
    });
    when(timeout('myTimer'), function (s) {
        console.log('t1 end');
        console.log('t1 started ' + s.start);
        console.log('t1 ended ' + new Date());
    });
    whenStart(function (host) {
        host.post('t1', {id: 1, sid: 1, start: 'yes'});
    });
}

with (d.flowchart('t2')) {
    with (stage('start')) {
        to('input').when(m.subject.eq('approve'));
    }
    with (stage('input')) {
        with (statechart('first')) {
            with (state('send')) {
                to('evaluate', function(s) {
                    s.start = new Date();
                    s.startTimer('first', 4);
                });
            }
            with (state('evaluate')) {
                to('end').when(timeout('first'), function(s) {
                    s.signal({id: 2, subject: 'approved', start: s.start});
                });
            }
        }
        with (statechart('second')) {
            with (state('send')) {
                to('evaluate', function(s) {
                    s.start = new Date();
                    s.startTimer('first', 3);
                });
            }
            with (state('evaluate')) {
                to('end').when(timeout('first'), function(s) {
                    s.signal({id: 3, subject: 'denied', start: s.start});
                });
            }
        }
        to('approve').when(m.subject.eq('approved'));
        to('deny').when(m.subject.eq('denied'));
    }
    with (stage('approve')) {
        run(function(s, atMost) {
            console.log('t2 approved');
            console.log('t2 started ' + m.start);
            console.log('t2 ended ' + new Date());
        });
    }
    with (stage('deny')) {
        run(function(s, m) {
            console.log('t2 denied');
            console.log('t2 started ' + m.start);
            console.log('t2 ended ' + new Date());
        });
    }
    whenStart(function (host) {
        host.post('t2', {id: 1, sid: 1, subject: 'approve'});
    });
}

d.runAll();