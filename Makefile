
CFLAGS=-std=c99 -D _BSD_SOURCE -D _POSIX_C_SOURCE=2
CC=gcc

all: target

target: connective_test

connective_test: connective.o net.o array.o connective_test.o

clean:
	@rm -fv *.o
