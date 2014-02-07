#include "smoke.h"

#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/sendfile.h>

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


  smoke::net_t net;
  std::unordered_map<int,fd_t> fmap;  

  int file_fd;
  const char* filename = argv[2];
  off_t offset = 0; 
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

#define LOG(...)

  net.on_connect([&](int fd){
    try{
      LOG("on_connect fd %d\n",fd);
      fmap[fd].wc=0;fmap[fd].rc=0;
    }
    catch(...){
     LOG("on_connect exception ...\n");
    }   
  });

  net.on_data([&](int fd,const char* data,int nread)->int{
    try{     
     LOG("net.on_data: fd %d, wc %d, rc %d\n%s\n",fd,fmap[fd].wc, fmap[fd].rc,data);
     if(fmap[fd].rc==0){
      fmap[fd].rc++;
      
      // GET / HTTP/1.1 
/*
      if( strncmp(data,"GET / HTTP/1.1",14) != 0){
        #define HTTP404 "HTTP/1.1 404 Not Found" CRLF "Connection: close" CRLF CRLF
        const char* s=HTTP404;
        int r=write(fd,s,sizeof(HTTP404));
        if(r<=0){LOG("write header error, %d\n",errno);}
        close(fd);
        return 0;
      }
*/
      std::ostringstream os_h;
      // THIS FOR DEMO ONLY, USE const char* FOR BETTER PERFORMANCE
      os_h<<"HTTP/1.1 200 OK\r\n"
          <<"Content-Type: text/html\r\n"
          <<"Connection: keep-alive\r\n"
          <<"Transfer-Encoding: chunked\r\n"
          <<"\r\n";
      auto str_h=os_h.str();
      int r=write(fd,str_h.c_str(),str_h.length());
      if(r<=0){LOG("write header error, %d\n",errno);}
     } else{
      LOG("unexpected data, ignore\n");
      return 0;
     }
    } catch(...){
     LOG("on_data exception ...\n");
    }   
    return 1; // 
  });


  net.on_write([&](int fd)->int{
    try{     
     LOG("net.on_write: fd %d, wc %d\n",fd,fmap[fd].wc);
     if(fmap[fd].wc>0 || fmap[fd].rc>0){ // skip first IO 
     // chunk len
      std::ostringstream os_h;
      os_h<< std::hex << file_size<<CRLF;
      auto str_h=os_h.str();
      
      int r=write(fd,str_h.c_str(),str_h.length());
      if(r<=0){ LOG("write header error, %d\n",errno); }

      // write chunk

      offset = 0;
      r = sendfile (fd, file_fd, &offset, file_size);  
      if(r<=0){ LOG("sendfile error, %d\n",errno); }

      // write final 0 
      r = write(fd,CRLF "0" CRLF CRLF,7);
      if(r<=0){ LOG("write 0 error, %d\n",errno); }

     // close fd

      close(fd);
      return 0;
     } // if
     fmap[fd].wc++;
     return 0;
    }
    catch(...){
     LOG("on_write exception ...\n");
    }
    return 0;   
  });


  smoke_net_run(&net,port);  

  return EXIT_SUCCESS;  
}
