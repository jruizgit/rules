{
    'targets': [
    {
        'target_name': 'rulesjs',
        'sources': [ 'src/rulesjs/rules.cc' ],
        'include_dirs': ["<!(node -e \"require('nan')\")"],
        'dependencies': [ 'src/rules/rules.gyp:rules' ],
        'defines': [ '_GNU_SOURCE' ],
        'cflags': [ '-Wall', '-O3' ]
    }
  ]
}
