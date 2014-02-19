#include "smoke_config.h"

#include "smoke.h"

#include "smoke_http.h"

#define HTTPOK "HTTP/1.1 200 OK" CRLF "Content-Type: text/html" CRLF "Content-Length: 2" CRLF "Connection: close" CRLF CRLF "ok"


typedef smoke::data::http_req_t http_req_t;

int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  int port = atoi(argv[1]);
  
  smoke::http::http_t app;

  app.get("/counter",[&](int fd,http_req_t& req)->int{
   DBG("get /counter %d\n",fd);
   if(write(fd,HTTPOK,sizeof(HTTPOK)-1)<=0){LOG("write error, %d\n",errno);}
   return 0;
  }); 

  app.get("/test/api",[&](int fd,http_req_t& req)->int{
   DBG("get /test/api %d\n",fd);
   if(write(fd,HTTPOK,sizeof(HTTPOK)-1)<=0){LOG("write error, %d\n",errno);}
   return 0;
  }); 

  app.post("/data",[&](int fd,http_req_t& req)->int{
   DBG("post /data %d\n",fd);
   if(write(fd,HTTPOK,sizeof(HTTPOK)-1)<=0){LOG("write error, %d\n",errno);}
   return 0;
  }); 

   
  app.run("127.0.0.1",port);

  return EXIT_SUCCESS;  
}
