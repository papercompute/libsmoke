#include "smoke.h"

#include <fstream>

int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  int port = atoi(argv[1]);


  smoke::net_t net;

 
  net.on_connect([](int fd){
     // do nothing
  });

  net.on_data([](int fd,const char* data,int nread)->int{
   // do something with data 
   int r=write(fd,data,nread);
   if(r<=0){ LOG("write error, %d\n",errno); }
   return 0;
  });

  smoke_net_run(&net,port);  

  return EXIT_SUCCESS;  
}
