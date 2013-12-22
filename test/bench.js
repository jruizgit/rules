// require('nodetime').profile({
//      accountKey: 'a9acc1594618e9fde8299b03dbe1d2577eed50d4',
//      appName: 'PingPong benchmark'
//    });

var d = require('../lib/durable');
var cluster = require('cluster');

if (cluster.isMaster) {
    for (var i = 0; i < 12; i++) {
        cluster.fork();
    }

    cluster.on("exit", function (worker, code, signal) {
        cluster.fork();
    });
}
else {
    d.run({
        ping$state: {
            s: {
                tr: {
                    run: function (s) {
                        s.i = 0;
                    },
                    to: 'r'
                }
            },
            r: {
                ft: {
                    whenAll: {
                        $m: { content: 'ping' },
                        $s: { $lt: { i: 6000 } }
                    },
                    run: function (s) {
                        ++s.i;

                        if (s.i % 1000 === 0) {
                            console.log('Ping\t' + s.i + '\t' + s.id + '\t' + new Date());
                        }
                        s.post({ program: 'pong', sid: s.id, id: s.id + '.' + s.i, content: 'pong' });
                    },
                    to: 'r'
                },
                lt: {
                    when: { $s: { i: 6000 }},
                    to: 'end'
                }
            },
            end: {
            }
        },
        pong$state: {
            s: {
                tr: {
                    run: function (s) {
                        s.i = 0;
                    },
                    to: 'r'
                }
            },
            r: {
                ft: {
                    whenAll: {
                        $m: { content: 'pong' },
                        $s: { $lt: { i: 6000 } }
                    },
                    run: function (s) {
                        ++s.i;

                        if (s.i % 1000 === 0) {
                            console.log('Pong\t' + s.i + '\t' + s.id + '\t' + new Date());
                        }
                        s.post({ program: 'ping', sid: s.id, id: s.id + '.' + s.i, content: 'ping' });
                    },
                    to: 'r'
                },
                lt: {
                    when: { $s: { i: 6000 }},
                    to: 'end'
                }
            },
            end: {
            }
        }
    }, '', ['/tmp/redis0.sock', '/tmp/redis1.sock', '/tmp/redis2.sock'], function (host) {
        host.post({ id: '0.1', program: 'ping', sid: 1, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            } else {
                console.log('ok');
            }
        });

        host.post({ id: '0.2', program: 'ping', sid: 2, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            } else {
                console.log('ok');
            }
        });

        host.post({ id: '0.3', program: 'ping', sid: 3, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            } else {
                console.log('ok');
            }
        });

        host.post({ id: '0.4', program: 'ping', sid: 4, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            } else {
                console.log('ok');
            }
        });

        host.post({ id: '0.5', program: 'ping', sid: 5, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            } else {
                console.log('ok');
            }
        });

        host.post({ id: '0.6', program: 'ping', sid: 6, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            } else {
                console.log('ok');
            }
        });

        host.post({ id: '0.7', program: 'ping', sid: 7, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            } else {
                console.log('ok');
            }
        });

        host.post({ id: '0.8', program: 'ping', sid: 8, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            } else {
                console.log('ok');
            }
        });

        host.post({ id: '0.9', program: 'ping', sid: 9, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            } else {
                console.log('ok');
            }
        });

        host.post({ id: '0.10', program: 'ping', sid: 10, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            } else {
                console.log('ok');
            }
        });

        host.post({ id: '0.11', program: 'ping', sid: 11, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            } else {
                console.log('ok');
            }
        });

        host.post({ id: '0.12', program: 'ping', sid: 12, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            } else {
                console.log('ok');
            }
        });


        host.post({ id: '0.13', program: 'ping', sid: 13, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            } else {
                console.log('ok');
            }
        });

        host.post({ id: '0.14', program: 'ping', sid: 14, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            } else {
                console.log('ok');
            }
        });

        host.post({ id: '0.15', program: 'ping', sid: 15, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            } else {
                console.log('ok');
            }
        });

        host.post({ id: '0.16', program: 'ping', sid: 16, content: 'ping' }, function (err) {
            if (err) {
                console.log(err);
            }
            else {
                console.log('ok');
            }
        });

        // host.post({ id:'0.17', program:'ping', sid:17, content: 'ping' }, function(err) {
        //     if (err) {
        //         console.log(err);
        //     }
        //     else {
        //         console.log('ok');
        //     }
        // });

        // host.post({ id:'0.18', program:'ping', sid:18, content: 'ping' }, function(err) {
        //     if (err) {
        //         console.log(err);
        //     }
        //     else {
        //         console.log('ok');
        //     }
        // });

        // host.post({ id:'0.19', program:'ping', sid:19, content: 'ping' }, function(err) {
        //     if (err) {
        //         console.log(err);
        //     }
        //     else {
        //         console.log('ok');
        //     }
        // });

        // host.post({ id:'0.20', program:'ping', sid:20, content: 'ping' }, function(err) {
        //     if (err) {
        //         console.log(err);
        //     }
        //     else {
        //         console.log('ok');
        //     }
        // });

        // host.post({ id:'0.21', program:'ping', sid:21, content: 'ping' }, function(err) {
        //     if (err) {
        //         console.log(err);
        //     }
        //     else {
        //         console.log('ok');
        //     }
        // });

        // host.post({ id:'0.22', program:'ping', sid:22, content: 'ping' }, function(err) {
        //     if (err) {
        //         console.log(err);
        //     }
        //     else {
        //         console.log('ok');
        //     }
        // });

        // host.post({ id:'0.23', program:'ping', sid:23, content: 'ping' }, function(err) {
        //     if (err) {
        //         console.log(err);
        //     }
        //     else {
        //         console.log('ok');
        //     }
        // });

        // host.post({ id:'0.24', program:'ping', sid:24, content: 'ping' }, function(err) {
        //     if (err) {
        //         console.log(err);
        //     }
        //     else {
        //         console.log('ok');
        //     }
        // });
    });
}