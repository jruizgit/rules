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
        assert('missManners', {id: 1, t: 'guest', name: '1', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 2, t: 'guest', name: '1', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 3, t: 'guest', name: '1', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 4, t: 'guest', name: '1', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 5, t: 'guest', name: '1', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 6, t: 'guest', name: '2', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 7, t: 'guest', name: '2', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 8, t: 'guest', name: '2', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 9, t: 'guest', name: '2', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 10, t: 'guest', name: '2', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 11, t: 'guest', name: '3', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 12, t: 'guest', name: '3', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 13, t: 'guest', name: '3', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 14, t: 'guest', name: '4', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 15, t: 'guest', name: '4', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 16, t: 'guest', name: '4', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 17, t: 'guest', name: '4', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 18, t: 'guest', name: '5', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 19, t: 'guest', name: '5', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 20, t: 'guest', name: '5', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 21, t: 'guest', name: '6', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 22, t: 'guest', name: '6', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 23, t: 'guest', name: '6', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 24, t: 'guest', name: '6', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 25, t: 'guest', name: '6', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 26, t: 'guest', name: '7', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 27, t: 'guest', name: '7', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 28, t: 'guest', name: '7', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 29, t: 'guest', name: '7', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 30, t: 'guest', name: '8', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 31, t: 'guest', name: '8', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 32, t: 'guest', name: '9', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 33, t: 'guest', name: '9', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 34, t: 'guest', name: '9', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 35, t: 'guest', name: '9', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 36, t: 'guest', name: '10', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 37, t: 'guest', name: '10', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 38, t: 'guest', name: '10', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 39, t: 'guest', name: '10', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 40, t: 'guest', name: '10', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 41, t: 'guest', name: '11', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 42, t: 'guest', name: '11', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 43, t: 'guest', name: '11', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 44, t: 'guest', name: '11', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 45, t: 'guest', name: '12', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 46, t: 'guest', name: '12', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 47, t: 'guest', name: '12', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 48, t: 'guest', name: '13', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 49, t: 'guest', name: '13', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 50, t: 'guest', name: '14', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 51, t: 'guest', name: '14', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 52, t: 'guest', name: '14', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 53, t: 'guest', name: '14', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 54, t: 'guest', name: '15', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 55, t: 'guest', name: '15', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 56, t: 'guest', name: '15', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 57, t: 'guest', name: '15', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 58, t: 'guest', name: '15', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 59, t: 'guest', name: '16', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 60, t: 'guest', name: '16', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 61, t: 'guest', name: '16', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 62, t: 'guest', name: '17', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 63, t: 'guest', name: '17', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 64, t: 'guest', name: '18', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 65, t: 'guest', name: '18', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 66, t: 'guest', name: '18', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 67, t: 'guest', name: '18', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 68, t: 'guest', name: '19', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 69, t: 'guest', name: '19', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 70, t: 'guest', name: '20', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 71, t: 'guest', name: '20', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 72, t: 'guest', name: '21', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 73, t: 'guest', name: '21', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 74, t: 'guest', name: '21', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 75, t: 'guest', name: '21', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 76, t: 'guest', name: '22', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 77, t: 'guest', name: '22', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 78, t: 'guest', name: '22', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 79, t: 'guest', name: '23', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 80, t: 'guest', name: '23', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 81, t: 'guest', name: '23', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 82, t: 'guest', name: '24', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 83, t: 'guest', name: '24', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 84, t: 'guest', name: '24', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 85, t: 'guest', name: '24', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 86, t: 'guest', name: '24', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 87, t: 'guest', name: '25', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 88, t: 'guest', name: '25', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 89, t: 'guest', name: '26', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 90, t: 'guest', name: '26', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 91, t: 'guest', name: '27', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 92, t: 'guest', name: '27', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 93, t: 'guest', name: '27', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 94, t: 'guest', name: '28', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 95, t: 'guest', name: '28', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 96, t: 'guest', name: '28', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 97, t: 'guest', name: '28', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 98, t: 'guest', name: '28', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 99, t: 'guest', name: '29', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 100, t: 'guest', name: '29', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 101, t: 'guest', name: '30', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 102, t: 'guest', name: '30', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 103, t: 'guest', name: '31', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 104, t: 'guest', name: '31', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 105, t: 'guest', name: '31', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 106, t: 'guest', name: '32', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 107, t: 'guest', name: '32', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 108, t: 'guest', name: '32', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 109, t: 'guest', name: '33', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 110, t: 'guest', name: '33', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 111, t: 'guest', name: '34', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 112, t: 'guest', name: '34', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 113, t: 'guest', name: '34', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 114, t: 'guest', name: '35', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 115, t: 'guest', name: '35', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 116, t: 'guest', name: '35', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 117, t: 'guest', name: '35', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 118, t: 'guest', name: '35', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 119, t: 'guest', name: '36', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 120, t: 'guest', name: '36', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 121, t: 'guest', name: '36', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 122, t: 'guest', name: '36', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 123, t: 'guest', name: '37', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 124, t: 'guest', name: '37', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 125, t: 'guest', name: '37', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 126, t: 'guest', name: '38', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 127, t: 'guest', name: '38', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 128, t: 'guest', name: '38', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 129, t: 'guest', name: '38', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 130, t: 'guest', name: '39', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 131, t: 'guest', name: '39', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 132, t: 'guest', name: '40', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 133, t: 'guest', name: '40', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 134, t: 'guest', name: '41', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 135, t: 'guest', name: '41', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 136, t: 'guest', name: '42', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 137, t: 'guest', name: '42', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 138, t: 'guest', name: '43', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 139, t: 'guest', name: '43', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 140, t: 'guest', name: '43', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 141, t: 'guest', name: '44', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 142, t: 'guest', name: '44', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 143, t: 'guest', name: '44', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 144, t: 'guest', name: '44', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 145, t: 'guest', name: '45', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 146, t: 'guest', name: '45', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 147, t: 'guest', name: '46', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 148, t: 'guest', name: '46', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 149, t: 'guest', name: '46', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 150, t: 'guest', name: '47', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 151, t: 'guest', name: '47', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 152, t: 'guest', name: '47', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 153, t: 'guest', name: '48', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 154, t: 'guest', name: '48', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 155, t: 'guest', name: '49', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 156, t: 'guest', name: '49', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 157, t: 'guest', name: '49', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 158, t: 'guest', name: '49', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 159, t: 'guest', name: '49', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 160, t: 'guest', name: '50', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 161, t: 'guest', name: '50', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 162, t: 'guest', name: '50', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 163, t: 'guest', name: '51', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 164, t: 'guest', name: '51', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 165, t: 'guest', name: '51', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 166, t: 'guest', name: '51', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 167, t: 'guest', name: '52', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 168, t: 'guest', name: '52', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 169, t: 'guest', name: '52', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 170, t: 'guest', name: '52', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 171, t: 'guest', name: '53', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 172, t: 'guest', name: '53', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 173, t: 'guest', name: '53', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 174, t: 'guest', name: '53', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 175, t: 'guest', name: '53', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 176, t: 'guest', name: '54', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 177, t: 'guest', name: '54', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 178, t: 'guest', name: '55', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 179, t: 'guest', name: '55', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 180, t: 'guest', name: '56', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 181, t: 'guest', name: '56', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 182, t: 'guest', name: '57', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 183, t: 'guest', name: '57', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 184, t: 'guest', name: '57', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 185, t: 'guest', name: '58', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 186, t: 'guest', name: '58', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 187, t: 'guest', name: '58', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 188, t: 'guest', name: '58', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 189, t: 'guest', name: '58', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 190, t: 'guest', name: '59', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 191, t: 'guest', name: '59', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 192, t: 'guest', name: '59', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 193, t: 'guest', name: '60', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 194, t: 'guest', name: '60', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 195, t: 'guest', name: '60', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 196, t: 'guest', name: '60', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 197, t: 'guest', name: '61', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 198, t: 'guest', name: '61', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 199, t: 'guest', name: '61', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 200, t: 'guest', name: '61', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 201, t: 'guest', name: '62', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 202, t: 'guest', name: '62', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 203, t: 'guest', name: '62', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 204, t: 'guest', name: '62', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 205, t: 'guest', name: '62', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 206, t: 'guest', name: '63', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 207, t: 'guest', name: '63', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 208, t: 'guest', name: '63', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 209, t: 'guest', name: '63', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 210, t: 'guest', name: '63', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 211, t: 'guest', name: '64', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 212, t: 'guest', name: '64', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 213, t: 'guest', name: '64', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 214, t: 'guest', name: '64', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 215, t: 'guest', name: '64', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 216, t: 'guest', name: '65', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 217, t: 'guest', name: '65', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 218, t: 'guest', name: '65', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 219, t: 'guest', name: '65', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 220, t: 'guest', name: '65', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 221, t: 'guest', name: '66', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 222, t: 'guest', name: '66', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 223, t: 'guest', name: '66', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 224, t: 'guest', name: '67', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 225, t: 'guest', name: '67', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 226, t: 'guest', name: '68', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 227, t: 'guest', name: '68', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 228, t: 'guest', name: '68', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 229, t: 'guest', name: '68', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 230, t: 'guest', name: '69', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 231, t: 'guest', name: '69', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 232, t: 'guest', name: '69', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 233, t: 'guest', name: '70', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 234, t: 'guest', name: '70', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 235, t: 'guest', name: '70', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 236, t: 'guest', name: '70', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 237, t: 'guest', name: '70', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 238, t: 'guest', name: '71', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 239, t: 'guest', name: '71', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 240, t: 'guest', name: '71', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 241, t: 'guest', name: '72', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 242, t: 'guest', name: '72', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 243, t: 'guest', name: '72', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 244, t: 'guest', name: '73', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 245, t: 'guest', name: '73', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 246, t: 'guest', name: '73', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 247, t: 'guest', name: '74', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 248, t: 'guest', name: '74', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 249, t: 'guest', name: '74', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 250, t: 'guest', name: '74', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 251, t: 'guest', name: '74', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 252, t: 'guest', name: '75', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 253, t: 'guest', name: '75', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 254, t: 'guest', name: '76', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 255, t: 'guest', name: '76', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 256, t: 'guest', name: '76', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 257, t: 'guest', name: '76', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 258, t: 'guest', name: '76', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 259, t: 'guest', name: '77', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 260, t: 'guest', name: '77', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 261, t: 'guest', name: '78', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 262, t: 'guest', name: '78', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 263, t: 'guest', name: '79', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 264, t: 'guest', name: '79', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 265, t: 'guest', name: '79', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 266, t: 'guest', name: '79', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 267, t: 'guest', name: '79', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 268, t: 'guest', name: '80', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 269, t: 'guest', name: '80', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 270, t: 'guest', name: '80', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 271, t: 'guest', name: '80', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 272, t: 'guest', name: '81', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 273, t: 'guest', name: '81', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 274, t: 'guest', name: '82', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 275, t: 'guest', name: '82', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 276, t: 'guest', name: '82', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 277, t: 'guest', name: '82', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 278, t: 'guest', name: '82', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 279, t: 'guest', name: '83', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 280, t: 'guest', name: '83', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 281, t: 'guest', name: '83', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 282, t: 'guest', name: '84', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 283, t: 'guest', name: '84', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 284, t: 'guest', name: '85', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 285, t: 'guest', name: '85', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 286, t: 'guest', name: '86', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 287, t: 'guest', name: '86', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 288, t: 'guest', name: '87', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 289, t: 'guest', name: '87', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 290, t: 'guest', name: '87', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 291, t: 'guest', name: '87', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 292, t: 'guest', name: '88', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 293, t: 'guest', name: '88', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 294, t: 'guest', name: '88', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 295, t: 'guest', name: '88', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 296, t: 'guest', name: '88', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 297, t: 'guest', name: '89', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 298, t: 'guest', name: '89', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 299, t: 'guest', name: '89', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 300, t: 'guest', name: '89', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 301, t: 'guest', name: '90', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 302, t: 'guest', name: '90', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 303, t: 'guest', name: '90', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 304, t: 'guest', name: '91', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 305, t: 'guest', name: '91', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 306, t: 'guest', name: '91', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 307, t: 'guest', name: '91', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 308, t: 'guest', name: '91', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 309, t: 'guest', name: '92', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 310, t: 'guest', name: '92', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 311, t: 'guest', name: '92', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 312, t: 'guest', name: '92', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 313, t: 'guest', name: '93', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 314, t: 'guest', name: '93', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 315, t: 'guest', name: '93', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 316, t: 'guest', name: '94', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 317, t: 'guest', name: '94', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 318, t: 'guest', name: '95', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 319, t: 'guest', name: '95', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 320, t: 'guest', name: '96', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 321, t: 'guest', name: '96', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 322, t: 'guest', name: '96', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 323, t: 'guest', name: '96', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 324, t: 'guest', name: '96', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 325, t: 'guest', name: '97', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 326, t: 'guest', name: '97', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 327, t: 'guest', name: '97', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 328, t: 'guest', name: '98', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 329, t: 'guest', name: '98', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 330, t: 'guest', name: '99', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 331, t: 'guest', name: '99', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 332, t: 'guest', name: '100', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 333, t: 'guest', name: '100', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 334, t: 'guest', name: '100', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 335, t: 'guest', name: '100', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 336, t: 'guest', name: '101', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 337, t: 'guest', name: '101', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 338, t: 'guest', name: '101', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 339, t: 'guest', name: '101', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 340, t: 'guest', name: '101', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 341, t: 'guest', name: '102', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 342, t: 'guest', name: '102', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 343, t: 'guest', name: '102', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 344, t: 'guest', name: '102', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 345, t: 'guest', name: '102', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 346, t: 'guest', name: '103', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 347, t: 'guest', name: '103', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 348, t: 'guest', name: '103', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 349, t: 'guest', name: '103', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 350, t: 'guest', name: '103', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 351, t: 'guest', name: '104', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 352, t: 'guest', name: '104', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 353, t: 'guest', name: '104', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 354, t: 'guest', name: '104', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 355, t: 'guest', name: '104', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 356, t: 'guest', name: '105', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 357, t: 'guest', name: '105', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 358, t: 'guest', name: '106', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 359, t: 'guest', name: '106', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 360, t: 'guest', name: '106', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 361, t: 'guest', name: '107', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 362, t: 'guest', name: '107', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 363, t: 'guest', name: '107', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 364, t: 'guest', name: '107', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 365, t: 'guest', name: '107', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 366, t: 'guest', name: '108', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 367, t: 'guest', name: '108', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 368, t: 'guest', name: '108', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 369, t: 'guest', name: '108', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 370, t: 'guest', name: '108', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 371, t: 'guest', name: '109', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 372, t: 'guest', name: '109', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 373, t: 'guest', name: '110', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 374, t: 'guest', name: '110', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 375, t: 'guest', name: '110', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 376, t: 'guest', name: '110', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 377, t: 'guest', name: '111', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 378, t: 'guest', name: '111', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 379, t: 'guest', name: '112', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 380, t: 'guest', name: '112', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 381, t: 'guest', name: '112', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 382, t: 'guest', name: '112', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 383, t: 'guest', name: '112', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 384, t: 'guest', name: '113', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 385, t: 'guest', name: '113', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 386, t: 'guest', name: '113', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 387, t: 'guest', name: '113', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 388, t: 'guest', name: '114', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 389, t: 'guest', name: '114', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 390, t: 'guest', name: '115', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 391, t: 'guest', name: '115', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 392, t: 'guest', name: '115', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 393, t: 'guest', name: '115', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 394, t: 'guest', name: '116', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 395, t: 'guest', name: '116', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 396, t: 'guest', name: '116', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 397, t: 'guest', name: '116', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 398, t: 'guest', name: '117', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 399, t: 'guest', name: '117', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 400, t: 'guest', name: '117', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 401, t: 'guest', name: '118', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 402, t: 'guest', name: '118', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 403, t: 'guest', name: '118', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 404, t: 'guest', name: '119', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 405, t: 'guest', name: '119', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 406, t: 'guest', name: '120', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 407, t: 'guest', name: '120', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 408, t: 'guest', name: '120', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 409, t: 'guest', name: '120', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 410, t: 'guest', name: '120', sex: 'm', hobby: 'h4'});
        assert('missManners', {id: 411, t: 'guest', name: '121', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 412, t: 'guest', name: '121', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 413, t: 'guest', name: '121', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 414, t: 'guest', name: '121', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 415, t: 'guest', name: '122', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 416, t: 'guest', name: '122', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 417, t: 'guest', name: '122', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 418, t: 'guest', name: '123', sex: 'm', hobby: 'h2'});
        assert('missManners', {id: 419, t: 'guest', name: '123', sex: 'm', hobby: 'h3'});
        assert('missManners', {id: 420, t: 'guest', name: '123', sex: 'm', hobby: 'h1'});
        assert('missManners', {id: 421, t: 'guest', name: '123', sex: 'm', hobby: 'h5'});
        assert('missManners', {id: 422, t: 'guest', name: '124', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 423, t: 'guest', name: '124', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 424, t: 'guest', name: '124', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 425, t: 'guest', name: '125', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 426, t: 'guest', name: '125', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 427, t: 'guest', name: '125', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 428, t: 'guest', name: '125', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 429, t: 'guest', name: '126', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 430, t: 'guest', name: '126', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 431, t: 'guest', name: '126', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 432, t: 'guest', name: '126', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 433, t: 'guest', name: '127', sex: 'f', hobby: 'h5'});
        assert('missManners', {id: 434, t: 'guest', name: '127', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 435, t: 'guest', name: '128', sex: 'f', hobby: 'h2'});
        assert('missManners', {id: 436, t: 'guest', name: '128', sex: 'f', hobby: 'h4'});
        assert('missManners', {id: 437, t: 'guest', name: '128', sex: 'f', hobby: 'h1'});
        assert('missManners', {id: 438, t: 'guest', name: '128', sex: 'f', hobby: 'h3'});
        assert('missManners', {id: 439, t: 'lastSeat', seat: 128});
    }
});

d.runAll();
