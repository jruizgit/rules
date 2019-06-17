from durable.lang import *
import math
import datetime
import json

_fact_count = 0
def create_and_post(fact):
    global _fact_count
    fact['id'] = _fact_count
    fact['sid'] = 1
    post('waltzdb', fact)
    _fact_count += 1

def create_and_assert(fact):
    global _fact_count
    fact['id'] = _fact_count
    fact['sid'] = 1
    assert_fact('waltzdb', fact)
    _fact_count += 1

def get_x(val):
    return math.floor(val / 100)

def get_y(val):
    return val % 100

def get_angle(p1, p2):
    delta_x = get_x(p2) - get_x(p1)
    delta_y = get_y(p2) - get_y(p1)
    if delta_x == 0:
        if delta_y > 0:
            return math.pi / 2
        elif delta_y < 0: 
            return -math.pi / 2
    elif delta_y == 0:
        if delta_x > 0:
            return 0
        elif delta_x < 0:
            return math.pi
    else:
        return math.atan2(delta_y, delta_x)

def get_inscribable_angle(base_point, p1, p2):
    angle1 = get_angle(base_point, p1)
    angle2 = get_angle(base_point, p2)
    temp = math.fabs(angle1 - angle2)
    if temp > math.pi:
        return math.fabs(2 * math.pi - temp)

    return temp

def make_3j_junction(j, base_point, p1, p2, p3):
    angle12 = get_inscribable_angle(base_point, p1, p2)
    angle13 = get_inscribable_angle(base_point, p1, p3)
    angle23 = get_inscribable_angle(base_point, p2, p3)
    sum1213 = angle12 + angle13
    sum1223 = angle12 + angle23
    sum1323 = angle13 + angle23

    total = 0
    if sum1213 < sum1223:
        if sum1213 < sum1323:
            total = sum1213
            j['p2'] = p1; j['p1'] = p2; j['p3'] = p3
        else:
            total = sum1323
            j['p2'] = p3; j['p1'] = p1; j['p3'] = p2
    else:
        if sum1223 < sum1323:
            total = sum1223
            j['p2'] = p2; j['p1'] = p1; j['p3'] = p3
        else:
            total = sum1323
            j['p2'] = p3; j['p1'] = p1; j['p3'] = p2

    if math.fabs(total - math.pi) < 0.001:
        j['name'] = 'tee'
    elif total > math.pi:
        j['name'] = 'fork' 
    else:
        j['name'] = 'arrow'

def unix_time(dt):
    epoch = datetime.datetime.utcfromtimestamp(0)
    delta = dt - epoch
    return delta.total_seconds()

def unix_time_millis(dt):
    return unix_time(dt) * 1000.0

with statechart('waltzdb'):
    with state('start'):
        @to('duplicate')
        @when_all(m.t == 'ready')
        def starting(c):
            c.s.gid = 1000
            c.s.start_time = unix_time_millis(datetime.datetime.now())

    with state('duplicate'):
        @to('detect_junctions')
        @when_all(pri(1))
        def done_reversing(c):
            print('detect_junctions')

        @to('duplicate')
        @when_all(c.line << m.t == 'line')
        def reverse_edges(c):
            print('Edge {0} {1}'.format(c.line.p1, c.line.p2))
            print('Edge {0} {1}'.format(c.line.p2, c.line.p1))
            c.post({'id': c.s.gid, 't': 'edge', 'p1': c.line.p1, 'p2': c.line.p2, 'joined': False})
            c.post({'id': c.s.gid + 1, 't': 'edge', 'p1': c.line.p2, 'p2': c.line.p1, 'joined': False})
            c.s.gid += 2

    with state('detect_junctions'):
        @to('detect_junctions')
        @when_all(c.e1 << (m.t == 'edge') & (m.joined == False),
                  c.e2 << (m.t == 'edge') & (m.joined == False) & (m.p1 == c.e1.p1) & (m.p2 != c.e1.p2),
                  c.e3 << (m.t == 'edge') & (m.joined == False) & (m.p1 == c.e1.p1) & (m.p2 != c.e1.p2) & (m.p2 != c.e2.p2))
        def make_3_junction(c):
            j = {'id': c.s.gid, 't': 'junction', 'base_point': c.e1.p1, 'j_t': '3j', 'visited': 'no'}
            make_3j_junction(j, c.e1.p1, c.e1.p2, c.e2.p2, c.e3.p2)
            print('Junction {0} {1} {2} {3} {4}'.format(j['name'], j['base_point'], j['p1'], j['p2'], j['p3']))
            c.assert_fact(j)
            c.e1.id = c.s.gid + 1; c.e1.joined = True; c.e1.j_t = '3j'; c.assert_fact(c.e1)
            c.e2.id = c.s.gid + 2; c.e2.joined = True; c.e2.j_t = '3j'; c.assert_fact(c.e2)
            c.e3.id = c.s.gid + 3; c.e3.joined = True; c.e3.j_t = '3j'; c.assert_fact(c.e3)
            c.s.gid += 4

        @to('detect_junctions')
        @when_all(c.e1 << (m.t == 'edge') & (m.joined == False),
                  c.e2 << (m.t == 'edge') & (m.joined == False) & (m.p1 == c.e1.p1) & (m.p2 != c.e1.p2),
                  none((m.t == 'edge') & (m.p1 == c.e1.p1) & (m.p2 != c.e1.p2) & (m.p2 != c.e2.p2))) 
        def make_l(c):
            j = {'id': c.s.gid, 't': 'junction', 'base_point': c.e1.p1, 'j_t': '2j', 'visited': 'no', 'name': 'L', 'p1': c.e1.p2, 'p2': c.e2.p2}
            print('Junction L {0} {1} {2}'.format(c.e1.p1, c.e1.p2, c.e2.p2))
            c.assert_fact(j)
            c.e1.id = c.s.gid + 1; c.e1.joined = True; c.e1.j_t = '2j'; c.assert_fact(c.e1)
            c.e2.id = c.s.gid + 2; c.e2.joined = True; c.e2.j_t = '2j'; c.assert_fact(c.e2)
            c.s.gid += 3

        @to('find_initial_boundary')
        @when_all(pri(1))
        def done_detecting(c):
            print('find_initial_boundary')

    with state('find_initial_boundary'):
        @to('find_second_boundary')
        @when_all(c.j << (m.t == 'junction') & (m.j_t == '2j') & (m.visited == 'no'),
                  c.e1 << (m.t == 'edge') & (m.p1 == c.j.base_point) & (m.p2 == c.j.p1),
                  c.e2 << (m.t == 'edge') & (m.p1 == c.j.base_point) & (m.p2 == c.j.p2),
                  none((m.t == 'junction') & (m.j_t == '2j') & (m.visited == 'no') & (m.base_point > c.j.base_point)))
        def initial_boundary_junction_l(c):
            c.assert_fact({'id': c.s.gid, 't': 'edge_label', 'p1': c.j.base_point, 'p2': c.j.p1, 'label_name': 'B', 'lid': '1'})
            c.assert_fact({'id': c.s.gid + 1, 't': 'edge_label', 'p1': c.j.base_point, 'p2': c.j.p2, 'label_name': 'B', 'lid': '1'})
            c.s.gid += 2
            print('find_second_boundary')

        @to('find_second_boundary')
        @when_all(c.j << (m.t == 'junction') & (m.j_t == '3j') & (m.name == 'arrow') & (m.visited == 'no'),
              c.e1 << (m.t == 'edge') & (m.p1 == c.j.base_point) & (m.p2 == c.j.p1),
              c.e2 << (m.t == 'edge') & (m.p1 == c.j.base_point) & (m.p2 == c.j.p2),
              c.e3 << (m.t == 'edge') & (m.p1 == c.j.base_point) & (m.p2 == c.j.p3),
              none((m.t == 'junction') & (m.j_t == '3j') & (m.visited == 'no') & (m.base_point > c.j.base_point)))
        def initial_boundary_junction_arrow(c):
            c.assert_fact({'id': c.s.gid, 't': 'edge_label', 'p1': c.j.base_point, 'p2': c.j.p1, 'label_name': 'B', 'lid': '14'})
            c.assert_fact({'id': c.s.gid + 1, 't': 'edge_label', 'p1': c.j.base_point, 'p2': c.j.p2, 'label_name': '+', 'lid': '14'})
            c.assert_fact({'id': c.s.gid + 2, 't': 'edge_label', 'p1': c.j.base_point, 'p2': c.j.p3, 'label_name': 'B', 'lid': '14'})
            c.s.gid += 3
            print('find_second_boundary')

    with state('find_second_boundary'):
        @to('labeling')
        @when_all(c.j << (m.t == 'junction') & (m.j_t == '2j') & (m.visited == 'no'),
                  c.e1 << (m.t == 'edge') & (m.p1 == c.j.base_point) & (m.p2 == c.j.p1),
                  c.e2 << (m.t == 'edge') & (m.p1 == c.j.base_point) & (m.p2 == c.j.p2),
                  none((m.t == 'junction') & (m.visited != 'no') & (m.base_point < c.j.base_point)))
        def second_boundary_junction_l(c):
            c.retract_fact(c.j); c.j.id = c.s.gid; c.j.visited = 'yes'; c.assert_fact(c.j)            
            c.assert_fact({'id': c.s.gid + 1, 't': 'edge_label', 'p1': c.j.base_point, 'p2': c.j.p1, 'label_name': 'B', 'lid': '1'})
            c.assert_fact({'id': c.s.gid + 2, 't': 'edge_label', 'p1': c.j.base_point, 'p2': c.j.p2, 'label_name': 'B', 'lid': '1'})
            c.s.gid += 3
            print('labeling')

        @to('labeling')
        @when_all(c.j << (m.t == 'junction') & (m.j_t == '3j') & (m.name == 'arrow') & (m.visited == 'no'),
                  c.e1 << (m.t == 'edge') & (m.p1 == c.j.base_point) & (m.p2 == c.j.p1),
                  c.e2 << (m.t == 'edge') & (m.p1 == c.j.base_point) & (m.p2 == c.j.p2),
                  c.e3 << (m.t == 'edge') & (m.p1 == c.j.base_point) & (m.p2 == c.j.p3),
                  none((m.t == 'junction') & (m.visited != 'no') & (m.base_point < c.j.base_point)))
        def second_boundary_junction_arrow(c):
            c.retract_fact(c.j); c.j.id = c.s.gid; c.j.visited = 'yes'; c.assert_fact(c.j)            
            c.assert_fact({'id': c.s.gid + 1, 't': 'edge_label', 'p1': c.j.base_point, 'p2': c.j.p1, 'label_name': 'B', 'lid': '14'})
            c.assert_fact({'id': c.s.gid + 2, 't': 'edge_label', 'p1': c.j.base_point, 'p2': c.j.p2, 'label_name': '+', 'lid': '14'})
            c.assert_fact({'id': c.s.gid + 3, 't': 'edge_label', 'p1': c.j.base_point, 'p2': c.j.p3, 'label_name': 'B', 'lid': '14'})
            c.s.gid += 4
            print('labeling')

    with state('labeling'):
        @to('visiting_3j')
        @when_all(c.j << (m.t == 'junction') & (m.j_t == '3j') & (m.visited == 'no'))
        def start_visit_3_junction(c):
            c.retract_fact(c.j); c.j.id = c.s.gid; c.j.visited = 'now'; c.assert_fact(c.j) 
            c.s.gid += 1
            print('visiting_3j')

        @to('visiting_2j')
        @when_all(pri(1),
                  c.j << (m.t == 'junction') & (m.j_t == '2j') & (m.visited == 'no'))
        def start_visit_2_junction(c):
            c.retract_fact(c.j); c.j.id = c.s.gid; c.j.visited = 'now'; c.assert_fact(c.j) 
            c.s.gid += 1
            print('visiting_2j')

        @to('end')
        @when_all(pri(2))
        def done_labeling(c):
            print('end {0}'.format(unix_time_millis(datetime.datetime.now()) - c.s.start_time))
            c.delete()

    with state('visiting_3j'):
        def visit_3j(c):
            print('Edge Label {0} {1} {2} {3}'.format(c.j.base_point, c.j.p1, c.l.n1, c.l.lid))
            print('Edge Label {0} {1} {2} {3}'.format(c.j.base_point, c.j.p2, c.l.n2, c.l.lid))
            print('Edge Label {0} {1} {2} {3}'.format(c.j.base_point, c.j.p3, c.l.n3, c.l.lid))
            c.assert_fact({'id': c.s.gid, 't': 'edge_label', 'p1': c.j.base_point, 'p2': c.j.p1, 'label_name': c.l.n1, 'lid': c.l.lid})
            c.assert_fact({'id': c.s.gid + 1, 't': 'edge_label', 'p1': c.j.base_point, 'p2': c.j.p2, 'label_name': c.l.n2, 'lid': c.l.lid})
            c.assert_fact({'id': c.s.gid + 2, 't': 'edge_label', 'p1': c.j.base_point, 'p2': c.j.p3, 'label_name': c.l.n3, 'lid': c.l.lid})
            c.s.gid += 3

        @to('visiting_3j')
        @when_all(c.j << (m.t == 'junction') & (m.visited == 'now') & (m.j_t == '3j'),
                  c.l << (m.t == 'label') & (m.name == c.j.name), 
                  c.el1 << (m.t == 'edge_label') & (m.p1 == c.j.p1) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n1),
                  c.el2 << (m.t == 'edge_label') & (m.p1 == c.j.p2) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n2),
                  c.el3 << (m.t == 'edge_label') & (m.p1 == c.j.p3) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n3),
                  none((m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.lid == c.l.lid)))
        def visit_3j_0(c):
            visit_3j(c)

        @to('visiting_3j')
        @when_all(c.j << (m.t == 'junction') & (m.visited == 'now') & (m.j_t == '3j'),
                  c.l << (m.t == 'label') & (m.name == c.j.name), 
                  none((m.t == 'edge_label') & (m.p1 == c.j.p1) & (m.p2 == c.j.base_point)),
                  c.el2 << (m.t == 'edge_label') & (m.p1 == c.j.p2) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n2),
                  c.el3 << (m.t == 'edge_label') & (m.p1 == c.j.p3) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n3),
                  none((m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.lid == c.l.lid)))
        def visit_3j_1(c):
            visit_3j(c)

        @to('visiting_3j')
        @when_all(c.j << (m.t == 'junction') & (m.visited == 'now') & (m.j_t == '3j'),
                  c.l << (m.t == 'label') & (m.name == c.j.name), 
                  c.el1 << (m.t == 'edge_label') & (m.p1 == c.j.p1) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n1),
                  none((m.t == 'edge_label') & (m.p1 == c.j.p2) & (m.p2 == c.j.base_point)),
                  c.el3 << (m.t == 'edge_label') & (m.p1 == c.j.p3) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n3),
                  none((m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.lid == c.l.lid)))
        def visit_3j_2(c):
            visit_3j(c)

        @to('visiting_3j')
        @when_all(c.j << (m.t == 'junction') & (m.visited == 'now') & (m.j_t == '3j'), 
                  c.l << (m.t == 'label') & (m.name == c.j.name), 
                  none((m.t == 'edge_label') & (m.p1 == c.j.p1) & (m.p2 == c.j.base_point)),
                  none((m.t == 'edge_label') & (m.p1 == c.j.p2) & (m.p2 == c.j.base_point)),
                  c.el3 << (m.t == 'edge_label') & (m.p1 == c.j.p3) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n3),
                  none((m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.lid == c.l.lid)))
        def visit_3j_3(c):
            visit_3j(c)

        @to('visiting_3j')
        @when_all(c.j << (m.t == 'junction') & (m.visited == 'now') & (m.j_t == '3j'),
                  c.l << (m.t == 'label') & (m.name == c.j.name), 
                  c.el1 << (m.t == 'edge_label') & (m.p1 == c.j.p1) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n1),
                  c.el2 << (m.t == 'edge_label') & (m.p1 == c.j.p2) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n2),
                  none((m.t == 'edge_label') & (m.p1 == c.j.p3) & (m.p2 == c.j.base_point)),
                  none((m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.lid == c.l.lid)))
        def visit_3j_4(c):
            visit_3j(c)

        @to('visiting_3j')
        @when_all(c.j << (m.t == 'junction') & (m.visited == 'now') & (m.j_t == '3j'),
                  c.l << (m.t == 'label') & (m.name == c.j.name), 
                  none((m.t == 'edge_label') & (m.p1 == c.j.p1) & (m.p2 == c.j.base_point)),
                  c.el2 << (m.t == 'edge_label') & (m.p1 == c.j.p2) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n2),
                  none((m.t == 'edge_label') & (m.p1 == c.j.p3) & (m.p2 == c.j.base_point)),
                  none((m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.lid == c.l.lid)))
        def visit_3j_5(c):
            visit_3j(c)

        @to('visiting_3j')
        @when_all(c.j << (m.t == 'junction') & (m.visited == 'now') & (m.j_t == '3j'),
                  c.l << (m.t == 'label') & (m.name == c.j.name), 
                  c.el1 << (m.t == 'edge_label') & (m.p1 == c.j.p1) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n1),
                  none((m.t == 'edge_label') & (m.p1 == c.j.p2) & (m.p2 == c.j.base_point)),
                  none((m.t == 'edge_label') & (m.p1 == c.j.p3) & (m.p2 == c.j.base_point)),
                  none((m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.lid == c.l.lid)))
        def visit_3j_6(c):
            visit_3j(c)

        @to('visiting_3j')
        @when_all(c.j << (m.t == 'junction') & (m.visited == 'now') & (m.j_t == '3j'),
                  c.l << (m.t == 'label') & (m.name == c.j.name), 
                  none((m.t == 'edge_label') & (m.p1 == c.j.p1) & (m.p2 == c.j.base_point)),
                  none((m.t == 'edge_label') & (m.p1 == c.j.p2) & (m.p2 == c.j.base_point)),
                  none((m.t == 'edge_label') & (m.p1 == c.j.p3) & (m.p2 == c.j.base_point)),
                  none((m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.lid == c.l.lid)))
        def visit_3j_7(c):
            visit_3j(c)

        @to('marking')
        @when_all(pri(1), (m.t == 'junction') & (m.visited == 'now') & (m.j_t == '3j'))
        def end_visit(c):
            print('marking')

    with state('visiting_2j'):
        def visit_2j(c):
            print('Edge Label {0} {1} {2} {3}'.format(c.j.base_point, c.j.p1, c.l.n1, c.l.lid))
            print('Edge Label {0} {1} {2} {3}'.format(c.j.base_point, c.j.p2, c.l.n2, c.l.lid))
            c.assert_fact({'id': c.s.gid, 't': 'edge_label', 'p1': c.j.base_point, 'p2': c.j.p1, 'label_name': c.l.n1, 'lid': c.l.lid})
            c.assert_fact({'id': c.s.gid + 1, 't': 'edge_label', 'p1': c.j.base_point, 'p2': c.j.p2, 'label_name': c.l.n2, 'lid': c.l.lid})
            c.s.gid += 2

        @to('visiting_2j')
        @when_all(c.j << (m.t == 'junction') & (m.visited == 'now') & (m.j_t == '2j'),
                  c.l << (m.t == 'label') & (m.name == c.j.name), 
                  c.el1 << (m.t == 'edge_label') & (m.p1 == c.j.p1) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n1),
                  c.el2 << (m.t == 'edge_label') & (m.p1 == c.j.p2) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n2),
                  none((m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.lid == c.l.lid)))
        def visit_2j_0(c):
            visit_2j(c)

        @to('visiting_2j')
        @when_all(c.j << (m.t == 'junction') & (m.visited == 'now') & (m.j_t == '2j'),
                  c.l << (m.t == 'label') & (m.name == c.j.name), 
                  none((m.t == 'edge_label') & (m.p1 == c.j.p1) & (m.p2 == c.j.base_point)),
                  c.el2 << (m.t == 'edge_label') & (m.p1 == c.j.p2) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n2),
                  none((m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.lid == c.l.lid)))
        def visit_2j_1(c):
            visit_2j(c)

        @to('visiting_2j')
        @when_all(c.j << (m.t == 'junction') & (m.visited == 'now') & (m.j_t == '2j'),
                  c.l << (m.t == 'label') & (m.name == c.j.name), 
                  c.el1 << (m.t == 'edge_label') & (m.p1 == c.j.p1) & (m.p2 == c.j.base_point) & (m.label_name == c.l.n1),
                  none((m.t == 'edge_label') & (m.p1 == c.j.p2) & (m.p2 == c.j.base_point)),
                  none((m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.lid == c.l.lid)))
        def visit_2j_2(c):
            visit_2j(c)

        @to('visiting_2j')
        @when_all(c.j << (m.t == 'junction') & (m.visited == 'now') & (m.j_t == '2j'),
                  c.l << (m.t == 'label') & (m.name == c.j.name), 
                  none((m.t == 'edge_label') & (m.p1 == c.j.p1) & (m.p2 == c.j.base_point)),
                  none((m.t == 'edge_label') & (m.p1 == c.j.p2) & (m.p2 == c.j.base_point)),
                  none((m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.lid == c.l.lid)))
        def visit_2j_3(c):
            visit_2j(c)

        @to('marking')
        @when_all(pri(1), (m.t == 'junction') & (m.visited == 'now') & (m.j_t == '2j'))
        def end_visit(c):
            print('marking')

    with state('marking'):
        @to('marking')
        @when_all(c.j << (m.t == 'junction') & (m.visited == 'now'),
                  c.e << (m.t == 'edge') & (m.p2 == c.j.base_point),
                  c.junction << (m.t == 'junction') & (m.base_point == c.e.p1) & (m.visited == 'yes'))
        def marking(c):
            c.retract_fact(c.junction); c.junction.id = c.s.gid; c.junction.visited = 'check'; c.assert_fact(c.junction) 
            c.s.gid += 1

        @to('marking')
        @when_all(pri(1), c.j << (m.t == 'junction') & (m.visited == 'now'))
        def stop_marking(c):
            c.retract_fact(c.j); c.j.id = c.s.gid; c.j.visited = 'yes'; c.assert_fact(c.j) 
            c.s.gid += 1

        @to('checking')
        @when_all(pri(2))
        def start_checking(c):
            print('checking')

    with state('checking'):
        @to('remove_label')
        @when_all(c.junction << (m.t == 'junction') & (m.visited == 'check'),
                  c.el1 << (m.t == 'edge_label') & (m.p1 == c.junction.base_point),
                  c.j << (m.t == 'junction') & (m.base_point == c.el1.p2) & (m.visited == 'yes'),
                  none((m.t == 'edge_label') & (m.p1 == c.el1.p2) & (m.p2 == c.junction.base_point) & (m.label_name == c.el1.label_name)))
        def checking(c):
            print('remove_label')
            c.assert_fact({'id': c.s.gid, 't': 'illegal', 'base_point': c.junction.base_point, 'lid': c.el1.lid})
            c.s.gid += 1

        @to('checking')
        @when_all(pri(1), c.j << (m.t == 'junction') & (m.visited == 'check'))
        def checking2(c):
            c.retract_fact(c.j); c.j.id = c.s.gid; c.j.visited = 'yes'; c.assert_fact(c.j) 
            c.s.gid += 1

        @to('labeling')
        @when_all(pri(2))
        def stop_checking(c):
            print('labeling')

    with state('remove_label'):
        @to('checking')
        @when_all(c.i << (m.t == 'illegal'),
                  c.j << (m.t == 'junction') & (m.j_t == '3j') & (m.base_point == c.i.base_point),
                  c.el1 << (m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.p2 == c.j.p1) & (m.lid == c.i.lid),
                  c.el2 << (m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.p2 == c.j.p2) & (m.lid == c.i.lid),
                  c.el3 << (m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.p2 == c.j.p3) & (m.lid == c.i.lid))
        def remove_label_3j(c):
            print('checking')
            c.retract_fact(c.i)
            c.retract_fact(c.el1)
            c.retract_fact(c.el2)
            c.retract_fact(c.el3)

        @to('checking')
        @when_all(c.i << (m.t == 'illegal'),
                  c.j << (m.t == 'junction') & (m.j_t == '2j') & (m.base_point == c.i.base_point),
                  c.el1 << (m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.p2 == c.j.p1) & (m.lid == c.i.lid),
                  c.el2 << (m.t == 'edge_label') & (m.p1 == c.j.base_point) & (m.p2 == c.j.p2) & (m.lid == c.i.lid))
        def remove_edge_2j(c):
            print('checking')
            c.retract_fact(c.i)
            c.retract_fact(c.el1)
            c.retract_fact(c.el2)

    state('end')

create_and_assert({'t':'label' ,'j_t':'2j' ,'name':'L' ,'lid':'1' ,'n1':'B' ,'n2':'B'})
create_and_assert({'t':'label' ,'j_t':'2j' ,'name':'L' ,'lid':'2' ,'n1':'+' ,'n2':'B'})
create_and_assert({'t':'label' ,'j_t':'2j' ,'name':'L' ,'lid':'3' ,'n1':'B' ,'n2':'+'})
create_and_assert({'t':'label' ,'j_t':'2j' ,'name':'L' ,'lid':'4' ,'n1':'-' ,'n2':'B'})
create_and_assert({'t':'label' ,'j_t':'2j' ,'name':'L' ,'lid':'5' ,'n1':'B' ,'n2':'-'})
create_and_assert({'t':'label' ,'j_t':'3j' ,'name':'fork' ,'lid':'6' ,'n1':'+' ,'n2':'+' ,'n3':'+'})
create_and_assert({'t':'label' ,'j_t':'3j' ,'name':'fork' ,'lid':'7' ,'n1':'-' ,'n2':'-' ,'n3':'-'})
create_and_assert({'t':'label' ,'j_t':'3j' ,'name':'fork' ,'lid':'8' ,'n1':'B' ,'n2':'-' ,'n3':'B'})
create_and_assert({'t':'label' ,'j_t':'3j' ,'name':'fork' ,'lid':'9' ,'n1':'-' ,'n2':'B' ,'n3':'B'})
create_and_assert({'t':'label' ,'j_t':'3j' ,'name':'fork' ,'lid':'10' ,'n1':'B' ,'n2':'B' ,'n3':'-'})
create_and_assert({'t':'label' ,'j_t':'3j' ,'name':'tee' ,'lid':'11' ,'n1':'B' ,'n2':'+' ,'n3':'B'})
create_and_assert({'t':'label' ,'j_t':'3j' ,'name':'tee' ,'lid':'12' ,'n1':'B' ,'n2':'-' ,'n3':'B'})
create_and_assert({'t':'label' ,'j_t':'3j' ,'name':'tee' ,'lid':'13' ,'n1':'B' ,'n2':'B' ,'n3':'B'})
create_and_assert({'t':'label' ,'j_t':'3j' ,'name':'arrow' ,'lid':'14' ,'n1':'B' ,'n2':'+' ,'n3':'B'})
create_and_assert({'t':'label' ,'j_t':'3j' ,'name':'arrow' ,'lid':'15' ,'n1':'-' ,'n2':'+' ,'n3':'-'})
create_and_assert({'t':'label' ,'j_t':'3j' ,'name':'arrow' ,'lid':'16' ,'n1':'+' ,'n2':'-' ,'n3':'+'})
create_and_post({'t':'line' ,'p1':50003 ,'p2':60003})
create_and_post({'t':'line' ,'p1':30005 ,'p2':30006})
create_and_post({'t':'line' ,'p1':80005 ,'p2':80006})
create_and_post({'t':'line' ,'p1':50008 ,'p2':60008})
create_and_post({'t':'line' ,'p1':0 ,'p2':20000})
create_and_post({'t':'line' ,'p1':20000 ,'p2':30000})
create_and_post({'t':'line' ,'p1':30000 ,'p2':40000})
create_and_post({'t':'line' ,'p1':0 ,'p2':2})
create_and_post({'t':'line' ,'p1':2 ,'p2':3})
create_and_post({'t':'line' ,'p1':3 ,'p2':4})
create_and_post({'t':'line' ,'p1':4 ,'p2':40004})
create_and_post({'t':'line' ,'p1':40004 ,'p2':40000})
create_and_post({'t':'line' ,'p1':40000 ,'p2':50001})
create_and_post({'t':'line' ,'p1':50001 ,'p2':50002})
create_and_post({'t':'line' ,'p1':50002 ,'p2':50003})
create_and_post({'t':'line' ,'p1':50003 ,'p2':50005})
create_and_post({'t':'line' ,'p1':50005 ,'p2':40004})
create_and_post({'t':'line' ,'p1':50005 ,'p2':30005})
create_and_post({'t':'line' ,'p1':30005 ,'p2':20005})
create_and_post({'t':'line' ,'p1':20005 ,'p2':10005})
create_and_post({'t':'line' ,'p1':10005 ,'p2':4})
create_and_post({'t':'line' ,'p1':60000 ,'p2':80000})
create_and_post({'t':'line' ,'p1':80000 ,'p2':90000})
create_and_post({'t':'line' ,'p1':90000 ,'p2':100000})
create_and_post({'t':'line' ,'p1':60000 ,'p2':60002})
create_and_post({'t':'line' ,'p1':60002 ,'p2':60003})
create_and_post({'t':'line' ,'p1':60003 ,'p2':60004})
create_and_post({'t':'line' ,'p1':60004 ,'p2':100004})
create_and_post({'t':'line' ,'p1':100004 ,'p2':100000})
create_and_post({'t':'line' ,'p1':100000 ,'p2':110001})
create_and_post({'t':'line' ,'p1':110001 ,'p2':110002})
create_and_post({'t':'line' ,'p1':110002 ,'p2':110003})
create_and_post({'t':'line' ,'p1':110003 ,'p2':110005})
create_and_post({'t':'line' ,'p1':110005 ,'p2':100004})
create_and_post({'t':'line' ,'p1':110005 ,'p2':90005})
create_and_post({'t':'line' ,'p1':90005 ,'p2':80005})
create_and_post({'t':'line' ,'p1':80005 ,'p2':70005})
create_and_post({'t':'line' ,'p1':70005 ,'p2':60004})
create_and_post({'t':'line' ,'p1':6 ,'p2':20006})
create_and_post({'t':'line' ,'p1':20006 ,'p2':30006})
create_and_post({'t':'line' ,'p1':30006 ,'p2':40006})
create_and_post({'t':'line' ,'p1':6 ,'p2':8})
create_and_post({'t':'line' ,'p1':8 ,'p2':9})
create_and_post({'t':'line' ,'p1':9 ,'p2':10})
create_and_post({'t':'line' ,'p1':10 ,'p2':40010})
create_and_post({'t':'line' ,'p1':40010 ,'p2':40006})
create_and_post({'t':'line' ,'p1':40006 ,'p2':50007})
create_and_post({'t':'line' ,'p1':50007 ,'p2':50008})
create_and_post({'t':'line' ,'p1':50008 ,'p2':50009})
create_and_post({'t':'line' ,'p1':50009 ,'p2':50011})
create_and_post({'t':'line' ,'p1':50011 ,'p2':40010})
create_and_post({'t':'line' ,'p1':50011 ,'p2':30011})
create_and_post({'t':'line' ,'p1':30011 ,'p2':20011})
create_and_post({'t':'line' ,'p1':20011 ,'p2':10011})
create_and_post({'t':'line' ,'p1':10011 ,'p2':10})
create_and_post({'t':'line' ,'p1':60006 ,'p2':80006})
create_and_post({'t':'line' ,'p1':80006 ,'p2':90006})
create_and_post({'t':'line' ,'p1':90006 ,'p2':100006})
create_and_post({'t':'line' ,'p1':60006 ,'p2':60008})
create_and_post({'t':'line' ,'p1':60008 ,'p2':60009})
create_and_post({'t':'line' ,'p1':60009 ,'p2':60010})
create_and_post({'t':'line' ,'p1':60010 ,'p2':100010})
create_and_post({'t':'line' ,'p1':100010 ,'p2':100006})
create_and_post({'t':'line' ,'p1':100006 ,'p2':110007})
create_and_post({'t':'line' ,'p1':110007 ,'p2':110008})
create_and_post({'t':'line' ,'p1':110008 ,'p2':110009})
create_and_post({'t':'line' ,'p1':110009 ,'p2':110011})
create_and_post({'t':'line' ,'p1':110011 ,'p2':100010})
create_and_post({'t':'line' ,'p1':110011 ,'p2':90011})
create_and_post({'t':'line' ,'p1':90011 ,'p2':80011})
create_and_post({'t':'line' ,'p1':80011 ,'p2':70011})
create_and_post({'t':'line' ,'p1':70011 ,'p2':60010})
create_and_post({'t':'line' ,'p1':170003 ,'p2':180003})
create_and_post({'t':'line' ,'p1':150005 ,'p2':150006})
create_and_post({'t':'line' ,'p1':200005 ,'p2':200006})
create_and_post({'t':'line' ,'p1':170008 ,'p2':180008})
create_and_post({'t':'line' ,'p1':120000 ,'p2':140000})
create_and_post({'t':'line' ,'p1':140000 ,'p2':150000})
create_and_post({'t':'line' ,'p1':150000 ,'p2':160000})
create_and_post({'t':'line' ,'p1':120000 ,'p2':120002})
create_and_post({'t':'line' ,'p1':120002 ,'p2':120003})
create_and_post({'t':'line' ,'p1':120003 ,'p2':120004})
create_and_post({'t':'line' ,'p1':120004 ,'p2':160004})
create_and_post({'t':'line' ,'p1':160004 ,'p2':160000})
create_and_post({'t':'line' ,'p1':160000 ,'p2':170001})
create_and_post({'t':'line' ,'p1':170001 ,'p2':170002})
create_and_post({'t':'line' ,'p1':170002 ,'p2':170003})
create_and_post({'t':'line' ,'p1':170003 ,'p2':170005})
create_and_post({'t':'line' ,'p1':170005 ,'p2':160004})
create_and_post({'t':'line' ,'p1':170005 ,'p2':150005})
create_and_post({'t':'line' ,'p1':150005 ,'p2':140005})
create_and_post({'t':'line' ,'p1':140005 ,'p2':130005})
create_and_post({'t':'line' ,'p1':130005 ,'p2':120004})
create_and_post({'t':'line' ,'p1':180000 ,'p2':200000})
create_and_post({'t':'line' ,'p1':200000 ,'p2':210000})
create_and_post({'t':'line' ,'p1':210000 ,'p2':220000})
create_and_post({'t':'line' ,'p1':180000 ,'p2':180002})
create_and_post({'t':'line' ,'p1':180002 ,'p2':180003})
create_and_post({'t':'line' ,'p1':180003 ,'p2':180004})
create_and_post({'t':'line' ,'p1':180004 ,'p2':220004})
create_and_post({'t':'line' ,'p1':220004 ,'p2':220000})
create_and_post({'t':'line' ,'p1':220000 ,'p2':230001})
create_and_post({'t':'line' ,'p1':230001 ,'p2':230002})
create_and_post({'t':'line' ,'p1':230002 ,'p2':230003})
create_and_post({'t':'line' ,'p1':230003 ,'p2':230005})
create_and_post({'t':'line' ,'p1':230005 ,'p2':220004})
create_and_post({'t':'line' ,'p1':230005 ,'p2':210005})
create_and_post({'t':'line' ,'p1':210005 ,'p2':200005})
create_and_post({'t':'line' ,'p1':200005 ,'p2':190005})
create_and_post({'t':'line' ,'p1':190005 ,'p2':180004})
create_and_post({'t':'line' ,'p1':120006 ,'p2':140006})
create_and_post({'t':'line' ,'p1':140006 ,'p2':150006})
create_and_post({'t':'line' ,'p1':150006 ,'p2':160006})
create_and_post({'t':'line' ,'p1':120006 ,'p2':120008})
create_and_post({'t':'line' ,'p1':120008 ,'p2':120009})
create_and_post({'t':'line' ,'p1':120009 ,'p2':120010})
create_and_post({'t':'line' ,'p1':120010 ,'p2':160010})
create_and_post({'t':'line' ,'p1':160010 ,'p2':160006})
create_and_post({'t':'line' ,'p1':160006 ,'p2':170007})
create_and_post({'t':'line' ,'p1':170007 ,'p2':170008})
create_and_post({'t':'line' ,'p1':170008 ,'p2':170009})
create_and_post({'t':'line' ,'p1':170009 ,'p2':170011})
create_and_post({'t':'line' ,'p1':170011 ,'p2':160010})
create_and_post({'t':'line' ,'p1':170011 ,'p2':150011})
create_and_post({'t':'line' ,'p1':150011 ,'p2':140011})
create_and_post({'t':'line' ,'p1':140011 ,'p2':130011})
create_and_post({'t':'line' ,'p1':130011 ,'p2':120010})
create_and_post({'t':'line' ,'p1':180006 ,'p2':200006})
create_and_post({'t':'line' ,'p1':200006 ,'p2':210006})
create_and_post({'t':'line' ,'p1':210006 ,'p2':220006})
create_and_post({'t':'line' ,'p1':180006 ,'p2':180008})
create_and_post({'t':'line' ,'p1':180008 ,'p2':180009})
create_and_post({'t':'line' ,'p1':180009 ,'p2':180010})
create_and_post({'t':'line' ,'p1':180010 ,'p2':220010})
create_and_post({'t':'line' ,'p1':220010 ,'p2':220006})
create_and_post({'t':'line' ,'p1':220006 ,'p2':230007})
create_and_post({'t':'line' ,'p1':230007 ,'p2':230008})
create_and_post({'t':'line' ,'p1':230008 ,'p2':230009})
create_and_post({'t':'line' ,'p1':230009 ,'p2':230011})
create_and_post({'t':'line' ,'p1':230011 ,'p2':220010})
create_and_post({'t':'line' ,'p1':230011 ,'p2':210011})
create_and_post({'t':'line' ,'p1':210011 ,'p2':200011})
create_and_post({'t':'line' ,'p1':200011 ,'p2':190011})
create_and_post({'t':'line' ,'p1':190011 ,'p2':180010})
create_and_post({'t':'line' ,'p1':110003 ,'p2':120003})
create_and_post({'t':'line' ,'p1':90005 ,'p2':90006})
create_and_post({'t':'line' ,'p1':140005 ,'p2':140006})
create_and_post({'t':'line' ,'p1':110008 ,'p2':120008})
create_and_post({'t':'line' ,'p1':290003 ,'p2':300003})
create_and_post({'t':'line' ,'p1':270005 ,'p2':270006})
create_and_post({'t':'line' ,'p1':320005 ,'p2':320006})
create_and_post({'t':'line' ,'p1':290008 ,'p2':300008})
create_and_post({'t':'line' ,'p1':240000 ,'p2':260000})
create_and_post({'t':'line' ,'p1':260000 ,'p2':270000})
create_and_post({'t':'line' ,'p1':270000 ,'p2':280000})
create_and_post({'t':'line' ,'p1':240000 ,'p2':240002})
create_and_post({'t':'line' ,'p1':240002 ,'p2':240003})
create_and_post({'t':'line' ,'p1':240003 ,'p2':240004})
create_and_post({'t':'line' ,'p1':240004 ,'p2':280004})
create_and_post({'t':'line' ,'p1':280004 ,'p2':280000})
create_and_post({'t':'line' ,'p1':280000 ,'p2':290001})
create_and_post({'t':'line' ,'p1':290001 ,'p2':290002})
create_and_post({'t':'line' ,'p1':290002 ,'p2':290003})
create_and_post({'t':'line' ,'p1':290003 ,'p2':290005})
create_and_post({'t':'line' ,'p1':290005 ,'p2':280004})
create_and_post({'t':'line' ,'p1':290005 ,'p2':270005})
create_and_post({'t':'line' ,'p1':270005 ,'p2':260005})
create_and_post({'t':'line' ,'p1':260005 ,'p2':250005})
create_and_post({'t':'line' ,'p1':250005 ,'p2':240004})
create_and_post({'t':'line' ,'p1':300000 ,'p2':320000})
create_and_post({'t':'line' ,'p1':320000 ,'p2':330000})
create_and_post({'t':'line' ,'p1':330000 ,'p2':340000})
create_and_post({'t':'line' ,'p1':300000 ,'p2':300002})
create_and_post({'t':'line' ,'p1':300002 ,'p2':300003})
create_and_post({'t':'line' ,'p1':300003 ,'p2':300004})
create_and_post({'t':'line' ,'p1':300004 ,'p2':340004})
create_and_post({'t':'line' ,'p1':340004 ,'p2':340000})
create_and_post({'t':'line' ,'p1':340000 ,'p2':350001})
create_and_post({'t':'line' ,'p1':350001 ,'p2':350002})
create_and_post({'t':'line' ,'p1':350002 ,'p2':350003})
create_and_post({'t':'line' ,'p1':350003 ,'p2':350005})
create_and_post({'t':'line' ,'p1':350005 ,'p2':340004})
create_and_post({'t':'line' ,'p1':350005 ,'p2':330005})
create_and_post({'t':'line' ,'p1':330005 ,'p2':320005})
create_and_post({'t':'line' ,'p1':320005 ,'p2':310005})
create_and_post({'t':'line' ,'p1':310005 ,'p2':300004})
create_and_post({'t':'line' ,'p1':240006 ,'p2':260006})
create_and_post({'t':'line' ,'p1':260006 ,'p2':270006})
create_and_post({'t':'line' ,'p1':270006 ,'p2':280006})
create_and_post({'t':'line' ,'p1':240006 ,'p2':240008})
create_and_post({'t':'line' ,'p1':240008 ,'p2':240009})
create_and_post({'t':'line' ,'p1':240009 ,'p2':240010})
create_and_post({'t':'line' ,'p1':240010 ,'p2':280010})
create_and_post({'t':'line' ,'p1':280010 ,'p2':280006})
create_and_post({'t':'line' ,'p1':280006 ,'p2':290007})
create_and_post({'t':'line' ,'p1':290007 ,'p2':290008})
create_and_post({'t':'line' ,'p1':290008 ,'p2':290009})
create_and_post({'t':'line' ,'p1':290009 ,'p2':290011})
create_and_post({'t':'line' ,'p1':290011 ,'p2':280010})
create_and_post({'t':'line' ,'p1':290011 ,'p2':270011})
create_and_post({'t':'line' ,'p1':270011 ,'p2':260011})
create_and_post({'t':'line' ,'p1':260011 ,'p2':250011})
create_and_post({'t':'line' ,'p1':250011 ,'p2':240010})
create_and_post({'t':'line' ,'p1':300006 ,'p2':320006})
create_and_post({'t':'line' ,'p1':320006 ,'p2':330006})
create_and_post({'t':'line' ,'p1':330006 ,'p2':340006})
create_and_post({'t':'line' ,'p1':300006 ,'p2':300008})
create_and_post({'t':'line' ,'p1':300008 ,'p2':300009})
create_and_post({'t':'line' ,'p1':300009 ,'p2':300010})
create_and_post({'t':'line' ,'p1':300010 ,'p2':340010})
create_and_post({'t':'line' ,'p1':340010 ,'p2':340006})
create_and_post({'t':'line' ,'p1':340006 ,'p2':350007})
create_and_post({'t':'line' ,'p1':350007 ,'p2':350008})
create_and_post({'t':'line' ,'p1':350008 ,'p2':350009})
create_and_post({'t':'line' ,'p1':350009 ,'p2':350011})
create_and_post({'t':'line' ,'p1':350011 ,'p2':340010})
create_and_post({'t':'line' ,'p1':350011 ,'p2':330011})
create_and_post({'t':'line' ,'p1':330011 ,'p2':320011})
create_and_post({'t':'line' ,'p1':320011 ,'p2':310011})
create_and_post({'t':'line' ,'p1':310011 ,'p2':300010})
create_and_post({'t':'line' ,'p1':230003 ,'p2':240003})
create_and_post({'t':'line' ,'p1':210005 ,'p2':210006})
create_and_post({'t':'line' ,'p1':260005 ,'p2':260006})
create_and_post({'t':'line' ,'p1':230008 ,'p2':240008})
create_and_post({'t':'line' ,'p1':410003 ,'p2':420003})
create_and_post({'t':'line' ,'p1':390005 ,'p2':390006})
create_and_post({'t':'line' ,'p1':440005 ,'p2':440006})
create_and_post({'t':'line' ,'p1':410008 ,'p2':420008})
create_and_post({'t':'line' ,'p1':360000 ,'p2':380000})
create_and_post({'t':'line' ,'p1':380000 ,'p2':390000})
create_and_post({'t':'line' ,'p1':390000 ,'p2':400000})
create_and_post({'t':'line' ,'p1':360000 ,'p2':360002})
create_and_post({'t':'line' ,'p1':360002 ,'p2':360003})
create_and_post({'t':'line' ,'p1':360003 ,'p2':360004})
create_and_post({'t':'line' ,'p1':360004 ,'p2':400004})
create_and_post({'t':'line' ,'p1':400004 ,'p2':400000})
create_and_post({'t':'line' ,'p1':400000 ,'p2':410001})
create_and_post({'t':'line' ,'p1':410001 ,'p2':410002})
create_and_post({'t':'line' ,'p1':410002 ,'p2':410003})
create_and_post({'t':'line' ,'p1':410003 ,'p2':410005})
create_and_post({'t':'line' ,'p1':410005 ,'p2':400004})
create_and_post({'t':'line' ,'p1':410005 ,'p2':390005})
create_and_post({'t':'line' ,'p1':390005 ,'p2':380005})
create_and_post({'t':'line' ,'p1':380005 ,'p2':370005})
create_and_post({'t':'line' ,'p1':370005 ,'p2':360004})
create_and_post({'t':'line' ,'p1':420000 ,'p2':440000})
create_and_post({'t':'line' ,'p1':440000 ,'p2':450000})
create_and_post({'t':'line' ,'p1':450000 ,'p2':460000})
create_and_post({'t':'line' ,'p1':420000 ,'p2':420002})
create_and_post({'t':'line' ,'p1':420002 ,'p2':420003})
create_and_post({'t':'line' ,'p1':420003 ,'p2':420004})
create_and_post({'t':'line' ,'p1':420004 ,'p2':460004})
create_and_post({'t':'line' ,'p1':460004 ,'p2':460000})
create_and_post({'t':'line' ,'p1':460000 ,'p2':470001})
create_and_post({'t':'line' ,'p1':470001 ,'p2':470002})
create_and_post({'t':'line' ,'p1':470002 ,'p2':470003})
create_and_post({'t':'line' ,'p1':470003 ,'p2':470005})
create_and_post({'t':'line' ,'p1':470005 ,'p2':460004})
create_and_post({'t':'line' ,'p1':470005 ,'p2':450005})
create_and_post({'t':'line' ,'p1':450005 ,'p2':440005})
create_and_post({'t':'line' ,'p1':440005 ,'p2':430005})
create_and_post({'t':'line' ,'p1':430005 ,'p2':420004})
create_and_post({'t':'line' ,'p1':360006 ,'p2':380006})
create_and_post({'t':'line' ,'p1':380006 ,'p2':390006})
create_and_post({'t':'line' ,'p1':390006 ,'p2':400006})
create_and_post({'t':'line' ,'p1':360006 ,'p2':360008})
create_and_post({'t':'line' ,'p1':360008 ,'p2':360009})
create_and_post({'t':'line' ,'p1':360009 ,'p2':360010})
create_and_post({'t':'line' ,'p1':360010 ,'p2':400010})
create_and_post({'t':'line' ,'p1':400010 ,'p2':400006})
create_and_post({'t':'line' ,'p1':400006 ,'p2':410007})
create_and_post({'t':'line' ,'p1':410007 ,'p2':410008})
create_and_post({'t':'line' ,'p1':410008 ,'p2':410009})
create_and_post({'t':'line' ,'p1':410009 ,'p2':410011})
create_and_post({'t':'line' ,'p1':410011 ,'p2':400010})
create_and_post({'t':'line' ,'p1':410011 ,'p2':390011})
create_and_post({'t':'line' ,'p1':390011 ,'p2':380011})
create_and_post({'t':'line' ,'p1':380011 ,'p2':370011})
create_and_post({'t':'line' ,'p1':370011 ,'p2':360010})
create_and_post({'t':'line' ,'p1':420006 ,'p2':440006})
create_and_post({'t':'line' ,'p1':440006 ,'p2':450006})
create_and_post({'t':'line' ,'p1':450006 ,'p2':460006})
create_and_post({'t':'line' ,'p1':420006 ,'p2':420008})
create_and_post({'t':'line' ,'p1':420008 ,'p2':420009})
create_and_post({'t':'line' ,'p1':420009 ,'p2':420010})
create_and_post({'t':'line' ,'p1':420010 ,'p2':460010})
create_and_post({'t':'line' ,'p1':460010 ,'p2':460006})
create_and_post({'t':'line' ,'p1':460006 ,'p2':470007})
create_and_post({'t':'line' ,'p1':470007 ,'p2':470008})
create_and_post({'t':'line' ,'p1':470008 ,'p2':470009})
create_and_post({'t':'line' ,'p1':470009 ,'p2':470011})
create_and_post({'t':'line' ,'p1':470011 ,'p2':460010})
create_and_post({'t':'line' ,'p1':470011 ,'p2':450011})
create_and_post({'t':'line' ,'p1':450011 ,'p2':440011})
create_and_post({'t':'line' ,'p1':440011 ,'p2':430011})
create_and_post({'t':'line' ,'p1':430011 ,'p2':420010})
create_and_post({'t':'line' ,'p1':350003 ,'p2':360003})
create_and_post({'t':'line' ,'p1':330005 ,'p2':330006})
create_and_post({'t':'line' ,'p1':380005 ,'p2':380006})
create_and_post({'t':'line' ,'p1':350008 ,'p2':360008})
create_and_post({'t':'line' ,'p1':530003 ,'p2':540003})
create_and_post({'t':'line' ,'p1':510005 ,'p2':510006})
create_and_post({'t':'line' ,'p1':560005 ,'p2':560006})
create_and_post({'t':'line' ,'p1':530008 ,'p2':540008})
create_and_post({'t':'line' ,'p1':480000 ,'p2':500000})
create_and_post({'t':'line' ,'p1':500000 ,'p2':510000})
create_and_post({'t':'line' ,'p1':510000 ,'p2':520000})
create_and_post({'t':'line' ,'p1':480000 ,'p2':480002})
create_and_post({'t':'line' ,'p1':480002 ,'p2':480003})
create_and_post({'t':'line' ,'p1':480003 ,'p2':480004})
create_and_post({'t':'line' ,'p1':480004 ,'p2':520004})
create_and_post({'t':'line' ,'p1':520004 ,'p2':520000})
create_and_post({'t':'line' ,'p1':520000 ,'p2':530001})
create_and_post({'t':'line' ,'p1':530001 ,'p2':530002})
create_and_post({'t':'line' ,'p1':530002 ,'p2':530003})
create_and_post({'t':'line' ,'p1':530003 ,'p2':530005})
create_and_post({'t':'line' ,'p1':530005 ,'p2':520004})
create_and_post({'t':'line' ,'p1':530005 ,'p2':510005})
create_and_post({'t':'line' ,'p1':510005 ,'p2':500005})
create_and_post({'t':'line' ,'p1':500005 ,'p2':490005})
create_and_post({'t':'line' ,'p1':490005 ,'p2':480004})
create_and_post({'t':'line' ,'p1':540000 ,'p2':560000})
create_and_post({'t':'line' ,'p1':560000 ,'p2':570000})
create_and_post({'t':'line' ,'p1':570000 ,'p2':580000})
create_and_post({'t':'line' ,'p1':540000 ,'p2':540002})
create_and_post({'t':'line' ,'p1':540002 ,'p2':540003})
create_and_post({'t':'line' ,'p1':540003 ,'p2':540004})
create_and_post({'t':'line' ,'p1':540004 ,'p2':580004})
create_and_post({'t':'line' ,'p1':580004 ,'p2':580000})
create_and_post({'t':'line' ,'p1':580000 ,'p2':590001})
create_and_post({'t':'line' ,'p1':590001 ,'p2':590002})
create_and_post({'t':'line' ,'p1':590002 ,'p2':590003})
create_and_post({'t':'line' ,'p1':590003 ,'p2':590005})
create_and_post({'t':'line' ,'p1':590005 ,'p2':580004})
create_and_post({'t':'line' ,'p1':590005 ,'p2':570005})
create_and_post({'t':'line' ,'p1':570005 ,'p2':560005})
create_and_post({'t':'line' ,'p1':560005 ,'p2':550005})
create_and_post({'t':'line' ,'p1':550005 ,'p2':540004})
create_and_post({'t':'line' ,'p1':480006 ,'p2':500006})
create_and_post({'t':'line' ,'p1':500006 ,'p2':510006})
create_and_post({'t':'line' ,'p1':510006 ,'p2':520006})
create_and_post({'t':'line' ,'p1':480006 ,'p2':480008})
create_and_post({'t':'line' ,'p1':480008 ,'p2':480009})
create_and_post({'t':'line' ,'p1':480009 ,'p2':480010})
create_and_post({'t':'line' ,'p1':480010 ,'p2':520010})
create_and_post({'t':'line' ,'p1':520010 ,'p2':520006})
create_and_post({'t':'line' ,'p1':520006 ,'p2':530007})
create_and_post({'t':'line' ,'p1':530007 ,'p2':530008})
create_and_post({'t':'line' ,'p1':530008 ,'p2':530009})
create_and_post({'t':'line' ,'p1':530009 ,'p2':530011})
create_and_post({'t':'line' ,'p1':530011 ,'p2':520010})
create_and_post({'t':'line' ,'p1':530011 ,'p2':510011})
create_and_post({'t':'line' ,'p1':510011 ,'p2':500011})
create_and_post({'t':'line' ,'p1':500011 ,'p2':490011})
create_and_post({'t':'line' ,'p1':490011 ,'p2':480010})
create_and_post({'t':'line' ,'p1':540006 ,'p2':560006})
create_and_post({'t':'line' ,'p1':560006 ,'p2':570006})
create_and_post({'t':'line' ,'p1':570006 ,'p2':580006})
create_and_post({'t':'line' ,'p1':540006 ,'p2':540008})
create_and_post({'t':'line' ,'p1':540008 ,'p2':540009})
create_and_post({'t':'line' ,'p1':540009 ,'p2':540010})
create_and_post({'t':'line' ,'p1':540010 ,'p2':580010})
create_and_post({'t':'line' ,'p1':580010 ,'p2':580006})
create_and_post({'t':'line' ,'p1':580006 ,'p2':590007})
create_and_post({'t':'line' ,'p1':590007 ,'p2':590008})
create_and_post({'t':'line' ,'p1':590008 ,'p2':590009})
create_and_post({'t':'line' ,'p1':590009 ,'p2':590011})
create_and_post({'t':'line' ,'p1':590011 ,'p2':580010})
create_and_post({'t':'line' ,'p1':590011 ,'p2':570011})
create_and_post({'t':'line' ,'p1':570011 ,'p2':560011})
create_and_post({'t':'line' ,'p1':560011 ,'p2':550011})
create_and_post({'t':'line' ,'p1':550011 ,'p2':540010})
create_and_post({'t':'line' ,'p1':470003 ,'p2':480003})
create_and_post({'t':'line' ,'p1':450005 ,'p2':450006})
create_and_post({'t':'line' ,'p1':500005 ,'p2':500006})
create_and_post({'t':'line' ,'p1':470008 ,'p2':480008})
create_and_post({'t':'ready'})

