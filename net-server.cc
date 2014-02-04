#include "smoke.h"

#include <fstream>
#include <atomic>


typedef smoke::socket_t sock_t;
std::atomic<int> in_s;
std::atomic<int> out_s;

int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  in_s=0;
  out_s=0;

  int port = atoi(argv[1]);


  smoke::net_t net;

 

  net.createServer([](sock_t& sock){
//      LOG("on_con\n");
    in_s++;
    sock.on_read([](sock_t& sock){
//      LOG("on_read\n");

     char buf[R_BUF_MAX];
     int nread=0;
     int r=sock.read(buf,R_BUF_MAX,nread);
     if(nread>0){
      buf[nread]=0;
//      sock.make_writable();
     }
     else{
      sock.so_close();
      LOG("ERROR, nread=%d,r=%d\n",nread,r);
      return;
     }
//    });

//    sock.on_write([](sock_t& sock){
      int nwritten=0;

      std::ostringstream os_h,os_b;      
            
      os_b<<"<!doctype html>\n<html><head><title>smoke test page</title></head>\n<body>" \
            "<h1 style='color:red'>smoke test page!!!</h1>\n"
            <<"connections: opened("<<in_s<<"): closed("<<out_s<<")\n"
            <<"</body></html>";

      auto str_b=os_b.str();

      os_h<<"HTTP/1.1 200 OK\r\n"
          <<"Content-Type: text/html\r\n"
          <<"Content-Length: "<<str_b.length()<<"\r\n"
          <<"Connection: close\r\n"
          <<"\r\n";

      auto str_h=os_h.str();

     sock.write(str_h.c_str(),str_h.length(),nwritten);
     sock.write(str_b.c_str(),str_b.length(),nwritten);
     sock.so_close();
    });

    sock.on_write([](sock_t& sock){
//      LOG("on_write\n");
    }); 
    sock.on_close([](sock_t& sock){ 
//      LOG("on_close\n");
     out_s++;
    });
  });

  


  net.listen(port);  
  

  return EXIT_SUCCESS;  
}
