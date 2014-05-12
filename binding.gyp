{
    'targets': [
    {
        'target_name': 'rules',
        'sources': [ 'src/rulesjs/rules.cc' ],
        'dependencies': [ 'src/rules/rules.gyp:rules' ],
        'defines': [ '_GNU_SOURCE' ],
        'cflags': [ '-Wall', '-O3' ]
    }
  ]
}
