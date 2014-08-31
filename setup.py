from distutils.core import setup, Extension

module1 = Extension('rules',
                    sources = ['src/rulespy/rules.c'],
                    include_dirs=['src/rules'],
                    extra_link_args=['build/release/rules.a', 'build/release/hiredis.a'])

setup (name = 'RulesEngine',
       version = '1.0',
       description = 'Durable Rules Engine',
       ext_modules = [module1])