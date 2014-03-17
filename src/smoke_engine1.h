/* 

smoke_engine1.h core module

multithreaded implementation

accepting thread(s)
processing thread(s) 

*/

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
#include <arpa/inet.h>
#include <sched.h>
#include <assert.h>
#include <aio.h>
#include <sys/uio.h>

#include <iostream>
#include <sstream>
#include <functional>
#include <string>
#include <map>
#include <unordered_set>
#include <unordered_map>

#include "defs.h"

namespace smoke {


struct net_t;


typedef std::function<void(int fd)> on_connect_fn_t; 
typedef std::function<void(int fd,int err)> on_close_fn_t;
typedef std::function<int(int fd)> on_read_fn_t;
typedef std::function<int(int fd)> on_write_fn_t;
typedef std::function<int(int fd,const char* data,int nread)> on_data_fn_t;


struct event_info_t
{
  int id;
  net_t *net;  
};

// global descriptors

int gafd[A_TH_N], gpfd[P_TH_N];
pthread_t ga_threads[A_TH_N];  
pthread_t gp_threads[P_TH_N];    
event_info_t gaei[A_TH_N];
event_info_t gpei[P_TH_N];

int net_init (net_t* net);


struct net_t
{
  net_t():
  on_connect_cb(nullptr),
  on_close_cb(nullptr),
  on_read_cb(nullptr),
  on_write_cb(nullptr),
  on_data_cb(nullptr)
  {
    net_init(this);
  };
  on_connect_fn_t on_connect_cb;  
  on_close_fn_t on_close_cb;  
  on_read_fn_t on_read_cb;  
  on_write_fn_t on_write_cb;  
  on_data_fn_t on_data_cb;  

  void on_data(on_data_fn_t cb){on_data_cb=cb;}    
  void on_read(on_read_fn_t cb){on_read_cb=cb;}    
  void on_write(on_write_fn_t cb){on_write_cb=cb;}    
  void on_connect(on_connect_fn_t cb){on_connect_cb=cb;}    
  void on_close(on_close_fn_t cb){on_close_cb=cb;}    
};



// validate
#ifdef USE_ON_DATA_CB 
 #ifdef USE_ON_READ_CB 
  #error "DO NOT USE BOTH ON_DATA & ON_READ"
 #endif
#endif



#define MAXEVENTS_A 256
#define MAXEVENTS_P 512

#define BACKLOG SOMAXCONN*8

#define WAIT_TO_A (-1)
#define WAIT_TO_P (-1)

#define R_BUF_MAX 4096


int make_socket_non_blocking (int sfd)
{
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1) {
    perror ("fcntl");
     return -1;
  }

  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1) {
    perror ("fcntl");
     return -1;
  }

  return 0;
}


int create_and_bind (const char* host,int port)
{

  int s;

 if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(1);
  }
    
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));    

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port); 

    if (inet_pton(AF_INET, host, &serv_addr.sin_addr) != 1){
     LOG("inet_pton error [%s:%u]\n",host,port);
     exit(1);
    }
    
    int yes = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt  SO_REUSEADDR");
        exit(1);
    }

    int flag = 1;
    if (setsockopt( s, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) ) == -1){
        perror("setsockopt  TCP_NODELAY");
        exit(1);
    }
    
    if (bind(s, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind error");
        close(s);
        exit(1);
    }

  return s;
}


int create_and_connect(const char* host, int port)
{
 int fd;

 struct sockaddr_in sock_addr; 
// int reuseaddr_on = 1;
 fd = socket(AF_INET, SOCK_STREAM, 0); 
 if (fd <= 0){
  LOG("socket error [%s:%u]",host,port);
  return -1;
 }


 memset(&sock_addr, 0, sizeof(sock_addr));
 sock_addr.sin_family = AF_INET;
 sock_addr.sin_port = htons(port);

 if (inet_pton(AF_INET, host, &sock_addr.sin_addr) <= 0){
  LOG("inet_pton error [%s:%u]",host,port);
  close(fd);
  return -1;
 }

 if (connect(fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr))<0)
 {
  LOG("connect error [%s:%u]",host,port);
  close(fd);
  return -1;
 } 

 make_socket_non_blocking(fd);

 return fd;
}


int create_and_connect_non_blocking(const char* host, int port)
{
 int fd;

 struct sockaddr_in sock_addr; 
// int reuseaddr_on = 1;
 fd = socket(AF_INET, SOCK_STREAM, 0); 
 if (fd <= 0){
  LOG("socket error [%s:%u]",host,port);
  return -1;
 }

 make_socket_non_blocking(fd);

 memset(&sock_addr, 0, sizeof(sock_addr));
 sock_addr.sin_family = AF_INET;
 sock_addr.sin_port = htons(port);

 if (inet_pton(AF_INET, host, &sock_addr.sin_addr) <= 0){
  LOG("inet_pton error [%s:%u]",host,port);
  close(fd);
  return -1;
 }

 if (connect(fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) == -1 && errno != EINPROGRESS) {
    // report errno
    LOG("connect error [%s:%u]",host,port);
    close(fd);
    return -1;
 }
 return fd;
}


#define EPOLL_EVENTS EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLIN | EPOLLOUT | EPOLLET

void* accepting_thread(event_info_t* ei)
{

  int i,s,nfds;
  int sfd,afd;

  static int t=0;

  struct epoll_event event;
  struct epoll_event *events;
  net_t *net = ei->net;
#ifdef USE_ON_CONNECT_CB                 
  on_connect_fn_t on_connect_cb=net->on_connect_cb;
  ASSERT(on_connect_cb);
#endif
   int id=ei->id;
   afd = gafd[id];

   events = (struct epoll_event *)calloc (MAXEVENTS_A, sizeof event);
   struct sockaddr in_addr; socklen_t in_len; int infd;
   in_len = sizeof in_addr;


    while(1){

    nfds = epoll_wait (afd, events, MAXEVENTS_A, WAIT_TO_A);    
     
    for (i = 0; i < nfds; i++){

    sfd = events [i].data.fd;

    if ( (events[i].events & EPOLLIN)!=EPOLLIN ) {
        close (events[i].data.fd);
        continue;
      }

              while (1){
                  infd = accept (sfd, &in_addr, &in_len);
                  if (infd == -1){
                    break;
                  }

                  make_socket_non_blocking (infd);

                  event.data.fd = infd;

                  event.events = EPOLL_EVENTS;

// http://linux.die.net/man/7/epoll When used as an edge-triggered interface, for performance reasons, 
// it is possible to add the file descriptor inside the epoll interface (EPOLL_CTL_ADD) 
// once by specifying (EPOLLIN|EPOLLOUT). This allows you to avoid continuously switching between 
// EPOLLIN and EPOLLOUT calling epoll_ctl(2) with EPOLL_CTL_MOD.


//        typedef union epoll_data {
//               void    *ptr;
//               int      fd;
//               uint32_t u32;
//               uint64_t u64;
//           } epoll_data_t;
//
//           struct epoll_event {
//               uint32_t     events; 
//               epoll_data_t data;   
//           };

#ifdef USE_ON_CONNECT_CB                 
                  on_connect_cb(infd);
#endif
                  s = epoll_ctl (gpfd[t], EPOLL_CTL_ADD, infd, &event);

                  if (s == -1) {
                      perror ("epoll_ctl");
                      break;
                  }

                  t=(t+1)%P_TH_N; // round robin
              }

    } // for

    } // while(1)

    return NULL;
}



void* processing_thread(event_info_t* ei)
{

  int i,nfds;
  int pfd;

  struct epoll_event event;
  struct epoll_event *events;

  net_t *net = ei->net;
#ifdef USE_ON_DATA_CB  
  char buf[R_BUF_MAX+1]; 
  int n,nread;
  on_data_fn_t on_data_cb=net->on_data_cb;
  ASSERT(on_data_cb);
#endif
#ifdef USE_ON_WRITE_CB  
  on_write_fn_t on_write_cb=net->on_write_cb;
  ASSERT(on_write_cb);
#endif
#ifdef USE_ON_READ_CB  
  on_read_fn_t on_read_cb=net->on_read_cb;
  ASSERT(on_read_cb);
#endif  
#ifdef USE_ON_CLOSE_CB  
  on_close_fn_t on_close_cb=net->on_close_cb;
  ASSERT(on_close_cb);
#endif  
  
  int fd;

  int id=ei->id;
   
  pfd = gpfd[id]; 

  events = (struct epoll_event *)calloc (MAXEVENTS_P, sizeof event);

  while (1) {
    
    nfds = epoll_wait (pfd, events, MAXEVENTS_P, WAIT_TO_P);

    for (i = 0; i < nfds; i++){

      fd = events [i].data.fd;
//      LOG("fd %d,es %x ev %d:%d\n",fd,events[i].events,events[i].events & EPOLLOUT, events[i].events & EPOLLIN);

      if ( events[i].events & ~(EPOLLIN | EPOLLOUT) ){
#ifdef USE_ON_CLOSE_CB
        on_close_cb(fd,1);  
#endif                
        close(fd);
        continue;
      }


      if (events[i].events & EPOLLIN) {
//       LOG("IN: fd %d, events %x\n",fd,events[i].events);

#ifdef USE_ON_READ_CB
        if(on_read_cb(fd)){
           event.data.fd = fd;  event.events = EPOLL_EVENTS;
           epoll_ctl (pfd, EPOLL_CTL_MOD, fd, &event);
        }
        continue;        
#endif        
#ifdef USE_ON_DATA_CB  
//        LOG("on_data_cb\n");
        n = 0; while ((nread = read (fd, buf + n, (R_BUF_MAX-1)-n))>0){          
          n += nread;
          if(n>=(R_BUF_MAX-1)){
            // buf overflow
#ifdef USE_ON_CLOSE_CB
          on_close_cb(fd,1);  
#endif                
          close(fd);
          goto __cont1;
          } 
        } // while
        if ((nread == -1 && errno!= EAGAIN) || (n==0)){
#ifdef USE_ON_CLOSE_CB
          on_close_cb(fd,1);  
#endif                
          close(fd);
          continue;
        }
        buf[n]=0;        
        if(on_data_cb(fd,buf,n)){
           event.data.fd = fd;  event.events = EPOLL_EVENTS;
           epoll_ctl (pfd, EPOLL_CTL_MOD, fd, &event);
        }
      __cont1:
#endif
       continue;
      } // IN


      if (events[i].events & EPOLLOUT) {

//        LOG("OUT: fd %d, events %x\n",fd,events[i].events);

#ifdef USE_ON_WRITE_CB  
//         LOG("on_write_cb\n");
         if(on_write_cb(fd)){
           event.data.fd = fd; event.events = EPOLL_EVENTS;
           epoll_ctl (pfd, EPOLL_CTL_MOD, fd, &event);
         }
#endif
      } // OUT


    } // for n

    } //while(1)

    return NULL;
}


int net_create_server(const char* host,int portno)
{
  int i;
  int sfd;
  struct epoll_event event;

  sfd = create_and_bind(host,portno);
  if (sfd == -1){
    FATAL("create_and_bind");
  }

  make_socket_non_blocking (sfd);
  
  for(i=0;i<A_TH_N;i++){  
   event.data.fd = sfd;
   event.events = EPOLLIN | EPOLLET;  
   if (epoll_ctl (gafd[i], EPOLL_CTL_ADD, sfd, &event) == -1) {
    FATAL ("epoll_ctl");
   }
  } // for

  if (listen (sfd, BACKLOG) == -1) {
    FATAL("listen");
  }

  return sfd;
}

int net_connect_client_sync(const char* host,int portno)
{
  int fd;
  static int t=0;
  struct epoll_event event;

  fd = create_and_connect(host,portno);
  if (fd == -1){
    FATAL("create_and_connect");
  }

  LOG("net_connect_client=%s:%d\n",host,portno);

//  make_socket_non_blocking (fd);

  event.data.fd = fd;
  event.events = EPOLL_EVENTS;

  if (epoll_ctl (gpfd[t], EPOLL_CTL_ADD, fd, &event) == -1) {
    FATAL ("epoll_ctl");
  }

  t=(t+1)%P_TH_N; // round robin
 
  return fd;
}

int net_connect_client_non_blocking(const char* host,int portno)
{
  int fd;
  static int t=0;
  struct epoll_event event;

  fd = create_and_connect_non_blocking(host,portno);
  if (fd == -1){
    FATAL("create_and_connect");
  }

//  LOG("net_connect_client_non_blocking=%s:%d\n",host,portno);

//  make_socket_non_blocking (fd);

  event.data.fd = fd;
  event.events = EPOLL_EVENTS;

  if (epoll_ctl (gpfd[t], EPOLL_CTL_ADD, fd, &event) == -1) {
    FATAL ("epoll_ctl");
  }

  t=(t+1)%P_TH_N; // round robin
 
  return fd;
}



int net_init (net_t* net)
{
 int i;
// int s;
  // accepting epoll
for(i=0;i<A_TH_N;i++){  
  gafd[i] = epoll_create1 (0);
  if (gafd[i] == -1) {
    FATAL ("epoll_create1");
  }
} // for

  // processing epoll
for(i=0;i<P_TH_N;i++){  
  gpfd[i] = epoll_create1 (0);
  if (gpfd[i] == -1) {
    FATAL ("epoll_create1");
  }
} // for

return 0;
}

int net_start (net_t* net)
{
 int i,s;

for(i=0;i<A_TH_N;i++){
  gaei[i].id=i;
  gaei[i].net=net;
  s = pthread_create(&ga_threads[i],NULL,(void*(*)(void*))&accepting_thread, &(gaei[i]));
  if(s){    
    FATALL("thread A error %d\n", s);
  }
} // for
  
for(i=0;i<P_TH_N;i++){
  gpei[i].id=i;
  gpei[i].net=net;
  s = pthread_create(&gp_threads[i],NULL,(void*(*)(void*))&processing_thread, &(gpei[i]));
  if (s){    
    FATALL("thread P error %d\n", s);
  }
} // for

return 0;
}

void net_loop()
{

LOG("smoke...\n");
// smoke
pthread_join(ga_threads[0], 0);    

LOG("not here...\n");

}


void net_run(net_t* net,const char* host, int port)
{

  net_create_server(host,port);
  net_start(net);
  net_loop();
}


} // smoke

#endif