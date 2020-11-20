#include "sort.h"

#include <ctime>
#include <time.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

#define LOADSIZE 4

const int MINCOUNT = 1 << 20;
const int MAXCOUNT = MINCOUNT;

inline int randomCount()
{
   return MINCOUNT + rand() % (MAXCOUNT - MINCOUNT + 1);
}

char randomChar()
{
   std::string str = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
   int pos = rand() % str.size();
   return str[pos];
}

pSort::dataType *generate(long num_of_records)
{
   //Creating an array of structure of type "dataType" declared in sort.h
   pSort::dataType *data = new pSort::dataType[num_of_records];

   for (int i = 0; i < num_of_records; i++)
   {
      data[i].key = rand();
      for (int j = 0; j < LOADSIZE; j++)
         data[i].payload[j] = randomChar();
      // printf("(%d: %d %c%c%c%c)", i, data[i].key, data[i].payload[0], data[i].payload[1], data[i].payload[2], data[i].payload[3]);
   }
   // printf("\n");
   return data;
}

bool compare (pSort::dataType a, pSort::dataType b)
{
  return a.key < b.key;
}

bool check_sorted(pSort::dataType *test_data, int num_of_records, pSort::dataType *copy_test_data)
{
   // printf("Sorting copy of the test data\n");
   // std::sort(copy_test_data, copy_test_data+num_of_records, compare);
   // for (int i = 1; i < num_of_records; i++){
   //    if (test_data[i].key != copy_test_data[i].key){
   //       std::cerr << ": Data has not been properly sorted" << std::endl;
   //       return false;
   //    }
   // }
   return true;
}

void runExperiment(pSort sorter, int num_of_records = 0, pSort::SortType type = pSort::BEST, bool term = true)
{

   //Creating an array of structure of type "dataType" declared in sort.h
   pSort::dataType *test_data = generate(num_of_records);
   pSort::dataType *copy_test_data = new pSort::dataType[num_of_records];
   // memcpy(copy_test_data, test_data, sizeof(pSort::dataType) * num_of_records);
   /*Calling functions defined in pSort library to sort records stored in test_data[]*/
   time_t begin, end;
   time(&begin);
   sorter.sort(test_data, num_of_records, type);
   time(&end);

   double timetaken = difftime(end, begin);
   
   if (check_sorted(test_data, num_of_records, copy_test_data))
   {
      std::cout << ": Successful in " << timetaken << std::endl;
   }
   else{
      std::cout << ": Failed in " << timetaken << std::endl;
   }
   if (term)
      delete test_data;
}

int main(int argc, char* argv[])
{
   pid_t pid = getpid();
   srand((unsigned int)time(NULL)+pid*pid);

   pSort sorter;

   //Calling your init() to set up MPI
   sorter.init();

   /*=================================================================*/
   runExperiment(sorter, atoi(argv[1]), static_cast<pSort::SortType>(atoi(argv[2]))); // For example

   //Calling your close() to finalize MPI
   sorter.close();

   return 0;

}