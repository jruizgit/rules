var d = require('../libjs/durable');

d.statechart('missManners', function() {
    start: {
        to: 'assign'
        whenAll: m.t == 'guest' 
        run: {
            s.count = 0;
            assert({ t: 'seating',
                     tid: s.count,
                     pid: 0,
                     path: true,
                     leftSeat: 1,
                     leftGuestName: m.name,
                     rightSeat: 1,
                     rightGuestName: m.name });
            assert({ t: 'path',
                     pid: s.count,
                     seat: 1,
                     guestName: m.name });
            s.count += 1;
            s.startTime = new Date();
            console.log('assign ' + c.m.name);
        }
    }

    assign: {
        to: 'make'
        whenAll: {
            seating = m.t == 'seating' && m.path == true
            rightGuest = m.t == 'guest' && m.name == seating.rightGuestName
            leftGuest = m.t == 'guest' && m.sex != rightGuest.sex && m.hobby == rightGuest.hobby
            none(m.t == 'path' && m.pid == seating.tid && m.guestName ==leftGuest.name)
            none(m.t == 'chosen' && m.cid == seating.tid && m.guestName == leftGuest.name && m.hobby == rightGuest.hobby)
        } 
        run: {
            assert({ t: 'seating',
                     tid: s.count,
                     pid: seating.tid,
                     path: false,
                     leftSeat: seating.rightSeat,
                     leftGuestName: seating.rightGuestName,
                     rightSeat: seating.rightSeat + 1,
                     rightGuestName: leftGuest.name });
            assert({ t: 'path',
                     pid: s.count,
                     seat: seating.rightSeat + 1,
                     guestName: leftGuest.name });
            assert({ t: 'chosen',
                     cid: seating.tid,
                     guestName: leftGuest.name,
                     hobby: rightGuest.hobby });
            s.count += 1;
        } 
    }

    make: {
        to: 'make'
        whenAll: {
            seating = m.t == 'seating' && m.path == false
            path = m.t == 'path' && m.pid == seating.pid
            none(m.t == 'path' && m.pid == seating.tid && m.guestName == path.guestName)
        }
        cap: 1000
        run: {
            for (var i = 0; i < m.length; ++i) {
                var frame = m[i];
                assert({ t: 'path',
                         pid: frame.seating.tid,
                         seat: frame.path.seat,
                         guestName: frame.path.guestName });
            }
        }

        to: 'check'
        whenAll: m.t == 'seating' && m.path == false
        pri: 1
        run: {
            retract(m);
            delete(m.id);
            m.path = true;
            assert(m);
            console.log('path sid: ' + m.tid + ', pid: ' + m.pid + ', left: ' + m.leftGuestName + ', right: ' + m.rightGuestName);
        }
    }

    check: {
        to: 'end'
        whenAll: {
            lastSeat = m.t == 'lastSeat'
            m.t == 'seating' && m.rightSeat == lastSeat.seat
        } 
        run: {
            console.log('end ' + (new Date() - s.startTime));
            deleteState();
        }

        to: 'assign'
    }
    
    end: {}

    whenStart: {
        assert('missManners', {id: 1, sid: 1, t: 'guest', name: 'n1', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 2, sid: 1, t: 'guest', name: 'n1', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 3, sid: 1, t: 'guest', name: 'n2', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 4, sid: 1, t: 'guest', name: 'n2', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 5, sid: 1, t: 'guest', name: 'n3', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 6, sid: 1, t: 'guest', name: 'n3', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 7, sid: 1, t: 'guest', name: 'n3', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 8, sid: 1, t: 'guest', name: 'n4', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 9, sid: 1, t: 'guest', name: 'n4', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 10, sid: 1, t: 'guest', name: 'n5', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 11, sid: 1, t: 'guest', name: 'n5', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 12, sid: 1, t: 'guest', name: 'n5', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 13, sid: 1, t: 'guest', name: 'n6', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 14, sid: 1, t: 'guest', name: 'n6', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 15, sid: 1, t: 'guest', name: 'n6', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 16, sid: 1, t: 'guest', name: 'n7', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 17, sid: 1, t: 'guest', name: 'n7', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 18, sid: 1, t: 'guest', name: 'n8', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 19, sid: 1, t: 'guest', name: 'n8', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 20, sid: 1, t: 'lastSeat', seat: 8});
    }
});

d.runAll();
