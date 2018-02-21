CC=gcc
CFLAGS=-Wall -g

CLEANUP=rm -rf myshell *.gch

all: compile

run: compile
	./myshell

compile:
	$(CC) $(CFLAGS) -o myshell myshell.c

clean:
	$(CLEANUP)

