#include "smoke.h"

#include <fstream>
#include <atomic>


std::atomic<int> in_s;
std::atomic<int> out_s;
std::atomic<int> err_s;

struct fd_t
{
 int wc;
 int rc;
};


int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  in_s=0;
  out_s=0;
  err_s=0;

  int port = atoi(argv[1]);


  smoke::net_t net;
  std::unordered_map<int,fd_t> fmap;  

 
  net.on_connect([&](int fd){
    in_s++;
    fmap[fd].wc=0;fmap[fd].rc=0;
   
  });

  net.on_close([](int fd,int err){
    err_s++;
  });

  net.on_data([&](int fd,const char* data,int nread)->int{
       fmap[fd].rc++;

       if( nread<17){
        close(fd);LOG("WTH!!!");
        out_s++;
        return 0;
       }

       if (strncmp(data,"GET / HTTP",10) != 0){
        #define HTTP404 "HTTP/1.1 404 Not Found" CRLF "Connection: close" CRLF CRLF
        const char* s=HTTP404;
        int r=write(fd,s,sizeof(HTTP404)-1);
        if(r<=0){LOG("write header error, %d\n",errno);}
        close(fd);
        out_s++;
        return 0;
       }
      

      return 1; 
  });

 net.on_write([&](int fd)->int{ 

  if(fmap[fd].wc>0 || fmap[fd].rc>0){ // skip first IO 

//      LOG("on_write:\n");

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
      if(r<=0){LOG("write error, %d\n",errno);}
      close(fd);  out_s++;
      return 0; // no write schedule 
      } // if first IO
      fmap[fd].wc++;       
      return 0; // no write schedule
    });


  smoke_net_run(&net,port);  

  return EXIT_SUCCESS;  
}
