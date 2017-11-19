
CC := nvcc
INC := includes/
SRC := src/
CFLAGS := -I $(INC)
SRCS := $(SRC)matrix.cu $(SRC)main.cu

TESTMAT := matrices/A.txt matrices/B.txt
TESTMAT_BIG := matrices/sample1.txt matrices/sample2.txt

all: build

build: $(SRCS)
	$(foreach SSRC, $(SRCS), $(CC) $(CFLAGS) -c $(SSRC) -o $(SSRC:.cu=.o);)
	$(CC) -o main $(SRCS:.cu=.o)

test: main $(TESTMAT)
	./main $(TESTMAT)

test_big: main $(TESTMAT_BIG)
	./main $(TESTMAT_BIG)

