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

  class Expression
    attr_reader :type, :op
    attr_accessor :left

    def initialize(type)
      @type = type
      @left = nil
      @right = nil
      @definitions = nil
    end
    
    def definition
      if not @op
        {@type => @left}
      else
        new_definition = nil
        righ_definition = @right
        if @right.kind_of? Expression
          righ_definition = @right.definition
        end

        if @op == :$or || @op == :$and
          new_definition = {@op => @definitions}
        elsif @op == :$eq
          new_definition = {@left => righ_definition}
        else
          new_definition = {@op => {@left => righ_definition}}
        end

        if @type == :$s
          {:$s => new_definition}
        else
          new_definition
        end
      end
    end

    def ==(other)
      @op = :$eq
      @right = other
      self
    end

    def !=(other)
      @op = :$neq
      @right = other
      self
    end

    def <(other)
      @op = :$lt
      @right = other
      self
    end

    def <=(other)
      @op = :$lte
      @right = other
      self
    end

    def >(other)
      @op = :$gt
      @right = other
      self
    end

    def >=(other)
      @op = :$gte
      @right = other
      self
    end

    def !
      @op = :$nex
      @right = 1
      self
    end

    def ~
      @op = :$ex
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
      raise ArgumentError, "Right type doesn't match" if other.type != @type
      @definitions = [self.definition] if !@definitions
      @op = op
      if other.op == @op
        @definitions + other.definition[@op] 
      else 
        @definitions << other.definition
      end
    end

    alias method_missing default

  end

  class Expressions
    attr_reader :type

    def initialize(type, expressions)
      @type = type
      @expressions = expressions
    end

    def definition
      index = 0
      new_definition = {}
      for expression in @expressions do
        expression_name = "m_#{index}"
        if expression.type == :$s
          new_definition[:$s] = expression.definition[:$s]
        elsif expression.type == :$all
          new_definition[expression_name + "$all"] = expression.definition
        elsif expression.type == :$any
          new_definition[expression_name + "$any"] = expression.definition
        else
          new_definition[expression_name] = expression.definition
        end
        index += 1
      end
      new_definition
    end

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

    def when_one(expression, paralel = nil, &block)
      define_rule :when, expression.definition, paralel, &block
    end

    def when_some(expression, paralel = nil, &block)
      define_rule :whenSome, expression.definition, paralel, &block
    end

    def when_all(*args, &block)
      if (args[-1].kind_of? Expression) || (args[-1].kind_of? Expressions) 
        define_rule :whenAll, Expressions.new(:$all, args).definition, nil, &block
      else
        define_rule :whenAll, Expressions.new(:$all, args[0..-1]).definition, args[-1], &block
      end
    end

    def when_any(*args, &block)
      if (args[-1].kind_of? Expression) || (args[-1].kind_of? Expressions)
        define_rule :whenAny, Expressions.new(:$any, args).definition, nil, &block
      else
        define_rule :whenAny, Expressions.new(:$any, args[0..-1]).definition, args[-1], &block
      end
    end

    def all(*args)
      Expressions.new :$all, args
    end

    def any(*args)
      Expressions.new :$any, args
    end

    def when_start(&block)
      @start = block
      self
    end

    def paralel
      true
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
      Expression.new(:$s)
    end
    
    def m
      Expression.new(:$m)
    end
    
    def timeout(name)
      expression = Expression.new(:$m)
      expression.left = :$t
      expression == name
    end
    
    protected

    def define_rule(operator, expression_definition, paralel, &block)
      index = @rule_index.to_s
      @rule_index += 1
      rule_name = "r_#{index}"
      rule = nil
      if paralel
        @paralel_rulesets = {}
        self.instance_exec &block
        rule = operator ? {operator => expression_definition, :run => @paralel_rulesets} : {:run => @paralel_rulesets}
      elsif block
        rule = operator ? {operator => expression_definition, :run => -> s {s.instance_exec s, &block}} : {:run => -> s {s.instance_exec s, &block}}
      else
        rule = operator ? {operator => expression_definition} : {}
      end
      @rules[rule_name] = rule
      rule
    end

  end


  class State < Ruleset

    def initialize(name, block)
      super name, block
    end
  
    def to(state_name, rule = nil, paralel = nil, &block)
      rule = define_rule(nil, nil, paralel, &block) if !rule
      rule[:to] = state_name
      if paralel
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
      elsif rule.key? :when
        stages[@current_stage][:to][stage_name] = rule[:when]
      elsif rule.key? :whenAll
        stages[@current_stage][:to][stage_name] = {:$all => rule[:whenAll]}
      elsif rule.key? :whenSome
        stages[@current_stage][:to][stage_name] = {:$some => rule[:whenSome]}
      else
        stages[@current_stage][:to][stage_name] = {:$any => rule[:whenAny]}
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
