#include "smoke_config.h"

#include "smoke.h"

#include <fstream>
#include <atomic>

#define HTTP404 "HTTP/1.1 404 Not Found" CRLF "Connection: close" CRLF CRLF


std::atomic<int> in_s;
std::atomic<int> out_s;

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

  in_s=out_s=0;

  int port = atoi(argv[1]);


  net.on_connect([&](int fd){
    srv::fd_t& fdd=srv::get_fdd(fd);
    DBG("on_connect %d\n",fd);
    fdd.clear();
    in_s++;
  });

  net.on_close([](int fd,int err){
    srv::fd_t& fdd=srv::get_fdd(fd);
    DBG("on_close %d\n",fd);
    out_s++;
  });


  net.on_read([](int fd)->int{
    srv::fd_t& fdd=srv::get_fdd(fd);
    if(fdd.rc>0){close(fd);out_s++;return 0;}
    fdd.rc++;

    int r;

    char rbuf[1024];
    r=read(fd,rbuf,1024);
    if(r<=0){close(fd);out_s++;return 0;}

    std::ostringstream os_h,os_b;
    os_b<<"<!doctype html>\n<html><head><title>smoke test page</title></head>\n<body>" \
            "<h1 style='color:red'>smoke test page!!!</h1>\n" \
            "connections: opened("<<in_s<<"): closed("<<out_s<<
            "): current("<<in_s-out_s<<")"<<"</body></html>\n";

    auto str_b=os_b.str();
    os_h<<"HTTP/1.1 200 OK\r\n" \
          "Content-Type: text/html\r\n" \
          "Content-Length: "<<str_b.length()<<"\r\n" \
          "Connection: close\r\n" \
          "\r\n" << str_b;

    auto str_h=os_h.str();
    r=write(fd,str_h.c_str(),str_h.length());
    if(r<=0){close(fd);out_s++;return 0;}

    close(fd);  
    out_s++;
    return 0; 
  });

  smoke::net_run(&net,"127.0.0.1",port);  

  return EXIT_SUCCESS;  
}
