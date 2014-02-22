#include "smoke_config.h"

#include "smoke.h"

#include "smoke_sock1.h"


#include <fstream>
#include <atomic>

#define HTTP404 "HTTP/1.1 404 Not Found" CRLF "Connection: close" CRLF CRLF


std::atomic<int> in_s;
std::atomic<int> out_s;
std::atomic<int> err_s;

int port;


typedef smoke::sock::sock_t sock_t;
typedef smoke::sock::nets_t nets_t;

int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  in_s=out_s=err_s=0;

  port = atoi(argv[1]);


  nets_t net1; 


   net1.createServer("127.0.0.1",port,[](sock_t& sock){
    DBG("srv::on_connect fd=%d\n",sock.fd);
 
   sock.on_data([](sock_t& sock,const char* data, int nread){
    DBG("sock.on_data fd=%d\n",sock.fd);
    return 1;
   });

   sock.on_write([](sock_t& sock){
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

   sock.on_end([](sock_t& sock){
    DBG("sock.on_end fd=%d\n",sock.fd);
   });

  });


  smoke::net_start(&net1.net);



//  smoke::net_run(&net,"127.0.0.1",port);  
  smoke::net_loop();
  return EXIT_SUCCESS;  
}
