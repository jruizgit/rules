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
                           'left_seat': 1, 
                           'left_guest_name': c.m.name,
                           'right_seat': 1,
                           'right_guest_name': c.m.name})
            c.assert_fact({'t': 'path',
                           'id': c.s.g_count + 1,
                           'p_id': c.s.count, 
                           'seat': 1, 
                           'guest_name': c.m.name})
            c.assert_fact('miss_manners.make',
                          {'t': 'path',
                           'id': c.s.g_count + 1,
                           'p_id': c.s.count, 
                           'seat': 1, 
                           'guest_name': c.m.name})
            c.s.count += 1
            c.s.g_count += 2
            print('assign {0}, {1}'.format(c.m.name, datetime.datetime.now().strftime('%I:%M:%S%p')))

    with state('assign'):
        @to('begin_make')
        @when_all(c.seating << (m.t == 'seating'),
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
                           'left_seat': c.seating.right_seat, 
                           'left_guest_name': c.seating.right_guest_name,
                           'right_seat': c.seating.right_seat + 1,
                           'right_guest_name': c.left_guest.name})
            c.assert_fact({'t': 'path',
                           'id': c.s.g_count + 1,
                           'p_id': c.s.count, 
                           'seat': c.seating.right_seat + 1, 
                           'guest_name': c.left_guest.name})
            c.assert_fact('miss_manners.make',
                          {'t': 'seating',
                           'id': c.s.g_count,
                           's_id': c.s.count, 
                           'p_id': c.seating.s_id, 
                           'left_seat': c.seating.right_seat, 
                           'left_guest_name': c.seating.right_guest_name,
                           'right_seat': c.seating.right_seat + 1,
                           'right_guest_name': c.left_guest.name})
            c.assert_fact('miss_manners.make',
                          {'t': 'path',
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
            
    with state('begin_make'):
        with to('end_make').when_all(pri(0)):
            with ruleset('make'):                   
                @when_all(count(1001),
                          c.seating << (m.t == 'seating'),
                          c.path << (m.t == 'path') & 
                                    (m.p_id == c.seating.p_id),
                          none((m.t == 'path') & 
                               (m.p_id == c.seating.s_id) & 
                               (m.guest_name == c.path.guest_name)), 
                          s.g_count > 1000)
                def make_path(c):
                    if (c.m):
                        for i in range(len(c.m)):
                            c.assert_fact({'t': 'path',
                                           'id': c.s.g_count,
                                           'p_id': c.m[i].seating.s_id, 
                                           'seat': c.m[i].path.seat, 
                                           'guest_name': c.m[i].path.guest_name})
                            c.assert_fact('miss_manners',
                                          {'t': 'path',
                                       'sid': 1, 
                                       'id': c.s.g_count,
                                       'p_id': c.m[i].seating.s_id, 
                                       'seat': c.m[i].path.seat, 
                                       'guest_name': c.m[i].path.guest_name})
                            c.s.g_count += 1
                    else:
                        c.assert_fact({'t': 'path',
                                       'id': c.s.g_count,
                                       'p_id': c.seating.s_id, 
                                       'seat': c.path.seat, 
                                       'guest_name': c.path.guest_name})
                        c.assert_fact('miss_manners',
                                      {'t': 'path',
                                   'sid': 1, 
                                   'id': c.s.g_count,
                                   'p_id': c.seating.s_id, 
                                   'seat': c.path.seat, 
                                   'guest_name': c.path.guest_name})

                        c.s.g_count += 1
                        

                @when_all(pri(1), 
                         c.seating << (m.t == 'seating'), 
                         (s.g_count > 1000))
                def path_done(c):
                    c.retract_fact(c.seating)
                    c.signal({'t': 'go', 
                              'id': c.s.g_count,
                              'g_count': c.s.g_count, 
                              's_id': c.seating.s_id, 
                              'p_id': c.seating.p_id,
                              'left_guest_name': c.seating.left_guest_name,
                              'right_guest_name': c.seating.right_guest_name})
                    
    with state('end_make'):
        @to('check')
        @when_all(m.t == 'go')
        def path_done(c):
            c.s.g_count = c.m.g_count + 1
            print('path sid: {0}, pid: {1}, left guest: {2}, right guest {3}, {4}'.format(c.m.s_id, c.m.p_id, c.m.left_guest_name, c.m.right_guest_name, datetime.datetime.now().strftime('%I:%M:%S%p')))

    with state('check'):
        @to('end')
        @when_all(c.last_seat << m.t == 'last_seat', 
                 (m.t == 'seating') & (m.right_seat == c.last_seat.seat))
        def done(c):
            print('end {0}'.format(datetime.datetime.now().strftime('%I:%M:%S%p')))
        
        to('assign').when_all(pri(1))

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
        host.patch_state('miss_manners', {'sid': 1, 'label': 'start', 'count': 0, 'g_count': 1000})
        host.patch_state('miss_manners.make', {'sid': 0, 'label': 'start', 'count': 0, 'g_count': 1000})

run_all(['/tmp/redis0.sock', '/tmp/redis1.sock'])

