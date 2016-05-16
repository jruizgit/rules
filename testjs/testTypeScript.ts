import * as d from 'durable';

d.ruleset('a0')
 	.whenAll(d.m['amount'].lt(100), (c) => { 
 		console.log('a0 approved from ' + c.s['sid']); 
 	})
 	.whenStart((host) => { 
 		host.post('a0', {id: 1, sid: 1, amount: 10}); 
 	});

d.ruleset('a1')
  	.whenAll(d.c['first'] = d.m['amount'].gt(100),
		     d.c['second'] = d.m['location'].neq(d.c['first']['location']), 
		     (c) => {
            	console.log('fraud1 detected ' + c['first']['location'] + ' ' + c['second']['location']);
        	 })
    .whenStart(function(host) {
        host.post('a1', { id: 1, sid: 1, amount: 200, location: 'US' });
        host.post('a1', { id: 2, sid: 1, amount: 200, location: 'CA' });
    });

d.statechart('a2')
    .state('input')
		.to('denied').whenAll(d.m['subject'].eq('approve').and(d.m['amount'].gt(1000)), (c) => {
        	console.log('a2 denied from: ' + c.s['sid']);
      	})
		.to('pending').whenAll(d.m['subject'].eq('approve').and(d.m['amount'].lte(1000)), (c) => {
            console.log('a2 request approval from: ' + c.s['sid']);
        })
        .end()
    .state('pending')
		.to('pending').whenAll(d.m['subject'].eq('approved'), (c) => {
            console.log('a2 second request approval from: ' + c.s['sid']);
            c.s['status'] = 'approved';
        })
		.to('approved').whenAll(d.s['status'].eq('approved'), (c) => {
            console.log('a2 approved from: ' + c.s['sid']);
        })
		.to('denied').whenAll(d.m['subject'].eq('denied'), (c) => {
            console.log('a2 denied from: ' + c.s['sid']);
        })
        .end()
    .state('denied').end()
    .state('approved').end()
    .whenStart((host) => {
        host.post('a2', { id: 1, sid: 1, subject: 'approve', amount: 100 });
        host.post('a2', { id: 2, sid: 1, subject: 'approved' });
        host.post('a2', { id: 3, sid: 2, subject: 'approve', amount: 100 });
        host.post('a2', { id: 4, sid: 2, subject: 'denied' });
        host.post('a2', { id: 5, sid: 3, subject: 'approve', amount: 10000 });
    });

d.flowchart('a3')
	.stage('input')
    	.to('request').whenAll(d.m['subject'].eq('approve').and(d.m['amount'].lte(1000)))
        .to('deny').whenAll(d.m['subject'].eq('approve').and(d.m['amount'].gt(1000)))
        .end()
    .stage('request')
    	.run((c) => {
            console.log('a3 request approval from: ' + c.s['sid']);
            if (c.s['status'])
                c.s['status'] = 'approved';
            else
                c.s['status'] = 'pending';
        })
        .to('approve').whenAll(d.s['status'].eq('approved'))
        .to('deny').whenAll(d.m['subject'].eq('denied'))
        .to('request').whenAll(d.m['subject'].eq('approved'))
        .end()
    .stage('approve')
        .run((c) => {
            console.log('a3 approved from: ' + c.s['sid']);
        })
        .end()
    .stage('deny')
        .run((c) => {
            console.log('a3 denied from: ' + c.s['sid']);
        })
 		.end()
    .whenStart((host) => {
        host.post('a3', { id: 1, sid: 1, subject: 'approve', amount: 100 });
        host.post('a3', { id: 2, sid: 1, subject: 'approved' });
        host.post('a3', { id: 3, sid: 2, subject: 'approve', amount: 100 });
        host.post('a3', { id: 4, sid: 2, subject: 'denied' });
        host.post('a3', { id: 5, sid: 3, subject: 'approve', amount: 10000 });
    });



d.runAll();