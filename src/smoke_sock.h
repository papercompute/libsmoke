/* 

smoke_sock.h data module 

*/

#ifndef _SMOKE_SOCK_H_
#define _SMOKE_SOCK_H_

#define SOCK_CACHE_RANGE 1024


namespace smoke {

namespace sock {

struct sock_t;

typedef std::function<void(sock_t& sock)> on_sock_connect_fn_t; 
typedef std::function<void(sock_t& sock)> on_sock_end_fn_t; 
typedef std::function<int(sock_t& sock, const char *s,int l)> on_sock_data_fn_t; 
typedef std::function<int(sock_t& sock)> on_sock_write_fn_t; 

// sock connection data
struct sock_t
{
 int fd;  
 int wc; // write counter 
 int rc; // read counter
 on_sock_data_fn_t on_sock_data_cb;
 on_sock_end_fn_t on_sock_end_cb;
 on_sock_write_fn_t on_sock_write_cb;
 sock_t():wc(0),rc(0){};
 void clear(){wc=rc=0;};
 void on_write(on_sock_write_fn_t cb){on_sock_write_cb=cb;}
 void on_data(on_sock_data_fn_t cb){on_sock_data_cb=cb;}
 void on_end(on_sock_end_fn_t cb){on_sock_end_cb=cb;}
 void end(){
  if(on_sock_end_cb){
    on_sock_end_cb(*this);
    close(fd);
  }
 };
 int write(const char* s,int l){
  return ::write(fd,s,l);
 }
 int write(const char* s){
  return ::write(fd,s,strlen(s));
 }
};

struct nets_t : net_t
{

 std::unordered_map<int,sock_t> fmap;  
 sock_t fdd[SOCK_CACHE_RANGE];
 sock_t& get_sock(int fd){
  if(fd>=0 && fd<SOCK_CACHE_RANGE){
  // get from faster array
  return fdd[fd];
  }
 // get from std::unordered_map
 return fmap[fd];
 };

 on_sock_connect_fn_t on_sock_connect_cb;


 nets_t()
 {
   init();
 }

 void run()
 {
  net_start(this);
 }

 void loop()
 {
  net_loop();
 }

 void init()
 {

  on_connect([this](int fd){
   DBG("on_connect %d\n",fd);
   sock_t& sock=this->get_sock(fd);
   sock.clear();sock.fd=fd;
   if(on_sock_connect_cb){
    on_sock_connect_cb(sock);
   }
  });

  on_close([this](int fd,int err){
   DBG("on_close %d\n",fd);
   sock_t& sock=this->get_sock(fd);
   if(sock.on_sock_end_cb){
    sock.on_sock_end_cb(sock);
   }
  });

  on_data([this](int fd,const char* data,int nread)->int{
   DBG("on_data %d\n",fd);
   sock_t& sock=this->get_sock(fd);
   if(sock.on_sock_data_cb){
    return sock.on_sock_data_cb(sock,data,nread);
   }
   return 0;
  });

  on_write([this](int fd)->int{
   DBG("on_write %d\n",fd);
   sock_t& sock=this->get_sock(fd);
   if(sock.on_sock_write_cb){
    return sock.on_sock_write_cb(sock);
   }
   return 0;
  });


 } // init

 int createServer(const char* host,int port,on_sock_connect_fn_t cb){
   int fd=net_create_server(host,port);
   on_sock_connect_cb = cb;
   return fd;   
 }  

 int connect(const char* host,int port,on_sock_connect_fn_t cb){
   int fd=net_connect_client(host,port);
   sock_t& sock=this->get_sock(fd);
   sock.clear();sock.fd=fd;
   if(cb){
    cb(sock);
   }
   return fd;   
 }  
};



}; // namespace sock

}; // namespace smoke

#endif