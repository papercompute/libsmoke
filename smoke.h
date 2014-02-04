#ifndef _SMOKE_H_
#define _SMOKE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include <sched.h>
#include <assert.h>


#include <iostream>
#include <sstream>
#include <functional>
#include <string>
#include <map>
#include <unordered_set>

namespace smoke {


#define CRLF "\r\n"
#define LF "\n"


#define LOG(...)                   \
  do {                                 \
    fprintf(stderr, __VA_ARGS__); \
    fflush(stderr);                    \
  } while (0)

#define LOGL(msg)                                        \
  do {                                                    \
    fprintf(stderr,                                       \
            "Log %s on line %d: %s\n",         \
            __FILE__,                                     \
            __LINE__,                                     \
            msg);                                         \
    fflush(stderr);                                       \
  } while (0)

#define FATAL(msg)                                        \
  do {                                                    \
    fprintf(stderr,                                       \
            "Fatal error in %s on line %d: %s\n",         \
            __FILE__,                                     \
            __LINE__,                                     \
            msg);                                         \
    fflush(stderr);                                       \
    abort();                                              \
  } while (0)

#define ASSERT(expr)                                      \
 do {                                                     \
  if (!(expr)) {                                          \
    fprintf(stderr,                                       \
            "Assertion failed in %s on line %d: %s\n",    \
            __FILE__,                                     \
            __LINE__,                                     \
            #expr);                                       \
    abort();                                              \
  }                                                       \
 } while (0)


#define R_BUF_MAX 4096
#define W_BUF_MAX 4096

struct socket_t;
struct net_t;
//struct req_t;
//struct res_t;

typedef std::map<std::string, std::string> http_headers_t;

int smoke_net_run (smoke::net_t* net, int portno);

struct buf_t
{
 char* ptr;
 int len;
};

struct socket_t
{
  int fd;
  int epfd;
  int id;
  int th_id;
  int is_closing;
  net_t* net;
  
  int make_readable();
  int make_writable();

  int write(const char* buf, int len, int& n);
  int read(const char* buf, int len, int& n);


  std::function<void(socket_t& sock)> on_read_cb;  
  std::function<void(socket_t& sock)> on_write_cb;  
  std::function<void(socket_t& sock)> on_close_cb;
  
  socket_t(net_t *n):is_closing(0),net(n){}

  void on_read(std::function<void(socket_t& sock)> cb){on_read_cb=cb;}    
  void on_write(std::function<void(socket_t& sock)> cb){on_write_cb=cb;}    
  void on_close(std::function<void(socket_t& sock)> cb){on_close_cb=cb;}  
  void so_close();
  
};

struct net_t
{

  int opened_socks;
  int closed_socks;
/*
  int cons(){return open_socks-closed_socks;};
*/
  net_t():opened_socks(0),closed_socks(0){};
  net_t& createServer(std::function<void(socket_t&)> cb)
  {
    on_connect_cb=cb;
    return *this;
  }

  bool listen(int port)
  {
    return smoke_net_run(this,port)==0;
  }

  std::function<void(socket_t& sock)> on_connect_cb;  

};


} // smoke

#endif
