#include "smoke_config.h"

#include "smoke_engine1.h"

#include <fstream>
#include <atomic>

#define HTTPOK "HTTP/1.1 200 OK" CRLF "Content-Type: text/html" CRLF "Content-Length: 2" CRLF "Connection: close" CRLF CRLF "ok"

smoke::net_t net;

std::atomic<int> in_s;
std::atomic<int> out_s;
std::atomic<int> err_s;

void serve(int port){

  in_s=out_s=err_s=0;

//  smoke::net_init(&net);

  net.on_connect([](int fd){
    in_s++;
  });

  net.on_close([](int fd,int err){
    err_s++;
  });

  net.on_data([](int fd,const char* data,int nread)->int{
    int r=write(fd,HTTPOK,sizeof(HTTPOK)-1);
    if(r!=sizeof(HTTPOK)-1){err_s++;}
    else out_s++;
    close(fd);      
    return 0; 
  });


  smoke::net_run(&net,"127.0.0.1",port);

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
