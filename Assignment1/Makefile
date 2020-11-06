SHELL = /bin/sh
CC = mpic++
CFLAGS = -Wall -g
SOURCES = $(SHELL echo src/*.cpp)

libpsort: $(SOURCES)
	$(CC) -shared -fPIC -o libpsort.so -I. -I ./include ./src/*.cpp

