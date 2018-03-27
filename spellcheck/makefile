CC=gcc -std=gnu11
CFLAGS=-Wall -g -lpthread

CLEANUP=rm -rf checker *.gch

all: compile

run: compile
	./checker

compile:
	$(CC) $(CFLAGS) -o checker checker.c

clean:
	$(CLEANUP)

