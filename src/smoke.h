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

#define LOGF(fmt, params...)        \
  do {                                 \
    fprintf(stderr, fmt "\n", params); \
    fflush(stderr);                    \
  } while (0)

#define LOGFL(fmt, params)                                \
  do {                                                    \
    fprintf(stderr,                                       \
            "Log %s on line %d: " fmt "\n",               \
            __FILE__,                                     \
            __LINE__,                                     \
            params);                                      \
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



struct net_t;

int smoke_net_run (smoke::net_t* net, int portno);


struct net_t
{
  net_t():
  on_connect_cb(nullptr),
  on_close_cb(nullptr),
  on_read_cb(nullptr),
  on_write_cb(nullptr),
  on_data_cb(nullptr)
  {};
  std::function<void(int fd)> on_connect_cb;  
  std::function<void(int fd,int err)> on_close_cb;  
  std::function<int(int fd)> on_read_cb;  
  std::function<int(int fd)> on_write_cb;  
  std::function<int(int fd,const char* data,int nread)> on_data_cb;  
  void on_data(std::function<int(int fd,const char* data,int nread)> cb){on_data_cb=cb;}    
  void on_read(std::function<int(int fd)> cb){on_read_cb=cb;}    
  void on_write(std::function<int(int fd)> cb){on_write_cb=cb;}    
  void on_connect(std::function<void(int fd)> cb){on_connect_cb=cb;}    
  void on_close(std::function<void(int fd,int err)> cb){on_close_cb=cb;}    
};


} // smoke

#endif
