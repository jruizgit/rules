import rules
import json

print('books1 *****')

handle = rules.create_ruleset(5, 'books1',  json.dumps({
    'ship': {
        'all': [
            {'order': {
                '$and': [
                    {'$lte': {'amount': 1000}},
                    {'country': 'US'},
                    {'currency': 'US'},
                    {'seller': 'bookstore'} 
                ]
            }},
            {'available':  {
                '$and': [
                    {'item': 'book'},
                    {'country': 'US'},
                    {'seller': 'bookstore'},
                    {'status': 'available'} 
                ]
            }}
        ]
    }
}))

rules.bind_ruleset(None , 0 , '/tmp/redis0.sock', handle)

rules.assert_event(handle, json.dumps({
    'id': 1,
    'sid': 'first',
    'name': 'John Smith',
    'address': '1111 NE 22, Seattle, Wa',
    'phone': '206678787',
    'country': 'US',
    'currency': 'US',
    'seller': 'bookstore',
    'item': 'book',
    'reference': '75323',
    'amount': 500
}))

rules.assert_event(handle, json.dumps({
    'id': 1,
    'sid': 'first',
    'item': 'book',
    'status': 'available',
    'country': 'US',
    'seller': 'bookstore'
}))

result = rules.start_action(handle)

print(repr(json.loads(result[0])))
print(repr(json.loads(result[1])))

rules.complete_action(handle, result[2], result[0])
rules.delete_ruleset(handle)

print('books2 ******')

handle = rules.create_ruleset(5, 'books2',  json.dumps({
    'ship': {
        'all': [ 
            {'m': {'$and': [
                {'country': 'US'},
                {'seller': 'bookstore'},
                {'currency': 'US'},
                {'$lte': {'amount': 1000}},
            ]}}
        ]
    },
    'order': {
        'all': [
            {'m': {'$and': [
                {'country': 'US'},
                {'seller': 'bookstore'},
                {'currency': 'US'},
                {'$lte': {'amount': 1000}},
            ]}}
        ]
    }
}))

rules.bind_ruleset(None , 0 , '/tmp/redis0.sock', handle)

rules.assert_event(handle, json.dumps({
    'id': 1,
    'sid': 'first',
    'name': 'John Smith',
    'address': '1111 NE 22, Seattle, Wa',
    'phone': '206678787',
    'country': 'US',
    'currency': 'US',
    'seller': 'bookstore',
    'item': 'book',
    'reference': '75323',
    'amount': 500
}))

result = rules.start_action(handle)

print(repr(json.loads(result[0])))
print(repr(json.loads(result[1])))

rules.complete_action(handle, result[2], result[0])
rules.delete_ruleset(handle)

print('books3 ******')

handle = rules.create_ruleset(5, 'books3',  json.dumps({
    'ship': {
        'all': [{'m': {'$nex': {'label': 1}}}]
   }
}))

rules.bind_ruleset(None , 0 , '/tmp/redis0.sock', handle)

rules.assert_event(handle, json.dumps({
    'id': 1,
    'sid': 'first',
    'name': 'John Smith',
    'address': '1111 NE 22, Seattle, Wa',
}))

result = rules.start_action(handle)

print(repr(json.loads(result[0])))
print(repr(json.loads(result[1])))

rules.complete_action(handle, result[2], result[0])
rules.delete_ruleset(handle)

print('books4 ******')

handle = rules.create_ruleset(5, 'books4', json.dumps({
    'ship': {
        'count': 5,
        'all': [
            {'m': {'$and': [
                {'$lte': {'amount': 1000}},
                {'subject': 'approve'}
            ]}}
        ]
    }
}))

rules.bind_ruleset(None , 0 , '/tmp/redis0.sock', handle)

rules.assert_events(handle, json.dumps([
    {'id': '0', 'sid': 1, 'subject': 'approve', 'amount': 100}, 
    {'id': '1', 'sid': 1, 'subject': 'approve', 'amount': 100},
    {'id': '2', 'sid': 1, 'subject': 'approve', 'amount': 100},
    {'id': '3', 'sid': 1, 'subject': 'approve', 'amount': 100},
    {'id': '4', 'sid': 1, 'subject': 'approve', 'amount': 100}, 
]))

result = rules.start_action(handle)

print(repr(json.loads(result[0])))
print(repr(json.loads(result[1])))

rules.complete_action(handle, result[2], result[0])
rules.delete_ruleset(handle)

print('approval1 ******')

handle = rules.create_ruleset(5, 'approval1',  json.dumps({
    'r1': {
        'all': [
            {'a$any': [
                {'b': {'subject': 'approve'}},
                {'c': {'subject': 'review'}}
            ]},
            {'d$any': [
                {'e': {'$lt': {'total': 1000}}},
                {'f': {'$lt': {'amount': 1000}}}
            ]}
        ]
    }
}))

rules.bind_ruleset(None , 0 , '/tmp/redis0.sock', handle)

rules.assert_event(handle, json.dumps({
    'id': 3,
    'sid': 'second',
    'subject': 'approve'
}))

rules.assert_event(handle, json.dumps({
    'id': 4,
    'sid': 'second',
    'amount': 100
}))

result = rules.start_action(handle)

print(repr(json.loads(result[0])))
print(repr(json.loads(result[1])))

rules.complete_action(handle, result[2], result[0])
rules.delete_ruleset(handle)

print('approval2 ******')

handle = rules.create_ruleset(5, 'approval2',  json.dumps({
    'r2': {
        'any': [
            {'a$all': [
                {'b': {'subject': 'approve'}},
                {'c': {'subject': 'review'}}
            ]},
            {'d$all': [
                {'e': {'$lt': {'total': 1000}}},
                {'f': {'$lt': {'amount': 1000}}}
            ]}
        ]
    }
}))

rules.bind_ruleset(None , 0 , '/tmp/redis0.sock', handle)

rules.assert_event(handle, json.dumps({
    'id': 5,
    'sid': 'second',
    'subject': 'approve'
}))

rules.assert_event(handle, json.dumps({
    'id': 6,
    'sid': 'second',
    'subject': 'review'
}))

result = rules.start_action(handle)

print(repr(json.loads(result[0])))
print(repr(json.loads(result[1])))

rules.complete_action(handle, result[2], result[0])

rules.assert_event(handle, json.dumps({
    'id': 7,
    'sid': 'second',
    'total': 100
}))

rules.assert_event(handle, json.dumps({
    'id': 8,
    'sid': 'second',
    'amount': 100
}))

result = rules.start_action(handle)

print(repr(json.loads(result[0])))
print(repr(json.loads(result[1])))

rules.delete_ruleset(handle)

handle = rules.create_ruleset(5, 'poc',  json.dumps({
    'test': {
        'all': [{'m': {'$lt': {'value': 0.2}}}]
   }
}))
rules.bind_ruleset(None , 0 , '/tmp/redis0.sock', handle)

rules.assert_events(handle, json.dumps([{
    'id': 1,
    'sid': 'sid',
    'value': 0.3,
    'is_anomaly': 0,
    'timestamp':1
}, 
{
    'id': 2,
    'sid': 'sid',
    'value': 0.1,
    'is_anomaly': 0,
    'timestamp': 2
},
{
    'id': 3,
    'sid': 'sid',
    'value': 0.09,
    'is_anomaly': 0,
    'timestamp':3
},
{
    'id': 4,
    'sid': 'sid',
    'value': 0.16,
    'is_anomaly': 0,
    'timestamp': 4
}
]))

result = rules.start_action(handle)

if result:
    print(repr(json.loads(result[0])))
    print(repr(json.loads(result[1])))

rules.complete_action(handle, result[2], result[0])

result = rules.start_action(handle)

if result:
    print(repr(json.loads(result[0])))
    print(repr(json.loads(result[1])))

rules.complete_action(handle, result[2], result[0])

result = rules.start_action(handle)

if result:
    print(repr(json.loads(result[0])))
    print(repr(json.loads(result[1])))

rules.complete_action(handle, result[2], result[0])

rules.delete_ruleset(handle)

