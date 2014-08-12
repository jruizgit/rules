r = require('../build/release/rules.node');

handle = r.createRuleset('rules',  
    JSON.stringify({  
        r1: { 
            whenSome: { $and: [{ amount: 10000 }, { subject: 'approve'}] }, 
            run: 'pending' 
        }
    })
);
r.deleteRuleset(handle);

console.log('created rules1');

handle = r.createRuleset('rules',  
    JSON.stringify({  
        r1: { 
            when: { $and: [{ amount: 10000 }, { subject: 'approve'}] }, 
            run: 'pending' 
        }
    })
);
r.deleteRuleset(handle);

console.log('created rules2');

handle = r.createRuleset('rules',  
    JSON.stringify({ 
        r1: { 
            when: { $or: [{ amount: 1000 }, { subject: 'ok' }] }, 
            run: 'pending' 
        }
    })
);
r.deleteRuleset(handle);

console.log('created rules3');

handle = r.createRuleset('rules',  
    JSON.stringify({ 
        r1: { 
            whenAll: { 
                a: { $lte: { amount: 10 } }, 
                b: { subject: 'yes' } 
            }, 
            run: 'pending' 
        }
    })
);
r.deleteRuleset(handle);

console.log('created rules4');

handle = r.createRuleset('rules',  
    JSON.stringify({
        r1: {
            when: { $lte: { amount: 10000 } },
            run: 'pending'
        }
    })
);
r.deleteRuleset(handle);

console.log('created rules5');

handle = r.createRuleset('rules', 
    JSON.stringify({ 
        r1: { 
            when: { $lte: { number: 10000 } }, 
            run: 'pending' 
        }, 
        r2: { 
            when: { $gte: { amount: 1 } }, 
            run: 'done' 
        }
    })
);
r.deleteRuleset(handle);

console.log('created rules6');

handle = r.createRuleset('rules', 
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
r.deleteRuleset(handle);

console.log('created rules7');

handle = r.createRuleset('rules', 
    JSON.stringify({ r1: {
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
r.deleteRuleset(handle);

console.log('created rules8');

handle = r.createRuleset('rules', 
    JSON.stringify({ r1: {
            whenAll: {
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
r.deleteRuleset(handle);

console.log('created rules9');

handle = r.createRuleset('rules', 
    JSON.stringify({ r1: {
            whenAny: {
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
r.deleteRuleset(handle);

console.log('created rules10');

handle = r.createRuleset('rules', 
    JSON.stringify({ r1: {
            whenAny: {
                c: { subject: 'review' },
                d$all: {
                    e: { $lt: { total: 1000 }},
                    f: { $lt: { amount: 1000 }}
                }
            },
            run: 'unitTest'
        }
    })
);
r.deleteRuleset(handle);

console.log('created rules11');

handle = r.createRuleset('rules', 
    JSON.stringify({ r1: {
            whenAny: {
                c: { subject: 'review' },
                d$all: {
                    e: { $lt: { total: 1000 }},
                    f: { $lt: { amount: 1000 }}
                },
                b: { subject: 'approve' }
            },
            run: 'unitTest'
        }
    })
);
r.deleteRuleset(handle);

console.log('created rules12');

handle = r.createRuleset('rules', 
    JSON.stringify({ r1: {
            whenAll: {
                c: { subject: 'review' },
                d$any: {
                    e: { $lt: { total: 1000 }},
                    f: { $lt: { amount: 1000 }}
                }
            },
            run: 'unitTest'
        }
    })
);
r.deleteRuleset(handle);

console.log('created rules13');
