#ifndef RADIXSORT_H
#define RADIXSORT_H

#include "sort.h"

namespace radixsort{
    void sort(pSort::dataType *data, int ndata, int nThreads, int b=8);
}

#endif