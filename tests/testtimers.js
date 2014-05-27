var d = require('../lib/durable');

d.run({
    timer1: {
        r1: {
            when: { start: 'yes' },
            run: function (s) {
                s.start = new Date().toString();
                s.startTimer('myTimer', 5000);
            }
        },
        r2: {
            when: { $t: 'myTimer' },
            run: function (s) {
                console.log('started @' + s.start);
                console.log('timeout @' + new Date());
            }
        }
    }
}, '', null, function(host) {
    host.post('timer1', { id: 1, sid: 1, start: 'yes' }, function (err) {
        if (err) {
            console.log(err);
        } else {
            console.log('ok');
        }
    });
});