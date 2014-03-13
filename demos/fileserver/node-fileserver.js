var express = require('express');
var app = express();
var path = require('path');

app.use(express.static(path.join(__dirname, 'public'))); 

app.listen(9090);
console.log('listen 9090');
