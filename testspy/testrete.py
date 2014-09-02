import rules
import json

handle = rules.create_ruleset('rules',  json.dumps({ 
    'r1': {
        'whenSome': {'$and': [{'amount': 10000}, {'subject': 'approve'}]}, 
        'run': 'pending' 
   }
}))

rules.delete_ruleset(handle)
print('created rules1')

handle = rules.create_ruleset('rules',  json.dumps({ 
    'r1': {
        'when': {'$and': [{'amount': 10000}, {'subject': 'approve'}]}, 
        'run': 'pending' 
   }
}))

rules.delete_ruleset(handle)
print('created rules2')

handle = rules.create_ruleset('rules',  json.dumps({ 
    'r1': {
        'when': {'$or': [{'amount': 10000}, {'subject': 'ok'}]}, 
        'run': 'pending' 
   }
}))

rules.delete_ruleset(handle)
print('created rules3')

handle = rules.create_ruleset('rules',  json.dumps({ 
    'r1': {
        'whenAll': {
            'a': {'$lte': {'amount': 10}}, 
            'b': {'subject': 'yes'} 
       }, 
        'run': 'pending' 
   }
}))

rules.delete_ruleset(handle)
print('created rules4')

handle = rules.create_ruleset('rules',  json.dumps({ 
    'r1': {
        'when': {'$lte': {'amount': 10000}},  
        'run': 'pending' 
   }
}))

rules.delete_ruleset(handle)
print('created rules5')

handle = rules.create_ruleset('rules',  json.dumps({ 
    'r1': {
        'when': {'$lte': {'number': 10000}}, 
        'run': 'pending' 
   }, 
    'r2': {
        'when': {'$gte': {'amount': 1}}, 
        'run': 'done' 
   }
}))

rules.delete_ruleset(handle)
print('created rules6')

handle = rules.create_ruleset('rules',  json.dumps({
    'r1': {
        'whenAll': {
            'a$any': {
                'b': {'subject': 'approve'},
                'c': {'subject': 'review'}
           },
            'd$any': {
                'e': {'$lt': {'total': 1000}},
                'f': {'$lt': {'amount': 1000}}
           }
       },
        'run': 'unitTest'
   }
}))

rules.delete_ruleset(handle)
print('created rules7')

handle = rules.create_ruleset('rules',  json.dumps({
    'r1': {
        'whenAny': {
            'a$all': {
                'b': {'subject': 'approve'},
                'c': {'subject': 'review'}
           },
            'd$all': {
                'e': {'$lt': {'total': 1000}},
                'f': {'$lt': {'amount': 1000}}
           }
       },
        'run': 'unitTest'
   }
}))

rules.delete_ruleset(handle)
print('created rules8')

handle = rules.create_ruleset('rules',  json.dumps({
    'r1': {
        'whenAll': {
            'a$all': {
                'b': {'subject': 'approve'},
                'c': {'subject': 'review'}
           },
            'd$all': {
                'e': {'$lt': {'total': 1000}},
                'f': {'$lt': {'amount': 1000}}
           }
       },
        'run': 'unitTest'
   }
}))

rules.delete_ruleset(handle)
print('created rules9')

handle = rules.create_ruleset('rules',  json.dumps({
    'r1': {
        'whenAny': {
            'a$any': {
                'b': {'subject': 'approve'},
                'c': {'subject': 'review'}
           },
            'd$any': {
                'e': {'$lt': {'total': 1000}},
                'f': {'$lt': {'amount': 1000}}
           }
       },
        'run': 'unitTest'
   }
}))

rules.delete_ruleset(handle)
print('created rules10')

handle = rules.create_ruleset('rules',  json.dumps({
    'r1': {
        'whenAny': {
            'c': {'subject': 'review'},
            'd$all': {
                'e': {'$lt': {'total': 1000}},
                'f': {'$lt': {'amount': 1000}}
           }
       },
        'run': 'unitTest'
   }
}))

rules.delete_ruleset(handle)
print('created rules11')

handle = rules.create_ruleset('rules',  json.dumps({
    'r1': {
        'whenAny': {
            'c': {'subject': 'review'},
            'd$all': {
                'e': {'$lt': {'total': 1000}},
                'f': {'$lt': {'amount': 1000}}
           },
            'b': {'subject': 'approve'}
       },
        'run': 'unitTest'
   }
}))

rules.delete_ruleset(handle)
print('created rules12')

handle = rules.create_ruleset('rules',  json.dumps({
    'r1': {
        'whenAll': {
            'c': {'subject': 'review'},
            'd$any': {
                'e': {'$lt': {'total': 1000}},
                'f': {'$lt': {'amount': 1000}}
           }
       },
        'run': 'unitTest'
   }
}))

rules.delete_ruleset(handle)
print('created rules13')