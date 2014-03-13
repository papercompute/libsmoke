#include "smoke_config.h"
#include "smoke.h"
#include "smoke_rough_parse.h"

#include <aio.h>


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

 file_t():fd(0),sz(0),hdr(0),hsz(0){

 }

 int init(const char* filename){
  struct stat stat_buf;
  int f = open(filename, O_RDONLY);
  if (f<=0) return -1;
  fd=f;
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
  return 1;
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

typedef srv::fd_t fd_t;

std::unordered_map<std::string,file_t> file_map;  


void serve()
{

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
      fd_t& fdd=srv::get_fdd(fd);
      DBG("on_read fd:%d, rc:%d, wc:%d\n",fd,fdd.rc,fdd.wc);
     
     if(fdd.rc>0){
       DBG("fdd.rc>0,rc:%d\n",fdd.rc);
       close(fd);err_s++; 
       return 0;
     }

     fdd.rc++;

     // 'GET /' 'filename '

     char rbuf[1024];
     int nr;

     nr=read(fd, rbuf, 1024);

      if( nr<6 || !STRCMP5(rbuf,"GET /") ){
        DBG("wrong http request\n");
        write(fd,HTTP404,sizeof(HTTP404)-1);
        close(fd);
        err_s++;
        return 0;
      }

      char* url=(char*)(rbuf+5);
      
      int r=smoke::parse::rough_parse_http_url_path(url,nr-5);
      
      if(r<0){
        DBG("wrong http url path\n");
        write(fd,HTTP404,sizeof(HTTP404)-1);
        close(fd);
        err_s++;
        return 0;
      }

      if(r == 0){
        url=(char*)"index.html";
        r=10;
      }else{
        url[r]=0;
      }

      file_t& f1=file_map[url];

      if(f1.fd==0){
        char url_buf[128];
        sprintf(url_buf,"%s/%s",FOLDER,url);

       if(f1.init(url_buf)<=0){
        DBG("f1.init(url_buf)<=0\n");
        if(write(fd,HTTP404,sizeof(HTTP404)-1)<=0){LOG("write 404 error, %d\n",errno);}
        close(fd);err_s++;
        return 0;
        }

      }
      else{
        DBG("file_map %s\n",url);
      }

      if(f1.fd<0){
       DBG("f1.fd<0\n");
        if(write(fd,HTTP404,sizeof(HTTP404)-1)<=0){LOG("write 404 error, %d\n",errno);}
        close(fd);err_s++;
        return 0;
      }

      ASSERT(f1.hdr);
      ASSERT(f1.hsz>0);

     int wn;
      wn=write(fd,f1.hdr,f1.hsz);
      if( wn<=0 ){
       printf("write error file1.hsz=%d,writen=%d\n",f1.hsz,wn); 
       close(fd);err_s++; 
       return 0;
      }

      off_t off = 0;
      wn = sendfile (fd, f1.fd, &off, f1.sz);  
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
