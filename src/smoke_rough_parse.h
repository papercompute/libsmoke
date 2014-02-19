/* 

smoke_rough_parse.h parse module

*/

#ifndef _SMOKE_ROUGH_PARSE_H_
#define _SMOKE_ROUGH_PARSE_H_

#include "smoke_config.h"

#include "smoke_data.h"

namespace smoke {
namespace parse {

typedef data::fd_t fd_t;
typedef data::http_req_t http_req_t;



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

int rough_parse_http_request(const char* s, int len, http_req_t& fdd, int& err)
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


}; // namespace parse
}; // namespace smoke

#endif