require "sinatra"
require "json"

module Interface
  class Application < Sinatra::Base
    set :bind, "0.0.0.0"

    @@host = nil

    def self.set_host(value)
      @@host = value
    end

    get "/:ruleset_name/state/:sid" do
      begin
        JSON.generate @@host.get_state(params["ruleset_name"], params["sid"])
      rescue Exception => e
        status 404
        e.to_s
      end
    end

    post "/:ruleset_name/state/:sid" do
      begin
        request.body.rewind
        state = JSON.parse request.body.read
        state["id"] = params["sid"]
        result = @@host.patch_state(params["ruleset_name"], state)
        JSON.generate({:outcome => result})
      rescue Exception => e
        status 500
        e.to_s
      end
    end

    get "/:ruleset_name/state" do
      begin
        JSON.generate @@host.get_state(params["ruleset_name"], "0")
      rescue Exception => e
        status 404
        e.to_s
      end
    end

    post "/:ruleset_name/state" do
      begin
        request.body.rewind
        state = JSON.parse request.body.read
        result = @@host.patch_state(params["ruleset_name"], state)
        JSON.generate({:outcome => result})
      rescue Exception => e
        status 500
        e.to_s
      end
    end

    post "/:ruleset_name/events/:sid" do
      begin
        request.body.rewind
        message = JSON.parse request.body.read
        message["sid"] = params["sid"]
        result = @@host.post(params["ruleset_name"], message)
        JSON.generate({:outcome => result})
      rescue Exception => e
        status 500
        e.to_s
      end
    end

    post "/:ruleset_name/events" do
      begin
        request.body.rewind
        message = JSON.parse request.body.read
        result = @@host.post(params["ruleset_name"], message)
        JSON.generate({:outcome => result})
      rescue Exception => e
        status 500
        e.to_s
      end
    end

    post "/:ruleset_name/facts/:sid" do
      begin
        request.body.rewind
        message = JSON.parse request.body.read
        message["sid"] = params["sid"]
        result = @@host.assert(params["ruleset_name"], message)
        JSON.generate({:outcome => result})
      rescue Exception => e
        status 500
        e.to_s
      end
    end

    post "/:ruleset_name/facts" do
      begin
        request.body.rewind
        message = JSON.parse request.body.read
        result = @@host.assert(params["ruleset_name"], message)
        JSON.generate({:outcome => result})
      rescue Exception => e
        status 500
        e.to_s
      end
    end

    get "/:ruleset_name/definition" do
      begin
        @@host.get_ruleset(params["ruleset_name"]).to_json
      rescue Exception => e
        status 404
        e.to_s
      end
    end

    post "/:ruleset_name/definition" do
      begin
        request.body.rewind
        @@host.set_ruleset params["ruleset_name"], JSON.parse(request.body.read)
      rescue Exception => e
        status 500
        e.to_s
      end
    end
  end
end
