// number accepting threads, 1 is ok
#define A_TH_N 2
// number processing threads, = numCPUs
#define P_TH_N 4

#define DBG LOG
//#define DBG(...)

#define FDD_CACHE_RANGE 4096*16

// CALLBACK CODE USE

// use on_connect
#define USE_ON_CONNECT_CB

// use on_data
//#define USE_ON_DATA_CB

// use on_read
#define USE_ON_READ_CB

// use on_write
//#define USE_ON_WRITE_CB

// use on_close
//#define USE_ON_CLOSE_CB

// public folder
#define FOLDER "public"

#define FILE1 index.html

// file chunk size
#define CHUNK_SIZE 0x100
#define CHUNK_HEX "100"
