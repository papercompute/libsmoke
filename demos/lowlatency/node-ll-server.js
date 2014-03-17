var express = require('express');
var app = express();
var counter = 0;

app.get('/', function(req, res){
	res.json({ user: 'tobi', counter: counter++ })
});

app.listen(9090);