#include "smoke_config.h"

#include "smoke.h"

#include <fstream>
#include <atomic>

#define HTTP404 "HTTP/1.1 404 Not Found" CRLF "Connection: close" CRLF CRLF


std::atomic<int> in_s;
std::atomic<int> out_s;
std::atomic<int> err_s;

int port;
smoke::net_t net; 


namespace srv{

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

on_sock_connect_fn_t on_sock_connect_cb;
//void on_connect(on_sock_connect_fn_t cb){on_sock_connect_cb=cb;}


int createServer(const char* host,int port,on_sock_connect_fn_t cb){
  int fd=smoke::net_create_server(host,port);
  on_sock_connect_cb = cb;
  return fd;   
}  

 int connect(const char* host,int port,on_sock_connect_fn_t cb){
   int fd=smoke::net_connect_client(host,port);
   srv::sock_t& sock=srv::get_sock(fd);
   sock.clear();sock.fd=fd;sock.t=1;
   if(cb){
    cb(sock);
   }
   return fd;   
}

void init(){

  net.on_connect([&](int fd){
    DBG("on_connect fd=%d\n",fd);
    in_s++;
    srv::sock_t& sock=srv::get_sock(fd);
    sock.clear();sock.fd=fd;
    srv::on_sock_connect_cb(sock);
  });

  net.on_close([](int fd,int err){
    DBG("on_close fd=%d\n",fd);
    srv::sock_t& sock=srv::get_sock(fd);
    if(sock.on_sock_end_cb){
      sock.on_sock_end_cb(sock);
    }
    err_s++;
  });

  net.on_data([&](int fd,const char* data,int nread)->int{
    srv::sock_t& sock=srv::get_sock(fd);
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
    srv::sock_t& sock=srv::get_sock(fd);
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

    close(fd);  out_s++;
    return 0; // no more write schedule 
  });


}

}; // srv




int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  in_s=out_s=err_s=0;

  port = atoi(argv[1]);


  srv::init(); 


   srv::createServer("127.0.0.1",port,[](srv::sock_t& sock){
    DBG("srv::on_connect fd=%d\n",sock.fd);
 
   sock.on_data([](srv::sock_t& sock,const char* data, int nread){
    DBG("sock.on_data fd=%d\n",sock.fd);
    return 1;
   });

   sock.on_write([](srv::sock_t& sock){
    DBG("sock.on_write fd=%d,wc:%d\n",sock.fd,sock.wc);
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

   sock.on_end([](srv::sock_t& sock){
    DBG("sock.on_end fd=%d\n",sock.fd);
   });

  });


  smoke::net_start(&net);


   srv::connect("127.0.0.1",port,[](srv::sock_t& sock){
    DBG("CLIENT !!! srv::on_connect fd=%d\n",sock.fd);
 
   sock.on_data([](srv::sock_t& sock,const char* data, int nread)->int{
    DBG("CLIENT !!!  sock.on_data fd=%d\n",sock.fd);
    sock.end();
    return 0;
   });

   sock.on_end([](srv::sock_t& sock){
    DBG("CLIENT !!! sock.on_end fd=%d\n",sock.fd);
   });

   sock.on_write([](srv::sock_t& sock)->int{
    DBG("CLIENT !!! sock.on_write fd=%d,wc=%d\n",sock.fd, sock.wc);
    write(sock.fd,"Hi from client",14);
    return 0;
   });


  });


//  smoke::net_run(&net,"127.0.0.1",port);  
  smoke::net_loop();
  return EXIT_SUCCESS;  
}
