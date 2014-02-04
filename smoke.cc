#include "smoke.h"

#define NR_CPUS_ON 4
#define A_TH_N 1
#define P_TH_N 1

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
  int sock_id=0;

  struct epoll_event event;
  struct epoll_event *events;
  smoke::net_t *net = ei->net;
  std::function<void(smoke::socket_t&)> on_connect_cb=net->on_connect_cb;
  smoke::socket_t *sock;

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

                  sock = new smoke::socket_t(net);
                  sock->id=sock_id++;
                  sock->fd = infd;
                  sock->epfd = ei->pfd[t];

                  t=(t+1)%P_TH_N; // round robin

                  event.data.ptr = sock;                  
                  event.events = EPOLLIN | EPOLLET;
                 
                  on_connect_cb(*sock);

                  s = epoll_ctl (sock->epfd, EPOLL_CTL_ADD, infd, &event);
                  

                  if (s == -1) {
                      perror ("epoll_ctl");
                      break;
                  }

                  net->opened_socks++;

     
              }

    } // for

    } // while(1)

    return NULL;
}





static void* processing_thread(event_info_t* ei)
{

  int i,nfds;//n,l,nread,nwrite,data_size;
  int pfd;
  
//  char* buf;
//  char rbuf[R_BUF_MAX]; 
  

  struct epoll_event event;
  struct epoll_event *events;
  smoke::socket_t *sock;  

  int id=ei->id;
   
  pfd = ei->pfd[id]; 

  events = (struct epoll_event *)calloc (MAXEVENTS_P, sizeof event);

  while (1) {
    
    nfds = epoll_wait (pfd, events, MAXEVENTS_P, WAIT_TO_P);

    for (i = 0; i < nfds; i++){

      sock=(smoke::socket_t*)events [i].data.ptr;
      sock->th_id=id;

//      fd = sock->fd;

      if (!(events[i].events & ( EPOLLIN | EPOLLOUT ))){             
        sock->so_close();
        continue;
      }


      if (events[i].events & EPOLLOUT) {
        if(sock->on_write_cb){
          sock->on_write_cb(*sock);
        }
       continue;
      } // OUT


      if (events[i].events & EPOLLIN) {
        sock->on_read_cb(*sock);
        continue;
      } // IN


    } // for n

    } //while(1)

    return NULL;
}


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

void smoke::socket_t::so_close(){
    if(on_close_cb){on_close_cb(*this);}
    close(fd);
    net->closed_socks++; // todo atomic
    delete this;
};



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

