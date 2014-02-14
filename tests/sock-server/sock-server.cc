#include "smoke_config.h"

#include "smoke.h"

#include "smoke_sock.h"

typedef smoke::sock::sock_t sock_t;
typedef smoke::sock::nets_t nets_t;

nets_t nets;

void serve(int port){

nets.createServer([](sock_t& s){
 DBG("nets.createServer\n");
 s.on_data([](sock_t& s,const char* data,int len)->int{
  DBG("s.on_data\n");
  s.write("hello there!\n");
  s.end();
  return 0;
 });
 s.on_end([](sock_t& s){
  DBG("s.on_end\n");
 });
});

smoke_net_run(&nets,"127.0.0.1",port);

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
