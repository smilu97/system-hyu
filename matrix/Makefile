
SRC := src/
CC  := gcc
INC := includes/
CFLAGS := -g -I $(INC) -lpthread

SRCS := $(SRC)main.c $(SRC)matrix.c

all: build

build: $(SRC)main.c
	$(foreach SSRC, $(SRCS),\
		$(CC) $(CFLAGS) -o $(SSRC:.c=.o) -c $(SSRC) ; \
	)
	$(CC) $(CFLAGS) -o main $(SRCS:.c=.o) -lpthread

test:
	make build
	./main matrices/A.txt matrices/B.txt 4
