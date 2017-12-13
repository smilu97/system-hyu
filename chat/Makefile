
CC := gcc
SRCS := src/client.c src/server.c
INC := includes/

CFLAGS := -I $(INC) -O2

all: build

build: $(SRCS)
	$(CC) $(CFLAGS) -o src/common.o -c src/common.c
	$(CC) $(CFLAGS) -o src/client.o -c src/client.c
	$(CC) $(CFLAGS) -o src/server.o -c src/server.c

	$(CC) $(CFLAGS) -o bin/client src/client.o src/common.o -lpthread
	$(CC) $(CFLAGS) -o bin/server src/server.o src/common.o -lpthread
