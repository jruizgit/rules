require "durable"

Durable.statechart :expense1 do
  state :input do
    to :denied, when_all((m.subject == "approve") & (m.amount > 1000)) do
      puts "expense denied"
    end
    to :pending, when_all((m.subject == "approve") & (m.amount <= 1000)) do
      puts "requesting expense approval"
    end
  end  
  state :pending do
    to :approved, when_all(m.subject == "approved") do
      puts "expense approved"
    end
    to :denied, when_all(m.subject == "denied") do
      puts "expense denied"
    end
  end
  state :approved
  state :denied
end

Durable.ruleset :animal1 do
  when_all c.first = (m.predicate == "eats") & (m.object == "flies"),  
          (m.predicate == "lives") & (m.object == "water") & (m.subject == first.subject) do
    assert :subject => first.subject, :predicate => "is", :object => "frog"
  end

  when_all c.first = (m.predicate == "eats") & (m.object == "flies"),  
          (m.predicate == "lives") & (m.object == "land") & (m.subject == first.subject) do
    assert :subject => first.subject, :predicate => "is", :object => "chameleon"
  end

  when_all (m.predicate == "eats") & (m.object == "worms") do
    assert :subject => m.subject, :predicate => "is", :object => "bird"
  end
  
  when_all (m.predicate == "is") & (m.object == "frog") do
    assert :subject => m.subject, :predicate => "is", :object => "green"
  end
    
  when_all (m.predicate == "is") & (m.object == "chameleon") do
    assert :subject => m.subject, :predicate => "is", :object => "green"
  end

  when_all (m.predicate == "is") & (m.object == "bird") do
    assert :subject => m.subject, :predicate => "is", :object => "black"
  end
    
  when_all +m.subject, count(11) do
    m.each { |f| puts "fact: #{f.subject} #{f.predicate} #{f.object}" }
  end
    
  when_start do
    assert :animal1, { :subject => "Kermit", :predicate => "eats", :object => "flies" }
    assert :animal1, { :subject => "Kermit", :predicate => "lives", :object => "water" }
    assert :animal1, { :subject => "Greedy", :predicate => "eats", :object => "flies" }
    assert :animal1, { :subject => "Greedy", :predicate => "lives", :object => "land" }
    assert :animal1, { :subject => "Tweety", :predicate => "eats", :object => "worms" }
  end
end


Durable.ruleset :test do
  # antecedent
  when_all m.subject == "World" do
    # consequent
    puts "Hello #{m.subject}"
  end
  # on ruleset start
  when_start do
    post :test, { :subject => "World" }
  end
end

Durable.ruleset :animal do
  # will be triggered by 'Kermit eats flies'
  when_all c.first = (m.predicate == "eats") & (m.object == "flies") do
    assert :subject => first.subject, :predicate => "is", :object => "frog"
  end

  when_all (m.predicate == "eats") & (m.object == "worms") do
    assert :subject => m.subject, :predicate => "is", :object => "bird"
  end
  
  # will be chained after asserting 'Kermit is frog'
  when_all (m.predicate == "is") & (m.object == "frog") do
    assert :subject => m.subject, :predicate => "is", :object => "green"
  end
    
  when_all (m.predicate == "is") & (m.object == "bird") do
    assert :subject => m.subject, :predicate => "is", :object => "black"
  end
    
  when_all +m.subject do
    puts "fact: #{m.subject} #{m.predicate} #{m.object}"
  end
    
  when_start do
    assert :animal, { :subject => "Kermit", :predicate => "eats", :object => "flies" }
  end
end

#curl -H "content-type: application/json" -X POST -d '{"subject": "Tweety", "predicate": "eats", "object": "worms"}' http://localhost:4567/animal/facts

Durable.ruleset :risk1 do
  when_all c.first = m.t == "purchase",
           c.second = m.location != first.location do
    # the event pair will only be observed once
    puts "fraud detected -> #{first.location}, #{second.location}"
  end

  when_start do
    # 'post' submits events, try 'assert' instead and to see differt behavior
    post :risk1, { :t => "purchase", :location => "US" }
    post :risk1, { :t => "purchase", :location => "CA" }
  end
end


# curl -H "content-type: application/json" -X POST -d '{"t": "purchase", "location": "BR"}' http://localhost:4567/risk/events
# curl -H "content-type: application/json" -X POST -d '{"t": "purchase", "location": "JP"}' http://localhost:4567/risk/events

Durable.ruleset :flow1 do
  # state condition uses 's'
  when_all s.status == "start" do
    # state update on 's'
    s.status = "next"
    puts "start"
  end

  when_all s.status == "next" do
    s.status = "last"
    puts "next"
  end

  when_all s.status == "last" do
    s.status = "end"
    puts "last"
    # deletes state at the end
    delete_state
  end
  # modifies context state
  when_start do
    patch_state :flow1, { :status => "start"}
  end
end


# curl -H "content-type: application/json" -X POST -d '{"status": "next"}' http://localhost:4567/flow/state

Durable.ruleset :expense2 do
  when_all (m.subject == "approve") | (m.subject == "ok") do
    puts "Approved subject: #{m.subject}"
  end

  when_start do
    post :expense2, { :subject => "approve" }
  end
end

Durable.ruleset :match do
  when_all (m.url.matches("(https?://)?([0-9a-z.-]+)%.[a-z]{2,6}(/[A-z0-9_.-]+/?)*")) do
    puts "match -> #{m.url}"
  end

  when_start do
    post :match, { :url => "https://github.com" }
    post :match, { :url => "http://github.com/jruizgit/rul!es" }
    post :match, { :url => "https://github.com/jruizgit/rules/blob/master/docs/rb/reference.md" }
    post :match, { :url => "//rules" }
    post :match, { :url => "https://github.c/jruizgit/rules" }
  end
end

Durable.ruleset :strings do
  when_all m.subject.matches("hello.*") do
    puts "string starts with hello: #{m.subject}"
  end

  when_all m.subject.matches(".*hello") do
    puts "string ends with hello: #{m.subject}"
  end

  when_all m.subject.imatches(".*Hello.*") do
    puts "string contains hello (case insensitive): #{m.subject}"
  end

  when_start do
    assert :strings, { :subject => "HELLO world" }
    assert :strings, { :subject => "world hello" }
    assert :strings, { :subject => "hello hi" }
    assert :strings, { :subject => "has Hello string" }
    assert :strings, { :subject => "does not match" }
  end
end

Durable.ruleset :indistinct do
  when_all distinct(false),
           c.first = m.amount > 10,
           c.second = m.amount > first.amount * 2,
           c.third = m.amount > (first.amount + second.amount) / 2 do
    puts "indistinct detected -> #{first.amount}" 
    puts "               -> #{second.amount}"
    puts "               -> #{third.amount}"
  end

  when_start do
    post :indistinct, { :t => "purchase", :amount => 50 }
    post :indistinct, { :t => "purchase", :amount => 200 }
    post :indistinct, { :t => "purchase", :amount => 251 } 
  end
end

Durable.ruleset :distinct do
  when_all c.first = m.amount > 10,
           c.second = m.amount > first.amount * 2,
           c.third = m.amount > (first.amount + second.amount) / 2 do
    puts "distinct detected -> #{first.amount}" 
    puts "               -> #{second.amount}"
    puts "               -> #{third.amount}"
  end

  when_start do
    post :distinct, { :t => "purchase", :amount => 50 }
    post :distinct, { :t => "purchase", :amount => 200 }
    post :distinct, { :t => "purchase", :amount => 251 } 
  end
end

Durable.ruleset :expense3 do
  when_all c.bill = (m.t == "bill") & (m.invoice.amount > 50),
           c.account = (m.t == "account") & (m.payment.invoice.amount == bill.invoice.amount) do
    puts "bill amount -> #{bill.invoice.amount}" 
    puts "account payment amount -> #{account.payment.invoice.amount}" 
  end

  when_start do
    post :expense3, { t:"bill", :invoice => { :amount => 1000 }}
    post :expense3, { t:"account", :payment => { :invoice => { :amount => 1000 }}}
  end
end

Durable.ruleset :risk do
  when_all c.first = m.t == "deposit",
           none(m.t == "balance"),
           c.third = m.t == "withrawal",
           c.fourth = m.t == "chargeback" do
    puts "fraud detected #{first.t} #{third.t} #{fourth.t}"
  end

  when_start do
    post :risk, { :t => "deposit" }
    post :risk, { :t => "withrawal" }
    post :risk, { :t => "chargeback" }
  end
end

Durable.ruleset :expense4 do
  when_any all(c.first = m.subject == "approve", 
               c.second = m.amount == 1000),
           all(c.third = m.subject == "jumbo", 
               c.fourth = m.amount == 10000) do
    if first
      puts "Approved #{first.subject} #{second.amount}"
    else
      puts "Approved #{third.subject} #{fourth.amount}"
    end
  end

  when_start do
    post :expense4, { :subject => "approve" }
    post :expense4, { :amount => 1000 }
    post :expense4, { :subject => "jumbo" }
    post :expense4, { :amount => 10000 }
  end
end

Durable.ruleset :attributes do
  when_all pri(3), m.amount < 300 do
    puts "attributes P3 -> #{m.amount}"
  end

  when_all pri(2), m.amount < 200 do
    puts "attributes P2 -> #{m.amount}"
  end

  when_all pri(1), m.amount < 100  do
    puts "attributes P1 -> #{m.amount}"
  end

  when_start do
    assert :attributes, { :amount => 50 }
    assert :attributes, { :amount => 150 }
    assert :attributes, { :amount => 250 }
  end
end


Durable.ruleset :flow2 do
  when_all m.action == "start" do
    raise "Unhandled Exception!"
  end
  
  # when the exception property exists
  when_all +s.exception do
    puts "#{s.exception}"
    s.exception = nil
  end

  when_start do
    post :flow2, { :action => "start" }
  end
end

Durable.statechart :expense5 do
  # initial state :input with two triggers
  state :input do
    # trigger to move to :denied given a condition
    to :denied, when_all((m.subject == "approve") & (m.amount > 1000)) do
      # action executed before state change
      puts "denied amount #{m.amount}"
    end

    to :pending, when_all((m.subject == "approve") & (m.amount <= 1000)) do
      puts "requesting approve amount #{m.amount}"
    end
  end  

  # intermediate state :pending with two triggers
  state :pending do
    to :approved, when_all(m.subject == "approved") do
      puts "expense approved"
    end

    to :denied, when_all(m.subject == "denied") do
      puts "expense denied"
    end
  end

  state :approved
  state :denied

  when_start do
    # events directed to default statechart instance
    post :expense5, { :subject => 'approve', :amount => 100 }
    post :expense5, { :subject => 'approved' }

    # events directed to statechart instance with id '1'
    post :expense5, { :sid => 1, :subject => 'approve', :amount => 100 }
    post :expense5, { :sid => 1, :subject => 'denied' }

    # events directed to statechart instance with id '2'
    post :expense, { :sid => 2, :subject => 'approve', :amount => 10000 }
  end
end


Durable.statechart :worker do
  # super-state :work has two states and one trigger
  state :work do
    # sub-sate :enter has only one trigger   
    state :enter do
      to :process, when_all(m.subject == "enter") do
        puts "start process"
      end
    end

    state :process do
      to :process, when_all(m.subject == "continue") do
        puts "continue processing"
      end
    end

    # the super-state trigger will be evaluated for all sub-state triggers
    to :canceled, when_all(pri(1), m.subject == "cancel") do
      puts "cancel process"
    end
  end

  state :canceled
  
  when_start do
    # will move the statechart to the 'work.process' sub-state
    post :worker, { :subject => "enter" }

     # will keep the statechart to the 'work.process' sub-state
    post :worker, { :subject => "continue" }
    post :worker, { :subject => "continue" }

    # will move the statechart out of the work state
    post :worker, { :subject => "cancel" }
  end
end


Durable.flowchart :expense do
  # initial stage :input has two conditions
  stage :input
  to :request, when_all((m.subject == "approve") & (m.amount <= 1000))
  to :deny, when_all((m.subject == "approve") & (m.amount > 1000))
  
  # intermediate stage :request has an action and three conditions
  stage :request do
    puts "requesting approve"
  end
  to :approve, when_all(m.subject == "approved")
  to :deny, when_all(m.subject == "denied")
  # reflexive condition: if met, returns to the same stage
  to :request, when_any(m.subject == "retry")
  
  stage :approve do
    puts "expense approved"
  end

  stage :deny do
    puts "expense denied"
  end

  when_start do
    # events for the default flowchart instance, approved after retry
    post :expense, { :subject => "approve", :amount => 100 }
    post :expense, { :subject => "retry" }
    post :expense, { :subject => "approved" }

    # events for the flowchart instance '1', denied after first try
    post :expense, {:sid => 1, :subject => "approve", :amount => 100}
    post :expense, {:sid => 1, :subject => "denied"}

     # event for the flowchart instance '2' immediately denied    
    post :expense, {:sid => 2, :subject => "approve", :amount => 10000}
  end
end


Durable.ruleset :expense6 do
  # this rule will trigger as soon as three events match the condition
  when_all count(3), m.amount < 100 do
    for f in m do
      puts "approved ->#{f}"
    end
  end

  # this rule will be triggered when 'expense' is asserted batching at most two results
  when_all cap(2), 
           c.expense = m.amount >= 100,
           c.approval = m.review == true do
    for f in m do
      puts "rejected ->#{f}"
    end
  end

  when_start do
    post_batch :expense6, { :amount => 10 },
                         { :amount => 20 },
                         { :amount => 100 },
                         { :amount => 30 },
                         { :amount => 200 },
                         { :amount => 400 }
    assert :expense6, { :review => true }
  end
end

Durable.ruleset :timer do
  when_any all(s.count == 0),
           # will trigger when MyTimer expires
           all(s.count < 5, 
               timeout("MyTimer")) do
    s.count += 1
    # MyTimer will expire in 5 seconds
    start_timer "MyTimer", 5
    puts "pulse -> #{Time.now}"
  end

  when_all m.cancel == true do
    cancel_timer "MyTimer"
    puts "canceled timer"
  end

  when_start do
    patch_state :timer, { :count => 0 }
  end
end

# curl -H "content-type: application/json" -X POST -d '{"cancel": true}' http://localhost:4567/timer/events

Durable.statechart :risk3 do
  state :start do
    to :meter do
      start_timer "RiskTimer", 5
    end
  end  

  state :meter do
    to :fraud, when_all(count(3), c.message = m.amount > 100) do
      for e in m do
        puts e.message
      end
    end

    to :exit, when_all(timeout("RiskTimer")) do
      puts "exit"
    end
  end

  state :fraud
  state :exit

  when_start do
    # three events in a row will trigger the fraud rule
    post 'risk3', { :amount => 200 } 
    post 'risk3', { :amount => 300 } 
    post 'risk3', { :amount => 400 }

    # two events will exit after 5 seconds
    post 'risk3', { :sid => 1, :amount => 500 } 
    post 'risk3', { :sid => 1, :amount => 600 } 
  end
end


Durable.statechart :risk4 do
  state :start do
    to :meter do
      # will start a manual reset timer
      start_timer "VelocityTimer", 5, true
    end
  end  

  state :meter do
    to :meter, when_all(cap(100), 
                        c.message = m.amount > 100,
                        timeout("VelocityTimer")) do
      puts "velocity: #{m.length} events in 5 seconds"
      # resets and restarts the manual reset timer
      reset_timer "VelocityTimer"
      start_timer "VelocityTimer", 5, true
    end

    to :meter, when_all(timeout("VelocityTimer")) do
      puts "velocity: no events in 5 seconds"
      reset_timer "VelocityTimer"
      start_timer "VelocityTimer", 5, true
    end
  end

  when_start do
    # the velocity will 4 events in 5 seconds
    post 'risk4', { :amount => 200 } 
    post 'risk4', { :amount => 300 } 
    post 'risk4', { :amount => 50 }
    post 'risk4', { :amount => 300 } 
    post 'risk4', { :amount => 400 }
  end
end

# curl -H "content-type: application/json" -X POST -d '{"amount": 200}' http://localhost:4567/risk4/events


Durable.ruleset :flow3 do

  # async actions take a callback argument to signal completion
  when_all s.state == "first" do |c, complete|
    Thread.new do
      sleep 3
      s.state = "second"
      puts "first completed"
      complete.call nil
    end
  end

  when_all s.state == "second" do |c, complete|
    Thread.new do
      sleep 6
      s.state = "third"
      puts "second completed"

      # completes the action after 6 seconds
      # use the first argument to signal an error
      complete.call Exception('error detected')
    end

    # overrides the 5 second default abandon timeout
    10
  end
  
  when_start do
    patch_state :flow3, { :state => "first" }
  end
end


Durable.ruleset :bookstore do
  # this rule will trigger for events with status
  when_all +m.status do
    puts "Reference #{m.reference} status #{m.status}"
  end

  when_all +m.name do
    puts "Added: #{m.name}"
    retract(:name => 'The new book',
            :reference => '75323',
            :price => 500,
            :seller => 'bookstore')
  end

  when_all none(+m.name) do
    puts "No books"
  end  

  when_start do
    # will return 0 because the fact assert was successful 
    puts assert :bookstore, {
                :name => 'The new book',
                :seller => 'bookstore',
                :reference => '75323',
                :price => 500}

    # will return 212 because the fact has already been asserted 
    puts assert :bookstore, {
                :reference => '75323',
                :name => 'The new book',
                :price => 500,
                :seller => 'bookstore'}

    # will return 0 because a new event is being posted
    puts post :bookstore, {
              :reference => '75323',
              :status => 'Active'}

    # will return 0 because a new event is being posted
    puts post :bookstore, {
              :reference => '75323',
              :status => 'Active'}
  end
end

Durable.ruleset :risk5 do
  # compares properties in the same event, this expression is evaluated in the client 
  when_all m.debit > m.credit * 2 do
    puts "debit #{m.debit} more than twice the credit #{m.credit}"
  end
  # compares two correlated events, this expression is evaluated in the backend
  when_all c.first = m.amount > 100,
           c.second = m.amount > first.amount + m.amount / 2  do
    puts "fraud detected -> #{first.amount}"
    puts "fraud detected -> #{second.amount}"
  end

  when_start do
    post :risk5, { :debit => 220, :credit => 100 }
    post :risk5, { :debit => 150, :credit => 100 }
    post :risk5, { :amount => 200 }
    post :risk5, { :amount => 500 }
  end
end


# Durable.ruleset :flow do

#   when_all m.status == "start" do
#     post :status => "next"
#     puts "start"
#   end
#   # the process will always exit here every time the action is run
#   # when restarting the process this action will be retried after a few seconds
#   when_all m.status == "next" do
#     post :status => "last"
#     puts "next"
#     Process.kill 9, Process.pid
#   end

#   when_all m.status == "last" do
#     puts "last"
#   end
  
#   when_start do
#     post :flow, { :status => "start" }
#   end
# end


Durable.run_all