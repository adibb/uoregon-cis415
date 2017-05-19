CFLAGS=-W -Wall -g -pthread -std=c11

all: message message2

message: lab7.c
	gcc $(CFLAGS) -o lab7 $^

message2: lab7_solution.c
	gcc $(CFLAGS) -o lab7_solution $^

clean:
	rm -f lab7 lab7_solution
