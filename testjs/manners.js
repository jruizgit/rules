var d = require('../libjs/durable');

with (d.statechart('missManners')) {
    with (state('start')) {
        to('assign').whenAll(m.t.eq('guest'), 
        function(c) {
            c.assert({t: 'seating',
                      id: c.s.gcount,
                      tid: c.s.count,
                      pid: 0,
                      path: true,
                      leftSeat: 1,
                      leftGuestName: c.m.name,
                      rightSeat: 1,
                      rightGuestName: c.m.name});
            c.assert({t: 'path',
                      id: c.s.gcount + 1,
                      pid: c.s.count,
                      seat: 1,
                      guestName: c.m.name});
            c.s.count += 1;
            c.s.gcount += 2;
            console.log('assign ' + c.m.name + ' ' + new Date());
        });
    }
    with (state('assign')) {
        to('make').whenAll(c.seating = and(m.t.eq('seating'), 
                                           m.path.eq(true)),
                           c.rightGuest = and(m.t.eq('guest'), 
                                              m.name.eq(c.seating.rightGuestName)),
                           c.leftGuest = and(m.t.eq('guest'), 
                                             m.sex.neq(c.rightGuest.sex), 
                                             m.hobby.eq(c.rightGuest.hobby)),
                           none(and(m.t.eq('path'),
                                    m.pid.eq(c.seating.tid),
                                    m.guestName.eq(c.leftGuest.name))),
                           none(and(m.t.eq('chosen'),
                                    m.cid.eq(c.seating.tid),
                                    m.guestName.eq(c.leftGuest.name),
                                    m.hobby.eq(c.rightGuest.hobby))), 
        function(c) {
            c.assert({t: 'seating',
                      id: c.s.gcount,
                      tid: c.s.count,
                      pid: c.seating.tid,
                      path: false,
                      leftSeat: c.seating.rightSeat,
                      leftGuestName: c.seating.rightGuestName,
                      rightSeat: c.seating.rightSeat + 1,
                      rightGuestName: c.leftGuest.name});
            c.assert({t: 'path',
                      id: c.s.gcount + 1,
                      pid: c.s.count,
                      seat: c.seating.rightSeat + 1,
                      guestName: c.leftGuest.name});
            c.assert({t: 'chosen',
                      id: c.s.gcount + 2,
                      cid: c.seating.tid,
                      guestName: c.leftGuest.name,
                      hobby: c.rightGuest.hobby});
            c.s.count += 1;
            c.s.gcount += 3;
        }); 
    }
    with (state('make')) {
        to('make').whenAll(c.seating = and(m.t.eq('seating'),
                                           m.path.eq(false)),
                           c.path = and(m.t.eq('path'),
                                        m.pid.eq(c.seating.pid)),
                           none(and(m.t.eq('path'),
                                    m.pid.eq(c.seating.tid),
                                    m.guestName.eq(c.path.guestName))), 
        function(c) {
            c.assert({t: 'path',
                      id: c.s.gcount,
                      pid: c.seating.tid,
                      seat: c.path.seat,
                      guestName: c.path.guestName});
            c.s.gcount += 1;
        });

        to('check').whenAll(pri(1), and(m.t.eq('seating'), m.path.eq(false)), 
        function(c) {
            c.retract(c.m);
            c.m.id = c.s.gcount;
            c.m.path = true;
            c.assert(c.m);
            c.s.gcount += 1;
            console.log('path sid: ' + c.m.tid + ', pid: ' + c.m.pid + ', left: ' + c.m.leftGuestName + ', right: ' + c.m.rightGuestName + ' ' + new Date());
        });
    }
    with (state('check')) {
        to('end').whenAll(c.lastSeat = m.t.eq('lastSeat'),
                          and(m.t.eq('seating'), m.rightSeat.eq(c.lastSeat.seat)), 
        function(c) {
            console.log('end ' + new Date());
        });
        to('assign');
    }
    
    state('end');

    whenStart(function (host) {
        host.assert('missManners', {id: 1, sid: 1, t: 'guest', name: 'n1', sex: 'm', hobby: 'h3'});
        host.assert('missManners', {id: 2, sid: 1, t: 'guest', name: 'n1', sex: 'm', hobby: 'h2'});
        host.assert('missManners', {id: 3, sid: 1, t: 'guest', name: 'n2', sex: 'm', hobby: 'h2'});
        host.assert('missManners', {id: 4, sid: 1, t: 'guest', name: 'n2', sex: 'm', hobby: 'h3'});
        host.assert('missManners', {id: 5, sid: 1, t: 'guest', name: 'n3', sex: 'm', hobby: 'h1'});
        host.assert('missManners', {id: 6, sid: 1, t: 'guest', name: 'n3', sex: 'm', hobby: 'h2'});
        host.assert('missManners', {id: 7, sid: 1, t: 'guest', name: 'n3', sex: 'm', hobby: 'h3'});
        host.assert('missManners', {id: 8, sid: 1, t: 'guest', name: 'n4', sex: 'f', hobby: 'h3'});
        host.assert('missManners', {id: 9, sid: 1, t: 'guest', name: 'n4', sex: 'f', hobby: 'h2'});
        host.assert('missManners', {id: 10, sid: 1, t: 'guest', name: 'n5', sex: 'f', hobby: 'h1'});
        host.assert('missManners', {id: 11, sid: 1, t: 'guest', name: 'n5', sex: 'f', hobby: 'h2'});
        host.assert('missManners', {id: 12, sid: 1, t: 'guest', name: 'n5', sex: 'f', hobby: 'h3'});
        host.assert('missManners', {id: 13, sid: 1, t: 'guest', name: 'n6', sex: 'f', hobby: 'h3'});
        host.assert('missManners', {id: 14, sid: 1, t: 'guest', name: 'n6', sex: 'f', hobby: 'h1'});
        host.assert('missManners', {id: 15, sid: 1, t: 'guest', name: 'n6', sex: 'f', hobby: 'h2'});
        host.assert('missManners', {id: 16, sid: 1, t: 'guest', name: 'n7', sex: 'f', hobby: 'h3'});
        host.assert('missManners', {id: 17, sid: 1, t: 'guest', name: 'n7', sex: 'f', hobby: 'h2'});
        host.assert('missManners', {id: 18, sid: 1, t: 'guest', name: 'n8', sex: 'm', hobby: 'h3'});
        host.assert('missManners', {id: 19, sid: 1, t: 'guest', name: 'n8', sex: 'm', hobby: 'h1'});
        host.assert('missManners', {id: 20, sid: 1, t: 'lastSeat', seat: 8});
        host.patchState('missManners', {sid: 1, label: 'start', count: 0, gcount: 1000});
    });
}

d.runAll();
