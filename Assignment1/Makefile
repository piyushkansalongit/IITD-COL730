SHELL = /bin/sh
CC = mpic++
CFLAGS = -Wall -g -shared -fPIC
SOURCES = $(SHELL echo src/*.cpp)

libpsort: $(SOURCES)
	$(CC) $(CFLAGS) -o libpsort.so -I. -I ./include ./src/*.cpp

