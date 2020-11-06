#pragma once

#define LOADSIZE 4

class pSort {

public:
   typedef enum {
   	BEST,
   	QUICK,
   	MERGE,
   	RADIX
   }  SortType;

   typedef struct {
      int key;
      char payload[LOADSIZE];
   } dataType;

   void init();
   void close();
   void sort(dataType *data, int ndata, SortType sorter=BEST);
};