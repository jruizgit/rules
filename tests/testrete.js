r = require('../build/release/rules.node');

handle = r.createRuleset('rules',  
    JSON.stringify({  
        r1: { 
            when: { $and: [{ amount: 10000 }, { subject: 'approve'}] }, 
            run: 'pending' 
        }
    })
);
r.deleteRuleset(handle);

handle = r.createRuleset('rules',  
    JSON.stringify({ 
        r1: { 
            when: { $or: [{ amount: 1000 }, { subject: 'ok' }] }, 
            run: 'pending' 
        }
    })
);
r.deleteRuleset(handle);

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

handle = r.createRuleset('rules',  
    JSON.stringify({
        r1: {
            when: { $lte: { amount: 10000 } },
            run: 'pending'
        }
    })
);
r.deleteRuleset(handle);

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
