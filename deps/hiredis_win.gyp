{
  'targets': [
    {
      'target_name': 'hiredis',
      'type': 'static_library',
      'dependencies': [ 'win32_interop.gyp:win32_interop' ],
      'direct_dependent_settings': {
        'include_dirs': [ '.' ],
      },
      'sources': [
        './hiredis_win/hiredis.c',
        './hiredis_win/net.c',
        './hiredis_win/sds.c',
        './hiredis_win/async.c',
      ],
      'configurations': {
        'Release': {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ExceptionHandling': 1
            }
          }
        }
      }
    }
  ]
}
