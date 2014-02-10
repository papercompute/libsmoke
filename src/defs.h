#ifndef _DEFS_H_
#define _DEFS_H_



#define CRLF "\r\n"
#define LF "\n"


#define LOG(...)                   \
  do {                                 \
    fprintf(stderr, __VA_ARGS__); \
    fflush(stderr);                    \
  } while (0)

#define LOGF(fmt, params...)        \
  do {                                 \
    fprintf(stderr, fmt "\n", params); \
    fflush(stderr);                    \
  } while (0)

#define LOGFL(fmt, params)                                \
  do {                                                    \
    fprintf(stderr,                                       \
            "Log %s on line %d: " fmt "\n",               \
            __FILE__,                                     \
            __LINE__,                                     \
            params);                                      \
    fflush(stderr);                                       \
  } while (0)

#define FATAL(msg)                                        \
  do {                                                    \
    fprintf(stderr,                                       \
            "Fatal error in %s on line %d: %s\n",         \
            __FILE__,                                     \
            __LINE__,                                     \
            msg);                                         \
    fflush(stderr);                                       \
    abort();                                              \
  } while (0)

#define ASSERT(expr)                                      \
 do {                                                     \
  if (!(expr)) {                                          \
    fprintf(stderr,                                       \
            "Assertion failed in %s on line %d: %s\n",    \
            __FILE__,                                     \
            __LINE__,                                     \
            #expr);                                       \
    abort();                                              \
  }                                                       \
 } while (0)

#define STRCMP1(a,b) (a == b) 
#define STRCMP2(a,b) ((a[1] == b[1]) && (a[0] == b[0]))
#define STRCMP3(a,b) ((a[2] == b[2]) && (a[1] == b[1]) && (a[0] == b[0]))
#define STRCMP4(a,b) ((a[3] == b[3]) && (a[2] == b[2]) && (a[1] == b[1]) && (a[0] == b[0]))
#define STRCMP5(a,b) ((a[4] == b[4]) && (a[3] == b[3]) && (a[2] == b[2]) && (a[1] == b[1]) && (a[0] == b[0]))
#define STRCMP6(a,b) ((a[5] == b[5]) && (a[4] == b[4]) && (a[3] == b[3]) && (a[2] == b[2]) && (a[1] == b[1]) && (a[0] == b[0]))
#define STRCMP7(a,b) ((a[6] == b[6]) && (a[5] == b[5]) && (a[4] == b[4]) && (a[3] == b[3]) && (a[2] == b[2]) && (a[1] == b[1]) && (a[0] == b[0]))
#define STRCMP8(a,b) ((a[7] == b[7]) && (a[6] == b[6]) && (a[5] == b[5]) && (a[4] == b[4]) && (a[3] == b[3]) && (a[2] == b[2]) && (a[1] == b[1]) && (a[0] == b[0]))
#define STRCMP9(a,b) ((a[8] == b[8]) && (a[7] == b[7]) && (a[6] == b[6]) && (a[5] == b[5]) && (a[4] == b[4]) && (a[3] == b[3]) && (a[2] == b[2]) && (a[1] == b[1]) && (a[0] == b[0]))
#define STRCMPA(a,b) ((a[9] == b[9]) && (a[8] == b[8]) && (a[7] == b[7]) && (a[6] == b[6]) && (a[5] == b[5]) && (a[4] == b[4]) && (a[3] == b[3]) && (a[2] == b[2]) && (a[1] == b[1]) && (a[0] == b[0]))

#endif