
all: build

build:
	gcc -I includes/ -o src/matrix.o -c src/matrix.c
	gcc -I includes/ -o src/main.o -c src/main.c
	gcc -o main src/main.o src/matrix.o -lpthread