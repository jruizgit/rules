from setuptools import setup, Extension
from codecs import open
from os import path

rules = Extension('rules',
                    sources = ['src/rulespy/rules.c'],
                    include_dirs=['src/rules'],
                    extra_link_args=['build/release/rules.a', 'build/release/hiredis.a'])

here = path.abspath(path.dirname(__file__))
with open(path.join(here, 'README.txt'), encoding='utf-8') as f:
    long_description = f.read()

setup (
    name = 'durable_rules',
    version = '0.31.16',
    description = 'for real time analytics',
    long_description=long_description,
    url='https://github.com/jruizgit/rules',
    author='Jesus Ruiz',
    author_email='jr3791@live.com',
    license='MIT',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'Topic :: Software Development :: Libraries',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 2.6',
        'Programming Language :: Python :: 2.7',
    ],
    keywords='rules_engine rete forward_chaining event_stream state_machine workflow streaming_analytics',
    install_requires=['werkzeug'],
    packages = ['durable'],
    package_dir = {'': 'libpy'},
    ext_modules = [rules]
)
