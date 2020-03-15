require "json"
require "timers"
require_relative "../src/rulesrb/rules"

module Engine

  class MessageNotHandledError < StandardError
    def initialize(message)
      super("Could not handle message: #{JSON.generate(message)}")
    end
  end

  class MessageObservedError < StandardError
    def initialize(message)
      super("Message has already been observed: #{JSON.generate(message)}")
    end
  end

  class Closure
    attr_reader :host, :handle, :_timers, :_cancelled_timers, :_messages, :_facts, :_retract, :_deleted
    attr_accessor :s

    def initialize(host, ruleset, state, message, handle)
      @s = Content.new(state)
      @handle = handle
      @host = host
      @_ruleset = ruleset
      @_start_time = Time.now
      @_completed = false
      @_deleted = false
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

      if !s.sid
        s.sid = "0" 
      end
    end

    def post(ruleset_name, message = nil)
      if message
        if message.kind_of? Content
          message = message._d
        end

        if !(message.key? :sid) && !(message.key? "sid")
          message[:sid] = @s.sid
        end
 
        @host.assert_event ruleset_name, message
      else
        message = ruleset_name
        if message.kind_of? Content
          message = message._d
        end

        if !(message.key? :sid) && !(message.key? "sid")
          message[:sid] = @s.sid
        end
 
        @_ruleset.assert_event message
      end
    end

    def assert(ruleset_name, fact = nil)
      if fact
        if fact.kind_of? Content
          fact = fact._d
        end

        if !(fact.key? :sid) && !(fact.key? "sid")
          fact[:sid] = @s.sid
        end
 
        @host.assert_fact ruleset_name, fact
      else
        fact = ruleset_name
        if fact.kind_of? Content
          fact = fact._d
        end

        if !(fact.key? :sid) && !(fact.key? "sid")
          fact[:sid] = @s.sid
        end

        @_ruleset.assert_fact fact
      end
    end

    def retract(ruleset_name, fact = nil)
      if fact
        if fact.kind_of? Content
          fact = fact._d
        end

        if !(fact.key? :sid) && !(fact.key? "sid")
          fact[:sid] = @s.sid
        end
 
        @host.retract_fact ruleset_name, fact
      else
        fact = ruleset_name
        if fact.kind_of? Content
          fact = fact._d
        end

        if !(fact.key? :sid) && !(fact.key? "sid")
          fact[:sid] = @s.sid
        end
        @_ruleset.retract_fact fact
      end
    end
    
    def start_timer(timer_name, duration, manual_reset = false)
      if manual_reset
        manual_reset = 1
      else
        manual_reset = 0
      end

      @_ruleset.start_timer @s.sid, timer_name, duration, manual_reset
    end

    def cancel_timer(timer_name)
      @_ruleset.cancel_timer @s.sid, timer_name
    end

    def renew_action_lease()
      if Time.now - @_start_time < 10000
        @_start_time = Time.now
        @_ruleset.renew_action_lease @s.sid
      end
    end

    def delete()
      @_deleted = true
    end

    def get_facts()
      @_ruleset.get_facts @s.sid
    end

    def get_pending_events()
      @_ruleset.get_pending_events @s.sid
    end

    def has_completed()
      if Time.now - @_start_time > 10000
        @_completed = true
      end
      value = @_completed
      @_completed = true
      value
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
        if value == nil
          @_d.delete(name[0..-2])
        else
          @_d[name[0..-2]] = value
        end
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
      @timers = Timers::Group.new

      if func.arity > 1
        @sync = false
      end
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
          c.s.exception = "#{e.to_s}, #{e.backtrace}"
        end

        if @next
          @next.run c, complete
        else
          complete.call nil
        end
      else
        begin
          time_left = @func.call c, -> e {
            if e
              c.s.exception = e.to_s
            end

            if @next
              @next.run c, complete
            else
              complete.call nil
            end
          }

          if time_left && (time_left.kind_of? Integer)
            max_time = Time.now + time_left
            my_timer = @timers.every(5) {
              if Time.now > max_time
                my_timer.cancel
                c.s.exception = "timeout expired"
                complete.call nil
              else
                c.renew_action_lease
              end
            }
            Thread.new do
              loop { @timers.wait }
            end
          end
        rescue Exception => e
          puts "unexpected error #{e}"
          puts e.backtrace
          c.s.exception = e.to_s
          complete.call nil
        end
      end
    end

  end


  class To < Promise

    def initialize(from_state, to_state, assert_state)
      super -> c {
        c.s.running = true
        if from_state != to_state
          begin
            if from_state
              if c.m && (c.m.kind_of? Array)
                c.retract c.m[0].chart_context
              else
                c.retract c.chart_context
              end
            end

            if assert_state
              c.assert(:label => to_state, :chart => 1)
            else
              c.post(:label => to_state, :chart => 1)
            end
          rescue MessageNotHandledError => e
          end
        end
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

      @handle = Rules.create_ruleset name, JSON.generate(ruleset_definition)
      @definition = ruleset_definition
    end

    def assert_event(message)
      handle_result Rules.assert_event(@handle, JSON.generate(message)), message
    end

    def assert_events(messages)
      handle_result Rules.assert_events(@handle, JSON.generate(messages)), messages
    end

    def assert_fact(fact)
      handle_result Rules.assert_fact(@handle, JSON.generate(fact)), fact
    end

    def assert_facts(facts)
      handle_result Rules.assert_facts(@handle, JSON.generate(facts)), facts
    end

    def retract_fact(fact)
      handle_result Rules.retract_fact(@handle, JSON.generate(fact)), fact
    end

    def retract_facts(facts)
      handle_result Rules.retract_facts(@handle, JSON.generate(facts)), facts
    end

    def start_timer(sid, timer, timer_duration, manual_reset)
      Rules.start_timer @handle, sid.to_s, timer_duration, manual_reset, timer
    end

    def cancel_timer(sid, timer_name)
      Rules.cancel_timer @handle, sid.to_s, timer_name.to_s
    end

    def update_state(state)
      state["$s"] = 1
      Rules.update_state @handle, JSON.generate(state)
    end

    def get_state(sid)
      if sid == nil
        sid = "0"
      end

      JSON.parse Rules.get_state(@handle, sid.to_s)
    end

    def delete_state(sid)
      if sid == nil
        sid = "0"
      end

      Rules.delete_state(@handle, sid.to_s)
    end

    def renew_action_lease(sid)
      if sid == nil
        sid = "0"
      end

      Rules.renew_action_lease @handle, sid.to_s
    end

    def get_facts(sid)
      if sid == nil
        sid = "0"
      end

      JSON.parse Rules.get_facts(@handle, sid.to_s)
    end

    def get_pending_events(sid)
      if sid == nil
        sid = "0"
      end

      JSON.parse Rules.get_events(@handle, sid)
    end

    def Ruleset.create_rulesets(host, ruleset_definitions)
      branches = {}
      for name, definition in ruleset_definitions do
        name = name.to_s
        if name.end_with? "$state"
          name = name[0..-7]
          branches[name] = Statechart.new name, host, definition
        elsif name.end_with? "$flow"
          name = name[0..-6]
          branches[name] = Flowchart.new name, host, definition
        else
          branches[name] = Ruleset.new name, host, definition
        end
      end

      branches
    end

    def dispatch_timers()
      Rules.assert_timers @handle
    end

    def to_json
      JSON.generate @definition
    end

    def do_actions(state_handle, complete)
      begin
        result = Rules.start_action_for_state(@handle, state_handle)
        if !result 
          complete.call(nil, nil)
        else
          flush_actions JSON.parse(result[0]), {:message => JSON.parse(result[1])}, state_handle, complete
        end
      rescue Exception => e
        complete.call(e, nil)
      end
    end

    def dispatch_ruleset()
      result = Rules.start_action(@handle)
      if result
        flush_actions JSON.parse(result[0]), {:message => JSON.parse(result[1])}, result[2], -> e, s { }
      end
    end

    private

    def handle_result(result, message)
      if result[0] == 1
        raise MessageNotHandledError, message
      elsif result[0] == 2
        raise MessageObservedError, message
      end

      result[1]
    end

    def flush_actions(state, result_container, state_offset, complete)
      while result_container.key? :message do
        action_name = nil
        for action_name, message in result_container[:message] do
          break
        end

        result_container.delete :message
        c = Closure.new @host, self, state, message, state_offset

        @actions[action_name].run c, -> e {
          if c.has_completed
            return
          end

          if e
            Rules.abandon_action @handle, c.handle
            complete.call e, nil
          else
            begin
              Rules.update_state @handle, JSON.generate(c.s._d)
              
              new_result = Rules.complete_and_start_action @handle, c.handle
              if new_result
                result_container[:message] = JSON.parse new_result
              else
                complete.call nil, state
              end
            rescue Exception => e
              puts "unknown exception #{e}"
              puts e.backtrace
              Rules.abandon_action @handle, c.handle
              complete.call e, nil
            end

            if c._deleted
              begin
                delete_state c.s.sid
              rescue Exception => e
              end
            end
          end
        }
        result_container[:async] = true
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
      start_state = {}
      reflexive_states = {}

      for state_name, state in chart_definition do
        next if state_name == "$type"

        qualified_name = state_name.to_s
        qualified_name = "#{parent_name}.#{state_name}" if parent_name
        start_state[qualified_name] = true

        for trigger_name, trigger in state do
          if ((trigger.key? :to) && (trigger[:to] == state_name)) ||
              ((trigger.key? "to") && (trigger["to"] == state_name)) ||
              (trigger.key? :count) || (trigger.key? "count") ||
              (trigger.key? :cap) || (trigger.key? "cap") 
            reflexive_states[qualified_name] = true
          end
        end
      end

      for state_name, state in chart_definition do
        next if state_name == "$type"
        
        qualified_name = state_name.to_s
        qualified_name = "#{parent_name}.#{state_name}" if parent_name

        triggers = {}
        if parent_triggers
          for parent_trigger_name, trigger in parent_triggers do
            parent_trigger_name = parent_trigger_name.to_s
            triggers["#{qualified_name}.#{parent_trigger_name}"] = trigger
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
          rules[parent_name + "$start"] = {:all => [{:chart_context => {:$and => [{:label => parent_name}, {:chart => 1}]}}], :run => To.new(nil, state_name, false)};
        else
          rules[:$start] = {:all => [{:chart_context => {:$and => [{:$nex => {:running => 1}}, {:$s => 1}]}}], :run => To.new(nil, state_name, false)};
        end
      end

      raise ArgumentError, "Chart #{@name} has no start state" if not started
    end
  end

  class Flowchart < Ruleset

    def initialize(name, host, chart_definition)
      @name = name
      @host = host
      ruleset_definition = {}
      transform chart_definition, ruleset_definition
      super name, host, ruleset_definition
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
                  (transition.key? :cap) || (transition.key? "cap")
                reflexive_stages[stage_name] = true
              end
            end
          end
        end
      end

      for stage_name, stage in chart_definition do
        next if stage_name == "$type"

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

    def initialize(ruleset_definitions = nil)
      @ruleset_directory = {}
      @ruleset_list = []
      
      @assert_event_func = Proc.new do |rules, arg|
        rules.assert_event arg
      end

      @assert_events_func = Proc.new do |rules, arg|
        rules.assert_events arg
      end

      @assert_fact_func = Proc.new do |rules, arg|
        rules.assert_fact arg
      end

      @assert_facts_func = Proc.new do |rules, arg|
        rules.assert_facts arg
      end

      @retract_fact_func = Proc.new do |rules, arg|
        rules.retract_fact arg
      end

      @retract_facts_func = Proc.new do |rules, arg|
        rules.retract_facts arg
      end

      @update_state_func = Proc.new do |rules, arg|
        rules.update_state arg
      end

      register_rulesets nil, ruleset_definitions if ruleset_definitions
      start_dispatch_ruleset_thread
      start_dispatch_timers_thread
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

    def set_rulesets(ruleset_definitions)
      register_rulesets ruleset_definitions
      for ruleset_name, ruleset_definition in ruleset_definitions do
        save_ruleset ruleset_name, ruleset_definition
      end
    end

    def post(ruleset_name, event, complete = nil)
      if event.kind_of? Array
        return post_batch ruleset_name, event
      end

      rules = get_ruleset(ruleset_name) 
      handle_function rules, @assert_event_func, event, complete
    end

    def post_batch(ruleset_name, events, complete = nil)
      rules = get_ruleset(ruleset_name) 
      handle_function rules, @assert_events_func, events, complete
    end

    def assert(ruleset_name, fact, complete = nil)
      if fact.kind_of? Array
        return assert_facts ruleset_name, fact
      end

      rules = get_ruleset(ruleset_name) 
      handle_function rules, @assert_fact_func, fact, complete
    end

    def assert_facts(ruleset_name, facts, complete = nil)
      rules = get_ruleset(ruleset_name) 
      handle_function rules, @assert_facts_func, facts, complete
    end

    def retract(ruleset_name, fact, complete = nil)
      if fact.kind_of? Array
        return retract_facts ruleset_name, fact
      end

      rules = get_ruleset(ruleset_name) 
      handle_function rules, @retract_fact_func, fact, complete
    end

    def retract_facts(ruleset_name, facts, complete = nil)
      rules = get_ruleset(ruleset_name) 
      handle_function rules, @retract_facts_func, facts, complete
    end

    def update_state(ruleset_name, state, complete = nil)
      rules = get_ruleset(ruleset_name) 
      handle_function rules, @update_state_func, state, complete
    end

    def get_state(ruleset_name, sid = nil)
      get_ruleset(ruleset_name).get_state sid
    end

    def delete_state(ruleset_name, sid = nil)
      get_ruleset(ruleset_name).delete_state sid
    end

    def renew_action_lease(ruleset_name, sid = nil)
      get_ruleset(ruleset_name).renew_action_lease sid
    end

    def get_facts(ruleset_name, sid = nil)
      get_ruleset(ruleset_name).get_facts sid
    end

    def get_pending_events(ruleset_name, sid = nil)
      get_ruleset(ruleset_name).get_pending_events sid
    end

    def register_rulesets(ruleset_definitions)
      rulesets = Ruleset.create_rulesets(self, ruleset_definitions)
      for ruleset_name, ruleset in rulesets do
        if @ruleset_directory.key? ruleset_name
          raise ArgumentError, "Ruleset with name #{ruleset_name} already registered"
        end

        @ruleset_directory[ruleset_name] = ruleset
        @ruleset_list << ruleset
      end

      rulesets.keys
    end

    private

    def handle_function(rules, func, args, complete)
      error = nil
      result = nil

      if not complete
        rules.do_actions func.call(rules, args), -> e, s {
          error = e
          result = s
        }

        if error
          raise error
        end

        result
      else
        begin
          rules.do_actions func.call(rules, args), complete
        rescue Exception => e
          complete.call e, nil
        end
      end 
    end

    def start_dispatch_timers_thread

      timer = Timers::Group.new

      thread_lambda = -> c {
        if @ruleset_list.empty?
          timer.after 0.5, &thread_lambda
        else
          ruleset = @ruleset_list[Thread.current[:index]]
          begin
            ruleset.dispatch_timers
          rescue Exception => e
            puts e.backtrace
            puts "Error #{e}"
          end
      
          timeout = 0
          if (Thread.current[:index] == (@ruleset_list.length-1))
            timeout = 0.2
          end
          Thread.current[:index] = (Thread.current[:index] + 1) % @ruleset_list.length
          timer.after timeout, &thread_lambda
        end
      }

      timer.after 0.1, &thread_lambda

      Thread.new do
        Thread.current[:index] = 0
        loop { timer.wait }
      end
    end

    def start_dispatch_ruleset_thread

      timer = Timers::Group.new

      thread_lambda = -> c {
        if @ruleset_list.empty?
          timer.after 0.5, &thread_lambda
        else
          ruleset = @ruleset_list[Thread.current[:index]]
          begin
            ruleset.dispatch_ruleset
          rescue Exception => e
            puts e.backtrace
            puts "Error #{e}"
          end
      
          timeout = 0
          if (Thread.current[:index] == (@ruleset_list.length-1))
            timeout = 0.2
          end
          Thread.current[:index] = (Thread.current[:index] + 1) % @ruleset_list.length
          timer.after timeout, &thread_lambda
        end
      }

      timer.after 0.1, &thread_lambda

      Thread.new do
        Thread.current[:index] = 0
        loop { timer.wait }
      end
    end
  end

end
