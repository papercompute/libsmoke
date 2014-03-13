/* 

smoke_fs.h file system module

*/

#ifndef _SMOKE_FS_H_
#define _SMOKE_FS_H_

namespace smoke
{

namespace fs
{

typedef std::function<void(const char* data,int err)> on_file_read_fn_t; 


 int readFile(const char* name,on_file_read_fn_t cb){
  struct stat stat_buf;
  int fd = open(filename, O_RDONLY);
  if (fd<=0){ return -1; }
  fstat(fd, &stat_buf);
  sz=stat_buf.st_size;
  return 0;
 }


} // fs

} // smoke


#endif
