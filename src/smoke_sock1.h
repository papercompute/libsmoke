/* 

smoke_sock.h data module 

*/

#ifndef _SMOKE_SOCK_H_
#define _SMOKE_SOCK_H_


namespace smoke {

namespace sock {



struct sock_t;

typedef std::function<void(sock_t& sock)> on_sock_end_fn_t; 
typedef std::function<int(sock_t& sock, const char *s,int l)> on_sock_data_fn_t; 
typedef std::function<int(sock_t& sock)> on_sock_write_fn_t; 
typedef std::function<void(sock_t& sock)> on_sock_connect_fn_t; 

// fd connection data
struct sock_t
{

 sock_t():fd(0),wc(0),rc(0),
 on_sock_data_cb(nullptr),
 on_sock_end_cb(nullptr),
 on_sock_write_cb(nullptr)
 {};

 int fd;
 int wc; // write counter 
 int rc; // read counter
 int t; // type 0 -server, 1 - client
 on_sock_data_fn_t on_sock_data_cb;
 on_sock_end_fn_t on_sock_end_cb;
 on_sock_write_fn_t on_sock_write_cb;

 
 void on_write(on_sock_write_fn_t cb){on_sock_write_cb=cb;}
 void on_data(on_sock_data_fn_t cb){on_sock_data_cb=cb;}
 void on_end(on_sock_end_fn_t cb){on_sock_end_cb=cb;}


 //sock_t():wc(0),rc(0){};
 void clear(){
  fd=wc=rc=t=0;
  on_sock_data_cb=nullptr;
  on_sock_end_cb=nullptr;
  on_sock_write_cb=nullptr;
 };
 void end(){
    DBG("sock::end()\n");
  if(on_sock_end_cb){
    DBG("on_sock_end_cb\n");
    on_sock_end_cb(*this);
    close(fd);
  }
 };
};

std::unordered_map<int,sock_t> fmap;  
sock_t sock[FDD_CACHE_RANGE];

sock_t& get_sock(int fd){
 if(fd>=0 && fd<FDD_CACHE_RANGE){
  // get from faster array
  return sock[fd];
 }
 // get from std::unordered_map
 return fmap[fd];
};

struct nets_t
{

net_t net;
//nets_t* that; 

nets_t(){
  init();
  //that=this;
}  
 

on_sock_connect_fn_t on_sock_connect_cb;


int createServer(const char* host,int port,on_sock_connect_fn_t cb){
  int fd=smoke::net_create_server(host,port);
  on_sock_connect_cb = cb;
  return fd;   
}  

void init(){
  nets_t* that = this;
  net.on_connect([that](int fd){
    DBG("on_connect fd=%d\n",fd);
    sock_t& sock=get_sock(fd);
    sock.clear();sock.fd=fd;
    that->on_sock_connect_cb(sock);
  });

  net.on_close([](int fd,int err){
    DBG("on_close fd=%d\n",fd);
    sock_t& sock=get_sock(fd);
    if(sock.on_sock_end_cb){
      sock.on_sock_end_cb(sock);
    }
  });

  net.on_data([](int fd,const char* data,int nread)->int{
    sock_t& sock=get_sock(fd);
    DBG("on_data fd=%d: wc:%d,rc:%d\ndata:%s\n",fd,sock.wc,sock.rc,data);
    if(sock.rc>0){ // no more writes
      LOG("sock.rc>0\n");
      return 0; 
    }
    sock.rc++;

    if(sock.on_sock_data_cb){
      return sock.on_sock_data_cb(sock,data,nread);
    }

    return 0; 
  });

  net.on_write([&](int fd)->int{ 
    sock_t& sock=get_sock(fd);
    DBG("on_write fd=%d: wc:%d,rc:%d\n",fd,sock.wc,sock.rc);
    //DBG("on_write fd=%d: wc:%d,rc:%d,t:%d\n",fd,sock.wc,sock.rc,sock.t);
    if(sock.t==0){
     if(!(sock.wc>0 || sock.rc>0)){ // skip first write 
      sock.wc++;       
      return 0; // no write schedule
     }
    }
    if(sock.on_sock_write_cb){
      return sock.on_sock_write_cb(sock);
    }

    return 0; // no more write schedule 
  });


} // init
}; // nets_t


 int connect(const char* host,int port,on_sock_connect_fn_t cb){
   int fd=smoke::net_connect_client(host,port);
   sock_t& sock=get_sock(fd);
   sock.clear();sock.fd=fd;sock.t=1;
   cb(sock);
   return fd;   
}


} // sock

} // smoke

#endif