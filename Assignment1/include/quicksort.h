#pragma once

#include "sort.h"
#include <utility>
#include <mpi.h>

#define TAG_LEFT_PARTITION_SIZE 11
#define TAG_RIGHT_PARTITION_SIZE 12
#define TAG_REDISTRIBUTION_INFO 13
#define TAG_LOCAL_MEDIAN 14
#define TAG_GLOBAL_MEDIAN 15
#define TAG_SMALLER_THAN_RECORDS 16
#define TAG_GREATER_THAN_RECORDS 17
#define TAG_HANDSHAKING 18

namespace quicksort
{
    pSort::dataType
    findGlobalMedian(int ID, int numProcs, MPI_Datatype mpi_dataType, pSort::dataType *data, int ndata, int begin, int end);
    
    std::pair<pSort::dataType*, int>
    redistribute(int ID, int numProcs, pSort::dataType *data, MPI_Datatype mpi_dataType, int left_partition_size, int right_partitionn_size, int begin, int end, bool del, bool best);
    
    std::pair<pSort::dataType*, int>
    parallelQuickSort(int ID, int numProcs, MPI_Datatype mpi_dataType, pSort::dataType *data, int ndata, int begin, int end, bool del, bool best);
} // namespace quicksort
