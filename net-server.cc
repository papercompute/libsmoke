#include "smoke.h"

#include <fstream>


typedef smoke::socket_t sock_t;

int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  int port = atoi(argv[1]);


  smoke::net_t net;
 

  net.createServer([](sock_t& sock){

    sock.on_read([](sock_t& sock){
     char buf[R_BUF_MAX];
     int nread=0;
     int r=sock.read(buf,R_BUF_MAX,nread);
     if(nread>0){
      buf[nread]=0;
      sock.make_writable();
     }
     else{
      sock.so_close();
     }
    });

    sock.on_write([](sock_t& sock){
      int nwritten=0;

      std::ostringstream os_h,os_b;      
            
      os_b<<"<!doctype html>\n<html><head><title>smoke test page</title></head>\n<body>" \
            "<h1 style='color:red'>smoke test page!!!</h1>\n"
            <<"connections: opened("<<sock.net->opened_socks<<"): closed("<<sock.net->closed_socks<<")\n"
            <<"</body></html>";

      auto str_b=os_b.str();

      os_h<<"HTTP/1.1 200 OK\r\n"
          <<"Content-Type: text/html\r\n"
          <<"Content-Length: "<<str_b.length()<<"\r\n"
          <<"Connection: close\r\n"
          <<"\r\n"<<str_b;

      auto str_a=os_h.str();

     sock.write(str_a.c_str(),str_a.length(),nwritten);
     sock.so_close();
    });

    sock.on_close([](sock_t& sock){ 
    });
  });

  


  net.listen(port);  
  

  return EXIT_SUCCESS;  
}
