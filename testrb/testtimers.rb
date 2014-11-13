require_relative "../librb/durable"

def start_timer
	-> s {
  	s.start = Time.now
  	s.start_timer "my_timer", 5
	}
end

def end_timer
	-> s {
		puts "End"
    puts "Started #{s.start}"
    puts "Ended #{Time.now}"
	} 
end

def start_first_timer
	-> s {
    s.start = Time.now
    s.start_timer "first", 4
  }
end   

def start_second_timer
	-> s {
    s.start = Time.now
    s.start_timer "second", 3
  }
end

def signal_approved
	-> s {
    s.signal :id => 2, :subject => "approved", :start => s.start
  }
end

def signal_denied
	-> s {
    s.signal :id => 3, :subject => "denied", :start => s.start
  }
end

def report_approved
	-> s {
    started = s.event["start"]
    puts "Approved"
    puts "Started #{started}"
    puts "Ended #{Time.now}"
  }
end

def report_denied
	-> s {
    started = s.event["start"]
    puts "Denied"
    puts "Started #{started}"
    puts "Ended #{Time.now}"
  }
end

Durable.run({
  :t1 => {
    :r1 => {:when => {:start => "yes"}, :run => start_timer},
    :r2 => {:when => {:$t => "my_timer"}, :run => end_timer}
  },
  "t2$state" => {
    :input => {
      :review => {
        :when => {:subject => "approve"},
        :run => {
          "first$state" => {
            :send => {:start => {:run => start_first_timer, :to => :evaluate}},
            :evaluate => {
              :wait => {:when => {:$t => :first}, :run => signal_approved, :to => :end}
            },
            :end => {}
          },
          "second$state" => {
            :send => {:start => {:run => start_second_timer, :to => :evaluate}},
            :evaluate => {
              :wait => {:when => {:$t => :second}, :run => signal_denied, :to => :end}
            },
            :end => {}
          }
        },
        :to => :pending
      }
    },
    :pending => {
      :approve => {:when => {:subject => "approved"}, :run => report_approved, :to => :approved},
      :deny => {:when => {:subject => "denied"}, :run => report_denied, :to => :denied}
    },
    :denied => {},
    :approved => {}
  }
}, ["/tmp/redis.sock"], -> host {
  host.post :t1, {:id => 1, :sid => 1, :start => "yes"}
  host.post :t2, {:id => 1, :sid => 1, :subject => "approve"}
})
