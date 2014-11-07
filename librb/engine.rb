require "json"
require "timers"
require_relative "../src/rulesrb/rules"

module Engine
  
  class Session
    attr_reader :handle, :state, :ruleset_name, :timers, :branches, :messages

    def initialize(state, event, handle, ruleset_name)
      @state = state
      @ruleset_name = ruleset_name
      @handle = handle
      @timers = {}
      @messages = {}
      @branches = {}
      if event.key? :$m
        @event = event[:$m]
      else
        @event = event
      end
    end

    def signal(message)
      name_index = @ruleset_name.rindex "."
      parent_ruleset_name = ruleset_name[0, name_index]
      name = ruleset_name[name_index + 1..-1]
      message_id = message[:id]
      message[:sid] = @id
      message[:id] = "#{name}.#{message_id}"
      post parent_ruleset_name, message
    end

    def post(ruleset_name, message)
      message_list = []
      if @messages.key? ruleset_name
        message_list = @messages[ruleset_name]
      else
        @messages[ruleset_name] = message_list
      end
      message_list << message
    end

    def start_timer(timer_name, duration)
      if @timers.key? timer_name
        raise ArgumentError, "Timer with name #{timer_name} already added"
      else
        @timers[timer_name] = duration
      end
    end

    def fork(branch_name, branch_state)
      if @branches.key? branch_name
        raise ArgumentError, "Branch with name #{branch_name} already forked"
      else
        @branches[branch_name] = branch_state
      end
    end

    
    def handle_state(name, value=nil)
      name = name.to_s
      if name.end_with? '='
        @state[name[0..-2]] = value
        nil
      else
        @state[name]
      end
    end

    alias method_missing handle_state

  end


  class Promise
    attr_accessor :root

    def initialize(func)
      @func = func
      @next = nil
      @sync = true
      @root = self
    end

    def continue_with(next_func)
      if next_func.insance_of? Promise
        @next = next_func
      elsif next_func.insance_of? Proc && next_func.lambda?
        @next = Promise.new next_func
      else
        raise ArgumentError, "Unexpected Promise Type"
      end

      @next.root = @root
      @next
    end

    def run(s, complete)
      if @sync
        begin
          @func.call s  
        rescue Exception => e
          complete.call e
          return
        end
        
        if @next
          @next.run s, complete
        else
          complete.call nil
        end
      else
        begin
          @func.call s, -> e {
            if e
              complete.call e
            elsif @next
              @next.run s, complete
            else
              complete.call nil
            end
          }
        rescue Exception => e
          complete.call e
        end  
      end
    end

  end


  class Fork < Promise

    def initialize(branch_names, state_filter = nil)
      super ->s {
        for branch_name in branch_names do
          state = s.state.dup
          state_filter.call state if state_filter
          s.fork branch_name 
        end 
      }
    end

  end


  class To < Promise

    def initialize(state)
      super -> s {
        s.state[:label] = state
      }
    end

  end


  class Ruleset
    attr_reader :definition

    def initialize(name, host, ruleset_definition)
      @actions = {}
      @name = name
      @host = host
      for rule_name, rule in ruleset_definition do
        rule_name = rule_name.to_s
        action = rule[:run]
        rule.delete :run
        if action.instance_of? String
          @actions[rule_name] = Promise.new host.get_action action
        elsif action.instance_of? Promise
          @actions[rule_name] = action.root
        elsif action.instance_of? Proc
          @actions[rule_name] = Promise.new action
        else
          @actions[rule_name] = Fork.new host.register_rulesets(name, action)
        end      
      end
      @handle = Rules.create_ruleset name, JSON.generate(ruleset_definition)  
      @definition = ruleset_definition
    end    

    def bind(databases)
      for db in databases do
        if db.instance_of? String
          Rules.bind_ruleset @handle, db, 0, nil
        else 
          Rules.bind_ruleset @handle, db[:host], db[:port], db[:password] 
        end
      end
    end

    def assert_event(message)
      Rules.assert_event @handle, JSON.generate(message)
    end

    def assert_events(messages)
      Rules.assert_events @handle, JSON.generate(messages)
    end

    def assert_state(state)
      Rules.assert_state @handle, JSON.generate(state)
    end

    def get_state(sid)
      JSON.parse Rules.get_state(@handle, sid)
    end
    
    def Ruleset.create_rulesets(parent_name, host, ruleset_definitions)
      branches = {}
      for name, definition in ruleset_definitions do  
        name = name.to_s
        if name.end_with? "$state"
          name = name[0..-6]
          name = "#{parent_name}.#{name}" if parent_name
          branches[name] = Statechart.new name, host, definition
        elsif name.end_with? "$flow"
          name = name[0..-5]
          name = "#{parent_name}.#{name}" if parent_name
          branches[name] = Flowchart.new name, host, definition
        else
          name = "#{parent_name}.#{name}" if parent_name
          branches[name] = Ruleset.new name, host, definition
        end
      end

      branches   
    end

    def dispatch_timers(complete)
      begin
        Rules.assert_timers @handle
      rescue Exception => e
        complete.call e
        return
      end

      complete.call nil
    end
      
    def dispatch(complete)
      result = nil
      begin
        result = Rules.start_action @handle
      rescue Exception => e
        complete.call e
        return
      end
      
      if !result
        complete.call nil
      else
        state = JSON.parse result[0]
        event = JSON.parse(result[1])[@name]
        action_name = nil
        for action_name, event in event do
          break
        end

        s = Session.new state, event, result[2], @name
        @actions[action_name].run s, -> e {
          if e
            Rules.abandon_action @handle, s.handle
            complete.call e
          else
            begin
              for branch_name, branch_state in s.branches do
                @host.patch_state branch_name, branch_state
              end

              for ruleset_name, messages in s.messages do
                if messages.length == 1
                  @host.post ruleset_name, messages[0]
                else
                  @host.post_batch ruleset_name, messages
                end
              end

              for timer_name, timer_duration in s.timers do
                timer = {:sid => s.id, :id => timer_name, :$t => timer_name}
                Rules.start_timer @handle, s.id, timer_duration, JSON.generate(timer)
              end

              Rules.complete_action @handle, s.handle, JSON.generate(s.state)
              complete.call nil
            rescue Exception => e
              Rules.abandon_action @handle, s.handle
              complete.call e
            end
          end
        }
      end
    end

  end


  class Host

    def initialize(ruleset_definitions = nil, databases = ["/tmp/redis.sock"])
      @ruleset_directory = {}
      @ruleset_list = []
      @databases = databases
      register_rulesets nil, ruleset_definitions if ruleset_definitions
    end

    def get_action(action_name)
      raise ArgumentError "Action with name #{action_name} not found"
    end

    def load_ruleset(ruleset_name)
      raise ArgumentError "Ruleset with name #{ruleset_name} not found"
    end

    def save_ruleset(ruleset_name, ruleset_definition)
    end

    def get_ruleset(ruleset_name)
      ruleset_name = ruleset_name.to_s
      if !(@ruleset_directory.key? ruleset_name)
        ruleset_definition = load_ruleset ruleset_name
        register_rulesets nil, ruleset_definition
      end

      @ruleset_directory[ruleset_name]
    end

    def set_ruleset(ruleset_name, ruleset_definition)
      register_rulesets nil, ruleset_definition
      save_ruleset ruleset_name, ruleset_definition
    end

    def get_state(ruleset_name, sid)
      get_ruleset(ruleset_name).get_state sid
    end

    def post_batch(ruleset_name, messages)
      get_ruleset(ruleset_name).assert_events messages
    end

    def post(ruleset_name, message)
      get_ruleset(ruleset_name).assert_event message
    end

    def patch_state(ruleset_name, state)
      get_ruleset(ruleset_name).assert_state state
    end

    def register_rulesets(parent_name, ruleset_definitions)
      rulesets = Ruleset.create_rulesets(parent_name, self, ruleset_definitions)
      for ruleset_name, ruleset in rulesets do
        if @ruleset_directory.key? ruleset_name
          raise ArgumentError "Ruleset with name #{ruleset_name} already registered" 
        end

        @ruleset_directory[ruleset_name] = ruleset
        @ruleset_list << ruleset
        ruleset.bind @databases
      end
    end

    def run
      index = 1
      timers = Timers::Group.new
  
      dispatch_ruleset = -> s {

        callback = -> e {
          if index % 10 == 0
            index += 1
            timers.after 1, &dispatch_ruleset
          else
            index += 1
            dispatch_ruleset.call 0
          end
        }

        timers_callback = -> e {
          puts "internal error #{e}" if e
          if index % 10 == 0 && @ruleset_list.length > 0
            ruleset = @ruleset_list[(index / 10) % @ruleset_list.length]
            ruleset.dispatch_timers callback
          else
            callback.call e
          end
        }

        if @ruleset_list.length > 0
          ruleset = @ruleset_list[index % @ruleset_list.length]
          ruleset.dispatch timers_callback
        else
          timers_callback.call nil
        end
      }

      timers.after 1, &dispatch_ruleset
      
      loop { timers.wait }
    end

  end


end
