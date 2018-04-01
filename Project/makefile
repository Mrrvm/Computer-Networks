CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -Wpedantic -Wshadow -Wstrict-overflow -fno-strict-aliasing
CFLAGS_RELEASE = -march=native -std=gnu11 -O2 -Os

all: service reqserv

service: 
	$(CC) defs.c sockets.c service.c $(CFLAGS) -o service

reqserv: 
	$(CC) reqserv.c $(CFLAGS) -o reqserv

clean:
	-rm $(wildcard *.o)

.PHONY: default all clean test service reqserv
