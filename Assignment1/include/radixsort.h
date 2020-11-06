#pragma once

#include "sort.h"
#include <utility>
#include <mpi.h>

namespace radixsort
{
    std::pair<pSort::dataType*, int>
    parallelRadixSort(int ID, int numProcs, MPI_Datatype mpi_dataType, pSort::dataType *data,  int ndata, int b, int digit, bool del);
}