require_relative "engine"


module Durable
  def self.run(ruleset_definitions = nil, databases = ["/tmp/redis.sock"], start = nil)
    main_host = Engine::Host.new ruleset_definitions, databases
    start.call main_host if start
    main_host.run
  end

  def self.run_ruleset(name, databases = ["/tmp/redis.sock"], &block) 
    ruleset = Ruleset.new name
    ruleset.instance_eval &block
    main_host = Engine::Host.new({ruleset.name => ruleset.rules}, databases)
    if ruleset.start
      main_host.instance_exec main_host, &ruleset.start
    end
    main_host.run
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
    attr_reader :name, :rules, :start

    def initialize(name)
      @name = name
      @expressions = []
      @rules = {}
      @rule_index = 0
      @expression_index = 0
      @start = nil
    end        

    def when_one(expression, &block)
      @rules[rule_name] = {:when => @expressions[0].definition, :run => -> s {s.instance_exec(s, &block)}}
      @expressions = []
      self
    end

    def when_some(expression, &block)
      @rules[rule_name] = {:whenSome => @expressions[0].definition, :run => -> s {s.instance_exec(s, &block)}}
      @expressions = []
      self
    end

    def when_all(expression, &block)
      @rules[rule_name] = {:whenAll => expressions_definition, :run => -> s {s.instance_exec(s, &block)}}
      self
    end

    def when_any(expression, &block)
      @rules[rule_name] = {:whenAny => expressions_definition, :run => -> s {s.instance_exec(s, &block)}}
      self
    end

    def when_start(&block)
      @start = block
      self
    end

    def s
      new_expression = Expression.new(:$s)
      @expressions << new_expression
      new_expression
    end
    
    def m
      new_expression = Expression.new(:$m)
      @expressions << new_expression
      new_expression
    end
    
    private

    def default(name, value = nil)
      new_expression = Expression.new(:$s)
      new_expression.left = name
      @expressions << new_expression
      new_expression
    end

    def expressions_definition
      new_definition = {}
      for expression in @expressions do
        if expression.type == :$s
          new_definition[:$s] = expression.definition[:$s]
        else
          new_definition[expression_name] = expression.definition
        end
      end
      @expressions = []
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

    alias method_missing default
  end
end
