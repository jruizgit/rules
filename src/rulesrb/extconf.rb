require 'mkmf'

rules_include = File.join(File.dirname(__FILE__), %w{.. .. src rules})
puts(rules_include)
rules_lib = File.join(File.dirname(__FILE__), %w{.. .. build release})
puts(rules_lib)
# Statically link to hiredis (mkmf can't do this for us)
$CFLAGS << " -I#{rules_include}"
$LDFLAGS << " #{rules_lib}/rules.a #{rules_lib}/hiredis.a"

create_makefile('rules')