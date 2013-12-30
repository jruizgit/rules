var mailer = require('nodemailer');
var stat = require('node-static');
var d = require('../lib/durable');

var transport = mailer.createTransport('SMTP',{
    service: 'Gmail',
    auth: {
        user: 'durablejs@gmail.com',
        pass: 'durablepass'
    }
});

d.run({
    approve$state: {
        input: {
            deny: {
                when: { $gt: { amount: 1000 } },
                run: deny,
                to: 'denied'
            },
            request: {
                when: { $lte: { amount: 1000 } },
                run: requestApproval,
                to: 'pending'
            }
        },
        pending: {
            request: {
                whenAll: {
                    $s: { $lt: { i: 3 } },
                    $m: { $t: 'timeout' }
                },
                run: requestApproval,
                to: 'pending'
            },
            approve: {
                when: { subject: 'approve' },
                run: approve,
                to: 'approved'
            },
            deny: {
                whenAny: {
                    $m: { subject: 'deny' },
                    s$all: {
                        $s: { $gte: { i: 3 } },
                        $m: { $t: 'timeout' }
                    }
                },
                run: deny,
                to: 'denied'
            }
        },
        denied: {
        },
        approved: {
        }
    }
}, '', null, function(host) {
    var fileServer = new stat.Server(__dirname);
    console.log(__dirname);
    host.getApp().get('/approve.html', function (request, response) {
        request.addListener('end', function () {
            fileServer.serveFile('/approve.html', 200, {}, request, response);
        }).resume();
    });

    host.post({ id: '1', program: 'approve', sid: 1, amount: 500, from: 'jr3791@live.com', to: 'jr3791@live.com' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
});

function deny (s, complete) {
    setState(s);
    var mailOptions = {
        from: 'approval',
        to: s.from,
        subject: 'Request for ' + s.amount + ' has been denied.'
    };
    transport.sendMail(mailOptions, complete);
}

function requestApproval (s, complete) {
    setState(s);
    if (!s.i) {
        s.i = 1;
    } else {
        ++s.i;
    }

    s.startTimer('timeout', 300);
    var mailOptions = {
        from: 'approval',
        to: s.to,
        subject: 'Requesting approval for ' + s.amount + ' from ' + s.from + ' try #' + s.i + '.',
        html: 'Go <a href="http://localhost:5000/approve.html?session=' + s.id + '">here</a> to approve or deny.'
    };
    transport.sendMail(mailOptions, complete);
}

function approve (s, complete) {
    var mailOptions = {
        from: 'approval',
        to: s.from,
        subject: 'Request for ' + s.amount + ' has been approved.'
    };
    transport.sendMail(mailOptions, complete);
}

function setState(s) {
    if (!s.from) {
        s.from = s.getOutput().from;
        s.amount = s.getOutput().amount;
        s.to = s.getOutput().to;
    }
}
