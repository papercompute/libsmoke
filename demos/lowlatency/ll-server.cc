#include "smoke_config.h"

#include "smoke.h"

#include <fstream>
#include <atomic>

#include <stdio.h>

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

  net.on_connect([](int fd){
    srv::fd_t& fdd=srv::get_fdd(fd);
    DBG("on_connect %d\n",fd);
    fdd.clear();
    in_s++;
  });

  net.on_close([](int fd,int err){
//    srv::fd_t& fdd=srv::get_fdd(fd);
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

struct iovec iov[4];

//struct iovec {
//    void  *iov_base;    // Starting address
//    size_t iov_len;     // Number of bytes to transfer
//};
// ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

#undef USE_STD
#define USE_STD
#ifdef USE_STD

std::string str_b=
    "{"
    "\n\"opened\": \""+std::to_string(in_s)+"\","
    "\n\"closed\": \""+std::to_string(out_s)+"\","
    "\n\"current\": \""+std::to_string(in_s-out_s)+"\""
    "\n}\n";

std::string str_h=
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: application/json; charset=utf-8\r\n"
    "X-Powered-By: smoke\r\n"
    "Content-Length: "+std::to_string(str_b.length())+"\r\n"
    "Connection: close\r\n"
    "\r\n";

iov[0].iov_base = (void*)str_h.c_str(); iov[0].iov_len = str_h.length();
iov[1].iov_base = (void*)str_b.c_str(); iov[1].iov_len = str_b.length();    

// http://linux.die.net/man/2/writev
r = writev(fd, iov, 2);


#else



char wb0[128]; int l0; char wb1[128]; int l1;
int in_=in_s; int out_=out_s;

l1=sprintf(wb1,"body:\nopened:%d,\nclosed:%d,\ncurrent%d\n",in_,out_,in_-out_);
l0=sprintf(wb0,"Content-Length: %d\r\nConnection: close\r\n\r\n",l1);

// http://en.wikipedia.org/wiki/List_of_HTTP_header_fields

#define HTTP200 "HTTP/1.1 200 OK\r\n"
iov[0].iov_base = (void*)HTTP200; iov[0].iov_len = sizeof(HTTP200)-1;
#define HTTP_XPB "X-Powered-By: smokengine\r\n"
#define Ix 1
iov[Ix].iov_base = (void*)HTTP_XPB;iov[Ix].iov_len = sizeof(HTTP_XPB)-1;
//#define Ix 2
iov[Ix+1].iov_base = (void*)wb0;  iov[Ix+1].iov_len = l0;
iov[Ix+2].iov_base = (void*)wb1;  iov[Ix+2].iov_len = l1;


// http://linux.die.net/man/2/writev
r = writev(fd, iov, 4);

#endif


    close(fd);  
    out_s++;
    return 0; 
  });

  smoke::net_run(&net,"127.0.0.1",port);  

  return EXIT_SUCCESS;  
}
