#include "smoke_config.h"

#include "smoke_engine1a.h"

#include <aio.h>

#include <list>

#include <fstream>
#include <atomic>

#define HTTP404 "HTTP/1.1 404 Not Found" CRLF "Connection: close" CRLF CRLF


std::atomic<int> in_s;
std::atomic<int> out_s;
std::atomic<int> err_s;
std::atomic<int> idl_s;

#define AIO_RBUF_MAX 4096

#define FD_ST_RD 1
#define FD_ST_WR 2

namespace srv{
// fd connection data
struct fd_t
{
 struct aiocb acb; 
 int wc; // write counter 
 int rc; // read counter
 int st; // state
 int t; // thread id
 fd_t():wc(0),rc(0),st(0){
  bzero( (char *)&acb, sizeof(acb) );
  acb.aio_fildes = 0;
  acb.aio_buf = new char[AIO_RBUF_MAX];
  acb.aio_nbytes = AIO_RBUF_MAX-1;
  acb.aio_offset = 0;
 };
 void clear(){wc=rc=st=0;};
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


//std::list<fd_t&> amap;  

}; // srv


int readIO(int fd, char* buf,int buf_len)
{
  int ret;
  struct aiocb my_aiocb;

  /* Zero out the aiocb structure (recommended) */
  bzero( (char *)&my_aiocb, sizeof(struct aiocb) );

  /* Allocate a data buffer for the aiocb request */
  my_aiocb.aio_buf = buf;
 
  /* Initialize the necessary fields in the aiocb */
  my_aiocb.aio_fildes = fd;
  my_aiocb.aio_nbytes = buf_len;
  my_aiocb.aio_offset = 0;

  ret = aio_read( &my_aiocb );
  if (ret < 0) perror("aio_read");

  //while ( aio_error( &my_aiocb ) == EINPROGRESS ) {};

  if ((ret = aio_return( &my_aiocb )) > 0) {
    DBG("got %d bytes on the read\n",ret);
    return ret;
  } else {
    DBG("read failed, consult errno=%d\n",errno);
    return 0;
  }

}



smoke::net_t net; 

int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  in_s=out_s=err_s=idl_s=0;

  int port = atoi(argv[1]);

/*
  net.on_idle([](int th){
    idl_s++;
    if(idl_s % 10000000 == 0){
      int i=idl_s;
      DBG("on_idle %d\n",i);
    }


  });
*/
 
  net.on_connect([&](int fd,int t){
    srv::fd_t& fdd=srv::get_fdd(fd);
    fdd.clear();
    fdd.acb.aio_fildes = fd;
    fdd.t=t;
    DBG("on_connect %d\n",fd);
    in_s++;
  });

  net.on_close([](int fd,int err){
    srv::fd_t& fdd=srv::get_fdd(fd);
    DBG("on_close %d:%d,%d\n",fd,fdd.wc,fdd.rc);
    err_s++;
  });

  net.on_read([&](int fd)->int{
    srv::fd_t& fdd=srv::get_fdd(fd);
    DBG("on_read %d:%d,%d\n",fd,fdd.wc,fdd.rc);

//    if(fdd.rc>0){ 
//      close(fd);err_s++;
//      return 0; 
//    }

//    fdd.rc++;

    //#define RBUF_MAX 4096
    //char rbuf[RBUF_MAX];
    //int r=read(fd,rbuf,RBUF_MAX);
    //int r=readIO(fd,rbuf,RBUF_MAX);

    if(fdd.rc == 0){
     int r=aio_read(&fdd.acb);
     if(r<0){
      DBG("aio_read error\n");
      close(fd);err_s++;
      return 0; 
     }
    }

    fdd.rc++;


   if ( aio_error( &fdd.acb ) == EINPROGRESS ){
      return 1;
   }

    // test read complition
    int ret;
    if ((ret = aio_return( &fdd.acb )) <= 0) {
      LOG("on_read aio_return error\n");
      close(fd);err_s++;
      return 0; 
    } 
    char* buf=(char*)fdd.acb.aio_buf;
    buf[ret]=0;
    DBG("aio %d read[%d]:\n%s\n",fd,ret,buf); 

    // goin write
    return 1; 
  });

  net.on_write([&](int fd)->int{ 
    srv::fd_t& fdd=srv::get_fdd(fd);
    DBG("on_write %d:%d,%d\n",fd,fdd.wc,fdd.rc);

    if(!(fdd.wc>0 || fdd.rc>0)){ // skip first write 
      fdd.wc++;       
      return 0; // no write schedule
    }

    //fdd.wc++;
/*
    // test read complition
    int ret;
    if ((ret = aio_return( &fdd.acb )) <= 0) {
      if(fdd.wc<10) return 1;
      DBG("aio_return error\n");
      close(fd);err_s++;
      return 0; 
    } 
    char* buf=(char*)fdd.acb.aio_buf;
    buf[ret]=0;
    DBG("aio %d read[%d]:\n%s\n",fd,ret,buf); 

*/

    char* buf=(char*)fdd.acb.aio_buf;

    if(fdd.wc == 1) {
    std::ostringstream os_h,os_b;      
            
    os_b<<"<!doctype html>\n<html><head><title>smoke test page</title></head>\n<body>" \
            "<h1 style='color:red'>smoke test page!!!</h1>\n" \
            "connections: opened("<<in_s<<
            "): closed("<<out_s<<
            "): current("<<in_s-out_s-err_s<<
            ") errd("<<err_s<<")\n" \
            "</body></html>\n";

    auto str_b=os_b.str();

    os_h<<"HTTP/1.1 200 OK\r\n" \
          "Content-Type: text/html\r\n" \
          "Content-Length: "<<str_b.length()<<"\r\n" \
          "Connection: close\r\n" \
          "\r\n" << str_b;

    auto str_h=os_h.str();
    
    memcpy(buf,str_h.c_str(),str_h.length());
    fdd.acb.aio_nbytes = str_h.length();

    int r=aio_write(&fdd.acb);
    ASSERT(r==0);
    } // if fdd.wc == 1

   fdd.wc++;


   if ( aio_error( &fdd.acb ) == EINPROGRESS ){
      return 1;
   }

    // test aio complition
    int ret;
    if ((ret = aio_return( &fdd.acb )) <= 0) {
      LOG("on_write: aio_return error\n");
      close(fd);err_s++;
      return 0; 
    } 

    //int r=write(fd,str_h.c_str(),str_h.length());
    //if(r<=0){LOGFL("write error, %d\n",errno);}
    close(fd);  out_s++;
    return 0; // no more write schedule 
  });


  smoke::net_run(&net,"127.0.0.1",port);  

  return EXIT_SUCCESS;  
}
