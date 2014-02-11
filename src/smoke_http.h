#ifndef _SMOLE_HTTP_H_
#define _SMOKE_HTTP_H_


#include <fstream>
#include <atomic>

namespace smoke
{

#define HTTP404 "HTTP/1.1 404 Not Found" CRLF "Connection: close" CRLF CRLF
#define HTTPOK "HTTP/1.1 200 OK" CRLF "Content-Type: text/html" CRLF "Content-Length: 2" CRLF "Connection: close" CRLF CRLF "ok"
#define MAX_HTTP_HEADERS_PER_REQUEST 16

std::atomic<int> in_s;
std::atomic<int> out_s;
std::atomic<int> err_s;


// http://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol
// HTTP REQUEST METHODS

#define HM_GET 1
#define HM_HEAD 2
#define HM_POST 3
#define HM_PUT 4
#define HM_DELETE 5
#define HM_TRACE 6
#define HM_OPTIONS 7
#define HM_CONNECT 8
#define HM_PATH 9

// HTTP REQUEST QUERY PARAMS
#define HP_PAGE 1
#define HP_Q 2
#define HP_ID 2

// HTTP headers http://en.wikipedia.org/wiki/List_of_HTTP_header_fields

#define HH_USER_AGENT 1
#define HH_HOST 2
#define HH_CONTENT_LENGTH 3
#define HH_CONTENT_TYPE 4
#define HH_CONNECTION 5
#define HH_COOKIE 6
//#define HH_ACCEPT 7
#define ACCEPT_CHARSET 8
//#define HH_ACCEPT_ENCODING 9
#define HH_ACCEPT_LANGUAGE 10

namespace srv
{

// fd connection data
struct fd_t
{
 int wc; // write counter 
 int rc; // read counter
 fd_t():wc(0),rc(0){};
 void clear(){wc=rc=0;};
};

struct fd_http_t : fd_t
{
 int method; // http req method
 char* url_path; // http req url
 char* url_query; // http req url
 // headers
#ifdef HH_USER_AGENT 
 const char* hh_user_agent;
#endif 
#ifdef HH_HOST 
 const char* hh_host;
#endif 
#ifdef HH_COOKIE 
 const char* hh_cookie;
#endif 
#ifdef HH_CONTENT_LENGTH 
 const char* hh_content_length;
#endif 
#ifdef HH_CONTENT_TYPE 
 const char* hh_content_type;
#endif 
#ifdef HH_CONNECTION 
 const char* hh_connection;
#endif 
#ifdef HH_ACCEPT 
 const char* hh_accept;
#endif 
#ifdef HH_ACCEPT_ENCODING 
 const char* hh_accept_encoding;
#endif 
#ifdef HH_ACCEPT_LANGUAGE 
 const char* hh_accept_language;
#endif 
 // body
 const char* body; // http req body
};

std::unordered_map<int,fd_http_t> fmap;  
fd_http_t fdd[FDD_CACHE_RANGE];

fd_t& get_fdd(int fd){
 if(fd>=0 && fd<FDD_CACHE_RANGE){
  // get from faster array
  return fdd[fd];
 }
 // get from std::unordered_map
 return fmap[fd];
};

}; // srv




namespace http
{

/*
 inline char * rough_parse_http_request_method(const char* s, const char *se, 
  int& method, int& err)
 {
  char *p=(char*)s;
  err=1;
  DBG("rough_parse_http_request_method %c\n",*p);
  // request method
   method=0;
   switch(*p){
    case 'G': method=HM_GET; p+=3; goto __meth;
    case 'H': method=HM_HEAD; p+=4; goto __meth;
    case 'P': p++;
     switch(*p){
     case 'O':method=HM_POST; p+=3; goto __meth;
     case 'U':method=HM_PUT;  p+=2; goto __meth;
     default:  return p;
     }
    case 'D': method=HM_DELETE; p+=6; goto __meth;
    default:  return p;
   };
   if(!method) return p;
   __meth:
   // delimiter
   if(*p!=' ') return p;
   ++p;
   err=0;
   return p; 
 }

inline char* rough_parse_http_request_url_path(const char* s, const char *se, 
  char*& url_path, int& err)
{
  char *p=(char*)s;
  char *p0;
  err=1;
  DBG("rough_parse_http_request_url_path %c\n",*p);
  p0=p; while(p<se){
    switch(*p){
      case '?': url_path=p0; *p=0; p++; err=2;return p;
      case ' ': url_path=p0; *p=0; p++; err=0;return p;
      default: break;
    }
    if(*p<' ')return p;
    ++p;
  }; // while
  return p;
} 

inline char* rough_parse_http_request_url_query_param(const char* s, const char *se, 
  char*& url_query, int& err)
{
  char *p=(char*)s;
  err=1;
  DBG("rough_parse_http_request_url_query_params %c\n",*p);
   // header field 
  p0=p; while(p<se){
    if(*p == CR){
      ++p;url_query=p0; 
      err=0; return p;
    }
    if(*p<=' ')return (p-s);
    ++p;
    }; // while

  return p;
} 

inline char* rough_parse_http_header(const char* s, const char *se, 
  srv::fd_http_t& fdd, int& err)
{
  char *p=(char*)s;
  //char *p0;
  err=1;
  DBG("rough_parse_http_request_url_query_params %c\n",*p);
  return p;
} 
*/
 int rough_parse_http_request(const char* s, int len, srv::fd_http_t& fdd, int& err)
 {
  const char *se=s+len;
  char *p=(char*)s;
  char *p0;
  err=1;

  // request method
   fdd.method=0;
   switch(*p){
    case 'G': fdd.method=HM_GET; p+=3; goto __meth;
    case 'H': fdd.method=HM_HEAD; p+=4; goto __meth;
    case 'P': p++;
     switch(*p){
     case 'O':fdd.method=HM_POST; p+=3; goto __meth;
     case 'U':fdd.method=HM_PUT;  p+=2; goto __meth;
     default:  return 0;
     }
    case 'D': fdd.method=HM_DELETE; p+=6; goto __meth;
    default:  return 0;
   };
   if(!fdd.method) return 0;
   __meth:
   // delimiter
   if(*p!=' ') return (p-s);
   ++p;
   // check len
   if((p-s-2)>=len) return (p-s);
   
   //request url /path?key1=val1&key2=val2...

   p0=p; while(p<se){
    switch(*p){
      case '?': fdd.url_path=p0; *p=0; p++; goto __url_params;
      case ' ': fdd.url_path=p0; *p=0; p++; goto __url_end;
      default: break;
    }
    if(*p<' ')return 0;
    ++p;
   }; // while

   return (p-s);

  __url_params:
  DBG("__url_params\n");
  //request url params key1=val1&key1=val1&... 
   return (p-s);

  __url_end:
  // skip next line
    p0=p; while(p<se){
      if(*p==CR){++p; if(*p!=LF) return (p-s); ++p; goto __http_headers;}
      ++p;      
    }; // while
   return (p-s);

   
  __http_headers:
    DBG("__http_headers %d\n",(p-s));
    // check CRLF
    if(*p==CR){++p; 
      if(*p==LF){ ++p;goto __req_end;}
      return (p-s);
    }

    DBG("*p %c\n",*p);
   
   // http://en.wikipedia.org/wiki/List_of_HTTP_header_fields
  // skip next line
  while(p<se){
   DBG("hdr:%c,%d\n",*p,(p-s));
   // header field 
   p0=p; while(p<se){
    switch(*p){
      case ':': ++p; if(*p==' '){++p;} goto __hdr_value;
      default: break;
    } // switch
    if(*p<=' ')return (p-s);
    ++p;
    }; // while
     
    return (p-s);

   __hdr_value:
   // header value 
   DBG("hdr value:%c,%d\n",*p,(p-s));
   p0=p; while(p<se){
    switch(*p){
      case CR: ++p; if(*p==LF){++p; goto __hdr_end;}
      default: break;
    } // switch
    if(*p<=' ')return (p-s);
    ++p;
    }; // while

    return (p-s);

    __hdr_end:
    // check CRLF
    if(*p==CR){++p; if(*p==LF){ ++p;goto __req_end;}}
   };// while
   return (p-s);

  __req_end:
   err=0;
   return (p-s);
 
 return 0;  
 }; // rough_parse_http_request


struct http_t
{
 
 smoke::net_t net;

 http_t(){
  init();
 };



 void init()
 {

  net.on_connect([&](int fd){
    in_s++;
    srv::fd_t& fdd=srv::get_fdd(fd);
    fdd.clear();
    DBG("on_connect %d\n",fd);
  });

  net.on_close([](int fd,int err){
    err_s++;
    DBG("on_close %d\n",fd);
  });

  net.on_data([&](int fd,const char* data,int nread)->int{
    srv::fd_t& fdd=srv::get_fdd(fd);

    if(fdd.rc>0){ // no more reads
      LOG("fdd.rc>0\n");
      goto __err; 
    }
    //data[nread]=0;
    DBG("request:\n%s\n",data);

    fdd.rc++;
    int err,r;
    r=rough_parse_http_request(data,nread,(srv::fd_http_t&)fdd,err);
    DBG("rough_parse_http_request: %d,%d\n",r,err);
    if(err)goto __err;

    if(write(fd,HTTPOK,sizeof(HTTPOK)-1)<=0){LOG("write ok error, %d\n",errno);}
    close(fd); out_s++;
    return 0;

__err:
    if(write(fd,HTTP404,sizeof(HTTP404)-1)<=0){LOG("write 404 error, %d\n",errno);}
    close(fd); err_s++;
    return 0;
    
  });


  } // init

  void run(const char* host,int port)
  {

      smoke_net_run(&net,host,port);  

  } // run
}; // http_t

}; // namespace http

}; // namespace smoke

#endif