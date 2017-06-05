
#
# Makefile for lab 7
#

CC  = gcc
CXX = g++

INCLUDES = 
CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = 
LDLIBS = 

.PHONY: default
default: http-server 

http-server:
http-server.o: 

.PHONY: clean
clean:
	rm -f *.o *~ a.out core http-server 

PHONY: all
all: clean default

