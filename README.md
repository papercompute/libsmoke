## libsmoke

[smoke](https://github.com/papercompute/smoke-epoll) high performance low latency multi-threaded/processed udp/tcp/http/ws server lib built on top of linux epoll/pfring

### current state: 

very experimental

### design goals:

* no complexity
* low latency & high performance

### applications:

* mass multiplayer games
* realtime apps

### sample code

```c
  // simple tcp server
  smoke::net_t net;

  net.on_connect([](int fd){
   // on connect event
  });

  net.on_close([](int fd,int err){
  // on close event
  });

  net.on_data([](int fd,const char* data,int nread)->int{
  // on data event

    int r=write(fd,"hello",5);
    close(fd);
    return 0; 
  });

  // run IO loop
  smoke_net_run(&net,"127.0.0.1",port);

```

### great libs & frameworks:

[nodejs](https://github.com/joyent/node) [libuv](https://github.com/joyent/libuv)
[go](http://golang.org) [erlang](http://www.erlang.org/)
[libev](http://software.schmorp.de/pkg/libev.html) [libevent](http://libevent.org/) [libphenom](http://facebook.github.io/libphenom/)
[tornado](http://www.tornadoweb.org)
[gevent](http://www.gevent.org)
[akka](http://akka.io/)
[play](http://www.playframework.com)






testing utils:

sudo sh -c "ulimit -n 65535 && exec su $LOGNAME"

./wrk -t32 -c10000 -d5m --timeout 60s --latency http://127.0.0.1:9090/

ab -n 1000000 -c 10000 http://localhost:9090/

curl -v http://localhost:9090/

