from durable.lang import *
import datetime

with statechart('miss_manners'):
    with state('start'):
        @to('assign')
        @when_all(m.t == 'guest')
        def assign_first_seating(c):
            c.assert_fact({'t': 'seating',
                           'id': c.s.g_count,
                           's_id': c.s.count, 
                           'p_id': 0, 
                           'path': 1, 
                           'left_seat': 1, 
                           'left_guest_name': c.m.name,
                           'right_seat': 1,
                           'right_guest_name': c.m.name})
            c.assert_fact({'t': 'path',
                           'id': c.s.g_count + 1,
                           'p_id': c.s.count, 
                           'seat': 1, 
                           'guest_name': c.m.name})
            c.s.count += 1
            c.s.g_count += 2
            print('assign {0}, {1}'.format(c.m.name, datetime.datetime.now().strftime('%I:%M:%S%p')))

    with state('assign'):
        @to('make')
        @when_all(c.seating << (m.t == 'seating') & 
                               (m.path == 1),
                  c.right_guest << (m.t == 'guest') & 
                                   (m.name == c.seating.right_guest_name),
                  c.left_guest << (m.t == 'guest') & 
                                  (m.sex != c.right_guest.sex) & 
                                  (m.hobby == c.right_guest.hobby),
                  none((m.t == 'path') & 
                       (m.p_id == c.seating.s_id) & 
                       (m.guest_name == c.left_guest.name)),
                  none((m.t == 'chosen') & 
                       (m.c_id == c.seating.s_id) & 
                       (m.guest_name == c.left_guest.name) & 
                       (m.hobby == c.right_guest.hobby)))
        def find_seating(c):
            c.assert_fact({'t': 'seating',
                           'id': c.s.g_count,
                           's_id': c.s.count, 
                           'p_id': c.seating.s_id, 
                           'path': 0, 
                           'left_seat': c.seating.right_seat, 
                           'left_guest_name': c.seating.right_guest_name,
                           'right_seat': c.seating.right_seat + 1,
                           'right_guest_name': c.left_guest.name})
            c.assert_fact({'t': 'path',
                           'id': c.s.g_count + 1,
                           'p_id': c.s.count, 
                           'seat': c.seating.right_seat + 1, 
                           'guest_name': c.left_guest.name})
            c.assert_fact({'t': 'chosen',
                           'id': c.s.g_count + 2,
                           'c_id': c.seating.s_id,
                           'guest_name': c.left_guest.name,
                           'hobby': c.right_guest.hobby})
            c.s.count += 1
            c.s.g_count += 3

    with state('make'):
        @to('make')
        @when_all(c.seating << (m.t == 'seating') & 
                               (m.path == 0),
                  c.path << (m.t == 'path') & 
                            (m.p_id == c.seating.p_id),
                  none((m.t == 'path') & 
                       (m.p_id == c.seating.s_id) & 
                       (m.guest_name == c.path.guest_name)))
        def make_path(c):
            c.assert_fact({'t': 'path',
                           'id': c.s.g_count,
                           'p_id': c.seating.s_id, 
                           'seat': c.path.seat, 
                           'guest_name': c.path.guest_name})
            c.s.g_count += 1

        @to('check')
        @when_all(pri(1), (m.t == 'seating') & (m.path == 0))
        def path_done(c):
            c.retract_fact(c.m)
            c.m.id = c.s.g_count
            c.m.path = 1
            c.assert_fact(c.m)
            c.s.g_count += 1
            print('path sid: {0}, pid: {1}, left guest: {2}, right guest {3}, {4}'.format(c.m.s_id, c.m.p_id, c.m.left_guest_name, c.m.right_guest_name, datetime.datetime.now().strftime('%I:%M:%S%p')))

    with state('check'):
        @to('end')
        @when_all(c.last_seat << m.t == 'last_seat', 
                 (m.t == 'seating') & (m.right_seat == c.last_seat.seat))
        def done(c):
            print('end {0}'.format(datetime.datetime.now().strftime('%I:%M:%S%p')))
        
        to('assign')

    state('end')

    @when_start
    def start(host):
    	host.assert_fact('miss_manners', {'id': 1, 'sid': 1, 't': 'guest', 'name': 'n1', 'sex': 'm', 'hobby': 'h3'})
        host.assert_fact('miss_manners', {'id': 2, 'sid': 1, 't': 'guest', 'name': 'n1', 'sex': 'm', 'hobby': 'h2'})
        host.assert_fact('miss_manners', {'id': 3, 'sid': 1, 't': 'guest', 'name': 'n2', 'sex': 'm', 'hobby': 'h2'})
        host.assert_fact('miss_manners', {'id': 4, 'sid': 1, 't': 'guest', 'name': 'n2', 'sex': 'm', 'hobby': 'h3'})
        host.assert_fact('miss_manners', {'id': 5, 'sid': 1, 't': 'guest', 'name': 'n3', 'sex': 'm', 'hobby': 'h1'})
        host.assert_fact('miss_manners', {'id': 6, 'sid': 1, 't': 'guest', 'name': 'n3', 'sex': 'm', 'hobby': 'h2'})
        host.assert_fact('miss_manners', {'id': 7, 'sid': 1, 't': 'guest', 'name': 'n3', 'sex': 'm', 'hobby': 'h3'})
        host.assert_fact('miss_manners', {'id': 8, 'sid': 1, 't': 'guest', 'name': 'n4', 'sex': 'f', 'hobby': 'h3'})
        host.assert_fact('miss_manners', {'id': 9, 'sid': 1, 't': 'guest', 'name': 'n4', 'sex': 'f', 'hobby': 'h2'})
        host.assert_fact('miss_manners', {'id': 10, 'sid': 1, 't': 'guest', 'name': 'n5', 'sex': 'f', 'hobby': 'h1'})
        host.assert_fact('miss_manners', {'id': 11, 'sid': 1, 't': 'guest', 'name': 'n5', 'sex': 'f', 'hobby': 'h2'})
        host.assert_fact('miss_manners', {'id': 12, 'sid': 1, 't': 'guest', 'name': 'n5', 'sex': 'f', 'hobby': 'h3'})
        host.assert_fact('miss_manners', {'id': 13, 'sid': 1, 't': 'guest', 'name': 'n6', 'sex': 'f', 'hobby': 'h3'})
        host.assert_fact('miss_manners', {'id': 14, 'sid': 1, 't': 'guest', 'name': 'n6', 'sex': 'f', 'hobby': 'h1'})
        host.assert_fact('miss_manners', {'id': 15, 'sid': 1, 't': 'guest', 'name': 'n6', 'sex': 'f', 'hobby': 'h2'})
        host.assert_fact('miss_manners', {'id': 16, 'sid': 1, 't': 'guest', 'name': 'n7', 'sex': 'f', 'hobby': 'h3'})
        host.assert_fact('miss_manners', {'id': 17, 'sid': 1, 't': 'guest', 'name': 'n7', 'sex': 'f', 'hobby': 'h2'})
        host.assert_fact('miss_manners', {'id': 18, 'sid': 1, 't': 'guest', 'name': 'n8', 'sex': 'm', 'hobby': 'h3'})
        host.assert_fact('miss_manners', {'id': 19, 'sid': 1, 't': 'guest', 'name': 'n8', 'sex': 'm', 'hobby': 'h1'})
        host.assert_fact('miss_manners', {'id': 20, 'sid': 1, 't': 'last_seat', 'seat': 8})
