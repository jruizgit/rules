require_relative '../librb/durable'

Durable.run_ruleset :approve do
  when_one (m.amount < 1000) | (m.amount > 10000) do |s|
    s.status = 'pending'
  end
  when_all [m.subject == 'approved', s.status == 'pending'] do |s|
    s.status = 'approved'
    puts 'approved'
  end
  when_start do
    puts 'starting'
    post :approve, {:id => 1, :sid => 1, :amount => 100}
    post :approve, {:id => 2, :sid => 1, :subject => 'approved'}
  end
end

Durable.run({
  :approve => {
    :r1 => {
      :when => {:$lt => { :amount => 1000}},
      :run => -> s { s.state['status'] = 'pending' }
    },
    :r2 => {
      :whenAll => {
        :$m => {:subject => 'approved'},
        :$s => {:status => 'pending'}
      },
      :run => -> s { s.state['status'] = 'approved'; puts 'approved' }
    }
  }
}, ['/tmp/redis.sock'], -> host {
  host.post 'approve', {:id => 1, :sid => 1, :amount => 100}
  host.post 'approve', {:id => 2, :sid => 1, :subject => 'approved'}
})

