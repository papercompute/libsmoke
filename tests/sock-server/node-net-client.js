var net = require('net');
var port = 9090;

var request1 =   "GET / HTTP/1.1\r\n"+
  "User-Agent: smoke\r\n"+
  "Host: 127.0.0.1\r\n"+
  "Accept: */*\r\n"+
  "\r\n"


var net = require('net');
var client = net.connect({port: port}, function() { //'connect' listener
  console.log('client connected');
  client.write(request1);
});
client.on('data', function(data) {
  console.log('data:',data.toString());
  //client.end();
});
client.on('end', function() {
  console.log('client disconnected');
});

