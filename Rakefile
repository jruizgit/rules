require "bundler"
Bundler::GemHelper.install_tasks

require "rbconfig"
require "rake/testtask"
require "rake/extensiontask"


Rake::ExtensionTask.new('rules') do |task|
  task.config_options = ARGV[1..-1] || []
  task.lib_dir = File.join(*['src', 'rulesrb'])
  task.ext_dir = File.join(*['src', 'rulesrb'])
end

namespace :rules do
  task :clean do
    RbConfig::CONFIG['configure_args'] =~ /with-make-prog\=(\w+)/
    make_program = $1 || ENV['make']
    unless make_program then
      make_program = (/mswin/ =~ RUBY_PLATFORM) ? 'nmake' : 'make'
    end
    system("cd deps/hiredis && #{make_program} clean")
    system("cd src/rules && #{make_program} clean")
  end
end

# "rake clean" should also clean bundled rules
Rake::Task[:clean].enhance(['rules:clean'])

# Build from scratch
task :rebuild => [:clean, :compile]


task :default => [:rebuild]
