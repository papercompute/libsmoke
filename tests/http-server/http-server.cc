#include "smoke_config.h"

#include "http_parser.h"

#include "smoke.h"

#include <fstream>
#include <atomic>

http_parser_settings parser_settings;

#define HTTP404 "HTTP/1.1 404 Not Found" CRLF "Connection: close" CRLF CRLF
#define HTTPOK "HTTP/1.1 200 OK" CRLF "Content-Type: text/html" CRLF "Content-Length: 2" CRLF "Connection: close" CRLF CRLF "ok"
#define MAX_HTTP_HEADERS_PER_REQUEST 16

std::atomic<int> in_s;
std::atomic<int> out_s;
std::atomic<int> err_s;

#define HH_USER_AGENT 0x01
#define HH_USER_AGENT_STR "User-Agent"

#define HH_HOST 0x02
#define HH_HOST_STR "Host"

#define HH_CONTENT_LENGTH 0x03
#define HH_CONTENT_LENGTH_STR "Content-Length"

#define HH_CONTENT_TYPE 0x04
#define HH_CONTENT_LENGTH_STR "Content-Type"

#define HH_CONNECTION 0x05
#define HH_CONNECTION_STR "Connection"

namespace srv
{

// fd connection data
struct fd_t
{
 int fd; // cached fd;
 int wc; // write counter 
 int rc; // read counter
 // http
 const char* url;
 // headers
#ifdef HH_USER_AGENT 
 const char* hh_user_agent;
#endif 
#ifdef HH_HOST 
 const char* hh_host;
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
 // body
 const char* body;
 http_parser parser;
 fd_t():wc(0),rc(0){parser.data = this;};
 void clear(){wc=rc=0;};
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

}; // srv


namespace smoke{

typedef std::function<int(int fd)> on_http_fn_t;

struct http_t
{
 smoke::net_t net;

 std::map<const char*,on_http_fn_t> get_url_map;
 std::map<const char*,on_http_fn_t> put_url_map;
 std::map<const char*,on_http_fn_t> post_url_map;
 std::map<const char*,on_http_fn_t> del_url_map;
 std::map<const char*,on_http_fn_t> head_url_map;

 void get(const char* url,on_http_fn_t cb){get_url_map[url]=cb;}    
 void put(const char* url,on_http_fn_t cb){put_url_map[url]=cb;}    
 void post(const char* url,on_http_fn_t cb){post_url_map[url]=cb;}    
 void del(const char* url,on_http_fn_t cb){del_url_map[url]=cb;}    
 void head(const char* url,on_http_fn_t cb){head_url_map[url]=cb;}    

 http_t(){
  init();
 };

 void init(){

// https://github.com/joyent/http-parser
// https://github.com/joyent/http-parser/blob/37a0ff8/test.c#L403

//typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
//typedef int (*http_cb) (http_parser*);

//struct http_parser_settings {
//  http_cb      on_message_begin;
//  http_data_cb on_url;
//  http_data_cb on_status;
//  http_data_cb on_header_field;
//  http_data_cb on_header_value;
//  http_cb      on_headers_complete;
//  http_data_cb on_body;
//  http_cb      on_message_complete;
//};

  parser_settings.on_url = [](http_parser* parser,const char *at, size_t length)->int{
   DBG("on_url\n");
   srv::fd_t* fdd=(srv::fd_t*)parser->data;
   ((char*) at)[length]=0;
   DBG("on_url:%s\n",at);
   fdd->url=at;
   return 0;
  };

  parser_settings.on_header_field = [](http_parser* parser,const char *at, size_t length)->int{
   srv::fd_t* fdd=(srv::fd_t*)parser->data;
   ((char*) at)[length]=0;
   DBG("on_header_field:%s\n",at);

   switch(at[0]){
#ifdef HH_USER_AGENT 
    case 'U': // User-Agent
     if(STRCMPA(at,"User-Agent")){
      fdd->ht=HH_USER_AGENT;
      return 0;
     } 
    break;
#endif    
    case 'U': // User-Agent
     if(STRCMPA(at,"User-Agent")){
      fdd->ht=HH_USER_AGENT;
      return 0;
     } 
    break;
    default:
    fdd->body=nullptr;
   };

   fdd->body=at;
   return 0;
  };

  parser_settings.on_header_value = [](http_parser* parser,const char *at, size_t length)->int{
   srv::fd_t* fdd=(srv::fd_t*)parser->data;
   ((char*) at)[length]=0;
   DBG("on_header_value:%s\n",at);
   return 0;
  };

  parser_settings.on_message_complete = [](http_parser* parser)->int{
   DBG("on_message_complete\n");
   srv::fd_t* fdd=(srv::fd_t*)parser->data;
   DBG("method %d\n",parser->method);

//  XX(0,  DELETE,      DELETE)       
//  XX(1,  GET,         GET)          
//  XX(2,  HEAD,        HEAD)         
//  XX(3,  POST,        POST)         
//  XX(4,  PUT,         PUT)          
 
   switch(parser->method){
    case 0: DBG("DELETE\n"); break;
    case 1: DBG("GET\n"); break;
    case 2: DBG("HEAD\n"); break;
    case 3: DBG("POST\n"); break;
    case 4: DBG("PUT\n"); break;
    default: DBG("UNKNOWN\n");
   }

   
   DBG("fd=%d,(%d,%d)\n",fdd->fd,fdd->rc,fdd->wc);
   return 0;
  };

  parser_settings.on_headers_complete = [](http_parser* parser)->int{
   srv::fd_t* fdd=(srv::fd_t*)parser->data;
   DBG("on_headers_complete fd=%d\n",fdd->fd);
   return 0;
  };

  parser_settings.on_body = [](http_parser* parser,const char *at, size_t length)->int{
   DBG("on_body\n");
   srv::fd_t* fdd=(srv::fd_t*)parser->data;
   ((char*) at)[length]=0;
   fdd->body=at;
   return 0;
  };

  net.on_connect([&](int fd){
    in_s++;
    srv::fd_t& fdd=srv::get_fdd(fd);
    fdd.clear();
    fdd.fd=fd;
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
      return 0; 
    }

    if(fdd.rc==0){
      http_parser_init(&fdd.parser, HTTP_REQUEST);
    }

    fdd.rc++;

    int parsed = http_parser_execute(&fdd.parser, &parser_settings, data, nread);
    if (parsed < nread) {
      LOG("parse error");
      goto __err;
    }


    DBG("Body:%s\n",fdd.body);


    if(write(fd,HTTPOK,sizeof(HTTPOK)-1)<=0){LOG("write ok error, %d\n",errno);}
    close(fd); out_s++;
    return 0;

__err:
    if(write(fd,HTTP404,sizeof(HTTP404)-1)<=0){LOG("write 404 error, %d\n",errno);}
    close(fd); err_s++;
    return 0;
    
  });


  }; // init

  void run(const char* host,int port)
  {

      smoke_net_run(&net,host,port);  

  }
}; // http_t


}; // smoke



int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  in_s=out_s=err_s=0;

  int port = atoi(argv[1]);
  
  smoke::http_t http; 
   
  http.run("127.0.0.1",port);

  return EXIT_SUCCESS;  
}
