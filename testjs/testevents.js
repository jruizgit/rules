r = require('../build/release/rules.node');
var cluster = require('cluster');

console.log('fraud');

handle = r.createRuleset('fraud',  
    JSON.stringify({
        suspect: {
            whenAll: {
                first: { t: 'purchase' },
                second: { $neq: { location: { first: 'location' } } }
            },
        }
    })
, 100);

r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

r.assertEvent(handle, 
    JSON.stringify({
        id: 1,
        sid: 1,
        t: 'purchase',
        amount: '100',
        location: 'US',
    })
);

r.assertEvent(handle, 
    JSON.stringify({
        id: 2,
        sid: 1,
        t: 'purchase',
        amount: '200',
        location: 'CA',
    })
);

result = r.startAction(handle);
console.log(JSON.parse(result[1]));
console.log(JSON.parse(result[2]));
r.completeAction(handle, result[0], result[1]);
r.deleteRuleset(handle);


local fame_list = nil
local message_list = {}
local key = ARGV[1]
local sid = ARGV[2]
local score = ARGV[3]

if #ARGV > 3 then 
    local message = {}
    for index = 4, index <= #ARGV, 2 do
        message[ARGV[index]] = ARGV[index + 1]
    end
    table.insert(message_list, message)
end

if key == "first!suspect!fraud" then
    local new_frame_list = {}
    if message_list ~= nil then
        save_event(key .. "!e!" .. sid, message_list[1])
    else 
        message_list = load_events(messages_key, key .. "!e!" .. sid)
    end
    for message in message_list do
        table.insert(new_frame_list, {["first"] = message})
    end

    frame_list = new_frame_list
    message_list = nil
    key = "second!suspect!fraud"
end

if key == "second!suspect!fraud" then
    local new_frame_list = {}
    if message_list ~= nil then
        frame_list = load_frames(messages_key, key .. "!f!" .. sid)
        save_event(key .. "!e!" .. sid, message_list[1])
    else 
        save_frames(key .. "!f!" .. sid, frame_list, nil)
        message_list = load_events(messages_key, key .. "!e!" .. sid)
    end
    for frame in frame_list do
        for message in message_list do
            if message["location"] ~= frame["first"]["location"] then
                frame["second"] = message
                table.insert(new_frame_list, frame)
            end
        end
    end

    frame_list = new_frame_list
    message_list = nil
    key = "fraud!a"
end

save_frames(key .. sid, frame_list, "suspect!fraud")
if #frame_list > 0 then
    redis.call("zadd", "fraud!a", score, sid)
end

local save_event = function(messages_key, key, message)
    redis.call("rpush", key, message["id"])
    redis.call("hset", messages_key, mid, cmsgpack.pack(message))
end

local load_events = function(messages_key, key) 
    local message_list = {}
    local packed_message_list_len = redis.call("llen", key)
    for index = 0, index <  packed_message_list_len, 1  do
        local mid = redis.call("lpop", key)
        local packed_message = redis.call("hget", messages_key, mid)
        if packed_message ~= nil then
            redis.call("rpush", key, mid)
            table.insert(message_list, cmsgpack.unpack(packed_message))
        end
    end

    return messages_list
end

local save_frames = function(key, frame_list, action_name)
    for frame in frame_list do
        local frame_to_pack = {}
        for name, message in frame do
            frame_to_pack[name] = message["id"]
        end
        if action_name ~= nil then
            redis.call("rpush", key, action_name)
        end
        redis.call("rpush", key, cmsgpack.pack(frame_to_pack))
    end
end

local load_frames = function(messages_key, key)
    local frame_list = {}
    local packed_frame_list_len = redis.call("llen", key)
    for index = 0, index <  packed_frame_list_len, 1  do
        local packed_frame = redis.call("lpop", key)
        local frame = cmsgpack.unpack(packed_frame)
        for name, mid in frame do 
            local packed_message = redis.call("hget", messages_key, mid)
            if packed_message == nil then
                frame = nil
                break
            else
                frame[name] = cmsgpack.unpack(packed_message)
            end
        end

        if frame ~= nil then
            redis.call("rpush", key, packed_frame)
            table.insert(frame_list, frame)
        end
    end
    return frame_list
end

// console.log('books');

// handle = r.createRuleset('books',  
//     JSON.stringify({
//         ship: {
//             whenAll: {
//                 order: { 
//                     $and: [
//                         { $lte: { amount: 1000 } },
//                         { country: 'US' },
//                         { currency: 'US' },
//                         { seller: 'bookstore'} 
//                     ]
//                 },
//                 available:  { 
//                     $and: [
//                         { item: 'book' },
//                         { country: 'US' },
//                         { seller: 'bookstore' },
//                         { status: 'available'} 
//                     ]
//                 }
//             },
//         }
//     })
// , 100);

// r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

// r.assertEvent(handle, 
//     JSON.stringify({
//         id: 1,
//         sid: 'first',
//         name: 'John Smith',
//         address: '1111 NE 22, Seattle, Wa',
//         phone: '206678787',
//         country: 'US',
//         currency: 'US',
//         seller: 'bookstore',
//         item: 'book',
//         reference: '75323',
//         amount: 500
//     })
// );

// r.assertEvent(handle,
//     JSON.stringify({
//         id: 2,
//         sid: 'first',
//         item: 'book',
//         status: 'available',
//         country: 'US',
//         seller: 'bookstore'
//     })
// );

// result = r.startAction(handle);
// console.log(JSON.parse(result[1]));
// console.log(JSON.parse(result[2]));
// r.completeAction(handle, result[0], result[1]);
// r.deleteRuleset(handle);

// console.log('books2');

// handle = r.createRuleset('books2',  
//     JSON.stringify({
//         ship: {
//             whenAll: {
//                 order: { 
//                     $and: [
//                         { country: 'US' },
//                         { seller: 'bookstore'},
//                         { currency: 'US' },
//                         { $lte: { amount: 1000 } },
//                     ]
//                 },
//                 available:  { 
//                     $and: [
//                         { country: 'US' },
//                         { seller: 'bookstore' },
//                         { status: 'available'},
//                         { item: 'book' },
//                     ]
//                 }
//             }
//         }
//     })
// , 100);

// r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

// r.assertEvent(handle, 
//     JSON.stringify({
//         id: 1,
//         sid: 'first',
//         name: 'John Smith',
//         address: '1111 NE 22, Seattle, Wa',
//         phone: '206678787',
//         country: 'US',
//         currency: 'US',
//         seller: 'bookstore',
//         item: 'book',
//         reference: '75323',
//         amount: 500
//     })
// );

// r.assertEvent(handle,
//     JSON.stringify({
//         id: 2,
//         sid: 'first',
//         item: 'book',
//         status: 'available',
//         country: 'US',
//         seller: 'bookstore'
//     })
// );

// result = r.startAction(handle);
// console.log(JSON.parse(result[1]));
// console.log(JSON.parse(result[2]));
// r.completeAction(handle, result[0], result[1]);
// r.deleteRuleset(handle);

// console.log('books3');

// handle = r.createRuleset('books3',  
//     JSON.stringify({
//         ship: {
//             when: { 
//                 $and: [
//                     { country: 'US' },
//                     { seller: 'bookstore'},
//                     { currency: 'US' },
//                     { $lte: { amount: 1000 } },
//                 ]
//             },
//         },
//         order: {
//             when: { 
//                 $and: [
//                     { country: 'US' },
//                     { seller: 'bookstore'},
//                     { currency: 'US' },
//                     { $lte: { amount: 1000 } },
//                 ]
//             },
//         },
//     })
// , 100);

// r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

// r.assertEvent(handle, 
//     JSON.stringify({
//         id: 1,
//         sid: 'first',
//         name: 'John Smith',
//         address: '1111 NE 22, Seattle, Wa',
//         phone: '206678787',
//         country: 'US',
//         currency: 'US',
//         seller: 'bookstore',
//         item: 'book',
//         reference: '75323',
//         amount: 500
//     })
// );

// result = r.startAction(handle);
// console.log(JSON.parse(result[1]));
// console.log(JSON.parse(result[2]));
// r.completeAction(handle, result[0], result[1]);
// r.deleteRuleset(handle);

// console.log('books4');

// handle = r.createRuleset('books4',  
//     JSON.stringify({
//         ship: {
//             when: { $nex: { label: 1 }}
//         }
//     })
// , 100);

// r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

// r.assertEvent(handle, 
//     JSON.stringify({
//         id: 1,
//         sid: 'first',
//         name: 'John Smith',
//         address: '1111 NE 22, Seattle, Wa',
//         phone: '206678787',
//         country: 'US',
//         currency: 'US',
//         seller: 'bookstore',
//         item: 'book',
//         reference: '75323',
//         amount: 500
//     })
// );

// result = r.startAction(handle);
// console.log(JSON.parse(result[1]));
// console.log(JSON.parse(result[2]));
// r.completeAction(handle, result[0], result[1]);
// r.deleteRuleset(handle);


// console.log('approval1');

// handle = r.createRuleset('approval1', 
//     JSON.stringify({ r1: {
//             whenAll: {
//                 a$any: {
//                     b: { subject: 'approve' },
//                     c: { subject: 'review' }
//                 },
//                 d$any: {
//                     e: { $lt: { total: 1000 }},
//                     f: { $lt: { amount: 1000 }}
//                 }
//             }
//         }
//     })
// , 100);

// r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

// r.assertEvent(handle,
//     JSON.stringify({
//         id: 3,
//         sid: 'second',
//         subject: 'approve'
//     })
// );

// r.assertEvent(handle,
//     JSON.stringify({
//         id: 4,
//         sid: 'second',
//         amount: 100
//     })
// );

// result = r.startAction(handle);
// console.log(JSON.parse(result[1]));
// console.log(JSON.parse(result[2]));
// r.completeAction(handle, result[0], result[1]);
// r.deleteRuleset(handle);

// console.log('approval2');

// handle = r.createRuleset('approval2', 
//     JSON.stringify({ r2: {
//             whenAny: {
//                 a$all: {
//                     b: { subject: 'approve' },
//                     c: { subject: 'review' }
//                 },
//                 d$all: {
//                     e: { $lt: { total: 1000 }},
//                     f: { $lt: { amount: 1000 }}
//                 }
//             }
//         }
//     })
// , 100);

// r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

// r.assertEvent(handle,
//     JSON.stringify({
//         id: 1,
//         sid: 'third',
//         subject: 'approve'
//     })
// );

// r.assertEvent(handle,
//     JSON.stringify({
//         id: 2,
//         sid: 'third',
//         subject: 'review'
//     })
// );

// result = r.startAction(handle);
// console.log(JSON.parse(result[1]));
// console.log(JSON.parse(result[2]));
// r.completeAction(handle, result[0], result[1]);

// r.deleteRuleset(handle);

// console.log('approval3');

// handle = r.createRuleset('approval3', 
//     JSON.stringify({
//         r1: { 
//             when: {$lte: {amount: {$s: 'maxAmount'}}}
//         }
//     })
// , 4);

// r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

// console.log(r.assertState(handle,
//     JSON.stringify({
//         id: 1,
//         maxAmount: 100
//     })
// ));

// console.log(r.assertEvent(handle,
//     JSON.stringify({
//         id: 1,
//         sid: 1,
//         amount: 1000
//     })
// ));

// console.log(r.assertEvent(handle,
//     JSON.stringify({
//         id: 2,
//         sid: 1,
//         amount: 10
//     })
// ));

// result = r.startAction(handle);
// console.log(JSON.parse(result[1]));
// console.log(JSON.parse(result[2]));
// r.completeAction(handle, result[0], result[1]);

// r.assertState(handle,
//     JSON.stringify({
//         id: 1,
//         maxAmount: 10000
//     })
// );

// console.log(r.assertEvent(handle,
//     JSON.stringify({
//         id: 3,
//         sid: 1,
//         amount: 1000
//     })
// ));

// console.log(r.assertEvent(handle,
//     JSON.stringify({
//         id: 4,
//         sid: 2,
//         amount: 1000
//     })
// ));

// console.log(r.assertEvent(handle,
//     JSON.stringify({
//         id: 5,
//         sid: 3,
//         amount: 1000
//     })
// ));

// console.log(r.assertEvent(handle,
//     JSON.stringify({
//         id: 6,
//         sid: 4,
//         amount: 1000
//     })
// ));

// console.log(r.assertEvent(handle,
//     JSON.stringify({
//         id: 7,
//         sid: 5,
//         amount: 1000
//     })
// ));

// console.log(r.assertEvent(handle,
//     JSON.stringify({
//         id: 8,
//         sid: 1,
//         amount: 1000
//     })
// ));

// result = r.startAction(handle);
// console.log(JSON.parse(result[1]));
// console.log(JSON.parse(result[2]));
// r.completeAction(handle, result[0], result[1]);

// r.deleteRuleset(handle);

// console.log('approval4');

// handle = r.createRuleset('approval4', 
//     JSON.stringify({
//         r1: { 
//             when: {$lte: {amount: {$s: { name: 'maxAmount', time: 60, id: 1}}}}
//         },
//         r2: { 
//             when: {$gte: {amount: {$s: { name: 'minAmount', time: 60, id: 2}}}}
//         },
//     })
// , 4);

// r.bindRuleset(handle, '/tmp/redis.sock', 0, null);

// console.log(r.assertState(handle,
//     JSON.stringify({
//         id: 1,
//         maxAmount: 300
//     })
// ));

// console.log(r.assertState(handle,
//     JSON.stringify({
//         id: 2,
//         minAmount: 200
//     })
// ));

// console.log(r.assertEvent(handle,
//     JSON.stringify({
//         id: 1,
//         sid: 3,
//         amount: 500 
//     })
// ));

// result = r.startAction(handle);
// console.log(JSON.parse(result[1]));
// console.log(JSON.parse(result[2]));
// r.completeAction(handle, result[0], result[1]);

// console.log(r.assertEvent(handle,
//     JSON.stringify({
//         id: 2,
//         sid: 3,
//         amount: 100
//     })
// ));

// result = r.startAction(handle);
// console.log(JSON.parse(result[1]));
// console.log(JSON.parse(result[2]));
// r.completeAction(handle, result[0], result[1]);

// r.deleteRuleset(handle);