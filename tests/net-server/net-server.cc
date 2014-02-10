#include "smoke_config.h"

#include "smoke.h"

#include <fstream>
#include <atomic>

smoke::net_t net;

std::atomic<int> in_s;
std::atomic<int> out_s;
std::atomic<int> err_s;

void serve(int port){

  in_s=out_s=err_s=0;

  net.on_connect([](int fd){
    in_s++;
  });

  net.on_close([](int fd,int err){
    err_s++;
  });

  net.on_data([](int fd,const char* data,int nread)->int{
 
  //    LOGF("on_data[%d]:\n%s",nread,data);

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
      ASSERT(r>0);
      close(fd);
      out_s++;
      return 0; 
    });


smoke_net_run(&net,port);

}


int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }
  int port = atoi(argv[1]);

  serve(port);
    
  return EXIT_SUCCESS;  
}
