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
  "User-Agent: smoke\r\n" \
  "Host: 173.194.32.162\r\n" \
  "Accept: */*\r\n" \
  "\r\n"


typedef smoke::sock::sock_t sock_t;
typedef smoke::sock::nets_t nets_t;

nets_t nets;
int port;

void serve(){

nets.run();

//nets.connect("173.194.32.162",80,[](sock_t& s){
nets.connect("127.0.0.1",port,[](sock_t& s){
 DBG("c.connect<<\n");
 if(write(s.fd,HTTP_REQUEST,sizeof(HTTP_REQUEST)-1)<=0){
  LOG("write error\n");
 }

 s.on_data([](sock_t& s,const char* data,int len)->int{
  DBG("c.on_data\n%s\n",data);
//  s.end();
  s.rc++;
  if(s.rc>3){
   s.end();
   return 0;
  }
  return 1;
 });

 s.on_write([](sock_t& s){
  LOG("c.on_write\n");
  write(s.fd,HTTP_REQUEST,sizeof(HTTP_REQUEST)-1);
  return 0;
 });

 s.on_end([](sock_t& s){
  DBG("c.on_end\n");
 });

 DBG("c.connect>>\n");

});

DBG("to loop\n");

nets.loop();
}


int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  port = atoi(argv[1]);

  serve();

  return EXIT_SUCCESS;  
}
