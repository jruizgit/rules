require 'json'
require 'durable'

puts('books1 *****')
handle = Rules.create_ruleset 'books1', JSON.generate({
  :ship => {
    :all => [
      {:order => {
        :$and => [
            {:$lte => {:amount => 1000}},
            {:country => 'US'},
            {:currency => 'US'},
            {:seller => 'bookstore'}
        ]
      }},
      {:available => {
        :$and => [
            {:item => 'book'},
            {:country => 'US'},
            {:seller => 'bookstore'},
            {:status => 'available'} 
        ]
      }}
    ]
  }
}), 100

Rules.bind_ruleset handle, 'localhost', 6379, nil, 0

Rules.assert_event handle, JSON.generate({
  :id => 1,
  :sid => 'first',
  :name => 'John Smith',
  :address => '1111 NE 22, Seattle, Wa',
  :phone => '206678787',
  :country => 'US',
  :currency => 'US',
  :seller => 'bookstore',
  :item => 'book',
  :reference => '75323',
  :amount => 500
})

Rules.assert_event handle, JSON.generate({
  :id => 2,
  :sid => 'first',
  :item => 'book',
  :status => 'available',
  :country => 'US',
  :seller => 'bookstore'
})

result = Rules.start_action handle

puts JSON.parse(result[0])
puts JSON.parse(result[1])

Rules.complete_action handle, result[2], result[0]
Rules.delete_ruleset handle

puts 'books2 ******'

handle = Rules.create_ruleset 'books2',  JSON.generate({
  :ship => {
    :all => [
      :m => {:$and => [
          {:country => 'US'},
          {:seller => 'bookstore'},
          {:currency => 'US'},
          {:$lte => {:amount => 1000}},
      ]}
    ]
  },
  :order => {
    :all => [
      :m => {:$and => [
          {:country => 'US'},
          {:seller => 'bookstore'},
          {:currency => 'US'},
          {:$lte => {:amount => 1000}},
      ]}
    ]
  }
}), 100

Rules.bind_ruleset handle, 'localhost', 6379, nil, 0

Rules.assert_event handle, JSON.generate({
  :id => 1,
  :sid => 'first',
  :name => 'John Smith',
  :address => '1111 NE 22, Seattle, Wa',
  :phone => '206678787',
  :country => 'US',
  :currency => 'US',
  :seller => 'bookstore',
  :item => 'book',
  :reference => '75323',
  :amount => 500
})

result = Rules.start_action handle

puts JSON.parse(result[0])
puts JSON.parse(result[1])

Rules.complete_action handle, result[2], result[0]
Rules.delete_ruleset handle

puts 'books3 ******'

handle = Rules.create_ruleset 'books3', JSON.generate({
  :ship => {
    :all => [:m => {:$nex => {:label => 1}}]
 }
}), 100

Rules.bind_ruleset handle, 'localhost', 6379, nil, 0

Rules.assert_event handle, JSON.generate({
  :id => 1,
  :sid => 'first',
  :name => 'John Smith',
  :address => '1111 NE 22, Seattle, Wa'
})

result = Rules.start_action handle

puts JSON.parse(result[0])
puts JSON.parse(result[1])

Rules.complete_action handle, result[2], result[0]
Rules.delete_ruleset handle

puts 'books4 ******'

handle = Rules.create_ruleset 'books4', JSON.generate({
  :ship => {
    :count => 5,
    :all => [
      :m => {:$and => [
          {:$lte => {:amount => 1000}},
          {:subject => 'approve'}
      ]}
    ]
  }
}), 100

Rules.bind_ruleset handle, 'localhost', 6379, nil, 0

Rules.assert_events handle, JSON.generate([
  {:id => '0', :sid => 1, :subject => 'approve', :amount => 100}, 
  {:id => '1', :sid => 1, :subject => 'approve', :amount => 100},
  {:id => '2', :sid => 1, :subject => 'approve', :amount => 100},
  {:id => '3', :sid => 1, :subject => 'approve', :amount => 100},
  {:id => '4', :sid => 1, :subject => 'approve', :amount => 100}, 
])

result = Rules.start_action handle

puts JSON.parse(result[0])
puts JSON.parse(result[1])

Rules.complete_action handle, result[2], result[0]
Rules.delete_ruleset handle

puts 'approval1 ******'

handle = Rules.create_ruleset 'approval1', JSON.generate({
  :r1 => {
    :all => [
      {'a$any' => [
        {:b => {:subject => 'approve'}},
        {:c => {:subject => 'review'}}
      ]},
      {'d$any' => [
        {:e => {:$lt => {:total => 1000}}},
        {:f => {:$lt => {:amount => 1000}}}
      ]}
    ]
  }
}), 100

Rules.bind_ruleset handle, 'localhost', 6379, nil, 0

Rules.assert_event handle, JSON.generate({
  :id => 3,
  :sid => 'second',
  :subject => 'approve'
})

Rules.assert_event handle, JSON.generate({
  :id => 4,
  :sid => 'second',
  :amount => 100
})

result = Rules.start_action handle

puts JSON.parse(result[0])
puts JSON.parse(result[1])

Rules.complete_action handle, result[2], result[0]
Rules.delete_ruleset handle

puts 'approval2 ******'

handle = Rules.create_ruleset 'approval2', JSON.generate({
  :r2 => {
    :any => [
      {'a$all' => [
        {:b => {:subject => 'approve'}},
        {:c => {:subject => 'review'}}
      ]},
      {'d$all' => [
        {:e => {:$lt => {:total => 1000}}},
        {:f => {:$lt => {:amount => 1000}}}
      ]}
    ]
  }
}), 100

Rules.bind_ruleset handle, 'localhost', 6379, nil, 0

Rules.assert_event handle, JSON.generate({
  :id => 5,
  :sid => 'second',
  :subject => 'approve'
})

Rules.assert_event handle, JSON.generate({
  :id => 6,
  :sid => 'second',
  :subject => 'review'
})

result = Rules.start_action handle

puts JSON.parse(result[0])
puts JSON.parse(result[1])

Rules.complete_action handle, result[2], result[0]
Rules.delete_ruleset handle
