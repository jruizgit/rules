require_relative '../librb/durable'

Durable.ruleset :approve1 do
  when_one (m.amount < 1000) | (m.amount > 10000) do
    puts 'approving ' + m.amount.to_s
    s.status = 'pending'
  end
  when_all [m.subject == 'approved', s.status == 'pending'] do
    puts 'approved'
    s.status = 'approved'
  end
  when_start do
    post :approve1, {:id => 1, :sid => 1, :amount => 100}
    post :approve1, {:id => 2, :sid => 1, :subject => 'approved'}
  end
end

Durable.statechart :approve2 do
  state :input do
    to :denied, when_one((m.subject == 'approve') & (m.amount > 1000)) do
      puts "state denied: #{s.id}"
    end
    to :pending, when_one((m.subject == 'approve') & (m.amount <= 1000)) do
      puts "state request approval from: #{s.id}"
      s.status? ? s.status = "approved": s.status = "pending"
    end
  end  
  state :pending do
    to :pending, when_any([m.subject == 'approved', m.subject == 'ok']) do
      puts "state received approval for: #{s.id}"
      s.status = "approved"
    end
    to :approved, when_one(s.status == 'approved')
    to :denied, when_one(m.subject == 'denied') do
      puts "state denied: #{s.id}"
    end
  end
  state :approved
  state :denied
  when_start do
    post :approve2, {:id => 1, :sid => 1, :subject => "approve", :amount => 100}
    post :approve2, {:id => 2, :sid => 1, :subject => "approved"}
    post :approve2, {:id => 3, :sid => 2, :subject => "approve", :amount => 100}
    post :approve2, {:id => 4, :sid => 2, :subject => "denied"}
    post :approve2, {:id => 5, :sid => 3, :subject => "approve", :amount => 10000}
  end
end

Durable.flowchart :approve3 do
  stage :input
  to :request, when_one((m.subject == 'approve') & (m.amount <= 1000))
  to :deny, when_one((m.subject == 'approve') & (m.amount > 1000))
  
  stage :request do
    puts "flow requesting approval for: #{s.id}"
    s.status? ? s.status = "approved": s.status = "pending"
  end
  to :approve, when_one(s.status == 'approved')
  to :deny, when_one(m.subject == 'denied')
  to :request, when_any([m.subject == 'approved', m.subject == 'ok'])
  
  stage :approve do
    puts "flow aprroved: #{s.id}"
  end
  stage :deny do
    puts "flow denied: #{s.id}"
  end

  when_start do
    post :approve3, {:id => 1, :sid => 1, :subject => "approve", :amount => 100}
    post :approve3, {:id => 2, :sid => 1, :subject => "approved"}
    post :approve3, {:id => 3, :sid => 2, :subject => "approve", :amount => 100}
    post :approve3, {:id => 4, :sid => 2, :subject => "denied"}
    post :approve3, {:id => 5, :sid => 3, :subject => "approve", :amount => 10000}
  end
end

Durable.ruleset :p1 do
  when_one m.start == "yes", paralel do 
    ruleset :one do 
      when_one !s.start do
        s.start = 1
      end
      when_one s.start == 1 do
        puts "finish one"
        s.signal :id => 1, :end => "one"
        s.start = 2
      end 
    end
    ruleset :two do 
      when_one !s.start do
        s.start = 1
      end
      when_one s.start == 1 do
        puts "finish two"
        s.signal :id => 1, :end => "two"
        s.start = 2
      end 
    end
  end
  when_all [m.end == "one", m.end == "two"] do
    puts 'p1 approved'
    s.status = 'approved'
  end
  when_start do
    post :p1, {:id => 1, :sid => 1, :start => "yes"}
  end
end

Durable.statechart :p2 do
  state :input do
    to :process, when_one(m.subject == "approve") do
      puts "input #{m.quantity} from: #{s.ruleset_name}, #{s.id}"
      s.quantity = m.quantity
    end 
  end
  state :process do
    to :result, when_one(~s.quantity), paralel do
      statechart :first do
        state :evaluate do
          to :end, when_one(s.quantity <= 5) do
            puts "signaling approved from: #{s.ruleset_name}, #{s.id}"
            s.signal :id => 1, :subject => "approved"  
          end
        end
        state :end
      end
      statechart :second do
        state :evaluate do
          to :end, when_one(s.quantity > 5) do
            puts "signaling denied from: #{s.ruleset_name}, #{s.id}"
            s.signal :id => 1, :subject => "denied"
          end
        end
        state :end
      end
    end
  end
  state :result do
    to :approved, when_one(m.subject == "approved") do
      puts "approved from: #{s.ruleset_name}, #{s.id}"
    end
    to :denied, when_one(m.subject == "denied") do
      puts "denied from: #{s.ruleset_name}, #{s.id}"
    end
  end
  state :denied
  state :approved
  when_start do
    post :p2, {:id => 1, :sid => 1, :subject => "approve", :quantity => 3}
    post :p2, {:id => 2, :sid => 2, :subject => "approve", :quantity => 10}
  end
end

Durable.flowchart :p3 do
  stage :start; to :input, when_one(m.subject == "approve")
  stage :input do
    puts "input #{m.quantity} from: #{s.ruleset_name}, #{s.id}"
    s.quantity = m.quantity
  end
  to :process

  stage :process, paralel do
    flowchart :first do
      stage :start; to :end, when_one(s.quantity <= 5)
      stage :end do
        puts "signaling approved from: #{s.ruleset_name}, #{s.id}"
        s.signal :id => 1, :subject => "approved" 
      end
    end
    flowchart :second do
      stage :start; to :end, when_one(s.quantity > 5)
      stage :end do
        puts "signaling denied from: #{s.ruleset_name}, #{s.id}"
        s.signal :id => 1, :subject => "denied"
      end
    end
  end
  to :approve, when_one(m.subject == "approved")
  to :deny, when_one(m.subject == "denied")

  stage :approve do
    puts "approved from: #{s.ruleset_name}, #{s.id}"
  end
  stage :deny do
    puts "denied from: #{s.ruleset_name}, #{s.id}"
  end
  when_start do
    post :p3, {:id => 1, :sid => 1, :subject => "approve", :quantity => 3}
    post :p3, {:id => 2, :sid => 2, :subject => "approve", :quantity => 10}
  end
end

Durable.run_all

