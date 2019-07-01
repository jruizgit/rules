var d = require('../libjs/durable');

d.statechart('waltzdb', function() {
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

    start: {
        to: 'duplicate'
        whenAll: m.t == 'ready'
        run: {
            c.s.gid = 1000;
            c.s.startTime = new Date(); 
        }  
    }
    
    duplicate: {
        to: 'duplicate'
        whenAll: line = m.t == 'line'
        run: {
            console.log('Edge ' + line.p1 + ' ' + line.p2);
            console.log('Edge ' + line.p2 + ' ' + line.p1);
            post({id: s.gid, t: 'edge', p1: line.p1, p2: line.p2, joined: false});
            post({id: s.gid + 1, t: 'edge', p1: line.p2, p2: line.p1, joined: false});
            c.s.gid += 2
        }

        to: 'detectJunctions'
        pri: 1
        run: console.log('detectJunctions');
    }

    detectJunctions: {
        to: 'detectJunctions'
        whenAll: {
            e1 = m.t == 'edge' && m.joined == false
            e2 = m.t == 'edge' && m.joined == false && m.p1 == e1.p1 && m.p2 != e1.p2
            e3 = m.t == 'edge' && m.joined == false && m.p1 == e1.p1 && m.p2 != e1.p2 && m.p2 != e2.p2
        }
        run: {
            var j = {id: s.gid, t: 'junction', basePoint: e1.p1, jt: '3j', visited: 'no'};
            make3jJunction(j, e1.p1, e1.p2, e2.p2, e3.p2);
            console.log('Junction ' + j.name + ' ' + j.basePoint + ' ' + j.p1 + ' ' + j.p2 + ' ' + j.p3);
            assert(j);
            e1.id = s.gid + 1; e1.joined = true; e1.jt = '3j'; assert(e1);
            e2.id = s.gid + 2; e2.joined = true; e2.jt = '3j'; assert(e2);
            e3.id = s.gid + 3; e3.joined = true; e3.jt = '3j'; assert(e3);
            s.gid += 4;
        }

        to: 'detectJunctions'
        whenAll: {
            e1 = m.t == 'edge' && m.joined == false
            e2 = m.t == 'edge' && m.joined == false && m.p1 == e1.p1 && m.p2 != e1.p2
            none(m.t == 'edge' && m.p1 == e1.p1 && m.p2 != e1.p2 && m.p2 != e2.p2)
        }
        run: {
            var j = {id: s.gid, t: 'junction', basePoint: e1.p1, jt: '2j', visited: 'no', name: 'L', p1: e1.p2, p2: e2.p2};
            console.log('Junction L ' + e1.p1 + ' ' + e1.p2 + ' ' + e2.p2);
            assert(j)
            e1.id = s.gid + 1; e1.joined = true; e1.jt = '2j'; assert(e1);
            e2.id = s.gid + 2; e2.joined = true; e2.jt = '2j'; assert(e2);
            s.gid += 3;           
        }

        to: 'findInitialBoundary'
        pri: 1
        run: console.log('findInitialBoundary');
    }

    findInitialBoundary: {
        to: 'findSecondBoundary'
        whenAll: {
            j = m.t == 'junction' && m.jt == '2j' && m.visited == 'no'
            e1 = m.t == 'edge' && m.p1 == j.basePoint && m.p2 == j.p1
            e2 = m.t == 'edge' && m.p1 == j.basePoint && m.p2 == j.p2
            none(m.t == 'junction' && m.jt == '2j' && m.visited == 'no' && m.basePoint > j.basePoint)
        }
        run: {
            assert({id: s.gid, t: 'edgeLabel', p1: j.basePoint, p2: j.p1, labelName: 'B', lid: '1'});
            assert({id: s.gid + 1, t: 'edgeLabel', p1: j.basePoint, p2: j.p2, labelName: 'B', lid: '1'});
            s.gid += 2;
            console.log('findInitialBoundary');

        }

        to: 'findSecondBoundary'
        whenAll: {
            j = m.t == 'junction' && m.jt == '3j' && m.name == 'arrow' && m.visited == 'no'
            e1 = m.t == 'edge' && m.p1 == j.basePoint && m.p2 == j.p1
            e2 = m.t == 'edge' && m.p1 == j.basePoint && m.p2 == j.p2
            e3 = m.t == 'edge' && m.p1 == j.basePoint && m.p2 == j.p3
            none(m.t == 'junction' && m.jt == '3j' && m.visited == 'no' && m.basePoint > j.basePoint)
        }
        run: {
            assert({id: s.gid, t: 'edgeLabel', p1: j.basePoint, p2: j.p1, labelName: 'B', lid: '14'});
            assert({id: s.gid + 1, t: 'edgeLabel', p1: j.basePoint, p2: j.p2, labelName: '+', lid: '14'});
            assert({id: s.gid + 2, t: 'edgeLabel', p1: j.basePoint, p2: j.p3, labelName: 'B', lid: '14'});
            s.gid += 3;
            console.log('findSecondBoundary');
        }
    }    

    findSecondBoundary: {
        to: 'labeling'
        whenAll: {
            j = m.t == 'junction' && m.jt == '2j' && m.visited == 'no'
            e1 = m.t == 'edge' && m.p1 == j.basePoint && m.p2 == j.p1
            e2 = m.t == 'edge' && m.p1 == j.basePoint && m.p2 == j.p2
            none(m.t == 'junction' && m.visited != 'no' && m.basePoint < j.basePoint)
        }
        run: {
            retract(j); j.id = s.gid; j.visited = 'yes'; assert(j);            
            assert({id: s.gid + 1, t: 'edgeLabel', p1: j.basePoint, p2: j.p1, labelName: 'B', lid: '1'});
            assert({id: s.gid + 2, t: 'edgeLabel', p1: j.basePoint, p2: j.p2, labelName: 'B', lid: '1'});
            s.gid += 3;
            console.log('labeling');
        }

        to: 'labeling'
        whenAll: {
            j = m.t == 'junction' && m.jt == '3j' && m.name == 'arrow' && m.visited == 'no'
            e1 = m.t == 'edge' && m.p1 == j.basePoint && m.p2 == j.p1
            e2 = m.t == 'edge' && m.p1 == j.basePoint && m.p2 == j.p2
            e3 = m.t == 'edge' && m.p1 == j.basePoint && m.p2 == j.p3
            none(m.t == 'junction' && m.visited != 'no' && m.basePoint < j.basePoint)
        }
        run: {
            retract(j); j.id = s.gid; j.visited = 'yes'; assert(j);            
            c.assert({id: s.gid + 1, t: 'edgeLabel', p1: j.basePoint, p2: j.p1, labelName: 'B', lid: '14'});
            c.assert({id: s.gid + 2, t: 'edgeLabel', p1: j.basePoint, p2: j.p2, labelName: '+', lid: '14'});
            c.assert({id: s.gid + 3, t: 'edgeLabel', p1: j.basePoint, p2: j.p3, labelName: 'B', lid: '14'});
            s.gid += 4;
            console.log('labeling');
        }        
    }

    labeling: {
        to: 'visiting3j'
        whenAll: j = m.t == 'junction' && m.jt == '3j' && m.visited == 'no'
        run: {
            retract(j); j.id = s.gid; j.visited = 'now'; assert(j);
            s.gid += 1;
            console.log('visiting3j');
        }

        to: 'visiting2j'
        whenAll: j = m.t == 'junction' && m.jt == '2j' && m.visited == 'no'
        pri: 1
        run: {
            retract(j); j.id = s.gid; j.visited = 'now'; assert(j);
            s.gid += 1;
            console.log('visiting2j');
        }

        to: 'end'
        pri: 2
        run: {
            console.log('end ' + (new Date() - s.startTime));
            deleteState();
        }
    }
    
    function visit3j (c) {
        console.log('Edge Label ' + c.j.basePoint + ' ' + c.j.p1 + ' ' + c.l.n1 + ' ' + c.l.lid);
        console.log('Edge Label ' + c.j.basePoint + ' ' + c.j.p2 + ' ' + c.l.n2 + ' ' + c.l.lid);
        console.log('Edge Label ' + c.j.basePoint + ' ' + c.j.p3 + ' ' + c.l.n3 + ' ' + c.l.lid);
        c.assert({id: c.s.gid, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p1, labelName: c.l.n1, lid: c.l.lid});
        c.assert({id: c.s.gid + 1, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p2, labelName: c.l.n2, lid: c.l.lid});
        c.assert({id: c.s.gid + 2, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p3, labelName: c.l.n3, lid: c.l.lid});
        c.s.gid += 3;
    }

    visiting3j: {
        to: 'visiting3j'
        whenAll: {
            j = m.t == 'junction' && m.visited == 'now' && m.jt == '3j'
            l = m.t == 'label' && m.name == j.name
            el1 = m.t == 'edgeLabel' && m.p1 == j.p1 && m.p2 == j.basePoint && m.labelName == l.n1
            el2 = m.t == 'edgeLabel' && m.p1 == j.p2 && m.p2 == j.basePoint && m.labelName == l.n2
            el3 = m.t == 'edgeLabel' && m.p1 == j.p3 && m.p2 == j.basePoint && m.labelName == l.n3
            none(m.t == 'edgeLabel' && m.p1 == j.basePoint && m.lid == l.lid)
        }
        run: {
            visit3j(c);
        }

        to: 'visiting3j'
        whenAll: {
            j = m.t == 'junction' && m.visited == 'now' && m.jt == '3j'
            l = m.t == 'label' && m.name == j.name
            none(m.t == 'edgeLabel' && m.p1 == j.p1 && m.p2 == j.basePoint)
            el2 = m.t == 'edgeLabel' && m.p1 == j.p2 && m.p2 == j.basePoint && m.labelName == l.n2
            el3 = m.t == 'edgeLabel' && m.p1 == j.p3 && m.p2 == j.basePoint && m.labelName == l.n3
            none(m.t == 'edgeLabel' && m.p1 == j.basePoint && m.lid == l.lid)
        }
        run: {
            visit3j(c);
        } 

        to: 'visiting3j'
        whenAll: {
            j = m.t == 'junction' && m.visited == 'now' && m.jt == '3j'
            l = m.t == 'label' && m.name == j.name
            el1 = m.t == 'edgeLabel' && m.p1 == j.p1 && m.p2 == j.basePoint && m.labelName == l.n1
            none(m.t == 'edgeLabel' && m.p1 == j.p2 && m.p2 == j.basePoint)
            el3 = m.t == 'edgeLabel' && m.p1 == j.p3 && m.p2 == j.basePoint && m.labelName == l.n3
            none(m.t == 'edgeLabel' && m.p1 == j.basePoint && m.lid == l.lid)
        }
        run: {
            visit3j(c);
        }     

        to: 'visiting3j'
        whenAll: {
            j = m.t == 'junction' && m.visited == 'now' && m.jt == '3j'
            l = m.t == 'label' && m.name == j.name
            none(m.t == 'edgeLabel' && m.p1 == j.p1 && m.p2 == j.basePoint)
            none(m.t == 'edgeLabel' && m.p1 == j.p2 && m.p2 == j.basePoint)
            el3 = m.t == 'edgeLabel' && m.p1 == j.p3 && m.p2 == j.basePoint && m.labelName == l.n3
            none(m.t == 'edgeLabel' && m.p1 == j.basePoint && m.lid == l.lid)
        }
        run: {
            visit3j(c);
        }  

        to: 'visiting3j'
        whenAll: {
            j = m.t == 'junction' && m.visited == 'now' && m.jt == '3j'
            l = m.t == 'label' && m.name == j.name
            el1 = m.t == 'edgeLabel' && m.p1 == j.p1 && m.p2 == j.basePoint && m.labelName == l.n1
            el2 = m.t == 'edgeLabel' && m.p1 == j.p2 && m.p2 == j.basePoint && m.labelName == l.n2
            none(m.t == 'edgeLabel' && m.p1 == j.p3 && m.p2 == j.basePoint)
            none(m.t == 'edgeLabel' && m.p1 == j.basePoint && m.lid == l.lid)
        }
        run: {
            visit3j(c);
        }

        to: 'visiting3j'
        whenAll: {
            j = m.t == 'junction' && m.visited == 'now' && m.jt == '3j'
            l = m.t == 'label' && m.name == j.name
            none(m.t == 'edgeLabel' && m.p1 == j.p1 && m.p2 == j.basePoint)
            el2 = m.t == 'edgeLabel' && m.p1 == j.p2 && m.p2 == j.basePoint && m.labelName == l.n2
            none(m.t == 'edgeLabel' && m.p1 == j.p3 && m.p2 == j.basePoint)
            none(m.t == 'edgeLabel' && m.p1 == j.basePoint && m.lid == l.lid)
        }
        run: {
            visit3j(c);
        } 

        to: 'visiting3j'
        whenAll: {
            j = m.t == 'junction' && m.visited == 'now' && m.jt == '3j'
            l = m.t == 'label' && m.name == j.name
            el1 = m.t == 'edgeLabel' && m.p1 == j.p1 && m.p2 == j.basePoint && m.labelName == l.n1
            none(m.t == 'edgeLabel' && m.p1 == j.p2 && m.p2 == j.basePoint)
            none(m.t == 'edgeLabel' && m.p1 == j.p3 && m.p2 == j.basePoint)
            none(m.t == 'edgeLabel' && m.p1 == j.basePoint && m.lid == l.lid)
        }
        run: {
            visit3j(c);
        }

        to: 'visiting3j'
        whenAll: {
            j = m.t == 'junction' && m.visited == 'now' && m.jt == '3j'
            l = m.t == 'label' && m.name == j.name
            none(m.t == 'edgeLabel' && m.p1 == j.p1 && m.p2 == j.basePoint)
            none(m.t == 'edgeLabel' && m.p1 == j.p2 && m.p2 == j.basePoint)
            none(m.t == 'edgeLabel' && m.p1 == j.p3 && m.p2 == j.basePoint)
            none(m.t == 'edgeLabel' && m.p1 == j.basePoint && m.lid == l.lid)
        }
        run: {
            visit3j(c);
        }

        to: 'marking'
        whenAll: m.t == 'junction' && m.visited == 'now' && m.jt == '3j'
        pri: 1
        run: console.log('marking');
    }

    function visit2j (c) {
        console.log('Edge Label ' + c.j.basePoint + ' ' + c.j.p1 + ' ' + c.l.n1 + ' ' + c.l.lid);
        console.log('Edge Label ' + c.j.basePoint + ' ' + c.j.p2 + ' ' + c.l.n2 + ' ' + c.l.lid);
        c.assert({id: c.s.gid, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p1, labelName: c.l.n1, lid: c.l.lid});
        c.assert({id: c.s.gid + 1, t: 'edgeLabel', p1: c.j.basePoint, p2: c.j.p2, labelName: c.l.n2, lid: c.l.lid});
        c.s.gid += 2;
    }

    visiting2j: {
        to: 'visiting2j'
        whenAll: {
            j = m.t == 'junction' && m.visited == 'now' && m.jt == '2j'
            l = m.t == 'label' && m.name == j.name
            el1 = m.t == 'edgeLabel' && m.p1 == j.p1 && m.p2 == j.basePoint && m.labelName == l.n1
            el2 = m.t == 'edgeLabel' && m.p1 == j.p2 && m.p2 == j.basePoint && m.labelName == l.n2
            none(m.t == 'edgeLabel' && m.p1 == j.basePoint && m.lid == l.lid)
        }
        run: {
            visit2j(c);
        }

        to: 'visiting2j'
        whenAll: {
            j = m.t == 'junction' && m.visited == 'now' && m.jt == '2j'
            l = m.t == 'label' && m.name == j.name
            none(m.t == 'edgeLabel' && m.p1 == j.p1 && m.p2 == j.basePoint)
            el2 = m.t == 'edgeLabel' && m.p1 == j.p2 && m.p2 == j.basePoint && m.labelName == l.n2
            none(m.t == 'edgeLabel' && m.p1 == j.basePoint && m.lid == l.lid)
        }
        run: {
            visit2j(c);
        }

        to: 'visiting2j'
        whenAll: {
            j = m.t == 'junction' && m.visited == 'now' && m.jt == '2j'
            l = m.t == 'label' && m.name == j.name
            el1 = m.t == 'edgeLabel' && m.p1 == j.p1 && m.p2 == j.basePoint && m.labelName == l.n1
            none(m.t == 'edgeLabel' && m.p1 == j.p2 && m.p2 == j.basePoint)
            none(m.t == 'edgeLabel' && m.p1 == j.basePoint && m.lid == l.lid)
        }
        run: {
            visit2j(c);
        }

        to: 'visiting2j'
        whenAll: {
            j = m.t == 'junction' && m.visited == 'now' && m.jt == '2j'
            l = m.t == 'label' && m.name == j.name
            none(m.t == 'edgeLabel' && m.p1 == j.p1 && m.p2 == j.basePoint)
            none(m.t == 'edgeLabel' && m.p1 == j.p2 && m.p2 == j.basePoint)
            none(m.t == 'edgeLabel' && m.p1 == j.basePoint && m.lid == l.lid)
        }
        run: {
            visit2j(c);
        }

        to: 'marking'
        whenAll: m.t == 'junction' && m.visited == 'now' && m.jt == '2j'
        pri: 1
        run: console.log('marking');
    }

    marking: {
        to: 'marking'
        whenAll: {
            j = m.t == 'junction' && m.visited == 'now'
            e = m.t == 'edge' && m.p2 == j.basePoint
            junction = m.t == 'junction' && m.basePoint == e.p1 && m.visited == 'yes'
        }
        run: {
            retract(junction); junction.id = s.gid; junction.visited = 'check'; assert(junction); 
            s.gid += 1;
        }

        to: 'marking'
        whenAll: j = m.t == 'junction' && m.visited == 'now'
        pri: 1
        run: {
            retract(j); j.id = s.gid; j.visited = 'yes'; assert(j); 
            s.gid += 1;
        }

        to: 'checking'
        pri: 2
        run: console.log('checking');
    }

    checking: {
        to: 'removeLabel'
        whenAll: {
            junction = m.t == 'junction' && m.visited == 'check'
            el1 = m.t == 'edge_label' && m.p1 == junction.basePoint
            j = m.t == 'junction' && m.basePoint == el1.p2 && m.visited == 'yes'
            none(m.t == 'edge_label' && m.p1 == el1.p2 && m.p2 == junction.basePoint && m.labelName == el1.labelName)
        }
        run: {
            console.log('removeLabel');
            assert({id: s.gid, t: 'illegal', basePoint: junction.basePoint, lid: el1.lid});
            s.gid += 1;
        }

        to: 'checking'
        whenAll: j = m.t == 'junction' && m.visited == 'check'
        pri: 1
        run: {
            retract(j); j.id = s.gid; j.visited = 'yes'; assert(j); 
            s.gid += 1;
        }

        to: 'labeling'
        pri: 2
        run: console.log('labeling');
    }
  
    removeLabel: {
        to: 'checking'
        whenAll: {
            i = m.t == 'illegal'
            j = m.t == 'junction' && m.jt == '3j' && m.basePoint == i.basePoint
            el1 = m.t == 'edgeLabel' && m.p1 == j.basePoint && m.p2 == j.p1 && m.lid == i.lid
            el2 = m.t == 'edgeLabel' && m.p1 == j.basePoint && m.p2 == j.p2 && m.lid == i.lid
            el3 = m.t == 'edgeLabel' && m.p1 == j.basePoint && m.p2 == j.p3 && m.lid == i.lid
        }
        run: {
            console.log('checking');
            retract(i);
            retract(el1);
            retract(el2);
            retract(el3);
        }

        to: 'checking'
        whenAll: {
            i = m.t == 'illegal'
            j = m.t == 'junction' && m.jt == '2j' && m.basePoint == i.basePoint
            el1 = m.t == 'edgeLabel' && m.p1 == j.basePoint && m.p2 == j.p1 && m.lid == i.lid
            el2 = m.t == 'edgeLabel' && m.p1 == j.basePoint && m.p2 == j.p2 && m.lid == i.lid
        }
        run: {
            console.log('checking');
            retract(i);
            retract(el1);
            retract(el2);
        }
    }

    end: {}    
});

var factCount = 0;
function createAndPost (fact) {
    fact.id = factCount;
    fact.sid = 1;
    d.post('waltzdb', fact);
    factCount += 1;
}

function createAndAssert (fact) {
    fact.id = factCount;
    fact.sid = 1;
    d.assert('waltzdb', fact);
    factCount += 1;
}

createAndAssert({t:'label' ,jt:'2j' ,name:'L' ,lid:'1' ,n1:'B' ,n2:'B'});
createAndAssert({t:'label' ,jt:'2j' ,name:'L' ,lid:'2' ,n1:'+' ,n2:'B'});
createAndAssert({t:'label' ,jt:'2j' ,name:'L' ,lid:'3' ,n1:'B' ,n2:'+'});
createAndAssert({t:'label' ,jt:'2j' ,name:'L' ,lid:'4' ,n1:'-' ,n2:'B'});
createAndAssert({t:'label' ,jt:'2j' ,name:'L' ,lid:'5' ,n1:'B' ,n2:'-'});
createAndAssert({t:'label' ,jt:'3j' ,name:'fork' ,lid:'6' ,n1:'+' ,n2:'+' ,n3:'+'});
createAndAssert({t:'label' ,jt:'3j' ,name:'fork' ,lid:'7' ,n1:'-' ,n2:'-' ,n3:'-'});
createAndAssert({t:'label' ,jt:'3j' ,name:'fork' ,lid:'8' ,n1:'B' ,n2:'-' ,n3:'B'});
createAndAssert({t:'label' ,jt:'3j' ,name:'fork' ,lid:'9' ,n1:'-' ,n2:'B' ,n3:'B'});
createAndAssert({t:'label' ,jt:'3j' ,name:'fork' ,lid:'10' ,n1:'B' ,n2:'B' ,n3:'-'});
createAndAssert({t:'label' ,jt:'3j' ,name:'tee' ,lid:'11' ,n1:'B' ,n2:'+' ,n3:'B'});
createAndAssert({t:'label' ,jt:'3j' ,name:'tee' ,lid:'12' ,n1:'B' ,n2:'-' ,n3:'B'});
createAndAssert({t:'label' ,jt:'3j' ,name:'tee' ,lid:'13' ,n1:'B' ,n2:'B' ,n3:'B'});
createAndAssert({t:'label' ,jt:'3j' ,name:'arrow' ,lid:'14' ,n1:'B' ,n2:'+' ,n3:'B'});
createAndAssert({t:'label' ,jt:'3j' ,name:'arrow' ,lid:'15' ,n1:'-' ,n2:'+' ,n3:'-'});
createAndAssert({t:'label' ,jt:'3j' ,name:'arrow' ,lid:'16' ,n1:'+' ,n2:'-' ,n3:'+'});
createAndPost({t:'line' ,p1:50003 ,p2:60003});
createAndPost({t:'line' ,p1:30005 ,p2:30006});
createAndPost({t:'line' ,p1:80005 ,p2:80006});
createAndPost({t:'line' ,p1:50008 ,p2:60008});
createAndPost({t:'line' ,p1:0 ,p2:20000});
createAndPost({t:'line' ,p1:20000 ,p2:30000});
createAndPost({t:'line' ,p1:30000 ,p2:40000});
createAndPost({t:'line' ,p1:0 ,p2:2});
createAndPost({t:'line' ,p1:2 ,p2:3});
createAndPost({t:'line' ,p1:3 ,p2:4});
createAndPost({t:'line' ,p1:4 ,p2:40004});
createAndPost({t:'line' ,p1:40004 ,p2:40000});
createAndPost({t:'line' ,p1:40000 ,p2:50001});
createAndPost({t:'line' ,p1:50001 ,p2:50002});
createAndPost({t:'line' ,p1:50002 ,p2:50003});
createAndPost({t:'line' ,p1:50003 ,p2:50005});
createAndPost({t:'line' ,p1:50005 ,p2:40004});
createAndPost({t:'line' ,p1:50005 ,p2:30005});
createAndPost({t:'line' ,p1:30005 ,p2:20005});
createAndPost({t:'line' ,p1:20005 ,p2:10005});
createAndPost({t:'line' ,p1:10005 ,p2:4});
createAndPost({t:'line' ,p1:60000 ,p2:80000});
createAndPost({t:'line' ,p1:80000 ,p2:90000});
createAndPost({t:'line' ,p1:90000 ,p2:100000});
createAndPost({t:'line' ,p1:60000 ,p2:60002});
createAndPost({t:'line' ,p1:60002 ,p2:60003});
createAndPost({t:'line' ,p1:60003 ,p2:60004});
createAndPost({t:'line' ,p1:60004 ,p2:100004});
createAndPost({t:'line' ,p1:100004 ,p2:100000});
createAndPost({t:'line' ,p1:100000 ,p2:110001});
createAndPost({t:'line' ,p1:110001 ,p2:110002});
createAndPost({t:'line' ,p1:110002 ,p2:110003});
createAndPost({t:'line' ,p1:110003 ,p2:110005});
createAndPost({t:'line' ,p1:110005 ,p2:100004});
createAndPost({t:'line' ,p1:110005 ,p2:90005});
createAndPost({t:'line' ,p1:90005 ,p2:80005});
createAndPost({t:'line' ,p1:80005 ,p2:70005});
createAndPost({t:'line' ,p1:70005 ,p2:60004});
createAndPost({t:'line' ,p1:6 ,p2:20006});
createAndPost({t:'line' ,p1:20006 ,p2:30006});
createAndPost({t:'line' ,p1:30006 ,p2:40006});
createAndPost({t:'line' ,p1:6 ,p2:8});
createAndPost({t:'line' ,p1:8 ,p2:9});
createAndPost({t:'line' ,p1:9 ,p2:10});
createAndPost({t:'line' ,p1:10 ,p2:40010});
createAndPost({t:'line' ,p1:40010 ,p2:40006});
createAndPost({t:'line' ,p1:40006 ,p2:50007});
createAndPost({t:'line' ,p1:50007 ,p2:50008});
createAndPost({t:'line' ,p1:50008 ,p2:50009});
createAndPost({t:'line' ,p1:50009 ,p2:50011});
createAndPost({t:'line' ,p1:50011 ,p2:40010});
createAndPost({t:'line' ,p1:50011 ,p2:30011});
createAndPost({t:'line' ,p1:30011 ,p2:20011});
createAndPost({t:'line' ,p1:20011 ,p2:10011});
createAndPost({t:'line' ,p1:10011 ,p2:10});
createAndPost({t:'line' ,p1:60006 ,p2:80006});
createAndPost({t:'line' ,p1:80006 ,p2:90006});
createAndPost({t:'line' ,p1:90006 ,p2:100006});
createAndPost({t:'line' ,p1:60006 ,p2:60008});
createAndPost({t:'line' ,p1:60008 ,p2:60009});
createAndPost({t:'line' ,p1:60009 ,p2:60010});
createAndPost({t:'line' ,p1:60010 ,p2:100010});
createAndPost({t:'line' ,p1:100010 ,p2:100006});
createAndPost({t:'line' ,p1:100006 ,p2:110007});
createAndPost({t:'line' ,p1:110007 ,p2:110008});
createAndPost({t:'line' ,p1:110008 ,p2:110009});
createAndPost({t:'line' ,p1:110009 ,p2:110011});
createAndPost({t:'line' ,p1:110011 ,p2:100010});
createAndPost({t:'line' ,p1:110011 ,p2:90011});
createAndPost({t:'line' ,p1:90011 ,p2:80011});
createAndPost({t:'line' ,p1:80011 ,p2:70011});
createAndPost({t:'line' ,p1:70011 ,p2:60010});
createAndPost({t:'line' ,p1:170003 ,p2:180003});
createAndPost({t:'line' ,p1:150005 ,p2:150006});
createAndPost({t:'line' ,p1:200005 ,p2:200006});
createAndPost({t:'line' ,p1:170008 ,p2:180008});
createAndPost({t:'line' ,p1:120000 ,p2:140000});
createAndPost({t:'line' ,p1:140000 ,p2:150000});
createAndPost({t:'line' ,p1:150000 ,p2:160000});
createAndPost({t:'line' ,p1:120000 ,p2:120002});
createAndPost({t:'line' ,p1:120002 ,p2:120003});
createAndPost({t:'line' ,p1:120003 ,p2:120004});
createAndPost({t:'line' ,p1:120004 ,p2:160004});
createAndPost({t:'line' ,p1:160004 ,p2:160000});
createAndPost({t:'line' ,p1:160000 ,p2:170001});
createAndPost({t:'line' ,p1:170001 ,p2:170002});
createAndPost({t:'line' ,p1:170002 ,p2:170003});
createAndPost({t:'line' ,p1:170003 ,p2:170005});
createAndPost({t:'line' ,p1:170005 ,p2:160004});
createAndPost({t:'line' ,p1:170005 ,p2:150005});
createAndPost({t:'line' ,p1:150005 ,p2:140005});
createAndPost({t:'line' ,p1:140005 ,p2:130005});
createAndPost({t:'line' ,p1:130005 ,p2:120004});
createAndPost({t:'line' ,p1:180000 ,p2:200000});
createAndPost({t:'line' ,p1:200000 ,p2:210000});
createAndPost({t:'line' ,p1:210000 ,p2:220000});
createAndPost({t:'line' ,p1:180000 ,p2:180002});
createAndPost({t:'line' ,p1:180002 ,p2:180003});
createAndPost({t:'line' ,p1:180003 ,p2:180004});
createAndPost({t:'line' ,p1:180004 ,p2:220004});
createAndPost({t:'line' ,p1:220004 ,p2:220000});
createAndPost({t:'line' ,p1:220000 ,p2:230001});
createAndPost({t:'line' ,p1:230001 ,p2:230002});
createAndPost({t:'line' ,p1:230002 ,p2:230003});
createAndPost({t:'line' ,p1:230003 ,p2:230005});
createAndPost({t:'line' ,p1:230005 ,p2:220004});
createAndPost({t:'line' ,p1:230005 ,p2:210005});
createAndPost({t:'line' ,p1:210005 ,p2:200005});
createAndPost({t:'line' ,p1:200005 ,p2:190005});
createAndPost({t:'line' ,p1:190005 ,p2:180004});
createAndPost({t:'line' ,p1:120006 ,p2:140006});
createAndPost({t:'line' ,p1:140006 ,p2:150006});
createAndPost({t:'line' ,p1:150006 ,p2:160006});
createAndPost({t:'line' ,p1:120006 ,p2:120008});
createAndPost({t:'line' ,p1:120008 ,p2:120009});
createAndPost({t:'line' ,p1:120009 ,p2:120010});
createAndPost({t:'line' ,p1:120010 ,p2:160010});
createAndPost({t:'line' ,p1:160010 ,p2:160006});
createAndPost({t:'line' ,p1:160006 ,p2:170007});
createAndPost({t:'line' ,p1:170007 ,p2:170008});
createAndPost({t:'line' ,p1:170008 ,p2:170009});
createAndPost({t:'line' ,p1:170009 ,p2:170011});
createAndPost({t:'line' ,p1:170011 ,p2:160010});
createAndPost({t:'line' ,p1:170011 ,p2:150011});
createAndPost({t:'line' ,p1:150011 ,p2:140011});
createAndPost({t:'line' ,p1:140011 ,p2:130011});
createAndPost({t:'line' ,p1:130011 ,p2:120010});
createAndPost({t:'line' ,p1:180006 ,p2:200006});
createAndPost({t:'line' ,p1:200006 ,p2:210006});
createAndPost({t:'line' ,p1:210006 ,p2:220006});
createAndPost({t:'line' ,p1:180006 ,p2:180008});
createAndPost({t:'line' ,p1:180008 ,p2:180009});
createAndPost({t:'line' ,p1:180009 ,p2:180010});
createAndPost({t:'line' ,p1:180010 ,p2:220010});
createAndPost({t:'line' ,p1:220010 ,p2:220006});
createAndPost({t:'line' ,p1:220006 ,p2:230007});
createAndPost({t:'line' ,p1:230007 ,p2:230008});
createAndPost({t:'line' ,p1:230008 ,p2:230009});
createAndPost({t:'line' ,p1:230009 ,p2:230011});
createAndPost({t:'line' ,p1:230011 ,p2:220010});
createAndPost({t:'line' ,p1:230011 ,p2:210011});
createAndPost({t:'line' ,p1:210011 ,p2:200011});
createAndPost({t:'line' ,p1:200011 ,p2:190011});
createAndPost({t:'line' ,p1:190011 ,p2:180010});
createAndPost({t:'line' ,p1:110003 ,p2:120003});
createAndPost({t:'line' ,p1:90005 ,p2:90006});
createAndPost({t:'line' ,p1:140005 ,p2:140006});
createAndPost({t:'line' ,p1:110008 ,p2:120008});
createAndPost({t:'line' ,p1:290003 ,p2:300003});
createAndPost({t:'line' ,p1:270005 ,p2:270006});
createAndPost({t:'line' ,p1:320005 ,p2:320006});
createAndPost({t:'line' ,p1:290008 ,p2:300008});
createAndPost({t:'line' ,p1:240000 ,p2:260000});
createAndPost({t:'line' ,p1:260000 ,p2:270000});
createAndPost({t:'line' ,p1:270000 ,p2:280000});
createAndPost({t:'line' ,p1:240000 ,p2:240002});
createAndPost({t:'line' ,p1:240002 ,p2:240003});
createAndPost({t:'line' ,p1:240003 ,p2:240004});
createAndPost({t:'line' ,p1:240004 ,p2:280004});
createAndPost({t:'line' ,p1:280004 ,p2:280000});
createAndPost({t:'line' ,p1:280000 ,p2:290001});
createAndPost({t:'line' ,p1:290001 ,p2:290002});
createAndPost({t:'line' ,p1:290002 ,p2:290003});
createAndPost({t:'line' ,p1:290003 ,p2:290005});
createAndPost({t:'line' ,p1:290005 ,p2:280004});
createAndPost({t:'line' ,p1:290005 ,p2:270005});
createAndPost({t:'line' ,p1:270005 ,p2:260005});
createAndPost({t:'line' ,p1:260005 ,p2:250005});
createAndPost({t:'line' ,p1:250005 ,p2:240004});
createAndPost({t:'line' ,p1:300000 ,p2:320000});
createAndPost({t:'line' ,p1:320000 ,p2:330000});
createAndPost({t:'line' ,p1:330000 ,p2:340000});
createAndPost({t:'line' ,p1:300000 ,p2:300002});
createAndPost({t:'line' ,p1:300002 ,p2:300003});
createAndPost({t:'line' ,p1:300003 ,p2:300004});
createAndPost({t:'line' ,p1:300004 ,p2:340004});
createAndPost({t:'line' ,p1:340004 ,p2:340000});
createAndPost({t:'line' ,p1:340000 ,p2:350001});
createAndPost({t:'line' ,p1:350001 ,p2:350002});
createAndPost({t:'line' ,p1:350002 ,p2:350003});
createAndPost({t:'line' ,p1:350003 ,p2:350005});
createAndPost({t:'line' ,p1:350005 ,p2:340004});
createAndPost({t:'line' ,p1:350005 ,p2:330005});
createAndPost({t:'line' ,p1:330005 ,p2:320005});
createAndPost({t:'line' ,p1:320005 ,p2:310005});
createAndPost({t:'line' ,p1:310005 ,p2:300004});
createAndPost({t:'line' ,p1:240006 ,p2:260006});
createAndPost({t:'line' ,p1:260006 ,p2:270006});
createAndPost({t:'line' ,p1:270006 ,p2:280006});
createAndPost({t:'line' ,p1:240006 ,p2:240008});
createAndPost({t:'line' ,p1:240008 ,p2:240009});
createAndPost({t:'line' ,p1:240009 ,p2:240010});
createAndPost({t:'line' ,p1:240010 ,p2:280010});
createAndPost({t:'line' ,p1:280010 ,p2:280006});
createAndPost({t:'line' ,p1:280006 ,p2:290007});
createAndPost({t:'line' ,p1:290007 ,p2:290008});
createAndPost({t:'line' ,p1:290008 ,p2:290009});
createAndPost({t:'line' ,p1:290009 ,p2:290011});
createAndPost({t:'line' ,p1:290011 ,p2:280010});
createAndPost({t:'line' ,p1:290011 ,p2:270011});
createAndPost({t:'line' ,p1:270011 ,p2:260011});
createAndPost({t:'line' ,p1:260011 ,p2:250011});
createAndPost({t:'line' ,p1:250011 ,p2:240010});
createAndPost({t:'line' ,p1:300006 ,p2:320006});
createAndPost({t:'line' ,p1:320006 ,p2:330006});
createAndPost({t:'line' ,p1:330006 ,p2:340006});
createAndPost({t:'line' ,p1:300006 ,p2:300008});
createAndPost({t:'line' ,p1:300008 ,p2:300009});
createAndPost({t:'line' ,p1:300009 ,p2:300010});
createAndPost({t:'line' ,p1:300010 ,p2:340010});
createAndPost({t:'line' ,p1:340010 ,p2:340006});
createAndPost({t:'line' ,p1:340006 ,p2:350007});
createAndPost({t:'line' ,p1:350007 ,p2:350008});
createAndPost({t:'line' ,p1:350008 ,p2:350009});
createAndPost({t:'line' ,p1:350009 ,p2:350011});
createAndPost({t:'line' ,p1:350011 ,p2:340010});
createAndPost({t:'line' ,p1:350011 ,p2:330011});
createAndPost({t:'line' ,p1:330011 ,p2:320011});
createAndPost({t:'line' ,p1:320011 ,p2:310011});
createAndPost({t:'line' ,p1:310011 ,p2:300010});
createAndPost({t:'line' ,p1:230003 ,p2:240003});
createAndPost({t:'line' ,p1:210005 ,p2:210006});
createAndPost({t:'line' ,p1:260005 ,p2:260006});
createAndPost({t:'line' ,p1:230008 ,p2:240008});
createAndPost({t:'line' ,p1:410003 ,p2:420003});
createAndPost({t:'line' ,p1:390005 ,p2:390006});
createAndPost({t:'line' ,p1:440005 ,p2:440006});
createAndPost({t:'line' ,p1:410008 ,p2:420008});
createAndPost({t:'line' ,p1:360000 ,p2:380000});
createAndPost({t:'line' ,p1:380000 ,p2:390000});
createAndPost({t:'line' ,p1:390000 ,p2:400000});
createAndPost({t:'line' ,p1:360000 ,p2:360002});
createAndPost({t:'line' ,p1:360002 ,p2:360003});
createAndPost({t:'line' ,p1:360003 ,p2:360004});
createAndPost({t:'line' ,p1:360004 ,p2:400004});
createAndPost({t:'line' ,p1:400004 ,p2:400000});
createAndPost({t:'line' ,p1:400000 ,p2:410001});
createAndPost({t:'line' ,p1:410001 ,p2:410002});
createAndPost({t:'line' ,p1:410002 ,p2:410003});
createAndPost({t:'line' ,p1:410003 ,p2:410005});
createAndPost({t:'line' ,p1:410005 ,p2:400004});
createAndPost({t:'line' ,p1:410005 ,p2:390005});
createAndPost({t:'line' ,p1:390005 ,p2:380005});
createAndPost({t:'line' ,p1:380005 ,p2:370005});
createAndPost({t:'line' ,p1:370005 ,p2:360004});
createAndPost({t:'line' ,p1:420000 ,p2:440000});
createAndPost({t:'line' ,p1:440000 ,p2:450000});
createAndPost({t:'line' ,p1:450000 ,p2:460000});
createAndPost({t:'line' ,p1:420000 ,p2:420002});
createAndPost({t:'line' ,p1:420002 ,p2:420003});
createAndPost({t:'line' ,p1:420003 ,p2:420004});
createAndPost({t:'line' ,p1:420004 ,p2:460004});
createAndPost({t:'line' ,p1:460004 ,p2:460000});
createAndPost({t:'line' ,p1:460000 ,p2:470001});
createAndPost({t:'line' ,p1:470001 ,p2:470002});
createAndPost({t:'line' ,p1:470002 ,p2:470003});
createAndPost({t:'line' ,p1:470003 ,p2:470005});
createAndPost({t:'line' ,p1:470005 ,p2:460004});
createAndPost({t:'line' ,p1:470005 ,p2:450005});
createAndPost({t:'line' ,p1:450005 ,p2:440005});
createAndPost({t:'line' ,p1:440005 ,p2:430005});
createAndPost({t:'line' ,p1:430005 ,p2:420004});
createAndPost({t:'line' ,p1:360006 ,p2:380006});
createAndPost({t:'line' ,p1:380006 ,p2:390006});
createAndPost({t:'line' ,p1:390006 ,p2:400006});
createAndPost({t:'line' ,p1:360006 ,p2:360008});
createAndPost({t:'line' ,p1:360008 ,p2:360009});
createAndPost({t:'line' ,p1:360009 ,p2:360010});
createAndPost({t:'line' ,p1:360010 ,p2:400010});
createAndPost({t:'line' ,p1:400010 ,p2:400006});
createAndPost({t:'line' ,p1:400006 ,p2:410007});
createAndPost({t:'line' ,p1:410007 ,p2:410008});
createAndPost({t:'line' ,p1:410008 ,p2:410009});
createAndPost({t:'line' ,p1:410009 ,p2:410011});
createAndPost({t:'line' ,p1:410011 ,p2:400010});
createAndPost({t:'line' ,p1:410011 ,p2:390011});
createAndPost({t:'line' ,p1:390011 ,p2:380011});
createAndPost({t:'line' ,p1:380011 ,p2:370011});
createAndPost({t:'line' ,p1:370011 ,p2:360010});
createAndPost({t:'line' ,p1:420006 ,p2:440006});
createAndPost({t:'line' ,p1:440006 ,p2:450006});
createAndPost({t:'line' ,p1:450006 ,p2:460006});
createAndPost({t:'line' ,p1:420006 ,p2:420008});
createAndPost({t:'line' ,p1:420008 ,p2:420009});
createAndPost({t:'line' ,p1:420009 ,p2:420010});
createAndPost({t:'line' ,p1:420010 ,p2:460010});
createAndPost({t:'line' ,p1:460010 ,p2:460006});
createAndPost({t:'line' ,p1:460006 ,p2:470007});
createAndPost({t:'line' ,p1:470007 ,p2:470008});
createAndPost({t:'line' ,p1:470008 ,p2:470009});
createAndPost({t:'line' ,p1:470009 ,p2:470011});
createAndPost({t:'line' ,p1:470011 ,p2:460010});
createAndPost({t:'line' ,p1:470011 ,p2:450011});
createAndPost({t:'line' ,p1:450011 ,p2:440011});
createAndPost({t:'line' ,p1:440011 ,p2:430011});
createAndPost({t:'line' ,p1:430011 ,p2:420010});
createAndPost({t:'line' ,p1:350003 ,p2:360003});
createAndPost({t:'line' ,p1:330005 ,p2:330006});
createAndPost({t:'line' ,p1:380005 ,p2:380006});
createAndPost({t:'line' ,p1:350008 ,p2:360008});
createAndPost({t:'line' ,p1:530003 ,p2:540003});
createAndPost({t:'line' ,p1:510005 ,p2:510006});
createAndPost({t:'line' ,p1:560005 ,p2:560006});
createAndPost({t:'line' ,p1:530008 ,p2:540008});
createAndPost({t:'line' ,p1:480000 ,p2:500000});
createAndPost({t:'line' ,p1:500000 ,p2:510000});
createAndPost({t:'line' ,p1:510000 ,p2:520000});
createAndPost({t:'line' ,p1:480000 ,p2:480002});
createAndPost({t:'line' ,p1:480002 ,p2:480003});
createAndPost({t:'line' ,p1:480003 ,p2:480004});
createAndPost({t:'line' ,p1:480004 ,p2:520004});
createAndPost({t:'line' ,p1:520004 ,p2:520000});
createAndPost({t:'line' ,p1:520000 ,p2:530001});
createAndPost({t:'line' ,p1:530001 ,p2:530002});
createAndPost({t:'line' ,p1:530002 ,p2:530003});
createAndPost({t:'line' ,p1:530003 ,p2:530005});
createAndPost({t:'line' ,p1:530005 ,p2:520004});
createAndPost({t:'line' ,p1:530005 ,p2:510005});
createAndPost({t:'line' ,p1:510005 ,p2:500005});
createAndPost({t:'line' ,p1:500005 ,p2:490005});
createAndPost({t:'line' ,p1:490005 ,p2:480004});
createAndPost({t:'line' ,p1:540000 ,p2:560000});
createAndPost({t:'line' ,p1:560000 ,p2:570000});
createAndPost({t:'line' ,p1:570000 ,p2:580000});
createAndPost({t:'line' ,p1:540000 ,p2:540002});
createAndPost({t:'line' ,p1:540002 ,p2:540003});
createAndPost({t:'line' ,p1:540003 ,p2:540004});
createAndPost({t:'line' ,p1:540004 ,p2:580004});
createAndPost({t:'line' ,p1:580004 ,p2:580000});
createAndPost({t:'line' ,p1:580000 ,p2:590001});
createAndPost({t:'line' ,p1:590001 ,p2:590002});
createAndPost({t:'line' ,p1:590002 ,p2:590003});
createAndPost({t:'line' ,p1:590003 ,p2:590005});
createAndPost({t:'line' ,p1:590005 ,p2:580004});
createAndPost({t:'line' ,p1:590005 ,p2:570005});
createAndPost({t:'line' ,p1:570005 ,p2:560005});
createAndPost({t:'line' ,p1:560005 ,p2:550005});
createAndPost({t:'line' ,p1:550005 ,p2:540004});
createAndPost({t:'line' ,p1:480006 ,p2:500006});
createAndPost({t:'line' ,p1:500006 ,p2:510006});
createAndPost({t:'line' ,p1:510006 ,p2:520006});
createAndPost({t:'line' ,p1:480006 ,p2:480008});
createAndPost({t:'line' ,p1:480008 ,p2:480009});
createAndPost({t:'line' ,p1:480009 ,p2:480010});
createAndPost({t:'line' ,p1:480010 ,p2:520010});
createAndPost({t:'line' ,p1:520010 ,p2:520006});
createAndPost({t:'line' ,p1:520006 ,p2:530007});
createAndPost({t:'line' ,p1:530007 ,p2:530008});
createAndPost({t:'line' ,p1:530008 ,p2:530009});
createAndPost({t:'line' ,p1:530009 ,p2:530011});
createAndPost({t:'line' ,p1:530011 ,p2:520010});
createAndPost({t:'line' ,p1:530011 ,p2:510011});
createAndPost({t:'line' ,p1:510011 ,p2:500011});
createAndPost({t:'line' ,p1:500011 ,p2:490011});
createAndPost({t:'line' ,p1:490011 ,p2:480010});
createAndPost({t:'line' ,p1:540006 ,p2:560006});
createAndPost({t:'line' ,p1:560006 ,p2:570006});
createAndPost({t:'line' ,p1:570006 ,p2:580006});
createAndPost({t:'line' ,p1:540006 ,p2:540008});
createAndPost({t:'line' ,p1:540008 ,p2:540009});
createAndPost({t:'line' ,p1:540009 ,p2:540010});
createAndPost({t:'line' ,p1:540010 ,p2:580010});
createAndPost({t:'line' ,p1:580010 ,p2:580006});
createAndPost({t:'line' ,p1:580006 ,p2:590007});
createAndPost({t:'line' ,p1:590007 ,p2:590008});
createAndPost({t:'line' ,p1:590008 ,p2:590009});
createAndPost({t:'line' ,p1:590009 ,p2:590011});
createAndPost({t:'line' ,p1:590011 ,p2:580010});
createAndPost({t:'line' ,p1:590011 ,p2:570011});
createAndPost({t:'line' ,p1:570011 ,p2:560011});
createAndPost({t:'line' ,p1:560011 ,p2:550011});
createAndPost({t:'line' ,p1:550011 ,p2:540010});
createAndPost({t:'line' ,p1:470003 ,p2:480003});
createAndPost({t:'line' ,p1:450005 ,p2:450006});
createAndPost({t:'line' ,p1:500005 ,p2:500006});
createAndPost({t:'line' ,p1:470008 ,p2:480008});
createAndPost({t:'ready'});
