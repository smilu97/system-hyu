
CC := gcc

CFLAGS := -g

all: build

build:
	$(CC) $(CFLAGS) src/test_small_mutex.c  -o bin/test_small_mutex  -lpthread
	$(CC) $(CFLAGS) src/test_small_spin.c   -o bin/test_small_spin   -lpthread
	$(CC) $(CFLAGS) src/test_small_hybrid.c -o bin/test_small_hybrid -lpthread

	$(CC) $(CFLAGS) src/test_big_mutex.c    -o bin/test_big_mutex    -lpthread
	$(CC) $(CFLAGS) src/test_big_spin.c     -o bin/test_big_spin     -lpthread
	$(CC) $(CFLAGS) src/test_big_hybrid.c   -o bin/test_big_hybrid   -lpthread

run:
	bin/test_small_hybrid
	bin/test_small_mutex
	bin/test_small_spin

	bin/test_big_hybrid
	bin/test_big_mutex
	bin/test_big_spin
