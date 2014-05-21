/* 

smoke_str.h symlink module 

*/



#ifndef _SMOKE_DATA_H_
#define _SMOKE_DATA_H_

//switch(smoke::str::s_hash(s))

namespace smoke {
namespace str {

constexpr unsigned long long s_hash(const char* const s, int p = 0)
{
    return *s ? static_cast<unsigned char>(*s) + (s_hash(s + 1, p + 1) << p) : 0;
}


};
};


#endif