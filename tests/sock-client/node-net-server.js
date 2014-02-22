var net = require('net');
var port = 9090;
var server = net.createServer(function(c) { 
  console.log('connect');
  c.on('data', function (data) {
    console.log('data:',data.toString());
    c.write('HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 5\r\nConnection: close\r\n\r\nhello');
    //c.end();
  });  
  c.on('end', function() {
    console.log('end');
  });
  //c.write('hello\r\n');
});
server.listen(port, function() {
  console.log('net server, listen ',port);
});
