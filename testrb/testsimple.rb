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
      puts "denied: #{s.id}"
    end
    to :pending, when_one((m.subject == 'approve') & (m.amount <= 1000)) do
      puts "request approval from: #{s.id}"
      s.status = "pending"
    end
  end  
  state :pending do
    to :pending, when_any([m.subject == 'approved', m.subject == 'ok']) do
      puts "received approval for: #{s.id}"
      s.status = "approved"
    end
    to :approved, when_one(s.status == 'approved')
    to :denied, when_one(m.subject == 'denied') do
      puts "denied: #{s.id}"
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

Durable.run_all