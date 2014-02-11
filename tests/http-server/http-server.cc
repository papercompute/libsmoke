#include "smoke_config.h"

#include "smoke.h"

#include "smoke_http.h"


int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  int port = atoi(argv[1]);
  
  smoke::http::http_t app;

  app.get("/counter",[&](int fd)->int{
   DBG("get /counter %d\n",fd);
   return 0;
  }); 

  app.get("/test/api",[&](int fd)->int{
   DBG("get /test/api %d\n",fd);
   return 0;
  }); 

  app.post("/data",[&](int fd)->int{
   DBG("post /data %d\n",fd);
   return 0;
  }); 

   
  app.run("127.0.0.1",port);

  return EXIT_SUCCESS;  
}
