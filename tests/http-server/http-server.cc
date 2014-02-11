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
  
  smoke::http::http_t http1; 
   
  http1.run("127.0.0.1",port);

  return EXIT_SUCCESS;  
}
