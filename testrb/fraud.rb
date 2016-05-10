require "durable"

Durable.ruleset :fraud_detection do
  when_all span(10), (m.t == "debit_cleared") | (m.t == "credit_cleared") do
    debit_total = 0
    credit_total = 0
    if not s.balance
      s.balance = 0
      s.avg_balance = 0
      s.avg_withdraw = 0
    end    

    for tx in m do
      if tx.t == "debit_cleared"
        debit_total += tx.amount
      else
        credit_total += tx.amount
      end
    end

    s.balance = s.balance - debit_total + credit_total
    s.avg_balance = (s.avg_balance * 29 + s.balance) / 30
    s.avg_withdraw = (s.avg_withdraw * 29 + debit_total) / 30
  end

  when_all c.first = (m.t == "debit_request") & 
                     (m.amount > s.avg_withdraw * 3),
           c.second = (m.t == "debit_request") & 
                      (m.amount > s.avg_withdraw * 3) & 
                      (m.stamp > first.stamp) &
                      (m.stamp < first.stamp + 90) do
    puts "Medium risk #{first.amount}, #{second.amount}"
  end

  when_all c.first = m.t == "debit_request",
           c.second = (m.t == "debit_request") &
                      (m.amount > first.amount) & 
                      (m.stamp < first.stamp + 180),
           c.third = (m.t == "debit_request") & 
                     (m.amount > second.amount) & 
                     (m.stamp < first.stamp + 180),
           s.avg_balance < (first.amount + second.amount + third.amount) / 0.9 do
    puts "High risk #{first.amount}, #{second.amount}, #{third.amount}"
  end

  when_all timeout("customer") | (m.t == "customer") do
    if not s.c_count
        s.c_count = 100
    else
        s.c_count += 2
    end

    post "fraud_detection", {:id => s.c_count, :sid => 1, :t => "debit_cleared", :amount => s.c_count}
    post "fraud_detection", {:id => s.c_count + 1, :sid => 1, :t => "credit_cleared", :amount => (s.c_count - 100) * 2 + 100}
    start_timer "customer", 1
  end

  when_all timeout("fraudster") | (m.t == "fraudster") do
    if not s.f_count
        s.f_count = 1000
    else
        s.f_count += 1
    end

    post "fraud_detection", {:id => s.f_count, :sid => 1, :t => "debit_request", :amount => s.f_count - 800, :stamp => Time.now.to_i}
    start_timer "fraudster", 2
  end

  when_start do
    start_timer "fraud_detection", {:id => "c_1", :sid => 1, :t => "customer"}
    start_timer "fraud_detection", {:id => "f_1", :sid => 1, :t => "fraudster"}
  end
end

Durable.run_all
