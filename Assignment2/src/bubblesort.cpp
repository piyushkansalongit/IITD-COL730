#include "bubblesort.h"
#include "sort.h"
#include "util.h"
#include <omp.h>
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include "boost/format.hpp"
using boost::format;

void serialMerge(int ID, pSort::dataType *data, int ndata, pSort::dataType *buffer, int buffer_size)
{
    // printf("%d: [DEBUG] I'll merge and accept the %d smaller values\n", ID, ndata);
    pSort::dataType *temp = new pSort::dataType[ndata+buffer_size];
    int i=0, j=0, k=0;
    for (; i < ndata && j < buffer_size && k < ndata+buffer_size; k++)
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
    while(k!=ndata+buffer_size && i!=ndata){
        memcpy(&temp[k], &data[i], sizeof(pSort::dataType));
        i++;
        k++;
    }
    while(k!=ndata+buffer_size && j!=buffer_size){
        memcpy(&temp[k], &buffer[j], sizeof(pSort::dataType));
        j++;
        k++;
    }
    memcpy(data, temp, sizeof(pSort::dataType) * (ndata+buffer_size));
    delete[] temp;
    return;
}

void
bubblesort::sort(pSort::dataType *data, int ndata, int nThreads){
    printf("[INFO] Number of processors using for bubble sort = %d\n", nThreads);

    int beg_indices[nThreads];
    int end_indices[nThreads];
    int sizes[nThreads];

    #pragma omp parallel default(none) shared(data, ndata, beg_indices, end_indices, sizes, nThreads) num_threads(nThreads)
    {
        int ID, beg_ind, end_ind, size;

        ID =  omp_get_thread_num();

        beg_ind = ID * (ndata/nThreads);
        end_ind = std::min(ndata, (ID + 1) * (ndata/nThreads));
        if(ID == nThreads-1)
            end_ind += ndata%nThreads;
        size = end_ind - beg_ind;

        beg_indices[ID] = beg_ind;
        end_indices[ID] = end_ind;
        sizes[ID] = size;

        util::printDebugMsg(ID, (format("Received %d records from %d to %d") % size % beg_ind % end_ind).str());

        // util::printdata(ID, "Received data block", data+beg_ind, size);
        util::serialRadixSort(ID, data+beg_ind, size);
        // util::printdata(ID, "Sorted my block", data+beg_ind, size);

        bool sender = ID % 2 == 0 ? true : false;
        for (int i = 0; i < nThreads; i++)
        {
            #pragma omp barrier
            // cmpxchg your block with the processor with next higher rank if there's one.
            if (sender)
            {
                // Sending cycle (Last processor will never send first)
                if (ID + 1 != nThreads)
                {
                    // Merge with the thread in front of you
                    serialMerge(ID, data+beg_ind, size, data+beg_indices[ID+1], sizes[ID+1]);
                }
            }
            else
            {
                // Receiving cycle (First processor will never receive first!)
                if (ID != 0)
                {
                    // Wait for the thread behind you to merge
                }
            }
            sender = !sender;
        }

    }
    
}