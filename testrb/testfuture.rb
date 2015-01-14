require_relative '../librb/durable'

Durable.ruleset :fraud do
  when_ first = m.t == "purchase",
        second = m.location != first.location do
    puts "Fraud #{first.location}, #{second.location}"
  end
end

:fraud => {
  :r_0 => {
    :whenAll => {
      :first => {:t => "purchase"}, 
      :second => {:$neq => {:location => {:first => :location}}},
    },
  },
}

local left = "r_0!fraud!first!" .. ARGV[2]
local right = "r_0!fraud!second!" .. ARGV[2]
local target = "r_0!fraud!a"
local i = 0
local res = redis.call("zrange", right, i, 1)
while (res[1]) do
  local right_values = "fraud!m!" .. res[1]
  local right_location = redis.call("hget", right_values, "location")
  if (right_location != ARGV[3]) then
    local new_sig = res[1] .. "," .. ARGV[4]
    redis.call("zrem", right, res[1])
    redis.call("zadd", target, ARGV[5], new_sig)
    return true
  end
  i = i + 1
  res = redis.call("zrange", right, i, 1)
end
redis.call("zadd", left, ARGV[4])
return false

local left = "r_0!fraud!first!" .. ARGV[2]
local right = "r_0!fraud!second!" .. ARGV[2]
local target = "r_0!fraud!a"
local i = 0
local res = redis.call("zrange", left, i, 1)
while (res[1]) do
  local left_values = "fraud!m!" .. res[1]
  local left_location = redis.call("hget", left_values, "location")
  if (left_location != ARGV[3]) then
    local new_sig = res[1] .. "," .. ARGV[4]
    redis.call("zrem", left, res[1])
    redis.call("zadd", target, ARGV[5], new_sig)
    return true
  end
  i = i + 1
  res = redis.call("zrange", left, i, 1)
end
redis.call("zadd", right, ARGV[4])
return false


Durable.ruleset :fraud2 do
  when_ first = m.t == 'purchase',
        second = (m.t == 'purchase') & (m.ip == first.ip) & (m.cc != first.cc),
        third = (m.t == 'purchase') & (m.ip == second.ip) & (m.cc != first.cc) & (m.cc != second.cc) do
    puts "Fraud #{first.cc}, #{second.cc}, #{third.cc}"
  end
end

:fraud => {
  :r_0 => {
    :whenAll => {
      :first => {:t => "purchase"}, 
      :second => {:t => "purchase", :ip => {:first => {:$name => :ip}}, :$neq => {:cc => {:first => :cc}}},
      :third => {:t => "purchase", :ip => {:second => {:$name => :ip}}, :$neq => {:cc => {:second => :cc}}, :$neq => {:cc => {:first => :cc}}}
    },
  },
}


Durable.ruleset :fraud3 do
  state :start do
    to :standby
  end
  state :standby do
    to :metering, when_ m.transaction == "purchase" do
      s.ip = m.ip
      s.cc = m.cc
      s.start_timer :velocity, 60
    end
  end
  state :metering do
    to :fraud, when_ (s.ip).as first,
              ((m.t == "purchase") & (m.ip == first.ip) & (m.cc != first.cc)).as second,
              ((m.t == "purchase") & (m.ip == second.ip) & (m.cc != first.cc) & (m.cc != second.cc)).as third do
      puts "Fraud #{first.cc}, #{second.cc}, #{third.cc}"
    end
    to :standby, when_ timeout(:velocity) do
      puts "Fraud cleared"
    end
  end
  state :fraud
end