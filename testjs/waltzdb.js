var d = require('../libjs/durable');

var factCount = 0;
function createAndPost (host, fact) {
    fact.id = factCount;
    fact.sid = 1;
    host.post('waltzdb', fact);
    factCount += 1;
}

function createAndAssert (host, fact) {
    fact.id = factCount;
    fact.sid = 1;
    host.assert('waltzdb', fact);
    factCount += 1;
}

function getX (val) {
    return Math.floor(val / 100);
}

function getY (val) {
    return val % 100;
}

function getAngle (p1, p2) {
    var deltaX = getX(p2) - getX(p1);
    var deltaY = getY(p2) - getY(p1);
    if (deltaX == 0) {
        if (deltaY > 0) {
            return Math.PI / 2;
        } else if (deltaY < 0) { 
            return -Math.PI / 2;
        }
    } else if (deltaY == 0) {
        if (deltaX > 0) {
            return 0;
        } else if (deltaX < 0) {
            return Math.PI;
        }    
    } else {
        return Math.atan2(deltaY, deltaX);
    }
}

function getInscribableAngle (basePoint, p1, p2) {
    var angle1 = getAngle(basePoint, p1);
    var angle2 = getAngle(basePoint, p2);
    var temp = Math.abs(angle1 - angle2);
    if (temp > Math.PI) {
        return Math.abs(2 * Math.PI - temp);
    }

    return temp;
}

function make3jJunction (j, basePoint, p1, p2, p3) {
    var angle12 = getInscribableAngle(basePoint, p1, p2);
    var angle13 = getInscribableAngle(basePoint, p1, p3);
    var angle23 = getInscribableAngle(basePoint, p2, p3);
    var sum1213 = angle12 + angle13;
    var sum1223 = angle12 + angle23;
    var sum1323 = angle13 + angle23;

    var total = 0;
    if (sum1213 < sum1223) {
        if (sum1213 < sum1323) {
            total = sum1213;
            j.p2 = p1; j.p1 = p2; j.p3 = p3;
        } else {
            total = sum1323;
            j.p2 = p3; j.p1 = p1; j.p3 = p2;
        }
    } else {
        if (sum1223 < sum1323) {
            total = sum1223;
            j.p2 = p2; j.p1 = p1; j.p3 = p3;
        } else {
            total = sum1323;
            j.p2 = p3; j.p1 = p1; j.p3 = p2;
        }
    }

    if (Math.abs(total - Math.PI) < 0.001) {
        j.name = 'tee';
    } else if (total > Math.PI) {
        j.name = 'fork'; 
    } else {
        j.name = 'arrow';
    }
}

with (d.statechart('waltzdb')) {
    with (state('start')) {
        to('duplicate', 
        function (c) {
            c.s.gid = 1000;
            c.s.startTime = new Date();
        });
    }

    with (state('duplicate')) {
        to('duplicate').whenAll(cap(1000),
                                c.line = m.t.eq('line'), 
        function (c) {
            for (var i = 0; i < c.m.length; ++i) {
                var frame = c.m[i];
                console.log('Edge ' + frame.line.p1 + ' ' + frame.line.p2);
                console.log('Edge ' + frame.line.p2 + ' ' + frame.line.p1);
                c.post({id: c.s.gid, t: 'edge', p1: frame.line.p1, p2: frame.line.p2, joined: false});
                c.post({id: c.s.gid + 1, t: 'edge', p1: frame.line.p2, p2: frame.line.p1, joined: false});
                c.s.gid += 2
            }
        });

        to('detectJunctions').whenAll(pri(1), 
        function (c) {
            console.log('detectJunctions');
        });
    }

    with (state('detectJunctions')) {
        to('detectJunctions').whenAll(cap(1000),
                                      c.e1 = and(m.t.eq('edge'), m.joined.eq(false)),
                                      c.e2 = and(m.t.eq('edge'), m.joined.eq(false), m.p1.eq(c.e1.p1), m.p2.neq(c.e1.p2)),
                                      c.e3 = and(m.t.eq('edge'), m.joined.eq(false), m.p1.eq(c.e1.p1), m.p2.neq(c.e1.p2), m.p2.neq(c.e2.p2)),
        function (c) {
            for (var i = 0; i < c.m.length; ++i) {
                var frame = c.m[i];
                var j = {id: c.s.gid, t: 'junction', basePoint: frame.e1.p1, jt: '3j', visited: 'no'};
                make3jJunction(j, frame.e1.p1, frame.e1.p2, frame.e2.p2, frame.e3.p2);
                console.log('Junction ' + j.name + ' ' + j.basePoint + ' ' + j.p1 + ' ' + j.p2 + ' ' + j.p3);
                c.assert(j);
                frame.e1.id = c.s.gid + 1; frame.e1.joined = true; frame.e1.jt = '3j'; c.assert(frame.e1);
                frame.e2.id = c.s.gid + 2; frame.e2.joined = true; frame.e2.jt = '3j'; c.assert(frame.e2);
                frame.e3.id = c.s.gid + 3; frame.e3.joined = true; frame.e3.jt = '3j'; c.assert(frame.e3);
                c.s.gid += 4;
            }
         });

        to('detectJunctions').whenAll(cap(1000),
                                      c.e1 = and(m.t.eq('edge'), m.joined.eq(false)),
                                      c.e2 = and(m.t.eq('edge'), m.joined.eq(false), m.p1.eq(c.e1.p1), m.p2.neq(c.e1.p2)),
                                      none(and(m.t.eq('edge'), m.p1.eq(c.e1.p1), m.p2.neq(c.e1.p2), m.p2.neq(c.e2.p2))),
        function (c) {
            for (var i = 0; i < c.m.length; ++i) {
                var frame = c.m[i];
                var j = {id: c.s.gid, t: 'junction', basePoint: frame.e1.p1, jt: '2j', visited: 'no', name: 'L', p1: frame.e1.p2, p2: frame.e2.p2};
                console.log('Junction L ' + frame.e1.p1 + ' ' + frame.e1.p2 + ' ' + frame.e2.p2);
                c.assert(j)
                frame.e1.id = c.s.gid + 1; frame.e1.joined = true; frame.e1.jt = '2j'; c.assert(frame.e1);
                frame.e2.id = c.s.gid + 2; frame.e2.joined = true; frame.e2.jt = '2j'; c.assert(frame.e2);
                c.s.gid += 3;
            }
        });

        to('findInitialBoundary').whenAll(pri(1), 
        function (c) {
            console.log('findInitialBoundary');
        });                           
    }

    with (state('findInitialBoundary')) {
        to('findSecondBoundary').whenAll(c.j = and(m.t.eq('junction'), m.jt.eq('2j'), m.visited.eq('no')),
                                         c.e1 = and(m.t.eq('edge'), m.p1.eq(c.j.basePoint), m.p2.eq(c.j.p1)),
                                         c.e2 = and(m.t.eq('edge'), m.p1.eq(c.j.basePoint), m.p2.eq(c.j.p2)),
                                         none(and(m.t.eq('junction'), m.jt.eq('2j'), m.visited.eq('no'), m.basePoint.gt(c.j.basePoint))),
        function (c) {
            c.assert({id: c.s.gid, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p1, labelName: 'B', lid: '1'});
            c.assert({id: c.s.gid + 1, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p2, labelName: 'B', lid: '1'});
            c.s.gid += 2;
            console.log('findSecondBoundary');
        });
        
        to('findSecondBoundary').whenAll(c.j = and(m.t.eq('junction'), m.jt.eq('3j'), m.name.eq('arrow'), m.visited.eq('no')),
                                         c.e1 = and(m.t.eq('edge'), m.p1.eq(c.j.basePoint), m.p2.eq(c.j.p1)),
                                         c.e2 = and(m.t.eq('edge'), m.p1.eq(c.j.basePoint), m.p2.eq(c.j.p2)),
                                         c.e3 = and(m.t.eq('edge'), m.p1.eq(c.j.basePoint), m.p2.eq(c.j.p3)),
                                         none(and(m.t.eq('junction'), m.jt.eq('3j'), m.visited.eq('no'), m.basePoint.gt(c.j.basePoint))),
        function (c) {
            c.assert({id: c.s.gid, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p1, labelName: 'B', lid: '14'});
            c.assert({id: c.s.gid + 1, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p2, labelName: '+', lid: '14'});
            c.assert({id: c.s.gid + 2, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p3, labelName: 'B', lid: '14'});
            c.s.gid += 3;
            console.log('findSecondBoundary');
        });
    }

    with (state('findSecondBoundary')) {
        to('labeling').whenAll(c.j = and(m.t.eq('junction'), m.jt.eq('2j'), m.visited.eq('no')),
                               c.e1 = and(m.t.eq('edge'), m.p1.eq(c.j.basePoint), m.p2.eq(c.j.p1)),
                               c.e2 = and(m.t.eq('edge'), m.p1.eq(c.j.basePoint), m.p2.eq(c.j.p2)),
                               none(and(m.t.eq('junction'), m.visited.neq('no'), m.basePoint.lt(c.j.basePoint))),
        function (c) {
            c.retract(c.j); c.j.id = c.s.gid; c.j.visited = 'yes'; c.assert(c.j);            
            c.assert({id: c.s.gid + 1, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p1, labelName: 'B', lid: '1'});
            c.assert({id: c.s.gid + 2, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p2, labelName: 'B', lid: '1'});
            c.s.gid += 3;
            console.log('labeling');

        });

        to('labeling').whenAll(c.j = and(m.t.eq('junction'), m.jt.eq('3j'), m.name.eq('arrow'), m.visited.eq('no')),
                               c.e1 = and(m.t.eq('edge'), m.p1.eq(c.j.basePoint), m.p2.eq(c.j.p1)),
                               c.e2 = and(m.t.eq('edge'), m.p1.eq(c.j.basePoint), m.p2.eq(c.j.p2)),
                               c.e3 = and(m.t.eq('edge'), m.p1.eq(c.j.basePoint), m.p2.eq(c.j.p3)),
                               none(and(m.t.eq('junction'), m.visited.neq('no'), m.base_point.lt(c.j.basePoint))),
        function (c) {
            c.retract(c.j); c.j.id = c.s.gid; c.j.visited = 'yes'; c.assert(c.j);            
            c.assert({id: c.s.gid + 1, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p1, labelName: 'B', lid: '14'});
            c.assert({id: c.s.gid + 2, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p2, labelName: '+', lid: '14'});
            c.assert({id: c.s.gid + 3, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p3, labelName: 'B', lid: '14'});
            c.s.gid += 4;
            console.log('labeling');
        });
    }

    with (state('labeling')) {
        to('visiting3j').whenAll(c.j = and(m.t.eq('junction'), m.jt.eq('3j'), m.visited.eq('no')),
        function (c) {
            c.retract(c.j); c.j.id = c.s.gid; c.j.visited = 'now'; c.assert(c.j);
            c.s.gid += 1;
            console.log('visiting3j');
        });

        to('visiting2j').whenAll(pri(1),
                                 c.j = and(m.t.eq('junction'), m.jt.eq('2j'), m.visited.eq('no')),
        function (c) {
            c.retract(c.j); c.j.id = c.s.gid; c.j.visited = 'now'; c.assert(c.j); 
            c.s.gid += 1;
            console.log('visiting2j');
        });

        to('end').whenAll(pri(2), 
        function (c) {
            console.log('end ' + (new Date() - c.s.startTime));
        });
    }

    with(state('visiting3j')) {
        function visit3j (c) {
            console.log('Edge Label ' + c.j.basePoint + ' ' + c.j.p1 + ' ' + c.l.n1 + ' ' + c.l.lid);
            console.log('Edge Label ' + c.j.basePoint + ' ' + c.j.p2 + ' ' + c.l.n2 + ' ' + c.l.lid);
            console.log('Edge Label ' + c.j.basePoint + ' ' + c.j.p3 + ' ' + c.l.n3 + ' ' + c.l.lid);
            c.assert({id: c.s.gid, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p1, labelName: c.l.n1, lid: c.l.lid});
            c.assert({id: c.s.gid + 1, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p2, labelName: c.l.n2, lid: c.l.lid});
            c.assert({id: c.s.gid + 2, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p3, labelName: c.l.n3, lid: c.l.lid});
            c.s.gid += 3;
        }

        to('visiting3j').whenAll(c.j = and(m.t.eq('junction'), m.visited.eq('now'), m.jt.eq('3j')),
                                 c.l = and(m.t.eq('label'), m.name.eq(c.j.name)), 
                                 c.el1 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p1), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n1)),
                                 c.el2 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p2), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n2)),
                                 c.el3 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p3), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n3)),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.lid.eq(c.l.lid))),
        function (c) {
            visit3j(c);
        });

        to('visiting3j').whenAll(c.j = and(m.t.eq('junction'), m.visited.eq('now'), m.jt.eq('3j')),
                                 c.l = and(m.t.eq('label'), m.name.eq(c.j.name)), 
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p1), m.p2.eq(c.j.basePoint))),
                                 c.el2 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p2), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n2)),
                                 c.el3 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p3), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n3)),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.lid.eq(c.l.lid))),
        function (c) {
            visit3j(c);
        });

        to('visiting3j').whenAll(c.j = and(m.t.eq('junction'), m.visited.eq('now'), m.jt.eq('3j')),
                                 c.l = and(m.t.eq('label'), m.name.eq(c.j.name)), 
                                 c.el1 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p1), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n1)),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p2), m.p2.eq(c.j.basePoint))),
                                 c.el3 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p3), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n3)),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.lid.eq(c.l.lid))),
        function (c) {
            visit3j(c);
        });

        to('visiting3j').whenAll(c.j = and(m.t.eq('junction'), m.visited.eq('now'), m.jt.eq('3j')),
                                 c.l = and(m.t.eq('label'), m.name.eq(c.j.name)), 
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p1), m.p2.eq(c.j.basePoint))),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p2), m.p2.eq(c.j.basePoint))),
                                 c.el3 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p3), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n3)),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.lid.eq(c.l.lid))),
        function (c) {
            visit3j(c);
        });

        to('visiting3j').whenAll(c.j = and(m.t.eq('junction'), m.visited.eq('now'), m.jt.eq('3j')),
                                 c.l = and(m.t.eq('label'), m.name.eq(c.j.name)), 
                                 c.el1 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p1), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n1)),
                                 c.el2 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p2), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n2)),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p3), m.p2.eq(c.j.basePoint))),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.lid.eq(c.l.lid))),
        function (c) {
            visit3j(c);
        });

        to('visiting3j').whenAll(c.j = and(m.t.eq('junction'), m.visited.eq('now'), m.jt.eq('3j')),
                                 c.l = and(m.t.eq('label'), m.name.eq(c.j.name)), 
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p1), m.p2.eq(c.j.basePoint))),
                                 c.el2 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p2), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n2)),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p3), m.p2.eq(c.j.basePoint))),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.lid.eq(c.l.lid))),
        function (c) {
            visit3j(c);
        });

        to('visiting3j').whenAll(c.j = and(m.t.eq('junction'), m.visited.eq('now'), m.jt.eq('3j')),
                                 c.l = and(m.t.eq('label'), m.name.eq(c.j.name)), 
                                 c.el1 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p1), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n1)),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p2), m.p2.eq(c.j.basePoint))),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p3), m.p2.eq(c.j.basePoint))),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.lid.eq(c.l.lid))),
        function (c) {
            visit3j(c);
        });

        to('visiting3j').whenAll(c.j = and(m.t.eq('junction'), m.visited.eq('now'), m.jt.eq('3j')),
                                 c.l = and(m.t.eq('label'), m.name.eq(c.j.name)), 
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p1), m.p2.eq(c.j.basePoint))),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p2), m.p2.eq(c.j.basePoint))),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p3), m.p2.eq(c.j.basePoint))),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.lid.eq(c.l.lid))),
        function (c) {
            visit3j(c);
        });

        to('marking').whenAll(pri(1),
                              and(m.t.eq('junction'), m.visited.eq('now'), m.jt.eq('3j')), 
        function (c) {
            console.log('marking');
        });
    }

    with(state('visiting2j')) {
        function visit2j (c) {
            console.log('Edge Label ' + c.j.basePoint + ' ' + c.j.p1 + ' ' + c.l.n1 + ' ' + c.l.lid);
            console.log('Edge Label ' + c.j.basePoint + ' ' + c.j.p2 + ' ' + c.l.n2 + ' ' + c.l.lid);
            c.assert({id: c.s.gid, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p1, labelName: c.l.n1, lid: c.l.lid});
            c.assert({id: c.s.gid + 1, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p2, labelName: c.l.n2, lid: c.l.lid});
            c.s.gid += 2;
        }

        to('visiting2j').whenAll(c.j = and(m.t.eq('junction'), m.visited.eq('now'), m.jt.eq('2j')),
                                 c.l = and(m.t.eq('label'), m.name.eq(c.j.name)), 
                                 c.el1 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p1), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n1)),
                                 c.el2 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p2), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n2)),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.lid.eq(c.l.lid))),
        function (c) {
            visit2j(c);
        });

        to('visiting2j').whenAll(c.j = and(m.t.eq('junction'), m.visited.eq('now'), m.jt.eq('2j')),
                                 c.l = and(m.t.eq('label'), m.name.eq(c.j.name)), 
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p1), m.p2.eq(c.j.basePoint))),
                                 c.el2 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p2), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n2)),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.lid.eq(c.l.lid))),
        function (c) {
            visit2j(c);
        });

        to('visiting2j').whenAll(c.j = and(m.t.eq('junction'), m.visited.eq('now'), m.jt.eq('2j')),
                                 c.l = and(m.t.eq('label'), m.name.eq(c.j.name)), 
                                 c.el1 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p1), m.p2.eq(c.j.basePoint), m.labelName.eq(c.l.n1)),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p2), m.p2.eq(c.j.basePoint))),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.lid.eq(c.l.lid))),
        function (c) {
            visit2j(c);
        });

        to('visiting2j').whenAll(c.j = and(m.t.eq('junction'), m.visited.eq('now'), m.jt.eq('2j')),
                                 c.l = and(m.t.eq('label'), m.name.eq(c.j.name)), 
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p1), m.p2.eq(c.j.basePoint))),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.p2), m.p2.eq(c.j.basePoint))),
                                 none(and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.lid.eq(c.l.lid))),
        function (c) {
            visit2j(c);
        });

        to('marking').whenAll(pri(1),
                              and(m.t.eq('junction'), m.visited.eq('now'), m.jt.eq('2j')), 
        function (c) {
            console.log('marking');
        });
    }

    with (state('marking')) {
        to('marking').whenAll(c.j = and(m.t.eq('junction'), m.visited.eq('now')),
                              c.e = and(m.t.eq('edge'), m.p2.eq(c.j.basePoint)),
                              c.junction = and(m.t.eq('junction'), m.basePoint.eq(c.e.p1), m.visited.eq('yes')),
        function (c) {
            c.retract(c.junction); c.junction.id = c.s.gid; c.junction.visited = 'check'; c.assert(c.junction); 
            c.s.gid += 1;
        });

        to('marking').whenAll(pri(1),
                              c.j = and(m.t.eq('junction'), m.visited.eq('now')),
        function (c) {
            c.retract(c.j); c.j.id = c.s.gid; c.j.visited = 'yes'; c.assert(c.j); 
            c.s.gid += 1;
        });

        to('checking').whenAll(pri(2),
        function (c) {
            console.log('checking');
        });
    }

    with (state('checking')) {
        to('removeLabel').whenAll(c.junction = and(m.t.eq('junction'), m.visited.eq('check')),
                                  c.el1 = and(m.t.eq('edge_label'), m.p1.eq(c.junction.basePoint)),
                                  c.j = and(m.t.eq('junction'), m.basePoint.eq(c.el1.p2), m.visited.eq('yes')),
                                  none(and(m.t.eq('edge_label'), m.p1.eq(c.el1.p2), m.p2.eq(c.junction.basePoint), m.labelName.eq(c.el1.labelName))),
        function (c) {
            console.log('removeLabel');
            c.assert({id: c.s.gid, t: 'illegal', basePoint: c.junction.basePoint, lid: c.el1.lid});
            c.s.gid += 1;
        });

        to('checking').whenAll(pri(1),
                              c.j = and(m.t.eq('junction'), m.visited.eq('check')),
        function (c) {
            c.retract(c.j); c.j.id = c.s.gid; c.j.visited = 'yes'; c.assert(c.j); 
            c.s.gid += 1;
        });

        to('labeling').whenAll(pri(2),
        function (c) {
            console.log('labeling');
        });
    }

    with (state('removeLabel')) {
        to('checking').whenAll(c.i = m.t.eq('illegal'),
                               c.j = and(m.t.eq('junction'), m.jt.eq('3j'), m.basePoint.eq(c.i.basePoint)),
                               c.el1 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.p2.eq(c.j.p1), m.lid.eq(c.i.lid)),
                               c.el2 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.p2.eq(c.j.p2), m.lid.eq(c.i.lid)),
                               c.el3 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.p2.eq(c.j.p3), m.lid.eq(c.i.lid)),
        function (c) {
            console.log('checking');
            c.retract(c.i);
            c.retract(c.el1);
            c.retract(c.el2);
            c.retract(c.el3);
        });

        to('checking').whenAll(c.i = m.t.eq('illegal'),
                               c.j = and(m.t.eq('junction'), m.jt.eq('2j'), m.basePoint.eq(c.i.basePoint)),
                               c.el1 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.p2.eq(c.j.p1), m.lid.eq(c.i.lid)),
                               c.el2 = and(m.t.eq('edgeLabel'), m.p1.eq(c.j.basePoint), m.p2.eq(c.j.p2), m.lid.eq(c.i.lid)),
        function (c) {
            console.log('checking');
            c.retract(c.i);
            c.retract(c.el1);
            c.retract(c.el2);
        });
    }
    
    state('end');

    whenStart(function (host) {
        createAndAssert(host, {t:'label' ,jt:'2j' ,name:'L' ,lid:'1' ,n1:'B' ,n2:'B'});
        createAndAssert(host, {t:'label' ,jt:'2j' ,name:'L' ,lid:'2' ,n1:'+' ,n2:'B'});
        createAndAssert(host, {t:'label' ,jt:'2j' ,name:'L' ,lid:'3' ,n1:'B' ,n2:'+'});
        createAndAssert(host, {t:'label' ,jt:'2j' ,name:'L' ,lid:'4' ,n1:'-' ,n2:'B'});
        createAndAssert(host, {t:'label' ,jt:'2j' ,name:'L' ,lid:'5' ,n1:'B' ,n2:'-'});
        createAndAssert(host, {t:'label' ,jt:'3j' ,name:'fork' ,lid:'6' ,n1:'+' ,n2:'+' ,n3:'+'});
        createAndAssert(host, {t:'label' ,jt:'3j' ,name:'fork' ,lid:'7' ,n1:'-' ,n2:'-' ,n3:'-'});
        createAndAssert(host, {t:'label' ,jt:'3j' ,name:'fork' ,lid:'8' ,n1:'B' ,n2:'-' ,n3:'B'});
        createAndAssert(host, {t:'label' ,jt:'3j' ,name:'fork' ,lid:'9' ,n1:'-' ,n2:'B' ,n3:'B'});
        createAndAssert(host, {t:'label' ,jt:'3j' ,name:'fork' ,lid:'10' ,n1:'B' ,n2:'B' ,n3:'-'});
        createAndAssert(host, {t:'label' ,jt:'3j' ,name:'tee' ,lid:'11' ,n1:'B' ,n2:'+' ,n3:'B'});
        createAndAssert(host, {t:'label' ,jt:'3j' ,name:'tee' ,lid:'12' ,n1:'B' ,n2:'-' ,n3:'B'});
        createAndAssert(host, {t:'label' ,jt:'3j' ,name:'tee' ,lid:'13' ,n1:'B' ,n2:'B' ,n3:'B'});
        createAndAssert(host, {t:'label' ,jt:'3j' ,name:'arrow' ,lid:'14' ,n1:'B' ,n2:'+' ,n3:'B'});
        createAndAssert(host, {t:'label' ,jt:'3j' ,name:'arrow' ,lid:'15' ,n1:'-' ,n2:'+' ,n3:'-'});
        createAndAssert(host, {t:'label' ,jt:'3j' ,name:'arrow' ,lid:'16' ,n1:'+' ,n2:'-' ,n3:'+'});
        createAndPost(host, {t:'line' ,p1:50003 ,p2:60003});
        createAndPost(host, {t:'line' ,p1:30005 ,p2:30006});
        createAndPost(host, {t:'line' ,p1:80005 ,p2:80006});
        createAndPost(host, {t:'line' ,p1:50008 ,p2:60008});
        createAndPost(host, {t:'line' ,p1:0 ,p2:20000});
        createAndPost(host, {t:'line' ,p1:20000 ,p2:30000});
        createAndPost(host, {t:'line' ,p1:30000 ,p2:40000});
        createAndPost(host, {t:'line' ,p1:0 ,p2:2});
        createAndPost(host, {t:'line' ,p1:2 ,p2:3});
        createAndPost(host, {t:'line' ,p1:3 ,p2:4});
        createAndPost(host, {t:'line' ,p1:4 ,p2:40004});
        createAndPost(host, {t:'line' ,p1:40004 ,p2:40000});
        createAndPost(host, {t:'line' ,p1:40000 ,p2:50001});
        createAndPost(host, {t:'line' ,p1:50001 ,p2:50002});
        createAndPost(host, {t:'line' ,p1:50002 ,p2:50003});
        createAndPost(host, {t:'line' ,p1:50003 ,p2:50005});
        createAndPost(host, {t:'line' ,p1:50005 ,p2:40004});
        createAndPost(host, {t:'line' ,p1:50005 ,p2:30005});
        createAndPost(host, {t:'line' ,p1:30005 ,p2:20005});
        createAndPost(host, {t:'line' ,p1:20005 ,p2:10005});
        createAndPost(host, {t:'line' ,p1:10005 ,p2:4});
        createAndPost(host, {t:'line' ,p1:60000 ,p2:80000});
        createAndPost(host, {t:'line' ,p1:80000 ,p2:90000});
        createAndPost(host, {t:'line' ,p1:90000 ,p2:100000});
        createAndPost(host, {t:'line' ,p1:60000 ,p2:60002});
        createAndPost(host, {t:'line' ,p1:60002 ,p2:60003});
        createAndPost(host, {t:'line' ,p1:60003 ,p2:60004});
        createAndPost(host, {t:'line' ,p1:60004 ,p2:100004});
        createAndPost(host, {t:'line' ,p1:100004 ,p2:100000});
        createAndPost(host, {t:'line' ,p1:100000 ,p2:110001});
        createAndPost(host, {t:'line' ,p1:110001 ,p2:110002});
        createAndPost(host, {t:'line' ,p1:110002 ,p2:110003});
        createAndPost(host, {t:'line' ,p1:110003 ,p2:110005});
        createAndPost(host, {t:'line' ,p1:110005 ,p2:100004});
        createAndPost(host, {t:'line' ,p1:110005 ,p2:90005});
        createAndPost(host, {t:'line' ,p1:90005 ,p2:80005});
        createAndPost(host, {t:'line' ,p1:80005 ,p2:70005});
        createAndPost(host, {t:'line' ,p1:70005 ,p2:60004});
        createAndPost(host, {t:'line' ,p1:6 ,p2:20006});
        createAndPost(host, {t:'line' ,p1:20006 ,p2:30006});
        createAndPost(host, {t:'line' ,p1:30006 ,p2:40006});
        createAndPost(host, {t:'line' ,p1:6 ,p2:8});
        createAndPost(host, {t:'line' ,p1:8 ,p2:9});
        createAndPost(host, {t:'line' ,p1:9 ,p2:10});
        createAndPost(host, {t:'line' ,p1:10 ,p2:40010});
        createAndPost(host, {t:'line' ,p1:40010 ,p2:40006});
        createAndPost(host, {t:'line' ,p1:40006 ,p2:50007});
        createAndPost(host, {t:'line' ,p1:50007 ,p2:50008});
        createAndPost(host, {t:'line' ,p1:50008 ,p2:50009});
        createAndPost(host, {t:'line' ,p1:50009 ,p2:50011});
        createAndPost(host, {t:'line' ,p1:50011 ,p2:40010});
        createAndPost(host, {t:'line' ,p1:50011 ,p2:30011});
        createAndPost(host, {t:'line' ,p1:30011 ,p2:20011});
        createAndPost(host, {t:'line' ,p1:20011 ,p2:10011});
        createAndPost(host, {t:'line' ,p1:10011 ,p2:10});
        createAndPost(host, {t:'line' ,p1:60006 ,p2:80006});
        createAndPost(host, {t:'line' ,p1:80006 ,p2:90006});
        createAndPost(host, {t:'line' ,p1:90006 ,p2:100006});
        createAndPost(host, {t:'line' ,p1:60006 ,p2:60008});
        createAndPost(host, {t:'line' ,p1:60008 ,p2:60009});
        createAndPost(host, {t:'line' ,p1:60009 ,p2:60010});
        createAndPost(host, {t:'line' ,p1:60010 ,p2:100010});
        createAndPost(host, {t:'line' ,p1:100010 ,p2:100006});
        createAndPost(host, {t:'line' ,p1:100006 ,p2:110007});
        createAndPost(host, {t:'line' ,p1:110007 ,p2:110008});
        createAndPost(host, {t:'line' ,p1:110008 ,p2:110009});
        createAndPost(host, {t:'line' ,p1:110009 ,p2:110011});
        createAndPost(host, {t:'line' ,p1:110011 ,p2:100010});
        createAndPost(host, {t:'line' ,p1:110011 ,p2:90011});
        createAndPost(host, {t:'line' ,p1:90011 ,p2:80011});
        createAndPost(host, {t:'line' ,p1:80011 ,p2:70011});
        createAndPost(host, {t:'line' ,p1:70011 ,p2:60010});
        createAndPost(host, {t:'line' ,p1:170003 ,p2:180003});
        createAndPost(host, {t:'line' ,p1:150005 ,p2:150006});
        createAndPost(host, {t:'line' ,p1:200005 ,p2:200006});
        createAndPost(host, {t:'line' ,p1:170008 ,p2:180008});
        createAndPost(host, {t:'line' ,p1:120000 ,p2:140000});
        createAndPost(host, {t:'line' ,p1:140000 ,p2:150000});
        createAndPost(host, {t:'line' ,p1:150000 ,p2:160000});
        createAndPost(host, {t:'line' ,p1:120000 ,p2:120002});
        createAndPost(host, {t:'line' ,p1:120002 ,p2:120003});
        createAndPost(host, {t:'line' ,p1:120003 ,p2:120004});
        createAndPost(host, {t:'line' ,p1:120004 ,p2:160004});
        createAndPost(host, {t:'line' ,p1:160004 ,p2:160000});
        createAndPost(host, {t:'line' ,p1:160000 ,p2:170001});
        createAndPost(host, {t:'line' ,p1:170001 ,p2:170002});
        createAndPost(host, {t:'line' ,p1:170002 ,p2:170003});
        createAndPost(host, {t:'line' ,p1:170003 ,p2:170005});
        createAndPost(host, {t:'line' ,p1:170005 ,p2:160004});
        createAndPost(host, {t:'line' ,p1:170005 ,p2:150005});
        createAndPost(host, {t:'line' ,p1:150005 ,p2:140005});
        createAndPost(host, {t:'line' ,p1:140005 ,p2:130005});
        createAndPost(host, {t:'line' ,p1:130005 ,p2:120004});
        createAndPost(host, {t:'line' ,p1:180000 ,p2:200000});
        createAndPost(host, {t:'line' ,p1:200000 ,p2:210000});
        createAndPost(host, {t:'line' ,p1:210000 ,p2:220000});
        createAndPost(host, {t:'line' ,p1:180000 ,p2:180002});
        createAndPost(host, {t:'line' ,p1:180002 ,p2:180003});
        createAndPost(host, {t:'line' ,p1:180003 ,p2:180004});
        createAndPost(host, {t:'line' ,p1:180004 ,p2:220004});
        createAndPost(host, {t:'line' ,p1:220004 ,p2:220000});
        createAndPost(host, {t:'line' ,p1:220000 ,p2:230001});
        createAndPost(host, {t:'line' ,p1:230001 ,p2:230002});
        createAndPost(host, {t:'line' ,p1:230002 ,p2:230003});
        createAndPost(host, {t:'line' ,p1:230003 ,p2:230005});
        createAndPost(host, {t:'line' ,p1:230005 ,p2:220004});
        createAndPost(host, {t:'line' ,p1:230005 ,p2:210005});
        createAndPost(host, {t:'line' ,p1:210005 ,p2:200005});
        createAndPost(host, {t:'line' ,p1:200005 ,p2:190005});
        createAndPost(host, {t:'line' ,p1:190005 ,p2:180004});
        createAndPost(host, {t:'line' ,p1:120006 ,p2:140006});
        createAndPost(host, {t:'line' ,p1:140006 ,p2:150006});
        createAndPost(host, {t:'line' ,p1:150006 ,p2:160006});
        createAndPost(host, {t:'line' ,p1:120006 ,p2:120008});
        createAndPost(host, {t:'line' ,p1:120008 ,p2:120009});
        createAndPost(host, {t:'line' ,p1:120009 ,p2:120010});
        createAndPost(host, {t:'line' ,p1:120010 ,p2:160010});
        createAndPost(host, {t:'line' ,p1:160010 ,p2:160006});
        createAndPost(host, {t:'line' ,p1:160006 ,p2:170007});
        createAndPost(host, {t:'line' ,p1:170007 ,p2:170008});
        createAndPost(host, {t:'line' ,p1:170008 ,p2:170009});
        createAndPost(host, {t:'line' ,p1:170009 ,p2:170011});
        createAndPost(host, {t:'line' ,p1:170011 ,p2:160010});
        createAndPost(host, {t:'line' ,p1:170011 ,p2:150011});
        createAndPost(host, {t:'line' ,p1:150011 ,p2:140011});
        createAndPost(host, {t:'line' ,p1:140011 ,p2:130011});
        createAndPost(host, {t:'line' ,p1:130011 ,p2:120010});
        createAndPost(host, {t:'line' ,p1:180006 ,p2:200006});
        createAndPost(host, {t:'line' ,p1:200006 ,p2:210006});
        createAndPost(host, {t:'line' ,p1:210006 ,p2:220006});
        createAndPost(host, {t:'line' ,p1:180006 ,p2:180008});
        createAndPost(host, {t:'line' ,p1:180008 ,p2:180009});
        createAndPost(host, {t:'line' ,p1:180009 ,p2:180010});
        createAndPost(host, {t:'line' ,p1:180010 ,p2:220010});
        createAndPost(host, {t:'line' ,p1:220010 ,p2:220006});
        createAndPost(host, {t:'line' ,p1:220006 ,p2:230007});
        createAndPost(host, {t:'line' ,p1:230007 ,p2:230008});
        createAndPost(host, {t:'line' ,p1:230008 ,p2:230009});
        createAndPost(host, {t:'line' ,p1:230009 ,p2:230011});
        createAndPost(host, {t:'line' ,p1:230011 ,p2:220010});
        createAndPost(host, {t:'line' ,p1:230011 ,p2:210011});
        createAndPost(host, {t:'line' ,p1:210011 ,p2:200011});
        createAndPost(host, {t:'line' ,p1:200011 ,p2:190011});
        createAndPost(host, {t:'line' ,p1:190011 ,p2:180010});
        createAndPost(host, {t:'line' ,p1:110003 ,p2:120003});
        createAndPost(host, {t:'line' ,p1:90005 ,p2:90006});
        createAndPost(host, {t:'line' ,p1:140005 ,p2:140006});
        createAndPost(host, {t:'line' ,p1:110008 ,p2:120008});
        createAndPost(host, {t:'line' ,p1:290003 ,p2:300003});
        createAndPost(host, {t:'line' ,p1:270005 ,p2:270006});
        createAndPost(host, {t:'line' ,p1:320005 ,p2:320006});
        createAndPost(host, {t:'line' ,p1:290008 ,p2:300008});
        createAndPost(host, {t:'line' ,p1:240000 ,p2:260000});
        createAndPost(host, {t:'line' ,p1:260000 ,p2:270000});
        createAndPost(host, {t:'line' ,p1:270000 ,p2:280000});
        createAndPost(host, {t:'line' ,p1:240000 ,p2:240002});
        createAndPost(host, {t:'line' ,p1:240002 ,p2:240003});
        createAndPost(host, {t:'line' ,p1:240003 ,p2:240004});
        createAndPost(host, {t:'line' ,p1:240004 ,p2:280004});
        createAndPost(host, {t:'line' ,p1:280004 ,p2:280000});
        createAndPost(host, {t:'line' ,p1:280000 ,p2:290001});
        createAndPost(host, {t:'line' ,p1:290001 ,p2:290002});
        createAndPost(host, {t:'line' ,p1:290002 ,p2:290003});
        createAndPost(host, {t:'line' ,p1:290003 ,p2:290005});
        createAndPost(host, {t:'line' ,p1:290005 ,p2:280004});
        createAndPost(host, {t:'line' ,p1:290005 ,p2:270005});
        createAndPost(host, {t:'line' ,p1:270005 ,p2:260005});
        createAndPost(host, {t:'line' ,p1:260005 ,p2:250005});
        createAndPost(host, {t:'line' ,p1:250005 ,p2:240004});
        createAndPost(host, {t:'line' ,p1:300000 ,p2:320000});
        createAndPost(host, {t:'line' ,p1:320000 ,p2:330000});
        createAndPost(host, {t:'line' ,p1:330000 ,p2:340000});
        createAndPost(host, {t:'line' ,p1:300000 ,p2:300002});
        createAndPost(host, {t:'line' ,p1:300002 ,p2:300003});
        createAndPost(host, {t:'line' ,p1:300003 ,p2:300004});
        createAndPost(host, {t:'line' ,p1:300004 ,p2:340004});
        createAndPost(host, {t:'line' ,p1:340004 ,p2:340000});
        createAndPost(host, {t:'line' ,p1:340000 ,p2:350001});
        createAndPost(host, {t:'line' ,p1:350001 ,p2:350002});
        createAndPost(host, {t:'line' ,p1:350002 ,p2:350003});
        createAndPost(host, {t:'line' ,p1:350003 ,p2:350005});
        createAndPost(host, {t:'line' ,p1:350005 ,p2:340004});
        createAndPost(host, {t:'line' ,p1:350005 ,p2:330005});
        createAndPost(host, {t:'line' ,p1:330005 ,p2:320005});
        createAndPost(host, {t:'line' ,p1:320005 ,p2:310005});
        createAndPost(host, {t:'line' ,p1:310005 ,p2:300004});
        createAndPost(host, {t:'line' ,p1:240006 ,p2:260006});
        createAndPost(host, {t:'line' ,p1:260006 ,p2:270006});
        createAndPost(host, {t:'line' ,p1:270006 ,p2:280006});
        createAndPost(host, {t:'line' ,p1:240006 ,p2:240008});
        createAndPost(host, {t:'line' ,p1:240008 ,p2:240009});
        createAndPost(host, {t:'line' ,p1:240009 ,p2:240010});
        createAndPost(host, {t:'line' ,p1:240010 ,p2:280010});
        createAndPost(host, {t:'line' ,p1:280010 ,p2:280006});
        createAndPost(host, {t:'line' ,p1:280006 ,p2:290007});
        createAndPost(host, {t:'line' ,p1:290007 ,p2:290008});
        createAndPost(host, {t:'line' ,p1:290008 ,p2:290009});
        createAndPost(host, {t:'line' ,p1:290009 ,p2:290011});
        createAndPost(host, {t:'line' ,p1:290011 ,p2:280010});
        createAndPost(host, {t:'line' ,p1:290011 ,p2:270011});
        createAndPost(host, {t:'line' ,p1:270011 ,p2:260011});
        createAndPost(host, {t:'line' ,p1:260011 ,p2:250011});
        createAndPost(host, {t:'line' ,p1:250011 ,p2:240010});
        createAndPost(host, {t:'line' ,p1:300006 ,p2:320006});
        createAndPost(host, {t:'line' ,p1:320006 ,p2:330006});
        createAndPost(host, {t:'line' ,p1:330006 ,p2:340006});
        createAndPost(host, {t:'line' ,p1:300006 ,p2:300008});
        createAndPost(host, {t:'line' ,p1:300008 ,p2:300009});
        createAndPost(host, {t:'line' ,p1:300009 ,p2:300010});
        createAndPost(host, {t:'line' ,p1:300010 ,p2:340010});
        createAndPost(host, {t:'line' ,p1:340010 ,p2:340006});
        createAndPost(host, {t:'line' ,p1:340006 ,p2:350007});
        createAndPost(host, {t:'line' ,p1:350007 ,p2:350008});
        createAndPost(host, {t:'line' ,p1:350008 ,p2:350009});
        createAndPost(host, {t:'line' ,p1:350009 ,p2:350011});
        createAndPost(host, {t:'line' ,p1:350011 ,p2:340010});
        createAndPost(host, {t:'line' ,p1:350011 ,p2:330011});
        createAndPost(host, {t:'line' ,p1:330011 ,p2:320011});
        createAndPost(host, {t:'line' ,p1:320011 ,p2:310011});
        createAndPost(host, {t:'line' ,p1:310011 ,p2:300010});
        createAndPost(host, {t:'line' ,p1:230003 ,p2:240003});
        createAndPost(host, {t:'line' ,p1:210005 ,p2:210006});
        createAndPost(host, {t:'line' ,p1:260005 ,p2:260006});
        createAndPost(host, {t:'line' ,p1:230008 ,p2:240008});
        createAndPost(host, {t:'line' ,p1:410003 ,p2:420003});
        createAndPost(host, {t:'line' ,p1:390005 ,p2:390006});
        createAndPost(host, {t:'line' ,p1:440005 ,p2:440006});
        createAndPost(host, {t:'line' ,p1:410008 ,p2:420008});
        createAndPost(host, {t:'line' ,p1:360000 ,p2:380000});
        createAndPost(host, {t:'line' ,p1:380000 ,p2:390000});
        createAndPost(host, {t:'line' ,p1:390000 ,p2:400000});
        createAndPost(host, {t:'line' ,p1:360000 ,p2:360002});
        createAndPost(host, {t:'line' ,p1:360002 ,p2:360003});
        createAndPost(host, {t:'line' ,p1:360003 ,p2:360004});
        createAndPost(host, {t:'line' ,p1:360004 ,p2:400004});
        createAndPost(host, {t:'line' ,p1:400004 ,p2:400000});
        createAndPost(host, {t:'line' ,p1:400000 ,p2:410001});
        createAndPost(host, {t:'line' ,p1:410001 ,p2:410002});
        createAndPost(host, {t:'line' ,p1:410002 ,p2:410003});
        createAndPost(host, {t:'line' ,p1:410003 ,p2:410005});
        createAndPost(host, {t:'line' ,p1:410005 ,p2:400004});
        createAndPost(host, {t:'line' ,p1:410005 ,p2:390005});
        createAndPost(host, {t:'line' ,p1:390005 ,p2:380005});
        createAndPost(host, {t:'line' ,p1:380005 ,p2:370005});
        createAndPost(host, {t:'line' ,p1:370005 ,p2:360004});
        createAndPost(host, {t:'line' ,p1:420000 ,p2:440000});
        createAndPost(host, {t:'line' ,p1:440000 ,p2:450000});
        createAndPost(host, {t:'line' ,p1:450000 ,p2:460000});
        createAndPost(host, {t:'line' ,p1:420000 ,p2:420002});
        createAndPost(host, {t:'line' ,p1:420002 ,p2:420003});
        createAndPost(host, {t:'line' ,p1:420003 ,p2:420004});
        createAndPost(host, {t:'line' ,p1:420004 ,p2:460004});
        createAndPost(host, {t:'line' ,p1:460004 ,p2:460000});
        createAndPost(host, {t:'line' ,p1:460000 ,p2:470001});
        createAndPost(host, {t:'line' ,p1:470001 ,p2:470002});
        createAndPost(host, {t:'line' ,p1:470002 ,p2:470003});
        createAndPost(host, {t:'line' ,p1:470003 ,p2:470005});
        createAndPost(host, {t:'line' ,p1:470005 ,p2:460004});
        createAndPost(host, {t:'line' ,p1:470005 ,p2:450005});
        createAndPost(host, {t:'line' ,p1:450005 ,p2:440005});
        createAndPost(host, {t:'line' ,p1:440005 ,p2:430005});
        createAndPost(host, {t:'line' ,p1:430005 ,p2:420004});
        createAndPost(host, {t:'line' ,p1:360006 ,p2:380006});
        createAndPost(host, {t:'line' ,p1:380006 ,p2:390006});
        createAndPost(host, {t:'line' ,p1:390006 ,p2:400006});
        createAndPost(host, {t:'line' ,p1:360006 ,p2:360008});
        createAndPost(host, {t:'line' ,p1:360008 ,p2:360009});
        createAndPost(host, {t:'line' ,p1:360009 ,p2:360010});
        createAndPost(host, {t:'line' ,p1:360010 ,p2:400010});
        createAndPost(host, {t:'line' ,p1:400010 ,p2:400006});
        createAndPost(host, {t:'line' ,p1:400006 ,p2:410007});
        createAndPost(host, {t:'line' ,p1:410007 ,p2:410008});
        createAndPost(host, {t:'line' ,p1:410008 ,p2:410009});
        createAndPost(host, {t:'line' ,p1:410009 ,p2:410011});
        createAndPost(host, {t:'line' ,p1:410011 ,p2:400010});
        createAndPost(host, {t:'line' ,p1:410011 ,p2:390011});
        createAndPost(host, {t:'line' ,p1:390011 ,p2:380011});
        createAndPost(host, {t:'line' ,p1:380011 ,p2:370011});
        createAndPost(host, {t:'line' ,p1:370011 ,p2:360010});
        createAndPost(host, {t:'line' ,p1:420006 ,p2:440006});
        createAndPost(host, {t:'line' ,p1:440006 ,p2:450006});
        createAndPost(host, {t:'line' ,p1:450006 ,p2:460006});
        createAndPost(host, {t:'line' ,p1:420006 ,p2:420008});
        createAndPost(host, {t:'line' ,p1:420008 ,p2:420009});
        createAndPost(host, {t:'line' ,p1:420009 ,p2:420010});
        createAndPost(host, {t:'line' ,p1:420010 ,p2:460010});
        createAndPost(host, {t:'line' ,p1:460010 ,p2:460006});
        createAndPost(host, {t:'line' ,p1:460006 ,p2:470007});
        createAndPost(host, {t:'line' ,p1:470007 ,p2:470008});
        createAndPost(host, {t:'line' ,p1:470008 ,p2:470009});
        createAndPost(host, {t:'line' ,p1:470009 ,p2:470011});
        createAndPost(host, {t:'line' ,p1:470011 ,p2:460010});
        createAndPost(host, {t:'line' ,p1:470011 ,p2:450011});
        createAndPost(host, {t:'line' ,p1:450011 ,p2:440011});
        createAndPost(host, {t:'line' ,p1:440011 ,p2:430011});
        createAndPost(host, {t:'line' ,p1:430011 ,p2:420010});
        createAndPost(host, {t:'line' ,p1:350003 ,p2:360003});
        createAndPost(host, {t:'line' ,p1:330005 ,p2:330006});
        createAndPost(host, {t:'line' ,p1:380005 ,p2:380006});
        createAndPost(host, {t:'line' ,p1:350008 ,p2:360008});
        createAndPost(host, {t:'line' ,p1:530003 ,p2:540003});
        createAndPost(host, {t:'line' ,p1:510005 ,p2:510006});
        createAndPost(host, {t:'line' ,p1:560005 ,p2:560006});
        createAndPost(host, {t:'line' ,p1:530008 ,p2:540008});
        createAndPost(host, {t:'line' ,p1:480000 ,p2:500000});
        createAndPost(host, {t:'line' ,p1:500000 ,p2:510000});
        createAndPost(host, {t:'line' ,p1:510000 ,p2:520000});
        createAndPost(host, {t:'line' ,p1:480000 ,p2:480002});
        createAndPost(host, {t:'line' ,p1:480002 ,p2:480003});
        createAndPost(host, {t:'line' ,p1:480003 ,p2:480004});
        createAndPost(host, {t:'line' ,p1:480004 ,p2:520004});
        createAndPost(host, {t:'line' ,p1:520004 ,p2:520000});
        createAndPost(host, {t:'line' ,p1:520000 ,p2:530001});
        createAndPost(host, {t:'line' ,p1:530001 ,p2:530002});
        createAndPost(host, {t:'line' ,p1:530002 ,p2:530003});
        createAndPost(host, {t:'line' ,p1:530003 ,p2:530005});
        createAndPost(host, {t:'line' ,p1:530005 ,p2:520004});
        createAndPost(host, {t:'line' ,p1:530005 ,p2:510005});
        createAndPost(host, {t:'line' ,p1:510005 ,p2:500005});
        createAndPost(host, {t:'line' ,p1:500005 ,p2:490005});
        createAndPost(host, {t:'line' ,p1:490005 ,p2:480004});
        createAndPost(host, {t:'line' ,p1:540000 ,p2:560000});
        createAndPost(host, {t:'line' ,p1:560000 ,p2:570000});
        createAndPost(host, {t:'line' ,p1:570000 ,p2:580000});
        createAndPost(host, {t:'line' ,p1:540000 ,p2:540002});
        createAndPost(host, {t:'line' ,p1:540002 ,p2:540003});
        createAndPost(host, {t:'line' ,p1:540003 ,p2:540004});
        createAndPost(host, {t:'line' ,p1:540004 ,p2:580004});
        createAndPost(host, {t:'line' ,p1:580004 ,p2:580000});
        createAndPost(host, {t:'line' ,p1:580000 ,p2:590001});
        createAndPost(host, {t:'line' ,p1:590001 ,p2:590002});
        createAndPost(host, {t:'line' ,p1:590002 ,p2:590003});
        createAndPost(host, {t:'line' ,p1:590003 ,p2:590005});
        createAndPost(host, {t:'line' ,p1:590005 ,p2:580004});
        createAndPost(host, {t:'line' ,p1:590005 ,p2:570005});
        createAndPost(host, {t:'line' ,p1:570005 ,p2:560005});
        createAndPost(host, {t:'line' ,p1:560005 ,p2:550005});
        createAndPost(host, {t:'line' ,p1:550005 ,p2:540004});
        createAndPost(host, {t:'line' ,p1:480006 ,p2:500006});
        createAndPost(host, {t:'line' ,p1:500006 ,p2:510006});
        createAndPost(host, {t:'line' ,p1:510006 ,p2:520006});
        createAndPost(host, {t:'line' ,p1:480006 ,p2:480008});
        createAndPost(host, {t:'line' ,p1:480008 ,p2:480009});
        createAndPost(host, {t:'line' ,p1:480009 ,p2:480010});
        createAndPost(host, {t:'line' ,p1:480010 ,p2:520010});
        createAndPost(host, {t:'line' ,p1:520010 ,p2:520006});
        createAndPost(host, {t:'line' ,p1:520006 ,p2:530007});
        createAndPost(host, {t:'line' ,p1:530007 ,p2:530008});
        createAndPost(host, {t:'line' ,p1:530008 ,p2:530009});
        createAndPost(host, {t:'line' ,p1:530009 ,p2:530011});
        createAndPost(host, {t:'line' ,p1:530011 ,p2:520010});
        createAndPost(host, {t:'line' ,p1:530011 ,p2:510011});
        createAndPost(host, {t:'line' ,p1:510011 ,p2:500011});
        createAndPost(host, {t:'line' ,p1:500011 ,p2:490011});
        createAndPost(host, {t:'line' ,p1:490011 ,p2:480010});
        createAndPost(host, {t:'line' ,p1:540006 ,p2:560006});
        createAndPost(host, {t:'line' ,p1:560006 ,p2:570006});
        createAndPost(host, {t:'line' ,p1:570006 ,p2:580006});
        createAndPost(host, {t:'line' ,p1:540006 ,p2:540008});
        createAndPost(host, {t:'line' ,p1:540008 ,p2:540009});
        createAndPost(host, {t:'line' ,p1:540009 ,p2:540010});
        createAndPost(host, {t:'line' ,p1:540010 ,p2:580010});
        createAndPost(host, {t:'line' ,p1:580010 ,p2:580006});
        createAndPost(host, {t:'line' ,p1:580006 ,p2:590007});
        createAndPost(host, {t:'line' ,p1:590007 ,p2:590008});
        createAndPost(host, {t:'line' ,p1:590008 ,p2:590009});
        createAndPost(host, {t:'line' ,p1:590009 ,p2:590011});
        createAndPost(host, {t:'line' ,p1:590011 ,p2:580010});
        createAndPost(host, {t:'line' ,p1:590011 ,p2:570011});
        createAndPost(host, {t:'line' ,p1:570011 ,p2:560011});
        createAndPost(host, {t:'line' ,p1:560011 ,p2:550011});
        createAndPost(host, {t:'line' ,p1:550011 ,p2:540010});
        createAndPost(host, {t:'line' ,p1:470003 ,p2:480003});
        createAndPost(host, {t:'line' ,p1:450005 ,p2:450006});
        createAndPost(host, {t:'line' ,p1:500005 ,p2:500006});
        createAndPost(host, {t:'line' ,p1:470008 ,p2:480008});
    });
}

d.runAll(['/tmp/redis0.sock']);

