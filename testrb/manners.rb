require_relative "../librb/durable"

Durable.statechart :miss_manners do
  state :start do
    to :assign, when_all(m.t == "guest") do
      assert(:t => "seating",
             :id => s.g_count,
             :s_id => s.count,
             :p_id => 0,
             :path => 1, 
             :left_seat => 1,
             :left_guest_name => m.name,
             :right_seat => 1,
             :right_guest_name => m.name)
      assert(:t => "path",
             :id => s.g_count + 1,
             :s_id => s.count,
             :seat => 1, 
             :guest_name => m.name)
      s.count += 1
      s.g_count += 2
      puts("assign #{m.name}, #{Time.now}")
    end
  end
  state :assign do
    to :make, when_all(c.seating = (m.t == "seating") & 
                                   (m.path == 1), 
                       c.right_guest = (m.t == "guest") & 
                                       (m.name == seating.right_guest_name), 
                       c.left_guest = (m.t == "guest") & 
                                      (m.sex != right_guest.sex) & 
                                      (m.hobby == right_guest.hobby),
                       none((m.t == "path") & 
                            (m.p_id == seating.s_id) & 
                            (m.guest_name == left_guest.name)),
                       none((m.t == "chosen") & 
                            (m.c_id == seating.s_id) & 
                            (m.guest_name == left_guest.name) &
                            (m.hobby == right_guest.hobby))) do
      assert(:t => "seating",
             :id => s.g_count,
             :s_id => s.count,
             :p_id => seating.s_id,
             :path => 0,
             :left_seat => seating.right_seat,
             :left_guest_name => seating.right_guest_name,
             :right_seat => seating.right_seat + 1,
             :right_guest_name => left_guest.name)
      assert(:t => "path",
             :id => s.g_count + 1,
             :p_id => s.count,
             :seat => seating.right_seat + 1,
             :guest_name => left_guest.name)
      assert(:t => "chosen",
             :id => s.g_count + 2,
             :c_id => seating.s_id,
             :guest_name => left_guest.name,
             :hobby => right_guest.hobby)
      s.count += 1
      s.g_count += 3
    end
  end
  state :make do
    to :make, when_all(c.seating = (m.t == "seating") &
                                   (m.path == 0),
                       c.path = (m.t == "path") &
                                (m.p_id == seating.p_id),
                       none((m.t == "path") &
                            (m.p_id == seating.s_id) &
                            (m.guest_name == path.guest_name))) do
      assert(:t => "path",
             :id => s.g_count,
             :p_id => seating.s_id,
             :seat => path.seat,
             :guest_name => path.guest_name)
      s.g_count += 1
    end
    to :check, when_all(pri(1), (m.t == "seating") & (m.path == 0)) do
      retract(m)
      m.id = s.g_count
      m.path = 1
      assert(m)
      s.g_count += 1
      puts("path sid: #{m.s_id}, pid: #{m.p_id}, left guest: #{m.left_guest_name}, right guest: #{m.right_guest_name}, #{Time.now}")
    end
  end
  state :check do
    to :end, when_all(c.last_seat = (m.t == "last_seat"),
                     (m.t == "seating") & (m.right_seat == last_seat.seat)) do
      puts("end #{Time.now}")
    end
    to :assign
  end
  state :end
  when_start do
    assert :miss_manners, {:id => 1, :sid => 1, :t => "guest", :name => "n1", :sex => "m", :hobby => "h3"}
    assert :miss_manners, {:id => 2, :sid => 1, :t => "guest", :name => "n1", :sex => "m", :hobby => "h2"}
    assert :miss_manners, {:id => 3, :sid => 1, :t => "guest", :name => "n2", :sex => "m", :hobby => "h2"}
    assert :miss_manners, {:id => 4, :sid => 1, :t => "guest", :name => "n2", :sex => "m", :hobby => "h3"}
    assert :miss_manners, {:id => 5, :sid => 1, :t => "guest", :name => "n3", :sex => "m", :hobby => "h1"}
    assert :miss_manners, {:id => 6, :sid => 1, :t => "guest", :name => "n3", :sex => "m", :hobby => "h2"}
    assert :miss_manners, {:id => 7, :sid => 1, :t => "guest", :name => "n3", :sex => "m", :hobby => "h3"}
    assert :miss_manners, {:id => 8, :sid => 1, :t => "guest", :name => "n4", :sex => "f", :hobby => "h3"}
    assert :miss_manners, {:id => 9, :sid => 1, :t => "guest", :name => "n4", :sex => "f", :hobby => "h2"}
    assert :miss_manners, {:id => 10, :sid => 1, :t => "guest", :name => "n5", :sex => "f", :hobby => "h1"}
    assert :miss_manners, {:id => 11, :sid => 1, :t => "guest", :name => "n5", :sex => "f", :hobby => "h2"}
    assert :miss_manners, {:id => 12, :sid => 1, :t => "guest", :name => "n5", :sex => "f", :hobby => "h3"}
    assert :miss_manners, {:id => 13, :sid => 1, :t => "guest", :name => "n6", :sex => "f", :hobby => "h3"}
    assert :miss_manners, {:id => 14, :sid => 1, :t => "guest", :name => "n6", :sex => "f", :hobby => "h1"}
    assert :miss_manners, {:id => 15, :sid => 1, :t => "guest", :name => "n6", :sex => "f", :hobby => "h2"}
    assert :miss_manners, {:id => 16, :sid => 1, :t => "guest", :name => "n7", :sex => "f", :hobby => "h3"}
    assert :miss_manners, {:id => 17, :sid => 1, :t => "guest", :name => "n7", :sex => "f", :hobby => "h2"}
    assert :miss_manners, {:id => 18, :sid => 1, :t => "guest", :name => "n8", :sex => "m", :hobby => "h3"}
    assert :miss_manners, {:id => 19, :sid => 1, :t => "guest", :name => "n8", :sex => "m", :hobby => "h1"}
    assert :miss_manners, {:id => 20, :sid => 1, :t => "last_seat", :seat => 8}
    patch_state :miss_manners, {:sid => 1, :label => "start", :count => 0, :g_count => 1000}
  end
end

Durable.run_all

   
    	