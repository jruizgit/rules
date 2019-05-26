require 'mkmf'

RbConfig::MAKEFILE_CONFIG['CC'] = ENV['CC'] if ENV['CC']

rules_include = File.join(File.dirname(__FILE__), %w{.. .. src rules})
rules_lib = File.join(File.dirname(__FILE__), %w{.. .. src rules})

RbConfig::CONFIG['configure_args'] =~ /with-make-prog\=(\w+)/
make_program = $1 || ENV['make']
make_program ||= case RUBY_PLATFORM
when /mswin/
  'nmake'
when /(bsd|solaris)/
  'gmake'
else
  'make'
end

# Make sure rules is built...
Dir.chdir(rules_lib) do
  success = system("#{make_program} static")
  raise "Building rules failed" if !success
end

# Statically link to hiredis (mkmf can't do this for us)
$CFLAGS << " -I#{rules_lib} "
$LDFLAGS << " -L#{rules_lib} -lrules"

have_func("rb_thread_fd_select")
create_makefile('src/rulesrb/rules')
