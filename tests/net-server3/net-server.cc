#include "smoke.h"

#include <fstream>
#include <atomic>


std::atomic<int> in_s;
std::atomic<int> out_s;
std::atomic<int> err_s;

struct fd_t
{
 int wc;
 int rc;
};


int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  in_s=0;
  out_s=0;
  err_s=0;

  int port = atoi(argv[1]);


  smoke::net_t net;
  std::unordered_map<int,fd_t> fmap;  

 
  net.on_connect([&](int fd){
    in_s++;
    fmap[fd].wc=0;fmap[fd].rc=0;
  });

  net.on_close([](int fd,int err){
    err_s++;
  });

  net.on_data([&](int fd,const char* data,int nread)->int{

       fmap[fd].rc++;

       //LOG("on_data(%d):\n%s\n",nread,data);

       if( nread<16){
        LOG("wrong data");
        close(fd); out_s++;
        return 0;
       }

       if (strncmp(data,"GET / HTTP",10) != 0){
        LOG("no GET:%s",data);
        #define HTTP404 "HTTP/1.1 404 Not Found" CRLF "Connection: close" CRLF CRLF
        const char* s=HTTP404;
        int r=write(fd,s,sizeof(HTTP404)-1);
        if(r<=0){LOG("write HTTP404 header error, %d\n",errno);}
        close(fd); out_s++;
        return 0;
       }

      #define HTTP_CHUNKED "HTTP/1.1 200 OK" CRLF \
          "Content-Type: text/html" CRLF \
          "Connection: keep-alive" CRLF \
          "Transfer-Encoding: chunked" CRLF CRLF

      const char* s=HTTP_CHUNKED;
      int r=write(fd,s,sizeof(HTTP_CHUNKED)-1);
      if(r<=0){LOG("write HTTP_CHUNKED header error, %d\n",errno);}
 
      return 1; // shedule write 
  });




 net.on_write([&](int fd)->int{ 

  if(fmap[fd].wc>0 || fmap[fd].rc>0){ // skip first IO 

 //     LOG("on_write:\n");
 //  <HEX><CRLF><chunk><CRLF> ... 0<CRLF><CRLF>
      std::ostringstream os_b;
      os_b<<"<!doctype html>\n<html><head><title>smoke test page</title></head>\n<body>" \
            "<h1 style='color:red'>smoke test page!!!</h1>\n" \
            "connections: opened("<<in_s<<
            "): closed("<<out_s<<
            "): current("<<in_s-out_s-err_s<<
            ") errd("<<err_s<<")\n" \
            "</body></html>\n";

      auto str_b=os_b.str();

      std::ostringstream os_h;
      os_h<< std::hex << str_b.length()<<CRLF<<str_b<<CRLF<<"0"<<CRLF<<CRLF;
      auto str_h=os_h.str();
      
      int r=write(fd,str_h.c_str(),str_h.length());
      if(r<=0){ LOG("write chunk error, %d\n",errno); }


//      int r = write(fd,"5" CRLF "hello" CRLF ,3+5+2);
//      if(r<=0){ LOG("write error, %d\n",errno); }
      
//      r = write(fd,"0" CRLF CRLF ,5);
//      if(r<=0){ LOG("write error, %d\n",errno); }
      close(fd);  out_s++;
      return 0; // no write schedule 

  } // if first IO

  fmap[fd].wc++;       
  return 0; // no write schedule

 });


  smoke_net_run(&net,port);  

  return EXIT_SUCCESS;  
}
