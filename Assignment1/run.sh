#!/bin/sh

##########################################################################
# Use the block of commands below for creating a binary directly
##########################################################################
# mpic++ -o test -I. -I./include ./src/*.cpp *.cpp
# /usr/bin/time -v mpirun --use-hwthread-cpus -np  $1 ./test
# rm test

##########################################################################
# Use the block of commands below for creating a shared library directly
##########################################################################
# LD_LIBRARY_PATH=$(pwd):$LD_LIBRARY_PATH
# export LD_LIBRARY_PATH
# echo $LD_LIBRARY_PATH
# mpic++ -shared -fPIC -o libpsort.so -I./include -I. ./src/*.cpp
# mpic++ -L$(pwd) -o test tester.cpp -lpsort
# mpirun -np 4 ./test
# rm test libpsort.so

##########################################################################
# Use the block of commands below for creating a shared library using make
##########################################################################
LD_LIBRARY_PATH=$(pwd):$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
echo $LD_LIBRARY_PATH
make
mpic++ -L$(pwd) -o test tester.cpp -lpsort
mpirun -np $1 ./test
rm test libpsort.so