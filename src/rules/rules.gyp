{
    'targets': [
    {
        'target_name': 'rules',
        'type': 'static_library',
        'include_dirs': [ '.' ],
        'defines': [ '_GNU_SOURCE' ],
        'direct_dependent_settings': {
            'include_dirs': [ '.' ]
        },
        'sources': [
            'json.c',
            'rete.c',
            'events.c',
            'state.c',
            'regex.c'
        ],
        'conditions': [
        ['OS=="mac"', {
            'xcode_settings': {
                'GCC_C_LANGUAGE_STANDARD': 'c99'
            }
        }],
        ['OS=="solaris"', {
            'cflags+': [ '-std=c99' ]
        }],
        ['OS=="linux"', {
            'cflags+': [ '-std=c99' ]
        }]
      ]
    }
  ]
}
