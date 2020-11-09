#include "quicksort.h"
#include <omp.h>
#include <stdio.h>
#include "sort.h"

using namespace quicksort;

void
quicksort::sort(pSort::dataType *data, int ndata){

    int nThreads;
    // nThreads = std::min(omp_get_max_threads(), ndata);
    nThreads = 6;
    omp_set_num_threads(nThreads);
    printf("[INFO] Number of processors using for quick sort = %d\n", nThreads);

    #pragma omp parallel default(none) shared(data, ndata, nThreads)
    {
        
    }


}