from durable.lang import *
import redis
import time
import datetime
import traceback
import sys

def unix_time(dt):
    epoch = datetime.datetime.utcfromtimestamp(0)
    delta = dt - epoch
    return int(delta.total_seconds() * 1000.0)

def provide_durability(host, redis_host_name = 'localhost', port = 6379):
    r = redis.StrictRedis(redis_host_name, port, charset="utf-8", decode_responses=True)

    def get_hset_name(ruleset, sid):
        return 'h!{0}!{1}'.format(ruleset, sid)

    def get_list_name(ruleset, sid):
        return 'l!{0}!{1}'.format(ruleset, sid)

    def get_sset_name(ruleset):
        return 's!{0}'.format(ruleset)

    def format_message(action_type, content):
        return '{0},{1}'.format(action_type, content)

    def format_messages(results):
        if not results:
            return '[]'

        messages = ['[']
        for i in range(0, len(results)):
            messages.append(results[i])
            if i < (len(results) - 1):
                messages.append(',')

        messages.append(']') 
        return ''.join(messages)

    def store_message_callback(ruleset, sid, mid, action_type, content):
        try:
            r.hset(get_hset_name(ruleset, sid), mid, format_message(action_type, content))
        except BaseException as e:
            print(e)
            return 601

        return 0

    def delete_message_callback(ruleset, sid, mid):
        try:
            r.hdel(get_hset_name(ruleset, sid), mid)
        except BaseException as e:
            print(e)
            return 602

        return 0

    def queue_message_callback(ruleset, sid, action_type, content):
        try:
            result = r.zscore(get_sset_name(ruleset), sid)
            if not result:
                r.zadd(get_sset_name(ruleset), {sid: unix_time(datetime.datetime.now())})

            r.rpush(get_list_name(ruleset, sid), format_message(action_type, content))
        except BaseException as e:
            print(e)
            return 603 

        return 0

    def get_queued_messages_callback(ruleset, sid):
        try:
            r.zadd(get_sset_name(ruleset), {sid: unix_time(datetime.datetime.now()) + 5000})
            messages = r.lrange(get_list_name(ruleset, sid), 0, -1)
            if len(messages):
                r.delete(get_list_name(ruleset, sid))
                host.complete_get_queued_messages(ruleset, sid, format_messages(messages))
        except BaseException as e:
            print(e)
            return 604

        return 0

    def get_idle_state_callback(ruleset):
        try:
            results = r.zrangebyscore(get_sset_name(ruleset), 0, unix_time(datetime.datetime.now()))
            if (results and len(results)):
                sid = results[0]
                messages = r.hvals(get_hset_name(ruleset, sid))
                host.complete_get_idle_state(ruleset, sid, format_messages(messages))
        except BaseException as e:
            print(e)
            return 606

        return 0
            

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


time.sleep(30)