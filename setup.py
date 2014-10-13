from distutils.core import setup, Extension

rules = Extension('rules',
                    sources = ['src/rulespy/rules.c'],
                    include_dirs=['src/rules'],
                    extra_link_args=['build/release/rules.a', 'build/release/hiredis.a'])

setup (name = 'RulesEngine',
       version = '1.0',
       description = 'Durable Rules Engine',
       packages = ['durable'],
       package_dir = {'': 'libpy'},
       package_data={'': ['ux/*']},
       ext_modules = [rules])