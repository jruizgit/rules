require "durable"
require "sinatra"
require "redis"
require "json"

class MyHost < Engine::Host
  def initialize()
    super()
    @_redis = Redis.new
  end

  def get_action(action_name)
    print_lambda = -> c {
      c.s.label = action_name
      puts "Executing #{action_name}"
    }
  end

  def load_ruleset(ruleset_name)
    ruleset = @_redis.get(ruleset_name)
    puts(ruleset)
    puts JSON.parse(ruleset)
    JSON.parse(ruleset)
  end

  def save_ruleset(ruleset_name, ruleset_definition)
    ruleset = JSON.generate(ruleset_definition)
    @_redis.set ruleset_name, ruleset
  end
end

class MyApplication < Interface::Application
  get "/testdynamic.html" do
    send_file File.dirname(__FILE__) + "/testdynamic.html"
  end

  get "/durableVisual.js" do
    send_file File.dirname(__FILE__) + "/durableVisual.js"
  end

  get "/:ruleset_name/admin.html" do
    send_file File.dirname(__FILE__) + "/admin.html"
  end
end

main_host = MyHost.new
main_host.start!
MyApplication.set_host main_host
MyApplication.run!