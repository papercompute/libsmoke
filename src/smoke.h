/* 

smoke.h core module 

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


#include <iostream>
#include <sstream>
#include <functional>
#include <string>
#include <map>
#include <unordered_set>
#include <unordered_map>

#include "defs.h"

namespace smoke {




typedef std::function<void(int fd)> on_connect_fn_t; 
typedef std::function<void(int fd,int err)> on_close_fn_t;
typedef std::function<int(int fd)> on_read_fn_t;
typedef std::function<int(int fd)> on_write_fn_t;
typedef std::function<int(int fd,const char* data,int nread)> on_data_fn_t;

struct net_t;



int smoke_net_run (net_t* net, int portno);


struct net_t
{
  net_t():
  on_connect_cb(nullptr),
  on_close_cb(nullptr),
  on_read_cb(nullptr),
  on_write_cb(nullptr),
  on_data_cb(nullptr)
  {};
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


#include <smoke.h>

//#define NR_CPUS_ON 4

#define MAXEVENTS_A 256
#define MAXEVENTS_P 512

#define BACKLOG SOMAXCONN*8

#define WAIT_TO_A (-1)
#define WAIT_TO_P (-1)

#define R_BUF_MAX 4096

struct event_info_t
{
  int id;
  int sfd;
  int afd;
  int pfd[P_TH_N];

  net_t *net;  
};


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

#define EPOLL_EVENTS EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLIN | EPOLLOUT | EPOLLET

void* accepting_thread(event_info_t* ei)
{

  int i,s,nfds,t=0;
  int sfd,afd;

  struct epoll_event event;
  struct epoll_event *events;
  net_t *net = ei->net;
#ifdef USE_ON_CONNECT_CB                 
  on_connect_fn_t on_connect_cb=net->on_connect_cb;
  ASSERT(on_connect_cb);
#endif
  sfd = ei->sfd;  
  afd = ei->afd;  

   events = (struct epoll_event *)calloc (MAXEVENTS_A, sizeof event);


    while(1){
    nfds = epoll_wait (afd, events, MAXEVENTS_A, WAIT_TO_A);    

    for (i = 0; i < nfds; i++){

    if ( (events[i].events & EPOLLIN)!=EPOLLIN ) {
        close (events[i].data.fd);
        continue;
      }

              while (1){
                  struct sockaddr in_addr; socklen_t in_len; int infd;

                  in_len = sizeof in_addr;
                  infd = accept (sfd, &in_addr, &in_len);
                  if (infd == -1){
                    break;
                  }

                  make_socket_non_blocking (infd);

                  event.data.fd = infd;

                  event.events = EPOLL_EVENTS;

//http://linux.die.net/man/7/epoll When used as an edge-triggered interface, for performance reasons, it is possible to add the file descriptor inside the epoll interface (EPOLL_CTL_ADD) once by specifying (EPOLLIN|EPOLLOUT). This allows you to avoid continuously switching between EPOLLIN and EPOLLOUT calling epoll_ctl(2) with EPOLL_CTL_MOD.

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
                  s = epoll_ctl (ei->pfd[t], EPOLL_CTL_ADD, infd, &event);
                  

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
   
  pfd = ei->pfd[id]; 

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
#endif        
#ifdef USE_ON_DATA_CB  
//        LOG("on_data_cb\n");
        n = 0; while ((nread = read (fd, buf + n, R_BUF_MAX-n))>0){          
          n += nread;
          if(n>=R_BUF_MAX){
            // buf overflow
#ifdef USE_ON_CLOSE_CB
          on_close_cb(fd,1);  
#endif                
          close(fd);
          goto __cont1;
          } 
        }
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
#endif
      __cont1:
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



int smoke_net_run (net_t* net, const char* host,int portno)
{
  int i,j;
  int sfd, s;
  int afd[A_TH_N], pfd[P_TH_N];
  
  pthread_t a_threads[A_TH_N];  
  pthread_t p_threads[P_TH_N];    

  struct epoll_event event;

#ifdef CPU_AFFINITY
  int cpu;
  cpu_set_t cpuset;  
#endif

  sfd = create_and_bind(host,portno);
  if (sfd == -1)
    abort ();



  make_socket_non_blocking (sfd);

  s = listen (sfd, BACKLOG);
  if (s == -1) {
    perror ("listen");
     abort ();
  }


  // accepting 
for(i=0;i<A_TH_N;i++){  

  afd[i] = epoll_create1 (0);
  if (afd[i] == -1) {
    perror ("epoll_create1");
    abort ();
  }

  event.data.fd = sfd;
  event.events = EPOLLIN | EPOLLET;
  s = epoll_ctl (afd[i], EPOLL_CTL_ADD, sfd, &event);
  if (s == -1) {
    perror ("epoll_ctl");
     abort ();
  }
}


  // processing
for(i=0;i<P_TH_N;i++){  
  pfd[i] = epoll_create1 (0);
  if (pfd[i] == -1) {
    perror ("epoll_create1");
     abort ();
  }
}


event_info_t aei[A_TH_N];
event_info_t pei[P_TH_N];

#ifdef CPU_AFFINITY
cpu=0;
CPU_ZERO(&cpuset);
#endif

for(i=0;i<A_TH_N;i++){
  aei[i].id=i;
  aei[i].sfd=sfd; aei[i].afd=afd[i];
  for(j=0;j<P_TH_N;j++){aei[i].pfd[j]=pfd[j];}    
  aei[i].net=net;


  s = pthread_create(&a_threads[i],NULL,(void*(*)(void*))&accepting_thread, &(aei[i]));
  if(s){    
    printf("thread A error %d\n", s);
    abort();
  }

#ifdef CPU_AFFINITY
 CPU_SET(cpu, &cpuset);
 s = pthread_setaffinity_np(a_threads[i], sizeof(cpuset), &cpuset);
 if (s != 0){
   printf("pthread_setaffinity_np %d\n",s);
   abort();
 }
 cpu = (cpu + 1) % NR_CPUS_ON;
#endif

}
  
for(i=0;i<P_TH_N;i++){
  pei[i].id=i;
  pei[i].sfd=sfd; pei[i].afd=afd[i];
  for(j=0;j<P_TH_N;j++){pei[i].pfd[j]=pfd[j];}
  pei[i].net=net;


  s = pthread_create(&p_threads[i],NULL,(void*(*)(void*))&processing_thread, &pei[i]);
  if (s){    
    printf("thread P error %d\n", s);
    abort();
  }

#ifdef CPU_AFFINITY
 CPU_SET(cpu, &cpuset);
 s = pthread_setaffinity_np(p_threads[i], sizeof(cpuset), &cpuset);
 if (s != 0){
   printf("pthread_setaffinity_np (%d) cpu=%d ret=%d\n",i,cpu,s);
   abort();
 }
 cpu = (cpu + 1) % NR_CPUS_ON;
#endif

  
}

LOG("smoke...\n");
// smoke
pthread_join(a_threads[0], 0);    

return 0;
}

} // smoke

#endif