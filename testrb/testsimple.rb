require_relative '../librb/durable'


Durable.ruleset :a0 do
  when_all (m.amount < 100) | (m.subject == "approve") | (m.subject == "ok") do
    puts "a0 approved"
  end
  when_start do
    post :a0, {:id => 1, :sid => 1, :amount => 10}
    post :a0, {:id => 2, :sid => 2, :subject => 'approve'}
    post :a0, {:id => 3, :sid => 3, :subject => 'ok'}
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
    post :a1, {:id => 2, :sid => 1, :subject => 'approved'}
  end
end

Durable.statechart :a2 do
  state :input do
    to :denied, when_all((m.subject == 'approve') & (m.amount > 1000)) do
      puts "a2 state denied: #{s.sid}"
    end
    to :pending, when_all((m.subject == 'approve') & (m.amount <= 1000)) do
      puts "a2 state request approval from: #{s.sid}"
      s.status? ? s.status = "approved": s.status = "pending"
    end
  end  
  state :pending do
    to :pending, when_any(m.subject == 'approved', m.subject == 'ok') do
      puts "a2 state received approval for: #{s.sid}"
      s.status = "approved"
    end
    to :approved, when_all(s.status == 'approved')
    to :denied, when_all(m.subject == 'denied') do
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
  to :request, when_all((m.subject == 'approve') & (m.amount <= 1000))
  to :deny, when_all((m.subject == 'approve') & (m.amount > 1000))
  
  stage :request do
    puts "a3 flow requesting approval for: #{s.sid}"
    s.status? ? s.status = "approved": s.status = "pending"
  end
  to :approve, when_all(s.status == 'approved')
  to :deny, when_all(m.subject == 'denied')
  to :request, when_any(m.subject == 'approved', m.subject == 'ok')
  
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
  state :start do
    to :work
  end
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
    to :work, when_all(m.subject == "reset") do
      puts "a6 resetting"
    end
    to :canceled, when_all(m.subject == "cancel") do
      puts "a6 canceling"
    end
  end
  state :canceled
  when_start do
    post :a6, {:id => 1, :sid => 1, :subject => "enter"}
    post :a6, {:id => 2, :sid => 1, :subject => "continue"}
    post :a6, {:id => 3, :sid => 1, :subject => "continue"}
  end
end

# Durable.ruleset :a7 do
#   when_ m.amount < s.max_amount do
#     puts "a7 approved " + m.amount.to_s
#   end
#   when_start do
#     patch_state :a7, {:id => 1, :max_amount => 100}
#     post :a7, {:id => 1, :sid => 1, :amount => 10}
#     post :a7, {:id => 2, :sid => 1, :amount => 1000}
#   end
# end

# Durable.ruleset :a8 do
#   when_ (m.amount < s.max_amount) & (m.amount > s.id(:global).min_amount) do
#     puts "a8 approved " + m.amount.to_s
#   end
#   when_start do
#     patch_state :a8, {:id => 1, :max_amount => 500}
#     patch_state :a8, {:id => :global, :min_amount => 100}
#     post :a8, {:id => 1, :sid => 1, :amount => 10}
#     post :a8, {:id => 2, :sid => 1, :amount => 200}
#   end
# end

# Durable.ruleset :a9 do
#   when_ (m.amount < 100), at_least(3), at_most(6) do
#     puts "a9 approved ->" + m.to_s
#   end
#   when_start do
#     post_batch :a9, {:id => 1, :sid => 1, :amount => 10},
#                     {:id => 2, :sid => 1, :amount => 10},
#                     {:id => 3, :sid => 1, :amount => 10},
#                     {:id => 4, :sid => 1, :amount => 10}
#     post_batch :a9, {:id => 5, :sid => 1, :amount => 10},
#                     {:id => 6, :sid => 1, :amount => 10}
#   end
# end

# Durable.ruleset :a10 do
#   when_all (m.amount < 100), (m.subject == "approve"), at_least(3), at_most(6) do
#     puts "a10 approved ->" + m.to_s
#   end
#   when_start do
#     post_batch :a10, {:id => 1, :sid => 1, :amount => 10},
#                      {:id => 2, :sid => 1, :amount => 10},
#                      {:id => 3, :sid => 1, :amount => 10},
#                      {:id => 4, :sid => 1, :subject => "approve"}
#     post_batch :a10, {:id => 5, :sid => 1, :subject => "approve"},
#                      {:id => 6, :sid => 1, :subject => "approve"}
#   end
# end

# Durable.ruleset :a11 do
#   when_all (m.amount < 100), (m.subject == "please").at_least(3).at_most(6) do
#     puts "a11 approved ->" + m.to_s
#   end
#   when_start do
#     post_batch :a11, {:id => 1, :sid => 1, :amount => 10},
#                      {:id => 2, :sid => 1, :subject => "please"},
#                      {:id => 3, :sid => 1, :subject => "please"},
#                      {:id => 4, :sid => 1, :subject => "please"}
#   end
# end

# Durable.statechart :fraud2 do
#   state :start do
#     to :standby
#   end
#   state :standby do
#     to :metering, when_(m.amount > 100) do
#       start_timer :velocity, 30
#     end
#   end
#   state :metering do
#     to :fraud, when_(m.amount > 100, at_least(3)) do
#       puts "fraud 2 detected"
#     end
#     to :standby, when_(timeout :velocity) do
#       puts "fraud 2 cleared"
#     end
#   end
#   state :fraud
#   when_start do
#     post :fraud2, {:id => 1, :sid => 1, :amount => 200}
#     post :fraud2, {:id => 2, :sid => 1, :amount => 200}
#     post :fraud2, {:id => 3, :sid => 1, :amount => 200}
#   end
# end


# Durable.ruleset :p1 do
#   when_ m.start == "yes", paralel do 
#     ruleset :one do 
#       when_ !s.start do
#         s.start = 1
#       end
#       when_ s.start == 1 do
#         puts "p1 finish one"
#         s.signal :id => 1, :end => "one"
#         s.start = 2
#       end 
#     end
#     ruleset :two do 
#       when_ !s.start do
#         s.start = 1
#       end
#       when_ s.start == 1 do
#         puts "p1 finish two"
#         s.signal :id => 1, :end => "two"
#         s.start = 2
#       end 
#     end
#   end
#   when_all m.end == "one", m.end == "two" do
#     puts 'p1 approved'
#     s.status = 'approved'
#   end
#   when_start do
#     post :p1, {:id => 1, :sid => 1, :start => "yes"}
#   end
# end

# Durable.statechart :p2 do
#   state :input do
#     to :process, when_(m.subject == "approve") do
#       puts "p2 input #{m.quantity} from: #{s.id}"
#       s.quantity = m.quantity
#     end 
#   end
#   state :process do
#     to :result, when_(~s.quantity), paralel do
#       statechart :first do
#         state :evaluate do
#           to :end, when_(s.quantity <= 5) do
#             puts "p2 signaling approved from: #{s.id}"
#             s.signal :id => 1, :subject => "approved"  
#           end
#         end
#         state :end
#       end
#       statechart :second do
#         state :evaluate do
#           to :end, when_(s.quantity > 5) do
#             puts "p2 signaling denied from: #{s.id}"
#             s.signal :id => 1, :subject => "denied"
#           end
#         end
#         state :end
#       end
#     end
#   end
#   state :result do
#     to :approved, when_(m.subject == "approved") do
#       puts "p2 approved from: #{s.id}"
#     end
#     to :denied, when_(m.subject == "denied") do
#       puts "p2 denied from: #{s.id}"
#     end
#   end
#   state :denied
#   state :approved
#   when_start do
#     post :p2, {:id => 1, :sid => 1, :subject => "approve", :quantity => 3}
#     post :p2, {:id => 2, :sid => 2, :subject => "approve", :quantity => 10}
#   end
# end

# Durable.flowchart :p3 do
#   stage :start; to :input, when_(m.subject == "approve")
#   stage :input do
#     puts "p3 input #{m.quantity} from: #{s.id}"
#     s.quantity = m.quantity
#   end
#   to :process

#   stage :process, paralel do
#     flowchart :first do
#       stage :start; to :end, when_(s.quantity <= 5)
#       stage :end do
#         puts "p3 signaling approved from: #{s.id}"
#         s.signal :id => 1, :subject => "approved" 
#       end
#     end
#     flowchart :second do
#       stage :start; to :end, when_(s.quantity > 5)
#       stage :end do
#         puts "p3 signaling denied from: #{s.id}"
#         s.signal :id => 1, :subject => "denied"
#       end
#     end
#   end
#   to :approve, when_(m.subject == "approved")
#   to :deny, when_(m.subject == "denied")

#   stage :approve do
#     puts "p3 approved from: #{s.id}"
#   end
#   stage :deny do
#     puts "p3 denied from: #{s.id}"
#   end
#   when_start do
#     post :p3, {:id => 1, :sid => 1, :subject => "approve", :quantity => 3}
#     post :p3, {:id => 2, :sid => 2, :subject => "approve", :quantity => 10}
#   end
# end

# Durable.ruleset :t1 do
#   when_ m.start == "yes" do
#     s.start = Time.now
#     start_timer(:my_timer, 5)
#   end
#   when_ timeout :my_timer do
#     puts "t1 End"
#     puts "t1 Started #{s.start}"
#     puts "t1 Ended #{Time.now}"
#   end
#   when_start do
#     post :t1, {:id => 1, :sid => 1, :start => "yes"}
#   end
# end

# Durable.statechart :t2 do
#   state :input do
#     to :pending, when_(m.subject == "approve"), paralel do
#       statechart :first do
#         state :send do
#           to :evaluate do
#             s.start = Time.now
#             start_timer :first, 4
#           end
#         end
#         state :evaluate do
#           to :end, when_(timeout :first) do
#             s.signal :id => 2, :subject => "approved", :start => s.start
#           end
#         end
#         state :end
#       end
#       statechart :second do
#         state :send do
#           to :evaluate do
#             s.start = Time.now
#             start_timer :second, 3
#           end
#         end
#         state :evaluate do
#           to :end, when_(timeout :second) do
#             s.signal :id => 3, :subject => "denied", :start => s.start
#           end
#         end
#         state :end
#       end
#     end
#   end
#   state :pending do
#     to :approved, when_(m.subject == "approved") do
#       puts "t2 Approved"
#       puts "t2 Started #{m.start}"
#       puts "t2 Ended #{Time.now}"
#     end
#     to :denied, when_(m.subject == "denied") do
#       puts "t2 Denied t2"
#       puts "t2 Started #{m.start}"
#       puts "t2 Ended #{Time.now}"
#     end
#   end
#   state :approved
#   state :denied
#   when_start do
#     post :t2, {:id => 1, :sid => 1, :subject => "approve"}
#   end
# end

Durable.run_all

