try:
  from setuptools import setup, Extension
  from setuptools.command import install_lib as _install_lib
except ImportError:
  from distutils.core import setup, Extension
  from distutils.command import install_lib as _install_lib
from codecs import open
from os import path
from os import environ

environ['CFLAGS'] = '-std=c99 -D_GNU_SOURCE'

# Patch "install_lib" command to run build_clib before build_ext
# to properly work with easy_install.
# See: http://bugs.python.org/issue5243
class install_lib(_install_lib.install_lib):
  def build(self):
    if not self.skip_build:
      if self.distribution.has_pure_modules():
        self.run_command('build_py')
        if self.distribution.has_c_libraries():
          self.run_command('build_clib')
        if self.distribution.has_ext_modules():
          self.run_command('build_ext')

rules_lib = ('rules_py', 
             {'sources': ['deps/hiredis/%s.c' % src for src in ('hiredis', 'net', 'sds', 'read')] + 
             ['src/rules/%s.c' % src for src in ('json', 'net', 'rete', 'state', 'events')]})

rules = Extension('rules',
                  sources = ['src/rulespy/rules.c'],
                  include_dirs=['src/rules'])

here = path.abspath(path.dirname(__file__)) + '/docs/py'
with open(path.join(here, 'README.txt'), encoding='utf-8') as f:
    long_description = f.read()

setup (
    name = 'durable_rules',
    version = '0.33.68',
    description = 'for real time analytics (a Python Rules Engine)',
    long_description=long_description,
    url='https://github.com/jruizgit/rules',
    author='Jesus Ruiz',
    author_email='jr3791@live.com',
    license='MIT',
    classifiers=[
        'Operating System :: MacOS',
        'Operating System :: POSIX',
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'Topic :: Software Development :: Libraries',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: C',
        'Programming Language :: Python :: 2.6',
        'Programming Language :: Python :: 2.7',
    ],
    keywords='rules engine rete forward chaining event stream state machine workflow streaming analytics',
    install_requires=['werkzeug'],
    packages = ['durable'],
    package_dir = {'': 'libpy'},
    libraries = [rules_lib],
    ext_modules = [rules],
    # Override 'install_lib' command
    cmdclass={'install_lib': install_lib},
)
