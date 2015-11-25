require "json"
require "timers"
require_relative "../src/rulesrb/rules"

module Engine
  
  class Closure
    attr_reader :host, :handle, :ruleset_name, :_timers, :_branches, :_messages, :_facts, :_retract
    attr_accessor :s

    def initialize(host, state, message, handle, ruleset_name)
      @s = Content.new(state)
      @ruleset_name = ruleset_name
      @handle = handle
      @host = host
      @_timers = {}
      @_messages = {}
      @_branches = {}
      @_facts = {}
      @_retract = {}
      if message.kind_of? Hash
        @m = message
      else
        @m = []
        for one_message in message do
          if (one_message.key? "m") && (one_message.size == 1)
            one_message = one_message["m"]
          end
          @m << Content.new(one_message)
        end
      end
    end

    def post(ruleset_name, message = nil)
      if !message
        message = ruleset_name
        ruleset_name = @ruleset_name
      end

      if message.kind_of? Content
        message = message._d
      end

      if !(message.key? :sid) && !(message.key? "sid")
        message[:sid] = @s.sid
      end

      message_list = []
      if @_messages.key? ruleset_name
        message_list = @_messages[ruleset_name]
      else
        @_messages[ruleset_name] = message_list
      end
      message_list << message
    end

    def start_timer(timer_name, duration)
      if @_timers.key? timer_name
        raise ArgumentError, "Timer with name #{timer_name} already added"
      else
        @_timers[timer_name] = duration
      end
    end

    def assert(ruleset_name, fact = nil)
      if !fact
        fact = ruleset_name
        ruleset_name = @ruleset_name
      end

      if fact.kind_of? Content
        fact = fact._d.dup
      end

      if !(fact.key? :sid) && !(fact.key? "sid")
        fact[:sid] = @s.sid
      end

      fact_list = []
      if @_facts.key? ruleset_name
        fact_list = @_facts[ruleset_name]
      else
        @_facts[ruleset_name] = fact_list
      end
      fact_list << fact
    end

    def retract(ruleset_name, fact = nil)
      if !fact
        fact = ruleset_name
        ruleset_name = @ruleset_name
      end

      if fact.kind_of? Content
        fact = fact._d.dup
      end

      if !(fact.key? :sid) && !(fact.key? "sid")
        fact[:sid] = @s.sid
      end

      fact_list = []
      if @_retract.key? ruleset_name
        fact_list = @_retract[ruleset_name]
      else
        @_retract[ruleset_name] = fact_list
      end
      fact_list << fact
    end

    private

    def handle_property(name, value=nil)
      name = name.to_s
      if name.end_with? '?'
        @m.key? name[0..-2]
      elsif @m.kind_of? Hash
        current = @m[name]
        if current.kind_of? Hash
          Content.new current
        else
          current
        end
      else
        @m
      end
    end

    alias method_missing handle_property

  end


  class Content
    attr_reader :_d

    def initialize(data)
      @_d = data
    end

    def to_s
      @_d.to_s
    end
    
    private

    def handle_property(name, value=nil)
      name = name.to_s
      if name.end_with? '='
        @_d[name[0..-2]] = value
        nil
      elsif name.end_with? '?'
        @_d.key? name[0..-2]
      else
        current = @_d[name]
        if current.kind_of? Hash
          Content.new current
        else
          current
        end
      end
    end

    alias method_missing handle_property

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

    def run(c, complete)
      if @sync
        begin
          @func.call c  
        rescue Exception => e
          complete.call e
          return
        end
        
        if @next
          @next.run c, complete
        else
          complete.call nil
        end
      else
        begin
          @func.call c, -> e {
            if e
              complete.call e
            elsif @next
              @next.run c, complete
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


  class To < Promise

    def initialize(from_state, to_state, assert_state)
      super -> c {
        c.s.running = true
        if from_state != to_state
          if from_state
            if c.m && (c.m.kind_of? Array)
              c.retract c.m[0].chart_context
            else
              c.retract c.chart_context
            end 
          end
        
          id = rand(1000000000)
          if assert_state
            c.assert(:label => to_state, :chart => 1, :id => id)
          else
            c.post(:label => to_state, :chart => 1, :id => id)
          end
        end
      }
    end

  end


  class Ruleset
    attr_reader :definition

    def initialize(name, host, ruleset_definition, state_cache_size)
      @actions = {}
      @name = name
      @host = host
      for rule_name, rule in ruleset_definition do
        rule_name = rule_name.to_s
        action = nil
        if rule.key? "run"
          action = rule["run"]
          rule.delete "run"
        elsif rule.key? :run
          action = rule[:run]
          rule.delete :run
        end

        if !action
          raise ArgumentError, "Action for #{rule_name} is null"
        elsif action.kind_of? String
          @actions[rule_name] = Promise.new host.get_action action
        elsif action.kind_of? Promise
          @actions[rule_name] = action.root
        elsif action.kind_of? Proc
          @actions[rule_name] = Promise.new action
        end      
      end

      @handle = Rules.create_ruleset name, JSON.generate(ruleset_definition), state_cache_size  
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

    def start_assert_event(message)
      return Rules.start_assert_event @handle, JSON.generate(message)
    end

    def assert_events(messages)
      Rules.assert_events @handle, JSON.generate(messages)
    end

    def start_assert_events(messages)
      return Rules.start_assert_events @handle, JSON.generate(messages)
    end

    def start_timer(sid, timer_name, timer_duration)
      timer = {:sid => sid, :id => rand(1000000000), :$t => timer_name}
      Rules.start_timer @handle, sid.to_s, timer_duration, JSON.generate(timer)
    end

    def assert_fact(fact)
      Rules.assert_fact @handle, JSON.generate(fact)
    end

    def start_assert_fact(fact)
      return Rules.start_assert_fact @handle, JSON.generate(fact)
    end

    def assert_facts(facts)
      Rules.assert_facts @handle, JSON.generate(facts)
    end

    def start_assert_facts(facts)
      return Rules.start_assert_facts @handle, JSON.generate(facts)
    end

    def retract_fact(fact)
      Rules.retract_fact @handle, JSON.generate(fact)
    end

    def start_retract_fact(fact)
      return Rules.start_retract_fact @handle, JSON.generate(fact)
    end

    def retract_facts(facts)
      Rules.assert_facts @handle, JSON.generate(facts)
    end

    def start_retract_facts(facts)
      return Rules.start_retract_facts @handle, JSON.generate(facts)
    end

    def assert_state(state)
      Rules.assert_state @handle, JSON.generate(state)
    end

    def get_state(sid)
      JSON.parse Rules.get_state(@handle, sid)
    end

    def Ruleset.create_rulesets(parent_name, host, ruleset_definitions, state_cache_size)
      branches = {}
      for name, definition in ruleset_definitions do  
        name = name.to_s
        if name.end_with? "$state"
          name = name[0..-7]
          name = "#{parent_name}.#{name}" if parent_name
          branches[name] = Statechart.new name, host, definition, state_cache_size
        elsif name.end_with? "$flow"
          name = name[0..-6]
          name = "#{parent_name}.#{name}" if parent_name
          branches[name] = Flowchart.new name, host, definition, state_cache_size
        else
          name = "#{parent_name}.#{name}" if parent_name
          branches[name] = Ruleset.new name, host, definition, state_cache_size
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
      result_container = {}
      action_handle = nil
      action_binding = nil
      state = nil
      begin
        result = Rules.start_action @handle
        if result
          state = JSON.parse result[0]
          result_container = {:message => JSON.parse(result[1])}
          action_handle = result[2]
          action_binding = result[3]
        end
      rescue Exception => e
        complete.call e
        return
      end
      
      while result_container.key? :message do
        action_name = nil
        for action_name, message in result_container[:message] do
          break
        end

        result_container.delete :message
        c = Closure.new @host, state, message, action_handle, @name
        @actions[action_name].run c, -> e {
          if e
            Rules.abandon_action @handle, c.handle
            complete.call e
          else
            begin
              for timer_name, timer_duration in c._timers do
                start_timer c.s.sid, timer_name, timer_duration
              end
              binding  = 0
              replies = 0
              pending = {action_binding => 0}
              for ruleset_name, facts in c._retract do
                if facts.length == 1
                  binding, replies = @host.start_retract ruleset_name, facts[0]
                else
                  binding, replies = @host.start_retract_facts ruleset_name, facts
                end
                if pending.key? binding
                  pending[binding] = pending[binding] + replies
                else
                  pending[binding] = replies
                end
              end
              for ruleset_name, facts in c._facts do
                if facts.length == 1
                  binding, replies = @host.start_assert ruleset_name, facts[0]
                else
                  binding, replies = @host.start_assert_facts ruleset_name, facts
                end
                if pending.key? binding
                  pending[binding] = pending[binding] + replies
                else
                  pending[binding] = replies
                end
              end
              for ruleset_name, messages in c._messages do
                if messages.length == 1
                  binding, replies = @host.start_post ruleset_name, messages[0]
                else
                  binding, replies = @host.start_post_batch ruleset_name, messages
                end
                if pending.key? binding
                  pending[binding] = pending[binding] + replies
                else
                  pending[binding] = replies
                end
              end
              binding, replies = Rules.start_update_state @handle, c.handle, JSON.generate(c.s._d)
              if pending.key? binding
                pending[binding] = pending[binding] + replies
              else
                pending[binding] = replies
              end
              for binding, replies in pending do
                if binding != 0
                  if binding != action_binding
                    Rules.complete(binding, replies)
                  else
                    new_result = Rules.complete_and_start_action @handle, replies, c.handle
                    if new_result
                      result_container[:message] = JSON.parse new_result
                    end
                  end
                end
              end
            rescue Exception => e
              Rules.abandon_action @handle, c.handle
              complete.call e
            end
          end
        }
      end
      complete.call nil
    end

    def to_json
      JSON.generate @definition
    end

  end

  class Statechart < Ruleset

    def initialize(name, host, chart_definition, state_cache_size)
      @name = name
      @host = host
      ruleset_definition = {}
      transform nil, nil, nil, chart_definition, ruleset_definition
      super name, host, ruleset_definition, state_cache_size
      @definition = chart_definition
      @definition[:$type] = "stateChart"
    end

    def transform(parent_name, parent_triggers, parent_start_state, chart_definition, rules)
      start_state = {}
      reflexive_states = {}
      
      for state_name, state in chart_definition do
        qualified_name = state_name.to_s
        qualified_name = "#{parent_name}.#{state_name}" if parent_name
        start_state[qualified_name] = true

        for trigger_name, trigger in state do
          if ((trigger.key? :to) && (trigger[:to] == state_name)) || 
             ((trigger.key? "to") && (trigger["to"] == state_name)) ||
             (trigger.key? :count) || (trigger.key? "count") ||
             (trigger.key? :cap) || (trigger.key? "cap") ||
             (trigger.key? :span) || (trigger.key? "span")
            reflexive_states[qualified_name] = true
          end
        end
      end 

      for state_name, state in chart_definition do
        qualified_name = state_name.to_s
        qualified_name = "#{parent_name}.#{state_name}" if parent_name

        triggers = {}
        if parent_triggers
          for parent_trigger_name, trigger in parent_triggers do
            parent_trigger_name = parent_trigger_name.to_s
            trigger_name = parent_trigger_name[parent_trigger_name.rindex('.')..-1]
            triggers["#{qualified_name}.#{trigger_name}"] = trigger
          end
        end

        for trigger_name, trigger in state do
          trigger_name = trigger_name.to_s
          if trigger_name != "$chart" 
            if parent_name && (trigger.key? "to")
              to_name = trigger["to"].to_s
              trigger["to"] = "#{parent_name}.#{to_name}"
            elsif parent_name && (trigger.key? :to)
              to_name = trigger[:to].to_s
              trigger[:to] = "#{parent_name}.#{to_name}"
            end
            triggers["#{qualified_name}.#{trigger_name}"] = trigger
          end
        end

        if state.key? "$chart" 
          transform qualified_name, triggers, start_state, state["$chart"], rules
        elsif state.key? :$chart
          transform qualified_name, triggers, start_state, state[:$chart], rules
        else
          for trigger_name, trigger in triggers do

            trigger_name = trigger_name.to_s
            rule = {}
            state_test = {:chart_context => {:$and => [{:label => qualified_name}, {:chart => 1}]}}
            if trigger.key? :pri
              rule[:pri] = trigger[:pri]
            elsif trigger.key? "pri"
              rule[:pri] = trigger["pri"]
            end

            if trigger.key? :count
              rule[:count] = trigger[:count]
            elsif trigger.key? "count"
              rule[:count] = trigger["count"]
            end

            if trigger.key? :span
              rule[:span] = trigger[:span]
            elsif trigger.key? "span"
              rule[:span] = trigger["span"]
            end

            if trigger.key? :cap
              rule[:cap] = trigger[:cap]
            elsif trigger.key? "cap"
              rule[:cap] = trigger["cap"]
            end

            if (trigger.key? :all) || (trigger.key? "all")
              all_trigger = nil
              if trigger.key? :all
                all_trigger = trigger[:all]
              else
                all_trigger = trigger["all"]
              end
              rule[:all] = all_trigger.dup 
              rule[:all] << state_test
            elsif (trigger.key? :any) || (trigger.key? "any")
              any_trigger = nil
              if trigger.key? :any
                any_trigger = trigger[:any]
              else
                any_trigger = trigger["any"]
              end
              rule[:all] = [state_test, {"m$any" => any_trigger}]
            else 
              rule[:all] = [state_test]
            end
              
            if (trigger.key? "run") || (trigger.key? :run)
              trigger_run = nil
              if trigger.key? :run
                trigger_run = trigger[:run]
              else
                trigger_run = trigger["run"]
              end

              if trigger_run.kind_of? String
                rule[:run] = Promise.new @host.get_action(trigger_run)
              elsif trigger_run.kind_of? Promise
                rule[:run] = trigger_run
              elsif trigger_run.kind_of? Proc
                rule[:run] = Promise.new trigger_run
              end     
            end

            if (trigger.key? "to") || (trigger.key? :to)
              trigger_to = nil
              if trigger.key? :to
                trigger_to = trigger[:to]
              else
                trigger_to = trigger["to"]
              end
              trigger_to = trigger_to.to_s
              from_state = nil
              if reflexive_states.key? qualified_name
                from_state = qualified_name
              end
              assert_state = false
              if reflexive_states.key? trigger_to
                assert_state = true
              end

              if rule.key? :run
                rule[:run].continue_with To.new(from_state, trigger_to, assert_state)
              else
                rule[:run] = To.new from_state, trigger_to, assert_state
              end

              start_state.delete trigger_to if start_state.key? trigger_to
              if parent_start_state && (parent_start_state.key? trigger_to)
                parent_start_state.delete trigger_to
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
        state_name = state_name.to_s
        started = true

        if parent_name
          rules[parent_name + "$start"] = {:all => [{:chart_context => {:$and => [{:label => parent_name}, {:chart => 1}]}}], run: To.new(nil, state_name, false)};
        else
          rules[:$start] = {:all => [{:chart_context => {:$and => [{:$nex => {:running => 1}}, {:$s => 1}]}}], run: To.new(nil, state_name, false)};
        end
      end

      raise ArgumentError, "Chart #{@name} has no start state" if not started
    end
  end

  class Flowchart < Ruleset

    def initialize(name, host, chart_definition, state_cache_size)
      @name = name
      @host = host
      ruleset_definition = {}
      transform chart_definition, ruleset_definition
      super name, host, ruleset_definition, state_cache_size
      @definition = chart_definition
      @definition["$type"] = "flowChart"
    end

    def transform(chart_definition, rules)
      visited = {}
      reflexive_stages = {}
      for stage_name, stage in chart_definition do
        if (stage.key? :to) || (stage.key? "to")
          stage_to = (stage.key? :to) ? stage[:to]: stage["to"]
          if (stage_to.kind_of? String) || (stage_to.kind_of? Symbol)
            if stage_to == stage_name
              reflexive_stages[stage_name] = true
            end
          else
            for transition_name, transition in stage_to do
              if (transition_name == stage_name) || 
                 (transition.key? :count) || (transition.key? "count") ||
                 (transition.key? :cap) || (transition.key? "cap") ||
                 (transition.key? :span) || (transition.key? "span")
                reflexive_stages[stage_name] = true
              end
            end
          end
        end
      end

      for stage_name, stage in chart_definition do
        from_stage = nil
        if reflexive_stages.key? stage_name
          from_stage = stage_name
        end
        
        stage_name = stage_name.to_s
        stage_test = {:chart_context => {:$and => [{:label => stage_name}, {:chart => 1}]}}
        if (stage.key? :to) || (stage.key? "to")
          stage_to = (stage.key? :to) ? stage[:to]: stage["to"]
          if (stage_to.kind_of? String) || (stage_to.kind_of? Symbol)
            next_stage = nil
            rule = {:all => [stage_test]}
            if chart_definition.key? stage_to
              next_stage = chart_definition[stage_to]
            elsif chart_definition.key? stage_to.to_s
              next_stage = chart_definition[stage_to.to_s]
            else
              raise ArgumentError, "Stage #{stage_to.to_s} not found"
            end

            assert_stage = false
            if reflexive_stages.key? stage_to
              assert_stage = true
            end

            stage_to = stage_to.to_s
            if !(next_stage.key? :run) && !(next_stage.key? "run")
              rule[:run] = To.new from_stage, stage_to, assert_stage
            else
              next_stage_run = (next_stage.key? :run) ? next_stage[:run]: next_stage["run"]
              if next_stage_run.kind_of? String
                rule[:run] = To.new(from_stage, stage_to, assert_stage).continue_with Promise(@host.get_action(next_stage_run))
              elsif (next_stage_run.kind_of? Promise) || (next_stage_run.kind_of? Proc)
                rule[:run] = To.new(from_stage, stage_to, assert_stage).continue_with next_stage_run
              end
            end

            rules["#{stage_name}.#{stage_to}"] = rule
            visited[stage_to] = true
          else
            for transition_name, transition in stage_to do
              rule = {}
              next_stage = nil

              if transition.key? :pri
                rule[:pri] = transition[:pri]
              elsif transition.key? "pri"
                rule[:pri] = transition["pri"]
              end

              if transition.key? :count
                rule[:count] = transition[:count]
              elsif transition.key? "count"
                rule[:count] = transition["count"]
              end

              if transition.key? :span
                rule[:span] = transition[:span]
              elsif transition.key? "span"
                rule[:span] = transition["span"]
              end

              if transition.key? :cap
                rule[:cap] = transition[:cap]
              elsif transition.key? "cap"
                rule[:cap] = transition["cap"]
              end

              if (transition.key? :all) || (transition.key? "all")
                all_transition = nil
                if transition.key? :all
                  all_transition = transition[:all]
                else
                  all_transition = transition["all"]
                end
                rule[:all] = all_transition.dup 
                rule[:all] << stage_test
              elsif (transition.key? :any) || (transition.key? "any")
                any_transition = nil
                if transition.key? :any
                  any_transition = transition[:any]
                else
                  any_transition = transition["any"]
                end
                rule[:all] = [stage_test, {"m$any" => any_transition}]
              else 
                rule[:all] = [stage_test]
              end

              if chart_definition.key? transition_name
                next_stage = chart_definition[transition_name]
              else
                raise ArgumentError, "Stage #{transition_name.to_s} not found"
              end

              assert_stage = false
              if reflexive_stages.key? transition_name
                assert_stage = true
              end

              transition_name = transition_name.to_s
              if !(next_stage.key? :run) && !(next_stage.key? "run")
                rule[:run] = To.new from_stage, transition_name, assert_stage
              else
                next_stage_run = (next_stage.key? :run) ? next_stage[:run]: next_stage["run"]
                if next_stage_run.kind_of? String
                  rule[:run] = To.new(from_stage, transition_name, assert_stage).continue_with Promise(@host.get_action(next_stage_run))
                elsif (next_stage_run.kind_of? Promise) || (next_stage_run.kind_of? Proc)
                  rule[:run] = To.new(from_stage, transition_name, assert_stage).continue_with next_stage_run
                end
              end

              rules["#{stage_name}.#{transition_name}"] = rule
              visited[transition_name] = true
            end
          end
        end
      end

      started = false
      for stage_name, stage in chart_definition do
        stage_name = stage_name.to_s
        if !(visited.key? stage_name)
          if started
            raise ArgumentError, "Chart #{@name} has more than one start state"
          end

          started = true
          rule = {:all => [{:chart_context => {:$and => [{:$nex => {:running => 1}}, {:$s => 1}]}}]}
          if !(stage.key? :run) && !(stage.key? "run")
            rule[:run] = To.new nil, stage_name, false
          else
            stage_run = stage.key? :run ? stage[:run]: stage["run"]
            if stage_run.kind_of? String
              rule[:run] = To.new(nil, stage_name, false).continue_with Promise(@host.get_action(stage_run))
            elsif (stage_run.kind_of? Promise) || (stage_run.kind_of? Proc)
              rule[:run] = To.new(nil, stage_name, false).continue_with stage_run
            end
          end
          rules["$start.#{stage_name}"] = rule
        end
      end
    end
  end

  class Host

    def initialize(ruleset_definitions = nil, databases = [{:host => 'localhost', :port => 6379, :password => nil}], state_cache_size = 1024)
      @ruleset_directory = {}
      @ruleset_list = []
      @databases = databases
      @state_cache_size = state_cache_size
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

    def post_batch(ruleset_name, *events)
      get_ruleset(ruleset_name).assert_events events
    end

    def start_post_batch(ruleset_name, *events)
      return get_ruleset(ruleset_name).start_assert_events events
    end

    def post(ruleset_name, event)
      get_ruleset(ruleset_name).assert_event event
    end

    def start_post(ruleset_name, event)
      return get_ruleset(ruleset_name).start_assert_event event
    end

    def assert(ruleset_name, fact)
      get_ruleset(ruleset_name).assert_fact fact
    end

    def start_assert(ruleset_name, fact)
      return get_ruleset(ruleset_name).start_assert_fact fact
    end

    def assert_facts(ruleset_name, *facts)
      get_ruleset(ruleset_name).assert_facts facts
    end

    def start_assert_facts(ruleset_name, *facts)
      return get_ruleset(ruleset_name).start_assert_facts facts
    end

    def retract(ruleset_name, fact)
      get_ruleset(ruleset_name).retract_fact fact
    end

    def start_retract(ruleset_name, fact)
      return get_ruleset(ruleset_name).start_retract_fact fact
    end

    def retract_facts(ruleset_name, *facts)
      get_ruleset(ruleset_name).retract_facts facts
    end

    def start_retract_facts(ruleset_name, *facts)
      return get_ruleset(ruleset_name).start_retract_facts facts
    end

    def start_timer(ruleset_name, sid, timer_name, timer_duration)
      get_ruleset(ruleset_name).start_timer sid, timer_name, timer_duration
    end

    def patch_state(ruleset_name, state)
      get_ruleset(ruleset_name).assert_state state
    end

    def register_rulesets(parent_name, ruleset_definitions)
      rulesets = Ruleset.create_rulesets(parent_name, self, ruleset_definitions, @state_cache_size)
      for ruleset_name, ruleset in rulesets do
        if @ruleset_directory.key? ruleset_name
          raise ArgumentError, "Ruleset with name #{ruleset_name} already registered" 
        end

        @ruleset_directory[ruleset_name] = ruleset
        @ruleset_list << ruleset
        ruleset.bind @databases
      end

      rulesets.keys
    end

    def start!
      index = 1
      timers = Timers::Group.new
  
      dispatch_ruleset = -> c {

        callback = -> e {
          puts "internal error #{e}" if e
          if index % 10 == 0
            index += 1
            timers.after 0.01, &dispatch_ruleset
          else
            index += 1
            dispatch_ruleset.call nil
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

      timers.after 0.001, &dispatch_ruleset
      Thread.new do
        loop { timers.wait }
      end
    end

  end

end
