CC=gcc -std=gnu11
CFLAGS=-Wall -g -pthread -lpthread

CLEANUP=rm -rf checker *.gch

all: compile

run: compile
	./checker

compile:
	$(CC) $(CFLAGS) -o checker checker.c queue.h

clean:
	$(CLEANUP)

