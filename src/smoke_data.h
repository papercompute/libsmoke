/* 

smoke_data.h data module 

*/

#ifndef _SMOKE_DATA_H_
#define _SMOKE_DATA_H_

namespace smoke {

namespace data {


// fd connection data
struct fd_t
{
 int wc; // write counter 
 int rc; // read counter
 fd_t():wc(0),rc(0){};
 void clear(){wc=rc=0;};
};


struct http_req_t : fd_t
{
 int fd;
 int method; // http req method
 char* url_path; // http req url
 char* url_query; // http req url
 const char* body; // body
};

/*
struct fd_http_t : fd_t
{
 int method; // http req method
 char* url_path; // http req url
 char* url_query; // http req url
 //char* headers[8]; // headers
 const char* body; // body
};
*/

}; // namespace data

}; // namespace smoke

#endif