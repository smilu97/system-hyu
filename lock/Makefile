
CC := gcc

CFLAGS := -g

RUN_SMALL := 10 10000000
RUN_BIG := 4 10

all: build

build:
	$(CC) $(CFLAGS) -c src/hybrid.c -o obj/hybrid.o

	$(CC) $(CFLAGS) src/test_small_mutex.c  -o bin/test_small_mutex  -lpthread
	$(CC) $(CFLAGS) src/test_small_spin.c   -o bin/test_small_spin   -lpthread

	$(CC) $(CFLAGS) -c src/test_small_hybrid.c -o obj/test_small_hybrid.o
	$(CC) $(CFLAGS) obj/test_small_hybrid.o obj/hybrid.o -o bin/test_small_hybrid -lpthread

	$(CC) $(CFLAGS) src/test_big_mutex.c    -o bin/test_big_mutex    -lpthread
	$(CC) $(CFLAGS) src/test_big_spin.c     -o bin/test_big_spin     -lpthread

	$(CC) $(CFLAGS) -c src/test_big_hybrid.c   -o obj/test_big_hybrid.o
	$(CC) $(CFLAGS) obj/test_big_hybrid.o obj/hybrid.o -o bin/test_big_hybrid -lpthread

run:
	bin/test_small_mutex  $(RUN_SMALL)
	bin/test_small_spin   $(RUN_SMALL)
	bin/test_small_hybrid $(RUN_SMALL)

	bin/test_big_mutex    $(RUN_BIG)
	bin/test_big_spin     $(RUN_BIG)
	bin/test_big_hybrid   $(RUN_BIG)
