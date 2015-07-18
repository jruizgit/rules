var redis = require('redis'),
client = redis.createClient('/tmp/redis0.sock', {});
client.hset('test', 'first', JSON.stringify({
    id: 1,
    sid: 'first',
    name: 'John Smith',
    address: '1111 NE 22, Seattle, Wa',
    phone: '206678787',
    country: 'US',
    currency: 'US',
    seller: 'bookstore',
    item: 'book',
    reference: '75323',
    amount: 500
}));
for (var i = 0; i < 5; ++ i) {
	client.rpush('test2', JSON.stringify({
	    id: 1,
	    sid: 'first',
	    name: 'John Smith',
	    address: '1111 NE 22, Seattle, Wa',
	    phone: '206678787',
	    country: 'US',
	    currency: 'US',
	    seller: 'bookstore',
	    item: 'book',
	    reference: '75323',
	    amount: 500
	}));
}
client.script('load', 'for i = 1, 2000000, 1 do\n' +
	                  '    local result = redis.call("hget", "test", "first")\n' +
	                  'end\n',
function (err, result) {
	if (err) {
		console.log(err);
		return;
	}

	var hash1 = result;
	client.script('load', 'local hash = redis.call("hget", "test", "first")\n' +
						  'for i = 1, 40000000, 1 do\n' +
		                  '    local result = hash["name"]\n' +
		                  'end\n',
	function (err, result) {
		if (err) {
			console.log(err);
			return;
		}

		var hash2 = result;
		client.script('load', 'local hash = redis.call("hget", "test", "first")\n' +
			                  'local packed_hash = cmsgpack.pack(hash)\n' +
						      'for i = 1, 10000000, 1 do\n' +
						      '    hash = cmsgpack.unpack(packed_hash)\n' +
		                      '    local result = hash["name"]\n' +
		                      'end\n',
		function (err, result) {
			if (err) {
				console.log(err);
				return;
			}

			var hash3 = result;
			client.script('load', 'for i = 1, 500000, 1 do\n' +
						          '    local resul = redis.call("lrange", "test2", 0, -1)\n' +
		                          'end\n',
			function (err, result) {
				if (err) {
					console.log(err);
					return;
				}

				var hash4 = result;
				client.script('load', 'local list = redis.call("lrange", "test2", 0, -1)\n' +
					                  'local packed_list = cmsgpack.pack(list)\n' +
					                  'redis.call("hset", "test3", "first", packed_list)\n' +
								      'for i = 1, 500000, 1 do\n' +
								      '    packed_list = redis.call("hget", "test3", "first")\n' +
								      '    list = cmsgpack.unpack(packed_list)\n' +
				                      'end\n',
				function (err, result) {
					if (err) {
						console.log(err);
						return;
					}
					var hash5 = result;
					var start = new Date();

					var test6 = function(index) {
						client.evalsha(hash5, 0, function (err, result) {
							if (err) {
								console.log(err);
								return;
							}

							if (index == 0) {
								console.log('end 6 ' + (new Date() - start));
							} else {
								setImmediate(test6, index - 1);
							}
						});
					}

					var test5 = function(index) {
						client.evalsha(hash4, 0, function (err, result) {
							if (err) {
								console.log(err);
								return;
							}

							if (index == 0) {
								var newStart = new Date();
								console.log('end 5 ' + (newStart - start));
								start = newStart;
								setImmediate(test6, 0);
							} else {
								setImmediate(test5, index - 1);
							}
						});
					}

					// 7.3M/sec
					var test4 = function(index) {
						client.evalsha(hash3, 0, function (err, result) {
							if (err) {
								console.log(err);
								return;
							}

							if (index == 0) {
								var newStart = new Date();
								console.log('end 4 ' + (newStart - start));
								start = newStart;
								setImmediate(test5, 0);
							} else {
								setImmediate(test4, index - 1);
							}
						});
					}
					// 50M/sec
					var test3 = function(index) {
						client.evalsha(hash2, 0, function (err, result) {
							if (err) {
								console.log(err);
								return;
							}

							if (index == 0) {
								var newStart = new Date();
								console.log('end 3 ' + (newStart - start));
								start = newStart;
								setImmediate(test4, 0);
							} else {
								setImmediate(test3, index - 1);
							}
						});
					}
					// 2.2M/sec
					var test2 = function(index) {
						client.evalsha(hash1, 0, function (err, result) {
							if (err) {
								console.log(err);
								return;
							}

							if (index == 0) {
								var newStart = new Date();
								console.log('end 2 ' + (newStart - start));
								start = newStart;
								setImmediate(test3, 0);
							} else {
								setImmediate(test2, index - 1);
							}
						});
					}
					// 85836/sec
					var test1 = function(index) {
						var multi = client.multi();
						for (var i = 0; i < 100; ++ i) {
							multi.hget('test', 'first');
						}
						multi.exec(function (err, result) {
							if (err) {
								console.log(err);
								return;
							}
							if (index == 0) {
								var newStart = new Date();
								console.log('end 1 ' + (newStart - start));
								start = newStart;
								setImmediate(test2, 0);
							} else {
								setImmediate(test1, index - 1);
							}
						});
					}
					setImmediate(test1, 1000);
				});
			});
		});
	});
});
