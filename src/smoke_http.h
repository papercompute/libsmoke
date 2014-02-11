/* 

smoke_http.h http module 

*/

#ifndef _SMOKE_HTTP_H_
#define _SMOKE_HTTP_H_

#include "smoke_config.h"

#include "smoke_data.h"
#include "smoke_rough_parse.h"

#include <fstream>
#include <atomic>

namespace smoke {

namespace http {

std::atomic<int> in_s;
std::atomic<int> out_s;
std::atomic<int> err_s;

typedef data::fd_t fd_t;
typedef data::fd_http_t fd_http_t;

std::unordered_map<int,fd_http_t> fmap;  
fd_http_t fdd[FDD_CACHE_RANGE];

fd_t& get_fdd(int fd){
 if(fd>=0 && fd<FDD_CACHE_RANGE){
  // get from faster array
  return fdd[fd];
 }
 // get from std::unordered_map
 return fmap[fd];
};


typedef std::function<int(int fd)> on_http_get_fn_t;
typedef std::function<int(int fd)> on_http_post_fn_t;

struct http_t
{

 smoke::net_t net;

 std::unordered_map<std::string,on_http_get_fn_t> get_map;
 std::unordered_map<std::string,on_http_post_fn_t> post_map;

 void get(std::string path, on_http_get_fn_t cb){get_map[path]=cb;}
 void post(std::string path, on_http_get_fn_t cb){get_map[path]=cb;}
 
 http_t(){
  init();
 };

 void init()
 {

  net.on_connect([&](int fd){
    in_s++;
    fd_t& fdd=get_fdd(fd);
    fdd.clear();
    DBG("on_connect %d\n",fd);
  });

  net.on_close([](int fd,int err){
    err_s++;
    DBG("on_close %d\n",fd);
  });

  net.on_data([&](int fd,const char* data,int nread)->int{
    fd_t& fdd=get_fdd(fd);

    if(fdd.rc>0){ // no more reads
      LOG("fdd.rc>0\n");
      goto __err; 
    }
    //data[nread]=0;
    DBG("request:\n%s\n",data);

    fdd.rc++;
    int err,r;
    r=parse::rough_parse_http_request(data,nread,(fd_http_t&)fdd,err);
    DBG("rough_parse_http_request: %d,%d\n",r,err);
    if(err)goto __err;

    if(write(fd,HTTPOK,sizeof(HTTPOK)-1)<=0){LOG("write ok error, %d\n",errno);}
    close(fd); out_s++;
    return 0;

__err:
    if(write(fd,HTTP404,sizeof(HTTP404)-1)<=0){LOG("write 404 error, %d\n",errno);}
    close(fd); err_s++;
    return 0;
    
  });


  } // init

  void run(const char* host,int port)
  {
    smoke_net_run(&net,host,port);
  } // run

}; // http_t

}; // namespace http

}; // namespace smoke

#endif