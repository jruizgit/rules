require "sinatra"
require "json"

module Interface
  class Application < Sinatra::Base
    @@host = nil

    def self.set_host(value) 
      @@host = value
    end

    get "/:ruleset_name/:sid" do
      begin
        JSON.generate @@host.get_state(params["ruleset_name"], params["sid"])
      rescue Exception => e
        status 404
        e.to_s
      end
    end

    post "/:ruleset_name/:sid" do
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

    patch "/:ruleset_name/:sid" do
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

    get "/:ruleset_name" do
      begin
        @@host.get_ruleset(params["ruleset_name"]).to_json
      rescue Exception => e
        status 404
        e.to_s
      end
    end

    post "/:ruleset_name" do
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
