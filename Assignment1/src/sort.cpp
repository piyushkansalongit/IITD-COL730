#include "sort.h"
#include "util.h"
#include "quicksort.h"
#include "mergesort.h"
#include "radixsort.h"

#include <mpi.h>
#include <iostream>
#include <utility>

using namespace std;

void pSort::init()
{

    /**
     * Set up MPI for this process or main thread
    */
    MPI_Init(0, NULL);
}

void pSort::close()
{

    /**
     * The thread that called MPI_Init needs to call MPI_Finalize
    */
    MPI_Finalize();
}

void pSort::sort(pSort::dataType *data, int ndata, pSort::SortType sorter)
{

    int numProcs, ID;
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
    MPI_Comm_rank(MPI_COMM_WORLD, &ID);
    // printf("[DEBUG] %d : Received %d datapoints\n", ID, ndata);

    /**
     * Create your custom MPI Data Type
    */
    int count = 2;
    int array_of_blocklengths[] = {1, LOADSIZE};
    MPI_Aint array_of_displacements[] = {offsetof(dataType, key), offsetof(dataType, payload)};
    MPI_Datatype array_of_types[] = {MPI_INT, MPI_CHAR};
    MPI_Datatype tmp_type, mpi_dataType;
    MPI_Aint lb, extent;
    MPI_Type_create_struct(count, array_of_blocklengths, array_of_displacements, array_of_types, &tmp_type);
    MPI_Type_get_extent(tmp_type, &lb, &extent);
    MPI_Type_create_resized(tmp_type, lb, extent, &mpi_dataType);
    MPI_Type_commit(&mpi_dataType);

    printf("%d: [INGO] I've been given %d elements to sort\n", ID, ndata);

    pair<pSort::dataType *, int> sorted_data;
    switch (sorter)
    {
    case pSort::QUICK:
    {
        sorted_data = quicksort::parallelQuickSort(ID, numProcs, mpi_dataType, data, ndata, 0, numProcs - 1, false, true);
        break;
    }
    case pSort::RADIX:
        sorted_data = radixsort::parallelRadixSort(ID, numProcs, mpi_dataType, data, ndata, 8, 0, false);
        break;
    case pSort::MERGE:
    {
        sorted_data = bubblesort::parallelMergeSort(ID, numProcs, mpi_dataType, data, ndata);
        break;
    }
    case pSort::BEST:
    {
        // sorted_data = quicksort::parallelQuickSort(ID, numProcs, mpi_dataType, data, ndata, 0, numProcs - 1, false, true);
        sorted_data = radixsort::parallelRadixSort(ID, numProcs, mpi_dataType, data, ndata, 8, 0, false);
        break;
    }
    default:
        break;
    }

    if (sorted_data.first == data)
        return;

    for (int i = 1; i < sorted_data.second; i++)
    {
        if (sorted_data.first[i].key < sorted_data.first[i - 1].key)
        {
            printf("%d: [ERROR] Data received from collective routine not sorted\n", ID);
            return;
        }
    }
    printf("%d: [DEBUG] I brought back %d sorted records from the collective function\n", ID, sorted_data.second);

    int lag = 0, lag_offset = 0, diff = 0;
    pSort::dataType *buffer, *lag_buffer;
    if (ID != 0)
    {
        // This process will get to participate in 1 backward replacements
        util::MPI_recv(&diff, 1, MPI_INT, ID - 1, 0);
        if (diff != 0)
        {
            if (diff > 0)
            {
                buffer = new pSort::dataType[diff];
                // printf("%d: [DEBUG] Receiving %d records from %d\n", ID, diff, ID-1);
                util::MPI_recv(buffer, diff, mpi_dataType, ID - 1, 0);

                // You will need to merge it with your chunk
                pSort::dataType *updated_data = new pSort::dataType[diff + sorted_data.second];
                memcpy(updated_data, buffer, diff * sizeof(pSort::dataType));
                memcpy(updated_data + diff, sorted_data.first, sorted_data.second * sizeof(pSort::dataType));
                delete[] sorted_data.first;
                delete[] buffer;
                sorted_data.first = updated_data;
                sorted_data.second += diff;
            }
            else if (diff < 0)
            {
                diff *= (-1);

                // Give up some of your chunk
                if (diff <= sorted_data.second)
                {
                    // printf("%d: [DEBUG] Sending %d records to %d\n", ID, diff, ID-1);
                    util::MPI_send(sorted_data.first, diff, mpi_dataType, ID - 1, 0);
                    sorted_data.first += diff;
                    sorted_data.second -= diff;
                }
                else
                {
                    // You will have to request first
                    lag_buffer = new pSort::dataType[diff];
                    memcpy(lag_buffer, sorted_data.first, sorted_data.second * sizeof(pSort::dataType));
                    lag_offset = sorted_data.second;
                    lag = (diff - sorted_data.second);
                    delete[] sorted_data.first;
                    sorted_data.second = 0;
                }
            }
            diff = 0;
        }
    }

    // We will need to do a final redistribution
    if (ID + 1 != numProcs)
    {
        // This process will get to participate in 1 forward replacements
        diff = (sorted_data.second - ndata - lag);
        util::MPI_send(&diff, 1, MPI_INT, ID + 1, 0);
        if (diff != 0)
        {
            if (diff > 0)
            {
                // Give up your extra chunk
                // printf("%d: [DEBUG] Giving up %d records to %d\n", ID, diff, ID+1);
                util::MPI_send(sorted_data.first + ndata, diff, mpi_dataType, ID + 1, 0);
            }
            else if (diff < 0)
            {
                diff *= (-1);

                // Ask ID+1 to send you the difference
                buffer = new pSort::dataType[diff];
                // printf("%d: [DEBUG] Receiving %d records from %d\n", ID, diff, ID+1);
                util::MPI_recv(buffer, diff, mpi_dataType, ID + 1, 0);
                if (lag == 0)
                {
                    if (sorted_data.second != 0)
                    {
                        mempcpy(data, sorted_data.first, sorted_data.second * sizeof(pSort::dataType));
                    }
                    memcpy(data + sorted_data.second, buffer, diff * sizeof(pSort::dataType));
                    // delete[] sorted_data.first;
                }
                else
                {
                    memcpy(lag_buffer + lag_offset, buffer, lag * sizeof(pSort::dataType));
                    // printf("%d: [DEBUG] Giving up %d records to %d\n", ID, lag_offset+lag, ID-1);
                    util::MPI_send(lag_buffer, lag_offset + lag, mpi_dataType, ID - 1, 0);
                    memcpy(data, buffer + lag, ndata * sizeof(pSort::dataType));
                    // delete[] lag_buffer;
                }

                // delete[] buffer;
                return;
            }
        }
    }

    // Copy the sorted array to the original location
    memcpy(data, sorted_data.first, ndata * sizeof(pSort::dataType));
    // delete[] sorted_data.first;
    return;
}
