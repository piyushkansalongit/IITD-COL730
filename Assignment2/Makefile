SHELL = /bin/sh
CC = g++
CFLAGS = -std=c++11 -fopenmp -shared -fPIC
SOURCES = $(SHELL echo src/*.cpp)

libpsort: $(SOURCES)
	$(CC) $(CFLAGS) -o libpsort.so -I. -I ./include ./src/*cpp