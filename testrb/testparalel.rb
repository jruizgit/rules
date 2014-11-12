require_relative "../librb/durable"

def finish_one
  -> s {
    puts "finish one"
    s.signal :id => 1, :end => "one"
    s.start = 2
  }
end

def finish_two
  -> s {
    puts "finish two"
    s.signal :id => 1, :end => "two"
    s.start = 2
  }
end

def continue_flow
  -> s {
    s.start = 1
  }
end

def done
  -> s {
    puts "done"
  }
end

def get_input
  -> s {
    quantity = s.event["quantity"]
    puts "input #{quantity} from: #{s.ruleset_name}, #{s.id}"
    s.quantity = quantity
  }
end

def signal_approved
  -> s {
    puts "signaling approved from: #{s.ruleset_name}, #{s.id}"
    s.signal :id => 1, :subject => "approved"  
  }
end 

def signal_denied
  -> s {
    puts "signaling denied from: #{s.ruleset_name}, #{s.id}"
    s.signal :id => 1, :subject => "denied"  
  }
end

def report_approved
  -> s {
    puts "approved from: #{s.ruleset_name}, #{s.id}"
  }
end

def report_denied
  -> s {
    puts "denied from: #{s.ruleset_name}, #{s.id}"
  }
end

Durable.run({
	:p1 => {
    :r1 => {
      :when => {:start => "yes"},
      :run => {
        :one => {
          :r1 => {:when => {:$s => {:$nex => {:start => 1}}}, :run => continue_flow},
          :r2 => {:when => {:$s => {:start => 1}}, :run => finish_one}
        },
        :two => {
          :r1 => {:when => {:$s => {:$nex => {:start => 1}}}, :run => continue_flow },
          :r2 => {:when => {:$s => {:start => 1}}, :run => finish_two}
        }
      }
    },
    :r2 => {:whenAll => {:first => {:end => "one"}, :second => {:end => "two"}}, :run => done}
  },
  "p2$state" => {
    :input => {
      :get => {:when => {:subject => "approve"}, :run => get_input, :to => "process"},
    },
    :process => {
      :review => {
        :when => {:$s => {:$ex => {:quantity => 1}}},
        :run => {
          "first$state" => {
            :evaluate => {
              :wait => {:when => {:$s => {:$lte => {:quantity => 5}}}, :run => signal_approved, :to => "end"}
            },
            :end => {}
          },
          "second$state" => {
            :evaluate => {
              :wait => {:when => {:$s => {:$gt => {:quantity => 5}}}, :run => signal_denied, :to => "end"}
            },
            :end => {}
          }
        },
        :to => :result
      }
    },
    :result => {
      :approve => {:when => {:subject => "approved"}, :run => report_approved, :to => "approved"},
      :deny => {:when => {:subject => "denied"}, :run => report_denied, :to => "denied"}
    },
    :denied => {},
    :approved => {}
  },
  "p3$flow" => {
    :start => { :to => {:input => {:subject => "approve"}}},
    :input => { :run => get_input, :to => :process},
    :process => {
      :run => {
        "first$flow" => {
          :start => {:to => {:end => {:$s => {:$lte => {:quantity => 5 }}}}},
          :end => {:run => signal_approved}
        },
        "second$flow" => {
          :start => {:to => {:end => {:$s => {:$gt => {:quantity => 5 }}}}},
          :end => {:run => signal_denied}
        }
      },
      :to => {
        :approve => {:subject => "approved"},
        :deny => {:subject => "denied"},
      }
    },
    :approve => {:run => report_approved},
    :deny => {:run => report_denied}
  }
}, ["/tmp/redis.sock"], -> host {
  host.post :p1, {:id => 1, :sid => 1, :start => "yes"}

  host.post :p2, {:id => 1, :sid => 1, :subject => "approve", :quantity => 3}
  host.post :p2, {:id => 2, :sid => 2, :subject => "approve", :quantity => 10}

  host.post :p3, {:id => 1, :sid => 1, :subject => "approve", :quantity => 3}
  host.post :p3, {:id => 2, :sid => 2, :subject => "approve", :quantity => 10}
})
