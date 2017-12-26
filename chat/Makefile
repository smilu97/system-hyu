
CC := gcc
SRCS := src/client.c src/server.c
INC := includes/

CFLAGS := -I $(INC) -O2

all: build

server:
	$(CC) $(CFLAGS) -o src/common.o -c src/common.c
	$(CC) $(CFLAGS) -o src/server.o -c src/server.c
	$(CC) $(CFLAGS) -o bin/server src/common.o src/server.o -lpthread

client:
	$(CC) $(CFLAGS) -o src/common.o -c src/common.c
	$(CC) $(CFLAGS) -o src/client.o -c src/client.c
	$(CC) $(CFLAGS) -o bin/client src/common.o src/client.o -lpthread

build: $(SRCS)
	make server
	make client

clean:
	rm src/common.o src/client.o src/server.o
	-ipcrm -M 4444
