#include "sort.h"
#include "bubblesort.h"
#include "mergesort.h"
#include "quicksort.h"
#include "radixsort.h"

#include  <omp.h>
#include <stdio.h>

void pSort::init(){

}

void pSort::close(){

}

void pSort::sort(pSort::dataType *data, int ndata, pSort::SortType sorter){
    printf("[DEBUG] Received %d records to be sorted\n", ndata);

    switch (sorter)
    {
    case pSort::QUICK:
        quicksort::sort(data, ndata);
        break;
    case pSort::RADIX:
        radixsort::sort(data, ndata, 8);
        break;
    case pSort::MERGE:
        mergesort::sort(data, ndata);
        break;
    case pSort::BUBBLE:
        bubblesort::sort(data, ndata);
        break;
    default:
        break;
    }
}
