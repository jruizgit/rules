require 'json'
require_relative '../src/rulesrb/rules'

puts('books1 *****')
handle = Rules.create_ruleset 'books1', JSON.generate({
  :ship => {
    :whenAll => {
      :order => {
        :$and => [
            {:$lte => {:amount => 1000}},
            {:country => 'US'},
            {:currency => 'US'},
            {:seller => 'bookstore'} 
        ]
      },
      :available => {
        :$and => [
            {:item => 'book'},
            {:country => 'US'},
            {:seller => 'bookstore'},
            {:status => 'available'} 
        ]
      }
    },
    :run => 'ship'
  }
})

Rules.bind_ruleset handle, '/tmp/redis.sock', 0 , nil

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
    :when => {
      :$and => [
          {:country => 'US'},
          {:seller => 'bookstore'},
          {:currency => 'US'},
          {:$lte => {:amount => 1000}},
      ]
    },
    :run => 'ship'
  },
  :order => {
    :when => {
      :$and => [
          {:country => 'US'},
          {:seller => 'bookstore'},
          {:currency => 'US'},
          {:$lte => {:amount => 1000}},
      ]
    },
    :run => 'order'
  }
})

Rules.bind_ruleset handle, '/tmp/redis.sock', 0, nil

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
    :when => {:$nex => {:label => 1}},
    :run => 'ship'
 }
})

Rules.bind_ruleset handle, '/tmp/redis.sock', 0 , nil

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
    :whenSome => {
      :$and => [
          {:$lte => {:amount => 1000}},
          {:subject => 'approve'}
      ]
    },
    :run => 'ship'
  }
})

Rules.bind_ruleset handle, '/tmp/redis.sock', 0, nil

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
    :whenAll => {
      'a$any' => {
        :b => {:subject => 'approve'},
        :c => {:subject => 'review'}
      },
      'd$any' => {
        :e => {:$lt => {:total => 1000}},
        :f => {:$lt => {:amount => 1000}}
      }
    },
    :run => 'unitTest'
  }
})

Rules.bind_ruleset handle, '/tmp/redis.sock', 0, nil

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
    :whenAny => {
      'a$all' => {
        :b => {:subject => 'approve'},
        :c => {:subject => 'review'}
      },
      'd$all' => {
        :e => {:$lt => {:total => 1000}},
        :f => {:$lt => {:amount => 1000}}
      }
    },
    :run => 'unitTest'
  }
})

Rules.bind_ruleset handle, '/tmp/redis.sock', 0, nil

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
