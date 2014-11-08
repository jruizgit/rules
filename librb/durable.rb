require_relative "engine"


module Durable
  @@rulesets = {}
  @@start_blocks = []

  def self.run(ruleset_definitions = nil, databases = ["/tmp/redis.sock"], start = nil)
    main_host = Engine::Host.new ruleset_definitions, databases
    start.call main_host if start
    main_host.run
  end

  def self.run_all(databases = ["/tmp/redis.sock"])
    main_host = Engine::Host.new @@rulesets, databases
    for block in @@start_blocks
      main_host.instance_exec(main_host, &block)
    end
    main_host.run
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
      new_definition = nil
      if @op == :$or || @op == :$and
        new_definition = {@op => @definitions}
      elsif @op == :$eq
        new_definition = {@left => @right}
      else
        new_definition = {@op => {@left => @right}}
      end

      if @type == :$s
        {:$s => new_definition}
      else
        new_definition
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

  class Ruleset
    attr_reader :name, :start
    attr_accessor :rules

    def initialize(name, block)
      @name = name
      @rules = {}
      @rule_index = 0
      @expression_index = 0
      @start = nil
      self.instance_eval &block
    end        

    def when_one(expression, &block)
      if block
        add_rule :when => expression.definition, :run => -> s {s.instance_exec(s, &block)}
      else
        add_rule :when => expression.definition
      end
    end

    def when_some(expression, &block)
      if block
        add_rule :whenSome => expression.definition, :run => -> s {s.instance_exec(s, &block)}
      else
        add_rule :whenSome => expression.definition
      end
    end

    def when_all(expressions, &block)
      if block
        add_rule :whenAll => define(expressions), :run => -> s {s.instance_exec(s, &block)}
      else
        add_rule :whenAll => define(expressions)
      end
    end

    def when_any(expressions, &block)
      if block
        add_rule :whenAny => define(expressions), :run => -> s {s.instance_exec(s, &block)}
      else
        add_rule :whenAny => define(expressions)
      end 
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
    
    private

    def add_rule(rule)
      @rules[rule_name] = rule
      rule
    end

    def define(expressions)
      new_definition = {}
      for expression in expressions do
        if expression.type == :$s
          new_definition[:$s] = expression.definition[:$s]
        else
          new_definition[expression_name] = expression.definition
        end
      end
      new_definition
    end

    def rule_name
      index = @rule_index.to_s
      @rule_index += 1
      "r_#{index}"
    end

    def expression_name
      index = @expression_index.to_s
      @expression_index += 1
      "m_#{index}"
    end

  end


  class Statechart < Ruleset
    attr_reader :states

    def initialize(name, block)
      @states = {}
      @trigger_index = 0
      super name, block
    end

    def to(state_name, rule, &block)
      rule[:to] = state_name
      rule[:run] = -> s {s.instance_exec(s, &block)} if block
      self
    end

    def state(state_name, &block)
      self.instance_eval &block if block
      @states[state_name] = self.rules
      self.rules = {}
    end

    private

    def trigger_name
      index = @trigger_index.to_s
      @trigger_index += 1
      "t_#{index}"
    end

  end
end
