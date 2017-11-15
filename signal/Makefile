
BIN := bin/
SRC := src/

all: build

build:
	gcc -o $(BIN)stop $(SRC)stop.c
	gcc -o $(BIN)count $(SRC)count.c
	gcc -o $(BIN)sigtest $(SRC)mysignaltest.c

clean:
	-rm $(BIN)stop $(BIN)count $(BIN)sigtest *.txt
