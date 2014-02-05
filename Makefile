system_name := $(shell sh -c 'uname -s 2>/dev/null || echo not')

CXX=g++

CFLAGS=-O2 -Wall
LDFLAGS+=-pthread -lrt
CFLAGS+=-std=c++11



CODE=smoke
APP=net-server
#APP=chunk-server
#APP=echo-server

$(CODE): $(CODE).h $(CODE).cc $(APP).cc
	$(CXX) $(CFLAGS) $(CODE).cc $(APP).cc $(LDFLAGS) -o $(APP) 

tcp-echo-server:   
	$(CXX) $(CFLAGS) $(CODE).cc $@.cc $(LDFLAGS) -o $@ 


clean:
	rm $(APP)
