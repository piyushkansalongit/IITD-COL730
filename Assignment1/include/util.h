#pragma once

#include "sort.h"

#include <mpi.h>
#include <iostream>
#include <assert.h>
#include <string.h>

namespace util
{
    void printdata(int ID, pSort::dataType *data, int ndata);

    void MPI_send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag);
    void MPI_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag);
    void MPI_isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Request *request);
    void MPI_irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Request *request);
    void MPI_bcast(void *buf, int count, MPI_Datatype datatype, int root);
    
    void swap(int ID, pSort::dataType *data, int from, int to);

    int partition(int ID, pSort::dataType *data, int ndata, pSort::dataType pivot);

    pSort::dataType findLocalMedian(int ID, pSort::dataType *data, int ndata);
    pSort::dataType findLocalMedian(int ID, pSort::dataType *data, int ndata, int remaining);
    void findMedianProposal(int ID, pSort::dataType *data, int start, int end);
    int findGroupMedian(int ID, pSort::dataType *data, int start, int end);
    pSort::dataType* serialQuickSort(int ID, pSort::dataType *data, int ndata);
    pSort::dataType* serialCountingSort(int ID, pSort::dataType *data, int ndata, int b, int digit);
    pSort::dataType* serialRadixSort(int ID, pSort::dataType *data, int ndata, int b);
}