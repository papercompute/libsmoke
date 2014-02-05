#include "smoke.h"

#define NR_CPUS_ON 4
#define A_TH_N 1
#define P_TH_N 2

#define MAXEVENTS_A 256
#define MAXEVENTS_P 512

#define BACKLOG SOMAXCONN*8

#define WAIT_TO_A (-1)
#define WAIT_TO_P (-1)


struct event_info_t
{
  int id;
  int sfd;
  int afd;
  int pfd[P_TH_N];

  smoke::net_t *net;  
};


static int make_socket_non_blocking (int sfd)
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


static int create_and_bind (int port)
{

  int s;

 if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(1);
  }
    
    struct sockaddr_in serv_addr;
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port); 
    
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


static void* accepting_thread(event_info_t* ei)
{

  int i,s,nfds,t=0;
  int sfd,afd;

  struct epoll_event event;
  struct epoll_event *events;
  smoke::net_t *net = ei->net;
  std::function<void(int fd)> on_connect_cb=net->on_connect_cb;
  ASSERT(on_connect_cb!=NULL);

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
                  event.events = EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLIN | EPOLLOUT | EPOLLET;

//http://linux.die.net/man/7/epoll When used as an edge-triggered interface, for performance reasons, it is possible to add the file descriptor inside the epoll interface (EPOLL_CTL_ADD) once by specifying (EPOLLIN|EPOLLOUT). This allows you to avoid continuously switching between EPOLLIN and EPOLLOUT calling epoll_ctl(2) with EPOLL_CTL_MOD.

                 
                  on_connect_cb(infd);

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





static void* processing_thread(event_info_t* ei)
{

  int i,nfds;
  int pfd;

  struct epoll_event event;
  struct epoll_event *events;

  smoke::net_t *net = ei->net;
  std::function<void(int fd,const char* data,int nread)> on_data_cb=net->on_data_cb;
  ASSERT(on_data_cb!=NULL);


  char buf[R_BUF_MAX]; 
  int n,nread;
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
        close(fd);
        continue;
      }


      if (events[i].events & EPOLLIN) {

        n = 0; 
        while ((nread = read (fd, buf + n, R_BUF_MAX-1))>0){          
          n += nread;
        }

        if ((nread == -1 && errno!= EAGAIN) || (n==0)){
          close(fd);
          continue;
        }


        buf[n]=0;        
        on_data_cb(fd,buf,n);

      } // IN


    } // for n

    } //while(1)

    return NULL;
}

/*
int smoke::socket_t::write(const char* buf, int len, int& n)
{
 int nwritten;
 n = 0;    
 while ((nwritten = ::write (fd, buf + n, len-n))>0){          
   n += nwritten;
   if(len<=n){return n;}
  } // while

 return nwritten;
}

int smoke::socket_t::read(const char* buf, int buf_sz, int& n)
{
  int nread;
  n = 0;
  while ((nread = ::read (fd, (void*)(buf + n), buf_sz-1))>0){          
   n += nread;
  } // while

 return nread;
}


int smoke::socket_t::make_readable()
{
  struct epoll_event event;
  event.data.ptr = this; 
  event.events = EPOLLIN | EPOLLET;
  return epoll_ctl (epfd, EPOLL_CTL_MOD, fd, &event);
}

int smoke::socket_t::make_writable()
{
  struct epoll_event event;
  event.data.ptr = this; 
  event.events = EPOLLIN | EPOLLOUT | EPOLLET;
  return epoll_ctl (epfd, EPOLL_CTL_MOD, fd, &event);
}

int smoke::socket_t::make_del()
{
  struct epoll_event event;
  event.events = 0;
  return epoll_ctl (epfd, EPOLL_CTL_DEL, fd, &event);
}

int smoke::socket_t::check()
{
   int r;
   int code;
   size_t len = sizeof(int);

   r = getsockopt(fd, SOL_SOCKET, SO_ERROR, &code, &len);

   return r | code;
}


void smoke::socket_t::so_close(){
    LOG("so_close in\n");
    if(on_close_cb){on_close_cb(*this);}
    LOG("so_close 1\n");
    make_del();
    LOG("so_close 2\n");
    close(fd);
    LOG("so_close 3\n");
    is_closed++;
    delete this;
    LOG("so_close out\n");
};

*/

int smoke::smoke_net_run (smoke::net_t* net, int portno)
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

  sfd = create_and_bind(portno);
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

