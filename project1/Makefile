CC = gcc
CFLAGS = -W -Wall -g
SIDE_SOURCES = p1fxns.c

all: uspsv1 uspsv2 uspsv3

uspsv1: uspsv1.c $(SIDE_SOURCES)
	$(CC) $(CFLAGS) $(SIDE_SOURCES) $< -o uspsv1

uspsv2: uspsv2.c $(SIDE_SOURCES)
	$(CC) $(CFLAGS) $(SIDE_SOURCES) $< -o uspsv2

uspsv3: uspsv3.c $(SIDE_SOURCES)
	$(CC) $(CFLAGS) $(SIDE_SOURCES) $< -o uspsv3

clean:
	rm uspsv1 uspsv2 uspsv3
