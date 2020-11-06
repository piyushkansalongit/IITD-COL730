#pragma once

#include "sort.h"
#include <utility>
#include <mpi.h>

#define TAG_CLIENT_DATA_SIZE 10
#define TAG_CLIENT_DATA 11
#define TAG_SERVER_DATA_SIZE 12
#define TAG_SERVER_DATA 13

namespace bubblesort
{
    std::pair<pSort::dataType*, int> parallelMergeSort(int ID, int numProcs, MPI_Datatype mpi_dataType, pSort::dataType *data, int ndata);
    void serialMergeAndSelectFirst(int ID, pSort::dataType *data, int ndata, pSort::dataType *buffer, int buffer_size);
    void serialMergeAndSelectSecond(int ID, pSort::dataType *data, int ndata, pSort::dataType *buffer, int buffer_size);
} // namespace bubblesort
