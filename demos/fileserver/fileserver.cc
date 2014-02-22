#include "smoke_config.h"
#include "smoke.h"

#include <fstream>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/sendfile.h>
#include <atomic>

#define HTTP404 "HTTP/1.1 404 Not Found" CRLF "Connection: close" CRLF CRLF

#define HTTP_CHUNKED_HEADER_RESPONSE \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: text/html\r\n" \
  "Connection: keep-alive\r\n" \
  "Transfer-Encoding: chunked\r\n" \
  "\r\n"

#define HTTP_HEADER_RESPONSE \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: text/html\r\n" \
  "Connection: close\r\n" \
  "Content-Length: "


std::atomic<int> in_s;
std::atomic<int> out_s;
std::atomic<int> err_s;

struct file_t{
 int fd;
 int sz;
 char* hdr;
 int hsz;

 void init(const char* filename){
  struct stat stat_buf;
  fd = open(filename, O_RDONLY);
  ASSERT(fd>0);
  fstat(fd, &stat_buf);
  sz=stat_buf.st_size;

  std::ostringstream os;
  os<<"HTTP/1.1 200 OK\r\n" \
      "Content-Type: text/html\r\n" \
      "Content-Length: "<<sz<<"\r\n" \
      "Connection: close\r\n" \
      "\r\n";

  auto s=os.str();
  hsz=s.length();
  hdr=new char[hsz+1];
  strncpy(hdr,s.c_str(),hsz);
  DBG("open: hdr=%s\n",hdr);
 }

 void end(){
  close(fd);
  delete[] hdr;
 }

};

namespace srv{
// fd connection data
struct fd_t
{
 int wc; // write counter 
 int rc; // read counter
 int of; // ofset
 fd_t():wc(0),rc(0){};
 void clear(){wc=rc=of=0;};
};

std::unordered_map<int,fd_t> fmap;  
fd_t fdd[FDD_CACHE_RANGE];

fd_t& get_fdd(int fd){
 if(fd>=0 && fd<FDD_CACHE_RANGE){
  // get from faster array
  return fdd[fd];
 }
 // get from std::unordered_map
 return fmap[fd];
};

}; // srv

int serve_port;
smoke::net_t net;
file_t file1;

typedef srv::fd_t fd_t;

void serve()
{

  file1.init("public/index.html");

  net.on_connect([&](int fd){
      DBG("on_connect %d\n",fd);
      fd_t& fdd=srv::get_fdd(fd);
      fdd.clear();
      in_s++;
  });


  net.on_close([&](int fd,int err){
      DBG("on_close %d\n",fd);
      err_s++;
  });

  net.on_read([&](int fd)->int{
      char rbuf[32];
      int nr;

      fd_t& fdd=srv::get_fdd(fd);
      DBG("on_read fd:%d, rc:%d, wc:%d\n",fd,fdd.rc,fdd.wc);
     
     if(fdd.rc>0){
       DBG("fdd.rc>0,rc:%d\n",fdd.rc);
       close(fd);err_s++; 
       return 0;
     }

     fdd.rc++;

     // 'GET /' 'filename '

     nr=read(fd, rbuf, 5 + 10);

      if( nr<6 || !STRCMP5(rbuf,"GET /") ){
        DBG("wrong http request\n");
        if(write(fd,HTTP404,sizeof(HTTP404)-1)<=0){LOG("write 404 error, %d\n",errno);}
        close(fd);err_s++;
        return 0;
      }


     int wn;
      wn=write(fd,file1.hdr,file1.hsz);
      if( wn<=0 ){
       LOG("write error file1.hsz=%d,writen=%d\n",file1.hsz,wn); 
       close(fd);err_s++; 
       return 0;
      }

      off_t off = 0;
      wn = sendfile (fd, file1.fd, &off, file1.sz);  
      if(wn<=0){ LOG("sendfile error, %d\n",errno); }

      close(fd);out_s++; 
      return 0; 
  });

  smoke::net_run(&net,"127.0.0.1",serve_port);  

}


int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  serve_port = atoi(argv[1]);

  in_s=out_s=err_s=0;

  serve();  
  
  return EXIT_SUCCESS;  
}
