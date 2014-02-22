system_name := $(shell sh -c 'uname -s 2>/dev/null || echo not')

CXX=g++


CODEDIR=../../src

CFLAGS=-O2 -Wall
LDFLAGS+=-pthread -lrt
CFLAGS+=-std=c++11 -I$(CODEDIR) -I.


CODE=$(CODEDIR)/smoke
APP=fileserver

$(CODE): $(CODE).h $(APP).cc
	$(CXX) $(CFLAGS) $(APP).cc $(LDFLAGS) -o $(APP) 

clean:
	rm $(APP)
