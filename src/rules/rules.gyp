{
    'targets': [
    {
        'target_name': 'rules',
        'type': 'static_library',
        'include_dirs': [ '.' ],
        'direct_dependent_settings': {
            'include_dirs': [ '.' ]
        },
        'dependencies': [ '../../deps/hiredis.gyp:hiredis' ],
        'sources': [
            'json.c',
            'rete.c',
            'net.c',
            'events.c',
            'state.c'
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
