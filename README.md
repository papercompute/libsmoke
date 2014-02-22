## smoke engine

[smokengine](https://github.com/papercompute/smokengine) native performance for network apps

### current state: 

very experimental

### design goals:

* no complexity
* low latency & high performance

### applications:

* mass multiplayer games
* realtime apps

### sample code net

```c
  // plain tcp server
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

### sample code http

```c
 smoke::http::http_t app;

  app.get("/counter",[&](int fd,http_req_t& req)->int{
   DBG("get /counter %d\n",fd);
   if(write(fd,HTTPOK,sizeof(HTTPOK)-1)<=0){LOG("write error, %d\n",errno);}
   return 0;
  }); 

  app.get("/test/api",[&](int fd,http_req_t& req)->int{
   DBG("get /test/api %d\n",fd);
   if(write(fd,HTTPOK,sizeof(HTTPOK)-1)<=0){LOG("write error, %d\n",errno);}
   return 0;
  }); 

  app.post("/data",[&](int fd,http_req_t& req)->int{
   DBG("post /data %d\n",fd);
   if(write(fd,HTTPOK,sizeof(HTTPOK)-1)<=0){LOG("write error, %d\n",errno);}
   return 0;
  }); 

   
  app.run("127.0.0.1",port);

```

### sock fd data

```c

// server code

  nets_t net1; 

   net1.createServer("127.0.0.1",port,[](sock_t& sock){
 
   sock.on_data([](sock_t& sock,const char* data, int nread){
    DBG("sock.on_data fd=%d\n",sock.fd);
    return 1;
   });

   sock.on_write([](sock_t& sock){
    char buf[128];
    int l=sprintf(buf,"hello from sock.on_write %d\n",sock.wc); 
    if(write(sock.fd,buf,l)!=l){LOG("write error\n");};
    sock.wc++;
    if(sock.wc>3){
     sock.end(); 
     return 0;
    }
    return 1;
   });

   sock.on_end([](sock_t& sock){
    //
   });

  });

// client, non-blocking connect

smoke::sock::connect("127.0.0.1",port,[](sock_t& s){

 s.on_data([](sock_t& s,const char* data,int len)->int{
  s.rc++;
  if(s.rc>3){
   s.end();
   return 0;
  }
  return 1;
 });

 // fire after connect
 s.on_write([](sock_t& s){
  write(s.fd,HTTP_REQUEST,sizeof(HTTP_REQUEST)-1);
  return 0;
 });

 s.on_end([](sock_t& s){
 });


```

### app code structure

```c
// config modules
#include "smoke_config.h"

// core engine modules
#include "smoke.h"

// middleware modules
#include "smoke_http.h"

// globals
int host_port;
smoke::net_t net;

// app modules
#include "smoke_app1.h"

// app run
void serve()
{
  smoke::net_run(&net,"127.0.0.1",port);  
}

int main (int argc, char *argv[])
{

  host_port = atoi(argv[1]);

  serve();
}

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

