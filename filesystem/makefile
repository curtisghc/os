CC=gcc -std=gnu11
CFLAGS=-Wall -g

CLEANUP=rm -rf fs *.gch

all: compile

run: compile
	./fs

compile:
	$(CC) $(CFLAGS) -o fs fs.c

clean:
	$(CLEANUP)

