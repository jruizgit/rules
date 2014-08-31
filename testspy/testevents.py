import rules
import json

print('books1 *****')

handle = rules.createRuleset('books1',  json.dumps({ 
    'ship': {
        'whenAll': {
            'order': { 
                '$and': [
                    { '$lte': { 'amount': 1000 } },
                    { 'country': 'US' },
                    { 'currency': 'US' },
                    { 'seller': 'bookstore'} 
                ]
            },
            'available':  { 
                '$and': [
                    { 'item': 'book' },
                    { 'country': 'US' },
                    { 'seller': 'bookstore' },
                    { 'status': 'available'} 
                ]
            }
        },
        'run': 'ship'
    }
}))

rules.bindRuleset(handle, None , 0 , '/tmp/redis.sock')

rules.assertEvent(handle, json.dumps({
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

rules.assertEvent(handle, json.dumps({
    'id': 1,
    'sid': 'first',
    'item': 'book',
    'status': 'available',
    'country': 'US',
    'seller': 'bookstore'
}))

result = rules.startAction(handle)

print(repr(json.loads(result[0])))
print(repr(json.loads(result[1])))

rules.completeAction(handle, result[2], result[0])
rules.deleteRuleset(handle)

print('books2 ******')

handle = rules.createRuleset('books2',  json.dumps({ 
    'ship': {
        'when': { 
            '$and': [
                { 'country': 'US' },
                { 'seller': 'bookstore'},
                { 'currency': 'US' },
                { '$lte': { 'amount': 1000 } },
            ]
        },
        'run': 'ship'
    },
    'order': {
        'when': { 
            '$and': [
                { 'country': 'US' },
                { 'seller': 'bookstore'},
                { 'currency': 'US' },
                { '$lte': { 'amount': 1000 } },
            ]
        },
        'run': 'order'
    }
}))

rules.bindRuleset(handle, None , 0 , '/tmp/redis.sock')

rules.assertEvent(handle, json.dumps({
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

result = rules.startAction(handle)

print(repr(json.loads(result[0])))
print(repr(json.loads(result[1])))

rules.completeAction(handle, result[2], result[0])
rules.deleteRuleset(handle)

print('books3 ******')

handle = rules.createRuleset('books3',  json.dumps({ 
    'ship': {
        'when': { '$nex': { 'label': 1 }},
        'run': 'ship'
    }
}))

rules.bindRuleset(handle, None , 0 , '/tmp/redis.sock')

rules.assertEvent(handle, json.dumps({
    'id': 1,
    'sid': 'first',
    'name': 'John Smith',
    'address': '1111 NE 22, Seattle, Wa',
}))

result = rules.startAction(handle)

print(repr(json.loads(result[0])))
print(repr(json.loads(result[1])))

rules.completeAction(handle, result[2], result[0])
rules.deleteRuleset(handle)

print('books4 ******')

handle = rules.createRuleset('books4', json.dumps({ 
    'ship': {
        'whenSome': { 
            '$and': [
                { '$lte': { 'amount': 1000 } },
                { 'subject': 'approve' }
            ]
        },
        'run': 'ship'
    }
}))

rules.bindRuleset(handle, None , 0 , '/tmp/redis.sock')

rules.assertEvents(handle, json.dumps([
    { 'id': '0', 'sid': 1, 'subject': 'approve', 'amount': 100 }, 
    { 'id': '1', 'sid': 1, 'subject': 'approve', 'amount': 100 },
    { 'id': '2', 'sid': 1, 'subject': 'approve', 'amount': 100 },
    { 'id': '3', 'sid': 1, 'subject': 'approve', 'amount': 100 },
    { 'id': '4', 'sid': 1, 'subject': 'approve', 'amount': 100 }, 
]))

result = rules.startAction(handle)

print(repr(json.loads(result[0])))
print(repr(json.loads(result[1])))

rules.completeAction(handle, result[2], result[0])
rules.deleteRuleset(handle)

print('approval1 ******')

handle = rules.createRuleset('approval1',  json.dumps({ 
    'r1': {
        'whenAll': {
            'a$any': {
                'b': { 'subject': 'approve' },
                'c': { 'subject': 'review' }
            },
            'd$any': {
                'e': { '$lt': { 'total': 1000 }},
                'f': { '$lt': { 'amount': 1000 }}
            }
        },
        'run': 'unitTest'
    }
}))

rules.bindRuleset(handle, None , 0 , '/tmp/redis.sock')

rules.assertEvent(handle, json.dumps({
    'id': 3,
    'sid': 'second',
    'subject': 'approve'
}))

rules.assertEvent(handle, json.dumps({
    'id': 4,
    'sid': 'second',
    'amount': 100
}))

result = rules.startAction(handle)

print(repr(json.loads(result[0])))
print(repr(json.loads(result[1])))

rules.completeAction(handle, result[2], result[0])
rules.deleteRuleset(handle)

print('approval2 ******')

handle = rules.createRuleset('approval2',  json.dumps({ 
    'r2': {
        'whenAny': {
            'a$all': {
                'b': { 'subject': 'approve' },
                'c': { 'subject': 'review' }
            },
            'd$all': {
                'e': { '$lt': { 'total': 1000 }},
                'f': { '$lt': { 'amount': 1000 }}
            }
        },
        'run': 'unitTest'
    }
}))

rules.bindRuleset(handle, None , 0 , '/tmp/redis.sock')

rules.assertEvent(handle, json.dumps({
    'id': 5,
    'sid': 'second',
    'subject': 'approve'
}))

rules.assertEvent(handle, json.dumps({
    'id': 6,
    'sid': 'second',
    'subject': 'review'
}))

result = rules.startAction(handle)

print(repr(json.loads(result[0])))
print(repr(json.loads(result[1])))

rules.completeAction(handle, result[2], result[0])
rules.deleteRuleset(handle)



