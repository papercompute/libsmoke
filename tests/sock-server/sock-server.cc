#include "smoke_config.h"

#include "smoke_engine1.h"

#include "smoke_sock.h"

 #define HTTP_RESPONSE \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: text/html\r\n" \
  "Content-Length: 5\r\n" \
  "Connection: close\r\n" \
  "\r\n" \
  "hello"

 #define HTTP_REQUEST \
  "GET / HTTP/1.1\r\n" \
  "Accept: */*\r\n" \
  "\r\n"


typedef smoke::sock::sock_t sock_t;
typedef smoke::sock::nets_t nets_t;

nets_t nets;

void serve(int port){
/*
nets.createServer("127.0.0.1",port,[](sock_t& s){
 DBG("s.connect\n");
 s.on_data([](sock_t& s,const char* data,int len)->int{
  DBG("s.on_data\n%s\n",data);
  s.write(HTTP_RESPONSE,sizeof(HTTP_RESPONSE)-1);  
  s.end();
  return 0;
 });
 s.on_end([](sock_t& s){
  DBG("s.on_end\n");
 });
});
*/
nets.run();

nets.connect("127.0.0.1",port,[](sock_t& s){
 DBG("c.connect<<\n");
 s.write(HTTP_REQUEST,sizeof(HTTP_REQUEST)-1);

 s.on_data([](sock_t& s,const char* data,int len)->int{
  DBG("c.on_data\n%s\n",data);
  s.end();
  return 0;
 });
 s.on_end([](sock_t& s){
  DBG("c.on_end\n");
 });

 DBG("c.connect>>\n");

});

nets.loop();
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
