require "json"
require "timers"
require_relative "../src/rulesrb/rules"

module Engine
  
  class Session
    attr_reader :handle, :state, :ruleset_name, :timers, :branches, :messages, :event

    def initialize(state, event, handle, ruleset_name)
      @state = state
      @ruleset_name = ruleset_name
      @handle = handle
      @timers = {}
      @messages = {}
      @branches = {}
      if event.key? ":$m"
        @event = event[":$m"]
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
      elsif name.end_with? '?'
        state.key? name[0..-2]
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
      if next_func.kind_of? Promise
        @next = next_func
      elsif next_func.kind_of? Proc
        @next = Promise.new next_func
      else
        raise ArgumentError, "Unexpected Promise Type #{next_func}"
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
        action = rule["run"]
        rule.delete "run"
        if !action
          raise ArgumentError, "Action for #{rule_name} is null"
        elsif action.kind_of? String
          @actions[rule_name] = Promise.new host.get_action action
        elsif action.kind_of? Promise
          @actions[rule_name] = action.root
        elsif action.kind_of? Proc
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
        if db.kind_of? String
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
          name = name[0..-7]
          name = "#{parent_name}.#{name}" if parent_name
          branches[name] = Statechart.new name, host, definition
        elsif name.end_with? "$flow"
          name = name[0..-6]
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

  class Statechart < Ruleset

    def initialize(name, host, chart_definition)
      @name = name
      @host = host
      ruleset_definition = {}
      transform nil, nil, nil, chart_definition, ruleset_definition
      super name, host, ruleset_definition
      @definition = chart_definition
      @definition[:$type] = "stateChart"
    end

    def transform(parent_name, parent_triggers, parent_start_state, chart_definition, rules)
      state_filter = -> s { s.delete "label" if s.key? "label"}
      
      start_state = {}
      
      for state_name, state in chart_definition do
        qualified_name = state_name
        qualified_name = "#{parent_name}.#{state_name}" if parent_name
        start_state[qualified_name] = true
      end 

      for state_name, state in chart_definition do
        qualified_name = state_name
        qualified_name = "#{parent_name}.#{state_name}" if parent_name

        triggers = {}
        if parent_triggers
          for parent_trigger_name, trigger in parent_triggers do
            trigger_name = parent_trigger_name[parent_trigger_name.rindex('.')..-1]
            triggers["#{qualified_name}.#{trigger_name}"] = trigger
          end
        end

        for trigger_name, trigger in state do
          if (trigger_name != "$state") && (trigger.key? "to") && parent_name
            to_name = trigger["to"]
            trigger["to"] = "#{parent_name}.#{to_name}"
          end

          triggers["#{qualified_name}.#{trigger_name}"] = trigger
        end

        if state.key? "$chart" 
          transform qualified_name, triggers, start_state, state["$chart"], rules
        else
          for trigger_name, trigger in triggers do
            rule = {}
            state_test = {"label" => qualified_name}
            if trigger.key? "when"
              if trigger["when"].key? "$s"
                rule["when"] = {"$s" => {"$and" => [state_test, trigger["when"]["$s"]]}}
              else
                rule["whenAll"] = {"$s" => state_test, "$m" => trigger["when"]}
              end
            elsif trigger.key? "whenAll"
              test = {:$s => state_test}
              for test_name, current_test in trigger["whenAll"] do
                test_name = test_name.to_s
                if test_name != "$s"
                  test[test_name] = current_test
                else
                  test["$s"] = {"$and" => [state_test, current_test]}
                end 
              end
              rule["whenAll"] = test
            elsif trigger.key? "whenAny"
              rule["whenAll"] = {"$s" => state_test, "m$any" => trigger["whenAny"]}
            elsif trigger.key? "whenSome"
              rule["whenAll"] = {"$s" => state_test, "m$some" => trigger["whenSome"]}
            else
              rule["when"] = {"$s" => state_test}
            end

            if trigger.key? "run"
              if trigger["run"].kind_of? String
                rule["run"] = Promise.new host.get_action trigger["run"]
              elsif trigger["run"].kind_of? Promise
                rule["run"] = trigger["run"]
                trigger["run"] = "function"
              elsif trigger["run"].kind_of? Proc
                rule["run"] = Promise.new trigger["run"]
                trigger["run"] = "function"
              else
                rule["run"] = Fork.new host.register_rulesets(@name, trigger["run"])
              end     
            end

            if trigger.key? "to"
              if rule.key? "run"
                rule["run"].continue_with To.new(trigger["to"])
              else
                rule["run"] = To.new trigger["to"]
              end

              start_state.delete trigger["to"] if start_state.key? trigger["to"]
              if parent_start_state && (parent_start_state.key? trigger["to"])
                parent_start_state.delete trigger["to"] 
              end
            else
              raise ArgumentError, "Trigger #{trigger_name} destination not defined"
            end

            rules[trigger_name] = rule
          end
        end
      end

      started = false
      for state_name in start_state.keys do
        raise ArgumentError, "Chart #{@name} has more than one start state" if started
        started = true
        if parent_name
          rules[parent_name + "$start"] = {"when"  => {"$s" => {"label" => parent_name}}, "run" => To.new(state_name)}
        else
          rules["$start"] = {"when" => {"$s" => {"$nex" => {"label" => 1}}}, "run" => To.new(state_name)}
        end
      end

      raise ArgumentError, "Chart #{@name} has no start state" if not started
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
      raise ArgumentError, "Action with name #{action_name} not found"
    end

    def load_ruleset(ruleset_name)
      raise ArgumentError, "Ruleset with name #{ruleset_name} not found"
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
          raise ArgumentError, "Ruleset with name #{ruleset_name} already registered" 
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
