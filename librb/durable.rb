require_relative "engine"
require_relative "interface"

module Durable
  @@rulesets = {}
  @@start_blocks = []

  def self.run(ruleset_definitions = nil, databases = ["/tmp/redis.sock"], start = nil)
    main_host = Engine::Host.new ruleset_definitions, databases
    start.call main_host if start
    main_host.start!
    Interface::Application.set_host main_host
    Interface::Application.run!
  end

  def self.run_all(databases = ["/tmp/redis.sock"])
    main_host = Engine::Host.new @@rulesets, databases
    for block in @@start_blocks
      main_host.instance_exec main_host, &block
    end
    main_host.start!
    Interface::Application.set_host main_host
    Interface::Application.run!
  end

  def self.ruleset(name, &block) 
    ruleset = Ruleset.new name, block
    @@rulesets[name] = ruleset.rules
    @@start_blocks << ruleset.start if ruleset.start
  end

  def self.statechart(name, &block)
    statechart = Statechart.new name, block
    @@rulesets[name.to_s + "$state"] = statechart.states
    @@start_blocks << statechart.start if statechart.start
  end

  def self.flowchart(name, &block)
    flowchart = Flowchart.new name, block
    @@rulesets[name.to_s + "$flow"] = flowchart.stages
    @@start_blocks << flowchart.start if flowchart.start
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

        righ_definition = @right
        if @right.kind_of? Arithmetic
          righ_definition = @right.definition
        end

        return {@op => {:$l => left_definition, :$r => righ_definition}}
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

    def id(sid)
      Arithmetic.new @name, @left, sid
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
      @left = name
      self
    end

    alias method_missing default

  end

  class Expression
    attr_reader :__type, :__op
    attr_accessor :__name

    def initialize(type, left = nil)
      @__type = type
      @left = left
      @right = nil
      @definitions = nil
      @__name = nil
    end
    
    def definition
      new_definition = nil
      if @__op == :$or || @__op == :$and
        new_definition = {@__op => @definitions}
      else
        if not @left
          raise ArgumentError, "Property for #{@__op} not defined"
        end 
        righ_definition = @right
        if (@right.kind_of? Expression) || (@right.kind_of? Arithmetic)
          righ_definition = @right.definition
        end

        if @__op == :$eq
          new_definition = {@left => righ_definition}
        else
          new_definition = {@__op => {@left => righ_definition}}
        end
      end

      if @__type == :$s
        {:$and => [new_definition, {:$s => 1}]}
      else
        new_definition
      end
    end

    def ==(other)
      @__op = :$eq
      @right = other
      self
    end

    def !=(other)
      @__op = :$neq
      @right = other
      self
    end

    def <(other)
      @__op = :$lt
      @right = other
      self
    end

    def <=(other)
      @__op = :$lte
      @right = other
      self
    end

    def >(other)
      @__op = :$gt
      @right = other
      self
    end

    def >=(other)
      @__op = :$gte
      @right = other
      self
    end

    def -@
      @__op = :$nex
      @right = 1
      self
    end

    def +@
      @__op = :$ex
      @right = 1
      self
    end

    def |(other)
      merge other, :$or
      self
    end

    def &(other)
      merge other, :$and
      self
    end

    private 

    def default(name, value=nil)
      @left = name
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

    def definition
      index = 0
      new_definition = []
      for expression in @expressions do
        if (expression.kind_of? Expression) && expression.__name
          expression_name = expression.__name
        elsif @expressions.length == 1 
          expression_name = "m"
        else
          expression_name = "m_#{index}"
        end
        if expression.__type == :$all
          new_definition << {expression_name + "$all" => expression.definition()}
        elsif expression.__type == :$any
          new_definition << {expression_name + "$any" => expression.definition()}
        elsif  expression.__type == :$not
          new_definition << {expression_name + "$not" => expression.definition()[0]["m"]}
        else
          new_definition << {expression_name => expression.definition()}
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

    def paralel
      {:paralel => true}
    end

    def ruleset(name, &block)
      @paralel_rulesets[name] = Ruleset.new(name, block).rules
    end

    def statechart(name, &block)
      @paralel_rulesets[name.to_s + "$state"] = Statechart.new(name, block).states
    end

    def flowchart(name, &block)
      @paralel_rulesets[name.to_s + "$flow"] = Flowchart.new(name, block).stages
    end

    def s
      Arithmetic.new(:$s)
    end
    
    def m
      Expression.new(:$m)
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

    def span(value)
      {:span => value}
    end

    def timeout(name)
      expression = Expression.new(:$m)
      expression.left = :$t
      expression == name
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
      if options.key? :paralel
        @paralel_rulesets = {}
        self.instance_exec &block
        rule = operator ? {operator => expression_definition, :run => @paralel_rulesets} : {:run => @paralel_rulesets}
      elsif block
        run_lambda = -> c {
          c.instance_exec c, &block
        }
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

      if options.key? :span
        rule["span"] = options[:span]
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
  
    def to(state_name, rule = nil, paralel = {}, &block)
      rule = define_rule(nil, nil, paralel, &block) if !rule
      rule[:to] = state_name
      if paralel.key? :paralel
        @paralel_rulesets = {}
        self.instance_exec &block
        rule[:run] = @paralel_rulesets
      elsif block   
        rule[:run] = -> s {s.instance_exec(s, &block)}
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

    def stage(stage_name, paralel = nil, &block)
      if paralel
        @paralel_rulesets = {}
        self.instance_exec &block
        @stages[stage_name] = {:run => @paralel_rulesets, :to => {}}
      elsif block
        @stages[stage_name] = {:run => -> s {s.instance_exec(s, &block)}, :to => {}}
      else
        @stages[stage_name] = {:to => {}}
      end

      @current_stage = stage_name
      self
    end
  end

end
