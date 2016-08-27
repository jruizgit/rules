require "durable"

Durable.ruleset :a0_0 do
  when_all (m.subject == "go") | (m.subject == "approve") | (m.subject == "ok") do
    puts "a0_0 approved #{m.subject}"
  end
  when_start do
    post :a0_0, {:id => 1, :sid => 1, :subject => "go"}
    post :a0_0, {:id => 2, :sid => 1, :subject => "approve"}
    post :a0_0, {:id => 3, :sid => 1, :subject => "ok"}
  end
end

Durable.ruleset :a0_1 do
  when_all ((m.subject == "go") & ((m.amount < 100) | (m.amount > 1000))) do
    puts "a0_1 approved #{m.subject} #{m.amount}"
  end
  when_start do
    post :a0_1, {:id => 1, :sid => 1, :subject => "go", :amount => 50}
    post :a0_1, {:id => 2, :sid => 1, :subject => "go", :amount => 500}
    post :a0_1, {:id => 3, :sid => 1, :subject => "go", :amount => 50000}
  end
end

Durable.ruleset :a0_2 do
  when_all ((m.subject == "go") & ((m.amount < 100) | ((m.amount == 500) & (m.status == "waived")))) do
    puts "a0_2 approved #{m.subject} #{m.amount}"
  end
  when_start do
    post :a0_2, {:id => 1, :sid => 1, :subject => "go", :amount => 50}
    post :a0_2, {:id => 2, :sid => 1, :subject => "go", :amount => 500, :status => "waived"}
    post :a0_2, {:id => 3, :sid => 1, :subject => "go", :amount => 50000}
  end
end

Durable.ruleset :a1 do
  when_all (m.amount < 1000) | (m.amount > 10000) do
    puts "a1 approving " + m.amount.to_s
    s.status = "pending"
  end
  when_all m.subject == "approved", s.status == "pending" do
    puts "a1 approved #{s.id}"
    s.status = "approved"
  end
  when_start do
    post :a1, {:id => 1, :sid => 1, :amount => 100}
    post :a1, {:id => 2, :sid => 1, :subject => "approved"}
  end
end

Durable.statechart :a2 do
  state :input do
    to :denied, when_all((m.subject == "approve") & (m.amount > 1000)) do
      puts "a2 state denied: #{s.sid}"
    end
    to :pending, when_all((m.subject == "approve") & (m.amount <= 1000)) do
      puts "a2 state request approval from: #{s.sid}"
      s.status? ? s.status = "approved": s.status = "pending"
    end
  end  
  state :pending do
    to :pending, when_any(m.subject == "approved", m.subject == "ok") do
      puts "a2 state received approval for: #{s.sid}"
      s.status = "approved"
    end
    to :approved, when_all(s.status == "approved")
    to :denied, when_all(m.subject == "denied") do
      puts "a2 state denied: #{s.sid}"
    end
  end
  state :approved
  state :denied
  when_start do
    post :a2, {:id => 1, :sid => 1, :subject => "approve", :amount => 100}
    post :a2, {:id => 2, :sid => 1, :subject => "approved"}
    post :a2, {:id => 3, :sid => 2, :subject => "approve", :amount => 100}
    post :a2, {:id => 4, :sid => 2, :subject => "denied"}
    post :a2, {:id => 5, :sid => 3, :subject => "approve", :amount => 10000}
  end
end

Durable.flowchart :a3 do
  stage :input
  to :request, when_all((m.subject == "approve") & (m.amount <= 1000))
  to :deny, when_all((m.subject == "approve") & (m.amount > 1000))
  
  stage :request do
    puts "a3 flow requesting approval for: #{s.sid}"
    s.status? ? s.status = "approved": s.status = "pending"
  end
  to :approve, when_all(s.status == "approved")
  to :deny, when_all(m.subject == "denied")
  to :request, when_any(m.subject == "approved", m.subject == "ok")
  
  stage :approve do
    puts "a3 flow aprroved: #{s.sid}"
  end

  stage :deny do
    puts "a3 flow denied: #{s.sid}"
  end

  when_start do
    post :a3, {:id => 1, :sid => 1, :subject => "approve", :amount => 100}
    post :a3, {:id => 2, :sid => 1, :subject => "approved"}
    post :a3, {:id => 3, :sid => 2, :subject => "approve", :amount => 100}
    post :a3, {:id => 4, :sid => 2, :subject => "denied"}
    post :a3, {:id => 5, :sid => 3, :subject => "approve", :amount => 10000}
  end
end

Durable.ruleset :a4 do
  when_any all(m.subject == "approve", m.amount == 1000),
           all(m.subject == "jumbo", m.amount == 10000) do
    puts "a4 action #{s.sid}"
  end
  when_start do
    post :a4, {:id => 1, :sid => 1, :subject => "approve"}
    post :a4, {:id => 2, :sid => 1, :amount => 1000}
    post :a4, {:id => 3, :sid => 2, :subject => "jumbo"}
    post :a4, {:id => 4, :sid => 2, :amount => 10000}
  end
end
   
Durable.ruleset :a5 do
  when_all any(m.subject == "approve", m.subject == "jumbo"),
           any(m.amount == 100, m.amount == 10000) do
    puts "a5 action #{s.sid}"
  end
  when_start do
    post :a5, {:id => 1, :sid => 1, :subject => "approve"}
    post :a5, {:id => 2, :sid => 1, :amount => 100}
    post :a5, {:id => 3, :sid => 2, :subject => "jumbo"}
    post :a5, {:id => 4, :sid => 2, :amount => 10000}
  end
end

Durable.statechart :a6 do
  state :work do   
    state :enter do
      to :process, when_all(m.subject == "enter") do
        puts "a6 continue process"
      end
    end
    state :process do
      to :process, when_all(m.subject == "continue") do
        puts "a6 processing"
      end
    end
    to :canceled, when_all(pri(1), m.subject == "cancel") do
      puts "a6 canceling"
    end
  end
  state :canceled
  when_start do
    post :a6, {:id => 1, :sid => 1, :subject => "enter"}
    post :a6, {:id => 2, :sid => 1, :subject => "continue"}
    post :a6, {:id => 3, :sid => 1, :subject => "continue"}
    post :a6, {:id => 4, :sid => 1, :subject => "cancel"}
  end
end

Durable.ruleset :a7 do
  when_all m.amount < s.max_amount do
    puts "a7 approved " + m.amount.to_s
  end
  when_start do
    patch_state :a7, {:sid => 1, :max_amount => 100}
    post :a7, {:id => 1, :sid => 1, :amount => 10}
    post :a7, {:id => 2, :sid => 1, :amount => 1000}
  end
end

Durable.ruleset :a8 do
  when_all (m.amount < s.max_amount) & (m.amount > s.id(:global).min_amount) do
    puts "a8 approved " + m.amount.to_s
  end
  when_start do
    patch_state :a8, {:sid => 1, :max_amount => 500}
    patch_state :a8, {:sid => :global, :min_amount => 100}
    post :a8, {:id => 1, :sid => 1, :amount => 10}
    post :a8, {:id => 2, :sid => 1, :amount => 200}
  end
end

Durable.ruleset :a9 do
  when_all (m.amount < 100), count(3) do
    puts "a9 approved ->" + (m[0].amount + m[1].amount + m[2].amount).to_s
  end
  when_start do
    post_batch :a9, {:id => 1, :sid => 1, :amount => 10},
                    {:id => 2, :sid => 1, :amount => 10},
                    {:id => 3, :sid => 1, :amount => 10},
                    {:id => 4, :sid => 1, :amount => 10}
    post_batch :a9, {:id => 5, :sid => 1, :amount => 10},
                    {:id => 6, :sid => 1, :amount => 10}
  end
end

Durable.ruleset :a10 do
  when_all (m.amount < 100), (m.subject == "approve"), count(3) do
    puts "a10 approved ->" + m[0].m_0.amount.to_s + " " + m[0].m_1.subject
    puts "             ->" + m[1].m_0.amount.to_s + " " + m[1].m_1.subject
    puts "             ->" + m[2].m_0.amount.to_s + " " + m[2].m_1.subject
  end
  when_start do
    post_batch :a10, {:id => 1, :sid => 1, :amount => 10},
                     {:id => 2, :sid => 1, :amount => 10},
                     {:id => 3, :sid => 1, :amount => 10},
                     {:id => 4, :sid => 1, :subject => "approve"}
    post_batch :a10, {:id => 5, :sid => 1, :subject => "approve"},
                     {:id => 6, :sid => 1, :subject => "approve"}
  end
end

Durable.ruleset :a11 do
  when_all m.amount < s.max_amount + s.id(:global).min_amount do
    puts "a11 approved " + m.amount.to_s
  end
  when_start do
    patch_state :a11, {:sid => 1, :max_amount => 500}
    patch_state :a11, {:sid => :global, :min_amount => 100}
    post :a11, {:id => 1, :sid => 1, :amount => 10}
  end
end

Durable.statechart :fraud0 do
  state :start do
    to :standby
  end
  state :standby do
    to :metering, when_all(m.amount > 100) do
      start_timer :velocity, 30
    end
  end
  state :metering do
    to :fraud, when_all(m.amount > 100, count(3)) do
      puts "fraud 0 detected"
    end
    to :standby, when_all(timeout :velocity) do
      puts "fraud 0 cleared"
    end
  end
  state :fraud
  when_start do
    post :fraud0, {:id => 1, :sid => 1, :amount => 200}
    post :fraud0, {:id => 2, :sid => 1, :amount => 200}
    post :fraud0, {:id => 3, :sid => 1, :amount => 200}
    post :fraud0, {:id => 4, :sid => 1, :amount => 200}
  end
end

Durable.ruleset :fraud1 do
  when_all c.first = m.t == "purchase",
           c.second = m.location != first.location do
    puts "fraud1 detected " + first.location + " " + second.location
  end
  when_start do
    post :fraud1, {:id => 1, :sid => 1, :t => "purchase", :location => "US"}
    post :fraud1, {:id => 2, :sid => 1, :t => "purchase", :location => "CA"}
    post :fraud1, {:id => 3, :sid => 1, :t => "purchase", :location => "UK"}
    post :fraud1, {:id => 4, :sid => 1, :t => "purchase", :location => "GE"}
    post :fraud1, {:id => 5, :sid => 1, :t => "purchase", :location => "AU"}
    post :fraud1, {:id => 6, :sid => 1, :t => "purchase", :location => "MX"}
    post :fraud1, {:id => 7, :sid => 1, :t => "purchase", :location => "FR"}
    post :fraud1, {:id => 8, :sid => 1, :t => "purchase", :location => "ES"}
    post :fraud1, {:id => 9, :sid => 1, :t => "purchase", :location => "BR"}
    post :fraud1, {:id => 10, :sid => 1, :t => "purchase", :location => "IT"}
  end
end

Durable.ruleset :fraud2 do
  when_all c.first = m.t == "purchase",
           c.second = (m.ip == first.ip) & (m.cc != first.cc),
           c.third = (m.ip == second.ip) & (m.cc != first.cc) & (m.cc != second.cc) do
    puts "fraud2 detected " + first.cc + " " + second.cc + " " + third.cc
  end
  when_start do
    post :fraud2, {:id => 1, :sid => 1, :t => "purchase", :ip => "a", :cc => "1"}
    post :fraud2, {:id => 2, :sid => 1, :t => "purchase", :ip => "a", :cc => "2"}
    post :fraud2, {:id => 3, :sid => 1, :t => "purchase", :ip => "a", :cc => "3"}
  end
end

Durable.ruleset :fraud3 do
  when_all c.first = m.t == "purchase",
           c.second = m.amount > first.amount * 2 do
    puts "fraud3 detected " + first.amount.to_s + " " + second.amount.to_s
  end
  when_start do
    post :fraud3, {:id => 1, :sid => 1, :t => "purchase", :amount => 100}
    post :fraud3, {:id => 2, :sid => 1, :t => "purchase", :amount => 300}
  end
end

Durable.ruleset :fraud4 do
  when_all c.first = m.t == "purchase",
           c.second = m.amount > first.amount,
           c.third = m.amount > second.amount,
           c.fourth = m.amount > (first.amount + second.amount + third.amount) / 3 do
    puts "fraud4 detected -> " + first.amount.to_s 
    puts "                -> " + second.amount.to_s
    puts "                -> " + third.amount.to_s 
    puts "                -> " + fourth.amount.to_s
  end
  when_start do
    post :fraud4, {:id => 1, :sid => 1, :t => "purchase", :amount => 100}
    post :fraud4, {:id => 2, :sid => 1, :t => "purchase", :amount => 200}
    post :fraud4, {:id => 3, :sid => 1, :t => "purchase", :amount => 300}
    post :fraud4, {:id => 4, :sid => 1, :t => "purchase", :amount => 250}
  end
end

Durable.statechart :fraud8 do
  state :first do 
    to :second, when_all(m.amount > 100) do |c, complete|
      puts "fraud8 start async 1"
      Thread.new do
        sleep 1
        puts "fraud8 execute 1"
        complete.call nil
      end
    end
  end
  state :second do
    to :fraud do |c, complete|
      puts "fraud8 start async 2"
      Thread.new do
        sleep 1
        puts "fraud8 execute 2"
        complete.call nil
      end
    end
  end
  state :fraud
  when_start do
    post :fraud8, {:id => 1, :sid => 1, :amount => 200}
  end
end

Durable.ruleset :pri0 do
  when_all pri(3), m.amount < 300 do
    puts "pri0, 1 approved " + m.amount.to_s
  end
  when_all pri(2), m.amount < 200 do
    puts "pri0, 2 approved " + m.amount.to_s
  end
  when_all pri(1), m.amount < 100  do
    puts "pri0, 3 approved " + m.amount.to_s
  end
  when_start do
    post :pri0, {:id => 1, :sid => 1, :amount => 50}
    post :pri0, {:id => 2, :sid => 1, :amount => 150}
    post :pri0, {:id => 3, :sid => 1, :amount => 250}
  end
end

Durable.ruleset :fact0 do
  when_all c.first = m.t == "purchase",
           c.second = m.location != first.location,
           count(2) do
    puts "fact0 detected ->" + m[0].first.location + " " + m[0].second.location
    puts "                ->" + m[1].first.location + " " + m[1].second.location
  end
  when_start do
    assert :fact0, {:id => 1, :sid => 1, :t => "purchase", :location => "US"}
    assert :fact0, {:id => 2, :sid => 1, :t => "purchase", :location => "CA"}
  end
end

Durable.ruleset :fact1 do
  when_all pri(3), count(3), m.amount < 300 do
    puts "fact1, 1 approved ->" + m[0].amount.to_s
    puts "                 ->" + m[1].amount.to_s
    puts "                 ->" + m[2].amount.to_s
  end
  when_all pri(2), count(2), m.amount < 200 do
    puts "fact1, 2 approved ->" + m[0].amount.to_s
    puts "                 ->" + m[1].amount.to_s
  end
  when_all pri(1), m.amount < 100  do
    puts "fact1, 3 approved " + m.amount.to_s
  end
  when_start do
    assert :fact1, {:id => 1, :sid => 1, :amount => 50}
    assert :fact1, {:id => 2, :sid => 1, :amount => 150}
    assert :fact1, {:id => 3, :sid => 1, :amount => 250}
  end
end

Durable.ruleset :fact2 do
  when_all c.first = m.t == "deposit",
           none(m.t == "balance"),
           c.third = m.t == "withrawal",
           c.fourth = m.t == "chargeback" do
    puts "fact2 " + first.t + " " + third.t + " " + fourth.t + " from " + s.sid.to_s
  end
  when_start do
    post :fact2, {:id => 1, :sid => 1, :t => "deposit"}
    post :fact2, {:id => 2, :sid => 1, :t => "withrawal"}
    post :fact2, {:id => 3, :sid => 1, :t => "chargeback"}
    assert :fact2, {:id => 4, :sid => 1, :t => "balance"}
    post :fact2, {:id => 5, :sid => 1, :t => "deposit"}
    post :fact2, {:id => 6, :sid => 1, :t => "withrawal"}
    post :fact2, {:id => 7, :sid => 1, :t => "chargeback"}
    retract :fact2, {:id => 4, :sid => 1, :t => "balance"}
  end
end

q = Durable.create_queue :fact2
q.post({:id => 1, :sid => 2, :t => "deposit"})
q.post({:id => 2, :sid => 2, :t => "withrawal"})
q.post({:id => 3, :sid => 2, :t => "chargeback"})
q.assert({:id => 4, :sid => 2, :t => "balance"})
q.post({:id => 5, :sid => 2, :t => "deposit"})
q.post({:id => 6, :sid => 2, :t => "withrawal"})
q.post({:id => 7, :sid => 2, :t => "chargeback"})
q.retract({:id => 4, :sid => 2, :t => "balance"})
q.close

Durable.ruleset :t0 do
  when_all (timeout :my_timer) | (m.count == 0) do
    s.count += 1
    post :t0, {:id => s.count, :sid => 1, :t => "purchase"}
    start_timer(:my_timer, rand(3), "t_#{s.count}")
  end
  when_all span(5), m.t == "purchase" do 
    puts("t0 pulse -> #{m.count}")
  end 
  when_start do
    patch_state :t0, {:sid => 1, :count => 0}
  end
end

Durable.ruleset :t1 do
  when_all m.start == "yes" do
    s.start = Time.now
    start_timer :my_first_timer, 3
    start_timer :my_second_timer, 6
  end
  when_all timeout :my_first_timer do
    puts "t1 started #{s.start}"
    puts "t1 first_timer ended #{Time.now}"
    cancel_timer :my_second_timer
  end
  when_all timeout :my_second_timer do
    puts "t1 started #{s.start}"
    puts "t1 second_timer ended #{Time.now}"
  end
  when_start do
    post :t1, {:id => 1, :sid => 1, :start => "yes"}
  end
end

Durable.ruleset :t3 do
  when_all m.start == "yes" do |c, complete|
    puts "t3 first started #{c.m.id}"
    Thread.new do
      sleep 5
      puts "t3 first completed"
      post({:id => 2, :end => "yes"})
      complete.call nil
    end
    10
  end
  when_all m.end == "yes" do |c, complete|
    puts "t3 second started #{c.m.id}"
    Thread.new do
      sleep 7
      puts "t3 second completed"
      post({:id => 3, :end => "yes"})
      complete.call nil
    end
    5
  end
  when_all +m.exception do
    puts "t3 exception #{s.exception}"
    s.exception = nil
  end
  when_start do
    post :t3, {:id => 1, :sid => 1, :start => "yes"}
  end
end

Durable.ruleset :q0 do
  when_all m.start == "yes" do
    puts "q0 started"
    get_queue(:q0).post({:id => 2, :sid => 1, :end => "yes"})
  end
  when_all m.end == "yes" do
    puts "q0 ended"
  end
  when_start do
    post :q0, {:id => 1, :sid => 1, :start => "yes"}
  end
end

Durable.run_all

