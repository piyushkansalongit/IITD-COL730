#!/bin/sh

##################################################################################
# Use the block of commands below for creating a binary directly on local computer
##################################################################################
# g++ -std=c++11 -fopenmp -o test -I./include -I. src/*.cpp tester.cpp
# Quick sort
# ./test 134217728 1
# ./test 268435456 1
# ./test 536870912 1
# ./test 1073741824 1
# Merge sort
# ./test 134217728 2
# ./test 268435456 2
# ./test 536870912 2
# ./test 1073741824 2
# Radix sort
# ./test 134217728 3
# ./test 268435456 3
# ./test 536870912 3
# ./test 1073741824 3
# Bubble sort
# ./test 134217728 4
# ./test 268435456 4
# ./test 536870912 4
# ./test 1073741824 4

# rm test

##################################################################################
# Use the block of commands below for creating a binary directly on hpc
##################################################################################
# module load compiler/gcc/9.1.0
# module load lib/boost/1.72.0/gnu
# Quick sort
# ./test 134217728 1
# ./test 268435456 1
# ./test 536870912 1
# ./test 1073741824 1
# Merge sort
# ./test 134217728 2
# ./test 268435456 2
# ./test 536870912 2
# ./test 1073741824 2
# Radix sort
# ./test 134217728 3
# ./test 268435456 3
# ./test 536870912 3
# ./test 1073741824 3
# Bubble sort
# ./test 134217728 4
# ./test 268435456 4
# ./test 536870912 4
# ./test 1073741824 4
# rm test


##################################################################################
# Use the block of commands below for creating a shared library using make on local
##################################################################################
LD_LIBRARY_PATH=$(pwd):$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
echo $LD_LIBRARY_PATH
make
g++ -L$(pwd) -o test tester.cpp -lpsort
# Quick sort
# ./test 134217728 1
# ./test 268435456 1
# ./test 536870912 1
# ./test 1073741824 1
# Merge sort
# ./test 134217728 2
# ./test 268435456 2
# ./test 536870912 2
# ./test 1073741824 2
# Radix sort
./test 134217728 3
# ./test 268435456 3
# ./test 536870912 3
# ./test 1073741824 3
# Bubble sort
# ./test 134217728 4
# ./test 268435456 4
# ./test 536870912 4
# ./test 1073741824 4
rm test libpsort.so


##################################################################################
# Use the block of commands below for creating a shared library using make on hpc
##################################################################################
# module load compiler/gcc/9.1.0
# module load lib/boost/1.72.0/gnu
# LD_LIBRARY_PATH=$(pwd):$LD_LIBRARY_PATH
# export LD_LIBRARY_PATH
# echo $LD_LIBRARY_PATH
# make
# g++ -L$(pwd) -o test tester.cpp -lpsort
# Quick sort
# ./test 134217728 1
# ./test 268435456 1
# ./test 536870912 1
# ./test 1073741824 1
# Merge sort
# ./test 134217728 2
# ./test 268435456 2
# ./test 536870912 2
# ./test 1073741824 2
# Radix sort
# ./test 134217728 3
# ./test 268435456 3
# ./test 536870912 3
# ./test 1073741824 3
# Bubble sort
# ./test 134217728 4
# ./test 268435456 4
# ./test 536870912 4
# ./test 1073741824 4
# rm test libpsort.so