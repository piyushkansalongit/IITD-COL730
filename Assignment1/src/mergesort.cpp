#include "mergesort.h"

#include "util.h"

#include <stdio.h>

using namespace util;
using namespace std;

void bubblesort::serialMergeAndSelectFirst(int ID, pSort::dataType *data, int ndata, pSort::dataType *buffer, int buffer_size)
{
    // printf("%d: [DEBUG] I'll merge and accept the %d smaller values\n", ID, ndata);
    pSort::dataType *temp = new pSort::dataType[ndata];
    int i=0, j=0, k=0;
    for (; i < ndata && j < buffer_size && k < ndata; k++)
    {
        if (data[i].key < buffer[j].key)
        {
            memcpy(&temp[k], &data[i], sizeof(pSort::dataType));
            i++;
        }
        else
        {
            memcpy(&temp[k], &buffer[j], sizeof(pSort::dataType));
            j++;
        }
    }
    while(k!=ndata && i!=ndata){
        memcpy(&temp[k], &data[i], sizeof(pSort::dataType));
        i++;
        k++;
    }
    while(k!=ndata && j!=buffer_size){
        memcpy(&temp[k], &buffer[j], sizeof(pSort::dataType));
        j++;
        k++;
    }
    memcpy(data, temp, sizeof(pSort::dataType) * ndata);
    delete[] temp;
    return;
}

void bubblesort::serialMergeAndSelectSecond(int ID, pSort::dataType *data, int ndata, pSort::dataType *buffer, int buffer_size)
{
    // printf("%d: [DEBUG] I'll merge and accept the %d larger values\n", ID, ndata);
    pSort::dataType *temp = new pSort::dataType[ndata];
    int i = ndata - 1, j = buffer_size - 1, k = ndata - 1;
    for (; k >= 0; k--)
    {
        if (data[i].key > buffer[j].key)
        {
            memcpy(&temp[k], &data[i], sizeof(pSort::dataType));
            i--;
        }
        else{
            memcpy(&temp[k], &buffer[j], sizeof(pSort::dataType));
            j--;
        }
    }
    while(k>=0 && i>=0){
        memcpy(&temp[k], &data[i], sizeof(pSort::dataType));
        i--;
        k--;
    }
    while(k>=0 && j>=0){
        memcpy(&temp[k], &buffer[j], sizeof(pSort::dataType));
        j--;
        k--;
    }
    memcpy(data, temp, sizeof(pSort::dataType) * ndata);
    delete[] temp;
    return;
}

pair<pSort::dataType*, int>
bubblesort::parallelMergeSort(int ID, int numProcs, MPI_Datatype mpi_dataType, pSort::dataType *data, int ndata)
{

    // Sort the partition
    serialQuickSort(ID, data, ndata);

    // Begin Odd/Even Sorting
    bool sender = ID % 2 == 0 ? true : false;
    MPI_Status status;

    pSort::dataType *forward_buffer, *backward_buffer;
    int forward_buffer_size, backward_buffer_size;
    bool forward_flag = true, backward_flag = true;

    for (int i = 0; i < numProcs; i++)
    {
        // cmpxchg your block with the processor with next higher rank if there's one.
        if (sender)
        {
            // Sending cycle (Last processor will never send first)
            if (ID + 1 != numProcs)
            {
                if (forward_flag)
                {
                    // Send your data size
                    MPI_send(&ndata, 1, MPI_INT, ID + 1, TAG_CLIENT_DATA_SIZE);
                }

                // Send your data
                MPI_send(data, ndata, mpi_dataType, ID + 1, TAG_CLIENT_DATA);

                if (forward_flag)
                {
                    // Get the incoming buffer size
                    MPI_recv(&forward_buffer_size, 1, MPI_INT, ID + 1, TAG_SERVER_DATA_SIZE);
                    // Allocate the buffer
                    forward_buffer = new pSort::dataType[forward_buffer_size];
                }

                // Get the buffer
                MPI_recv(forward_buffer, forward_buffer_size, mpi_dataType, ID + 1, TAG_SERVER_DATA);

                // Merge both the lists and keep the first partitionSize elements.
                serialMergeAndSelectFirst(ID, data, ndata, forward_buffer, forward_buffer_size);

                forward_flag = false;
            }
        }
        else
        {
            // Receiving cycle (First processor will never receive first!)
            if (ID != 0)
            {
                if (backward_flag)
                {
                    // Get the incoming buffer size
                    MPI_recv(&backward_buffer_size, 1, MPI_INT, ID - 1, TAG_CLIENT_DATA_SIZE);
                    // Allocate the buffer
                    backward_buffer = new pSort::dataType[backward_buffer_size];
                }

                MPI_recv(backward_buffer, backward_buffer_size, mpi_dataType, ID - 1, TAG_CLIENT_DATA);

                if (backward_flag)
                {
                    // Send your data size
                    MPI_send(&ndata, 1, MPI_INT, ID - 1, TAG_SERVER_DATA_SIZE);
                }

                // Send your data
                MPI_send(data, ndata, mpi_dataType, ID - 1, TAG_SERVER_DATA);
                // printf("%d: [DEBUG] I sent  my data...\n", ID);

                // Merge both the lists and keep the last partitionSize elements.
                serialMergeAndSelectSecond(ID, data, ndata, backward_buffer, backward_buffer_size);

                backward_flag = false;
            }
        }
        sender = !sender;
    }
    if(!forward_flag)
        delete[] forward_buffer;
    if(!backward_flag)
        delete[] backward_buffer;

    return pair<pSort::dataType*, int>(data, ndata);
};