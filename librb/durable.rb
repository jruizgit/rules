require_relative "engine"

module Durable
  @@rulesets = {}
  @@main_host = nil

  def self.get_host()
    if !@@main_host
      @@main_host = Engine::Host.new
    end

    begin
      @@main_host.register_rulesets @@rulesets
    ensure
      @@rulesets = {}
    end
    
    @@main_host 
  end

  def self.post(ruleset_name, message, complete = nil)
    self.get_host().post ruleset_name, message, complete
  end
     
  def self.post_batch(ruleset_name, messages, complete = nil)
    self.get_host().post_batch ruleset_name, messages, complete
  end

  def self.assert(ruleset_name, fact, complete = nil)
    self.get_host().assert ruleset_name, fact, complete
  end

  def self.assert_facts(ruleset_name, facts, complete = nil)
    self.get_host().assert_facts ruleset_name, facts, complete
  end
  
  def self.retract(ruleset_name, fact, complete = nil)
    self.get_host().retract ruleset_name, fact, complete
  end
    
  def self.retract_facts(ruleset_name, facts, complete = nil)
    self.get_host().retract_facts ruleset_name, facts, complete
  end
  
  def self.update_state(ruleset_name, state, complete = nil)
    self.get_host().update_state ruleset_name, state, complete
  end

  def self.get_state(ruleset_name, sid = nil)
    self.get_host().get_state ruleset_name, sid
  end

  def self.delete_state(ruleset_name, sid = nil)
    self.get_host().delete_state ruleset_name, sid
  end

  def self.get_facts(ruleset_name, sid = nil)
    self.get_host().get_facts ruleset_name, sid
  end

  def self.get_pending_events(ruleset_name, sid = nil)
    self.get_host().get_pending_events ruleset_name, sid
  end

  def self.ruleset(name, &block) 
    ruleset = Ruleset.new name, block
    @@rulesets[name] = ruleset.rules
  end

  def self.statechart(name, &block)
    statechart = Statechart.new name, block
    @@rulesets[name.to_s + "$state"] = statechart.states
  end

  def self.flowchart(name, &block)
    flowchart = Flowchart.new name, block
    @@rulesets[name.to_s + "$flow"] = flowchart.stages
  end

  class Arithmetic

    def initialize(name, left = nil, sid = nil, op = nil, right = nil)
      @name = name
      @left = left
      @sid = sid
      @right = right
      @op = op
    end
    
    def definition
      if not @op
        if @sid
          {@name => {:name => @left, :id => @sid}}
        else
          {@name => @left}
        end
      else
        new_definition = nil
        left_definition = nil
        if @left.kind_of? Arithmetic
          left_definition = @left.definition
        else
          left_definition = {@name => @left}
        end

        right_definition = @right
        if @right.kind_of? Arithmetic
          right_definition = @right.definition
        end

        return {@op => {:$l => left_definition, :$r => right_definition}}
      end
    end

    def +(other)
      set_right(:$add, other)
    end

    def -(other)
      set_right(:$sub, other)
    end

    def *(other)
      set_right(:$mul, other)
    end

    def /(other)
      set_right(:$div, other)
    end

    def ==(other)
      return Expression.new(@name, @left) == other
    end

    def !=(other)
      return Expression.new(@name, @left) != other
    end

    def <(other)
      return Expression.new(@name, @left) < other
    end

    def <=(other)
      return Expression.new(@name, @left) <= other
    end

    def >(other)
      return Expression.new(@name, @left) > other
    end

    def >=(other)
      return Expression.new(@name, @left) >= other
    end

    def -@
      return -Expression.new(@name, @left)
    end

    def +@
      return +Expression.new(@name, @left)
    end

    def |(other)
      return Expression.new(@name, @left) | other
    end

    def &(other)
      return Expression.new(@name, @left) & other
    end

    def matches(other)
      return Expression.new(@name, @left).matches(other)
    end

    def imatches(other)
      return Expression.new(@name, @left).imatches(other)
    end


    private 

    def set_right(op, other) 
      if @right
        @left = Arithmetic.new @name, @left, @sid, @op, @right 
      end

      @op = op
      @right = other
      self
    end

    def default(name, value=nil)
      if @left
        @left = "#{@left}.#{name}"
      else
        @left = name
      end
      self
    end

    alias method_missing default

  end

  class Expression
    attr_reader :__type, :__op
    attr_accessor :__name

    def initialize(type, left = nil, op = nil, right = nil, definitions = nil, name = nil)
      @__type = type
      @left = left
      @__op = op
      @right = right
      @definitions = definitions
      @__name = name
    end
    
    def definition(parent_name=nil)
      new_definition = nil

      if @__op == :$or || @__op == :$and
        new_definition = {@__op => @definitions}
      else
        if not @left
          raise ArgumentError, "Property for #{@__type} and #{@__op} not defined"
        end 
        right_definition = @right
        if (@right.kind_of? Expression) || (@right.kind_of? Arithmetic)
          right_definition = @right.definition
        end

        if @__op == :$eq
          new_definition = {@left => right_definition}
        else
          if not @right
            new_definition = {@__type => @left}
          else
            new_definition = {@__op => {@left => right_definition}}
          end
        end
      end

      if @__type == :$s
        {:$and => [new_definition, {:$s => 1}]}
      else
        new_definition
      end
    end

    def ==(other)
      Expression.new(@__type, @left, :$eq, other, @definitions, @__name)
    end

    def !=(other)
      Expression.new(@__type, @left, :$neq, other, @definitions, @__name)
    end

    def <(other)
      Expression.new(@__type, @left, :$lt, other, @definitions, @__name)
    end

    def <=(other)
      Expression.new(@__type, @left, :$lte, other, @definitions, @__name)
    end

    def >(other)
      Expression.new(@__type, @left, :$gt, other, @definitions, @__name)
    end

    def >=(other)
      Expression.new(@__type, @left, :$gte, other, @definitions, @__name)
    end

    def matches(other)
      Expression.new(@__type, @left, :$mt, other, @definitions, @__name)
    end

    def imatches(other)
      Expression.new(@__type, @left, :$imt, other, @definitions, @__name)
    end

    def allItems(other)
      Expression.new(@__type, @left, :$iall, other, @definitions, @__name)
    end

    def anyItem(other)
      Expression.new(@__type, @left, :$iany, other, @definitions, @__name)
    end

    def -@
      Expression.new(@__type, @left, :$nex, 1, @definitions, @__name)
    end

    def +@
      Expression.new(@__type, @left, :$ex, 1, @definitions, @__name)
    end

    def |(other)
      merge other, :$or
      self
    end

    def &(other)
      merge other, :$and
      self
    end

    def +(other)
      return Arithmetic.new(@__type, @left, nil, :$add, other)
    end

    def -(other)
      return Arithmetic.new(@__type, @left, nil, :$sub, other)
    end

    def *(other)
      return Arithmetic.new(@__type, @left, nil, :$mul, other)
    end

    def /(other)
      return Arithmetic.new(@__type, @left, nil, :$div, other)
    end

    private 

    def default(name, value=nil)
      if @left
        @left = "#{@left}.#{name}"
      else
        @left = name
      end
      self
    end

    def merge(other, op)
      raise ArgumentError, "Right type doesn't match" if other.__type != @__type
      @definitions = [self.definition] if !@definitions
      @__op = op
      if other.__op && (other.__op == @__op)
        @definitions + other.definition[@__op] 
      else 
        @definitions << other.definition
      end
    end

    alias method_missing default

  end

  class Expressions
    attr_reader :__type

    def initialize(type, expressions)
      @__type = type
      @expressions = expressions
    end

    def definition(parent_name=nil)
      index = 0
      new_definition = []
      for expression in @expressions do
        if (expression.kind_of? Expression) && expression.__name
          expression_name = expression.__name
        elsif @expressions.length == 1 
          if parent_name
            expression_name = "#{parent_name}.m"
          else
            expression_name = "m"
          end
        else
          if parent_name
            expression_name = "#{parent_name}.m_#{index}"
          else
            expression_name = "m_#{index}"
          end
        end
        if expression.__type == :$all
          new_definition << {expression_name + "$all" => expression.definition(expression_name)}
        elsif expression.__type == :$any
          new_definition << {expression_name + "$any" => expression.definition(expression_name)}
        elsif  expression.__type == :$not
          new_definition << {expression_name + "$not" => expression.definition()[0]["m"]}
        else
          new_definition << {expression_name => expression.definition(expression_name)}
        end
        index += 1
      end
      new_definition
    end

  end

  class Closure

    def s
      Arithmetic.new(:$s)
    end

    private

    def handle_property(name, value=nil)
      name = name.to_s
      if name.end_with? '='
        name = name[0..-2] 
        value.__name = name
        return value        
      else
        Arithmetic.new(name)
      end
    end

    alias method_missing handle_property
  end

  class Ruleset
    attr_reader :name, :start
    attr_accessor :rules

    def initialize(name, block)
      @name = name
      @rules = {}
      @rule_index = 0
      @expression_index = 0
      @start = nil
      self.instance_exec &block
    end        

    def when_all(*args, &block)
      options, new_args = get_options(*args)
      define_rule :all, Expressions.new(:$all, new_args).definition, options, &block
    end

    def when_any(*args, &block)
      options, new_args = get_options(*args)
      define_rule :any, Expressions.new(:$any, new_args).definition, options, &block
    end

    def all(*args)
      Expressions.new :$all, args
    end

    def any(*args)
      Expressions.new :$any, args
    end

    def none(*args)
      Expressions.new :$not, args
    end

    def when_start(&block)
      @start = block
      self
    end

    def s
      Expression.new(:$s)
    end
    
    def m
      Expression.new(:$m)
    end
    
    def item
      Expression.new(:$i, :$i)
    end

    def c
      Closure.new()
    end

    def count(value)
      {:count => value}
    end

    def pri(value)
      {:pri => value}
    end

    def cap(value)
      {:cap => value}
    end

    def distinct(value)
      {:dist => value}
    end

    def timeout(name)
      all(c.base = Expression.new(:$m, "$timerName") == name, 
          c.timeout = Expression.new(:$m, "$time") >= Arithmetic.new(:base, "$baseTime"))
    end
    
    protected

    def get_options(*args)
      options = {}
      new_args = []
      for arg in args do
        if arg.kind_of? Hash
          options = options.merge!(arg)
        else
          new_args << arg 
        end
      end
      return options, new_args
    end

    def define_rule(operator, expression_definition, options, &block)
      index = @rule_index.to_s
      @rule_index += 1
      rule_name = "r_#{index}"
      rule = nil
      if block
        run_lambda = nil
        if block.arity < 2
          run_lambda = -> c {
            c.instance_exec c, &block
          }
        else
          run_lambda = -> (c, complete) {
            c.instance_exec c, complete, &block
          }
        end

        rule = operator ? {operator => expression_definition, :run => run_lambda} : {:run => run_lambda}
      else
        rule = operator ? {operator => expression_definition} : {}
      end

      if options.key? :count
        rule["count"] = options[:count]
      end

      if options.key? :pri
        rule["pri"] = options[:pri]
      end

      if options.key? :cap
        rule["cap"] = options[:cap]
      end

      if options.key? :dist
        if (options[:dist])
          rule["dist"] = 1
        else
          rule["dist"] = 0
        end
      end

      @rules[rule_name] = rule
      rule
    end

    private

    def handle_property(name, value=nil)
      return Arithmetic.new(name.to_s)
    end

    alias method_missing handle_property

  end


  class State < Ruleset

    def initialize(name, block)
      super name, block
    end
  
    def to(state_name, rule = nil, &block)
      rule = define_rule(nil, nil, {}, &block) if !rule
      rule[:to] = state_name
      if block
        if block.arity < 2
          rule[:run] = -> c {
            c.instance_exec c, &block
          }
        else
          rule[:run] = -> (c, complete) {
            c.instance_exec c, complete, &block
          }
        end
      else 
        rule[:run] = -> c {}
      end
      self
    end

    def state(state_name, &block)
      @rules[:$chart] = {} if (!@rules.key? :$chart)
      if block
        @rules[:$chart][state_name] = State.new(state_name, block).rules
      else
        @rules[:$chart][state_name] = {}
      end
    end

  end

  class Statechart
    attr_reader :states, :start

    def initialize(name, block)
      @states = {}
      self.instance_exec &block
    end

    def state(state_name, &block)
      if block
        states[state_name] = State.new(state_name, block).rules 
      else
        states[state_name] = {}
      end
    end

    def when_start(&block)
      @start = block
      self
    end

  end

  class Flowchart < Ruleset
    attr_reader :stages

    def initialize(name, block)
      @stages = {}
      @current_stage = nil
      super name, block
    end

    def to(stage_name, rule = nil)
      if !rule
        stages[@current_stage][:to] = stage_name
      elsif rule.key? :all
        stages[@current_stage][:to][stage_name] = {:all => rule[:all]}
      else
        stages[@current_stage][:to][stage_name] = {:any => rule[:any]}
      end
    end

    def stage(stage_name, &block)
      if block
        run_lambda = nil
        if block.arity < 2
          run_lambda = -> c {
            c.instance_exec c, &block
          }
        else
          run_lambda = -> (c, complete) {
            c.instance_exec c, complete, &block
          }
        end

        @stages[stage_name] = {:run => run_lambda, :to => {}}
      else
        @stages[stage_name] = {:to => {}}
      end

      @current_stage = stage_name
      self
    end
  end

end
