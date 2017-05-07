CC = gcc
CFLAGS = -W -Wall -g 
SIDE_SOURCES = p1fxns.c

all: uspsv1 uspsv2

uspsv1: uspsv1.c
	$(CC) $(CFLAGS) $(SIDE_SOURCES) $< -o uspsv1

uspsv2: uspsv2.c
	$(CC) $(CFLAGS) $(SIDE_SOURCES) $< -o uspsv2

clean:
	rm uspsv1 uspsv2
