{
  'targets': [
    {
      'target_name': 'win32_interop',
      'type': 'static_library',
      'configurations': {
        'Release': {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ExceptionHandling': 1
            }
          }
        }
      },
      'direct_dependent_settings': {
        'include_dirs': [ '.' ],
      },
      'sources': [
        './win32_interop/win32_error.c',
        './win32_interop/win32_ansi.c',
        './win32_interop/win32_fdapi.cpp',
        './win32_interop/win32_fdapi_crt.cpp',
        './win32_interop/win32_rfdmap.cpp',
        './win32_interop/win32_variadic_functor.cpp',
        './win32_interop/win32_common.cpp',
      ],
      'defines!' : [ '_HAS_EXCEPTIONS=0' ]
    }
  ]
}