g++ -o test -fopenmp -I./include -I. src/*.cpp tester.cpp
./test
rm test