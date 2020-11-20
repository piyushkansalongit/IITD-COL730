#include "sort.h"
#include "bubblesort.h"
#include "mergesort.h"
#include "quicksort.h"
#include "radixsort.h"

#include  <omp.h>
#include <algorithm>
#include <stdio.h>

void pSort::init(){
    omp_set_nested(omp_get_max_active_levels());
}

void pSort::close(){

}

void pSort::sort(pSort::dataType *data, int ndata, pSort::SortType sorter){
    printf("[DEBUG] Received %d records to be sorted\n", ndata);

    int nThreads;
    nThreads = std::min(omp_get_num_procs(), ndata);
    switch (sorter)
    {
    case pSort::QUICK:
        quicksort::sort(data, ndata, nThreads);
        break;
    case pSort::RADIX:
        radixsort::sort(data, ndata, nThreads);
        break;
    case pSort::MERGE:
        mergesort::sort(data, ndata, nThreads);
        break;
    case pSort::BUBBLE:
        bubblesort::sort(data, ndata, nThreads);
        break;
    case pSort::BEST:
        radixsort::sort(data, ndata, nThreads);
        break;
    default:
        break;
    }
}
