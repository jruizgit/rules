require_relative 'engine'

module Durable
  def self.run(ruleset_definitions = nil, databases = ['/tmp/redis.sock'], start = nil)
    main_host = Engine::Host.new ruleset_definitions, databases
    start.call main_host if start
    main_host.run
  end
end
