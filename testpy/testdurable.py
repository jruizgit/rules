from durable.lang import *

def provide_durability(host, port = None, redisHostName = None, options = None):

    def store_message_callback(ruleset, sid, mid, actionType, content):
        print('store_message_callback')

    def delete_message_callback(ruleset, sid, mid):
        print('delete_message_callback')

    def queue_message_callback(ruleset, sid, actionType, content):
        print('queue_message_callback {0}, {1}, {2}, {3}'.fromat(ruleset, sid, actionType, content))
        return 0

    def get_idle_state_callback(ruleset, sid):
        print('get_idle_state_callback')


    def get_queued_messages_callback(ruleset):
        print('get_queued_messages_callback')


    host.set_store_message_callback(store_message_callback)
    host.set_delete_message_callback(delete_message_callback)
    host.set_queue_message_callback(queue_message_callback)
    host.set_get_idle_state_callback(get_idle_state_callback)
    host.set_get_queued_messages_callback(get_queued_messages_callback)


provide_durability(get_host())


with ruleset('test'):
    
    @when_all(c.first << m.subject == 'Hello',
              c.second << m.subject == 'World')
    def say_hello(c):
        if not c.s.count: 
            c.s.count = 1
        else:
            c.s.count += 1

        print('{0} {1} from: {2} count: {3}'.format(c.first.subject, c.second.subject, c.s.sid, c.s.count))

post('test', { 'subject': 'Hello', 'sid': 'a' })
post('test', { 'subject': 'World', 'sid': 'b' })

post('test', { 'subject': 'Hello', 'sid': 'b' })
post('test', { 'subject': 'World', 'sid': 'a' })
