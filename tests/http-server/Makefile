system_name := $(shell sh -c 'uname -s 2>/dev/null || echo not')

CXX=g++


CODEDIR=../../src

HTTP_PARSER_DIR=../../deps/http-parser/
HTTP_PARSER_PACKAGE=$(HTTP_PARSER_DIR)/http_parser.o

CFLAGS=-O2 -Wall
LDFLAGS+=-pthread -lrt
CFLAGS+=-std=c++11 -I$(CODEDIR) -I. -I$(HTTP_PARSER_DIR)


CODE=$(CODEDIR)/smoke
APP=http-server


$(CODE): $(CODE).h $(APP).cc
	$(CXX) $(CFLAGS) $(HTTP_PARSER_PACKAGE) $(APP).cc $(LDFLAGS) -o $(APP) 

clean:
	rm $(APP)
