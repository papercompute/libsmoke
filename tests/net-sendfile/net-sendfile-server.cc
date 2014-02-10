#include "smoke_config.h"

#include "smoke.h"

#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/sendfile.h>
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
  if (argc != 3) {
    fprintf (stderr, "Usage: %s [port] [file]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  int port = atoi(argv[1]);

  in_s=0;
  out_s=0;
  err_s=0;

  smoke::net_t net;
  std::unordered_map<int,fd_t> fmap;  

  int file_fd;
  const char* filename = argv[2];
  //off_t offset = 0; 
  struct stat stat_buf;
  int file_size;
  
  // ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);
  file_fd = open(filename, O_RDONLY);
  if (file_fd == -1) {
        fprintf(stderr, "unable to open '%s': %s\n", filename, strerror(errno));
        exit(1);
  }
  fstat(file_fd, &stat_buf);
  file_size=stat_buf.st_size;


  net.on_connect([&](int fd){
      in_s++;
//      LOG("on_connect %d\n",fd);
      fmap[fd].wc=0;fmap[fd].rc=0;
  });


  net.on_close([&](int fd,int err){
 //     LOG("on_close %d\n",fd);
      err_s++;
  });

  net.on_data([&](int fd,const char* data,int nread)->int{
     if(fmap[fd].rc>0){
      close(fd);err_s++;
      return 0;
     } 
//     LOG("on_data fd:%d [%d]:\n%s\n",fd,nread,data);
     fmap[fd].rc++;
      
      // GET / HTTP/1.1 

      if( strncmp(data,"GET / HTTP",10) != 0){
        #define HTTP404 "HTTP/1.1 404 Not Found" CRLF "Connection: close" CRLF CRLF
        const char* s=HTTP404;
        int r=write(fd,s,sizeof(HTTP404)-1);
        if(r<=0){LOG("write 404 error, %d\n",errno);}
        close(fd);err_s++;
        return 0;
      }
    return 1; // 
  });


  net.on_write([&](int fd)->int{
     if(!(fmap[fd].wc>0 || fmap[fd].rc>0)){ 
      // skip first IO 
      fmap[fd].wc++;
      return 0;
     }

//     LOG("on_write %d\n",fd);

      std::ostringstream os_h;      

      os_h<<"HTTP/1.1 200 OK\r\n" \
          "Content-Type: text/html\r\n" \
          "Content-Length: "<<file_size<<"\r\n" \
          "Connection: close\r\n" \
          "\r\n";

      auto str_h=os_h.str();
      int r=write(fd,str_h.c_str(),str_h.length());
      if(r<=0){ LOG("write error, %d\n",errno); }

      
      off_t off = 0;
      r = sendfile (fd, file_fd, &off, file_size);  
      if(r<=0){ LOG("sendfile error, %d\n",errno); }

      
//      LOG("close %d\n",fd);
      close(fd); out_s++;
      return 0;
  });


  smoke_net_run(&net,"127.0.0.1",port);  

  return EXIT_SUCCESS;  
}
