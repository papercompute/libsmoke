#include "smoke_config.h"

#include "smoke.h"

#include <fstream>
#include <atomic>

#define HTTP404 "HTTP/1.1 404 Not Found" CRLF "Connection: close" CRLF CRLF

std::atomic<int> in_s;
std::atomic<int> out_s;
std::atomic<int> err_s;

namespace srv{
// fd connection data
struct fd_t
{
 int wc; // write counter 
 int rc; // read counter
 fd_t():wc(0),rc(0){};
 void clear(){wc=rc=0;};
};

std::unordered_map<int,fd_t> fmap;  
fd_t fdd[FDD_CACHE_RANGE];

fd_t& get_fdd(int fd){
 if(fd>=0 && fd<FDD_CACHE_RANGE){
  // get from faster array
  return fdd[fd];
 }
 // get from std::unordered_map
 return fmap[fd];
};

}; // srv



smoke::net_t net; 

int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  in_s=out_s=err_s=0;

  int port = atoi(argv[1]);


 
  net.on_connect([&](int fd){
    in_s++;
    srv::fd_t& fdd=srv::get_fdd(fd);
    fdd.clear();
    DBG("on_connect %d\n",fd);
  });

  net.on_close([](int fd,int err){
    err_s++;
    DBG("on_close %d\n",fd);
  });

  net.on_data([&](int fd,const char* data,int nread)->int{
    srv::fd_t& fdd=srv::get_fdd(fd);
    if(fdd.rc>0){ // no more writes
      LOG("fdd.rc>0\n");
      return 0; 
    }
    fdd.rc++;

    if ( nread<12 || strncmp(data,"GET / HTTP",10) != 0){
      int r=write(fd,HTTP404,sizeof(HTTP404)-1);
      if(r<=0){LOG("write header error, %d\n",errno);}
      close(fd);
      out_s++;
      return 0;
    }

    return 1; 
  });

  net.on_write([&](int fd)->int{ 
    srv::fd_t& fdd=srv::get_fdd(fd);
    if(!(fdd.wc>0 || fdd.rc>0)){ // skip first write 
      fdd.wc++;       
      return 0; // no write schedule
    }
    DBG("on_write %d:%d,%d\n",fd,fdd.wc,fdd.rc);

    std::ostringstream os_h,os_b;      
            
    os_b<<"<!doctype html>\n<html><head><title>smoke test page</title></head>\n<body>" \
            "<h1 style='color:red'>smoke test page!!!</h1>\n" \
            "connections: opened("<<in_s<<
            "): closed("<<out_s<<
            "): current("<<in_s-out_s-err_s<<
            ") errd("<<err_s<<")\n" \
            "</body></html>\n";

    auto str_b=os_b.str();

    os_h<<"HTTP/1.1 200 OK\r\n" \
          "Content-Type: text/html\r\n" \
          "Content-Length: "<<str_b.length()<<"\r\n" \
          "Connection: close\r\n" \
          "\r\n" << str_b;

    auto str_h=os_h.str();
    int r=write(fd,str_h.c_str(),str_h.length());
    if(r<=0){LOGFL("write error, %d\n",errno);}
    close(fd);  out_s++;
    return 0; // no write schedule 
  });


  smoke_net_run(&net,port);  

  return EXIT_SUCCESS;  
}
