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
#include <unordered_map>

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
  int id;
  int th_id;
};

struct net_t
{
  std::function<void(int fd)> on_connect_cb;  
  std::function<void(int fd,const char* data,int nread)> on_data_cb;  
  void on_data(std::function<void(int fd,const char* data,int nread)> cb){on_data_cb=cb;}    
  void on_connect(std::function<void(int fd)> cb){on_connect_cb=cb;}    
};


} // smoke

#endif
