#include "radixsort.h"
#include "util.h"

#include <vector>

using namespace util;
using namespace std;

pair<pSort::dataType *, int>
radixsort::parallelRadixSort(int ID, int numProcs, MPI_Datatype mpi_dataType, pSort::dataType *data, int ndata, int b, int digit, bool del)
{
    // printf("%d: [DEBUG] Entering parallel sort with ndata=%d and digit=%d\n", ID, ndata, digit);
    // printdata(ID, data, ndata);

    if (numProcs == 1)
    {
        serialRadixSort(ID, data, ndata, b);
        return pair<pSort::dataType *, int>(data, ndata);
    }

    int num_buckets = 1 << b;
    if (digit == 32 / b)
    {
        return pair<pSort::dataType *, int>(data, ndata);
    }

    pSort::dataType *local_buckets[num_buckets];
    int local_counts[numProcs][num_buckets];
    memset(local_counts, 0, sizeof(local_counts));

    // Determine the counts first
    for (int i = 0; i < ndata; i++)
    {
        unsigned int key = data[i].key;
        key = key << (32 / b - 1 - digit) * b;
        key = key >> (32 / b - 1) * b;
        local_counts[ID][key] += 1;
    }
    // Do the memory allocation for the buckets
    for (int bucket_idx = 0; bucket_idx < num_buckets; bucket_idx++)
    {
        local_buckets[bucket_idx] = new pSort::dataType[local_counts[ID][bucket_idx]];
        memset(local_buckets[bucket_idx], 0, sizeof(pSort::dataType) * local_counts[ID][bucket_idx]);
    }
    // Fill the buckets
    for (int i = 0; i < ndata; i++)
    {
        unsigned int key = data[i].key;
        key = key << (32 / b - 1 - digit) * b;
        key = key >> (32 / b - 1) * b;
        memcpy(local_buckets[key], &data[i], sizeof(pSort::dataType));
        local_buckets[key]++;
    }
    // Reset the pointers to the buckets
    for (int i = 0; i < num_buckets; i++)
    {
        local_buckets[i] -= local_counts[ID][i];
    }

    for (int i = 0; i < numProcs; i++)
    {
        MPI_bcast(&local_counts[i], num_buckets, MPI_INT, i);
    }
    // printf("%d: [DEBUG] My local count array is: ", ID);
    // for (int i = 0; i < num_buckets; i++)
    // {
    //     printf("%d:%d ", i, local_counts[ID][i]);
    // }
    // printf("\n");

    int global_counts[num_buckets];
    memset(global_counts, 0, sizeof(global_counts));
    for (int id = 0; id < numProcs; id++)
    {
        for (int i = 0; i < num_buckets; i++)
        {
            global_counts[i] += local_counts[id][i];
        }
    }
    // printf("%d: [DEBUG] My global count array is: ", ID);
    // for (int i = 0; i < num_buckets; i++)
    // {
    //     printf("%d:%d ", i, global_counts[i]);
    // }
    // printf("\n");


    // Identify your start and end index in the bucket array
    int glob_beg_idx, glob_end_idx, allotted_num_buckets;
    glob_beg_idx = ID * (num_buckets / numProcs);
    glob_end_idx = min(num_buckets, (ID + 1) * (num_buckets / numProcs));
    if (ID == numProcs - 1)
        glob_end_idx += num_buckets % numProcs;
    allotted_num_buckets = glob_end_idx - glob_beg_idx;
    // printf("%d: [DEBUG] I'll be keeping %d buckets from %d to %d post this iteration\n", ID, allotted_num_buckets, glob_beg_idx, glob_end_idx);


    // Allocate space to store incoming set of buckets
    int new_data_size = 0;
    int offsets[allotted_num_buckets];
    memset(offsets, 0, sizeof(offsets));
    for (int bucket_idx = glob_beg_idx; bucket_idx < glob_end_idx; bucket_idx++)
    {
        new_data_size += global_counts[bucket_idx];
        if (bucket_idx != glob_beg_idx)
            offsets[bucket_idx - glob_beg_idx] = global_counts[bucket_idx - 1];
    }


    // Convert the offsets to prefix sum
    for (int bucket_idx = glob_beg_idx + 1; bucket_idx < glob_end_idx; bucket_idx++)
    {
        offsets[bucket_idx - glob_beg_idx] += offsets[bucket_idx - glob_beg_idx - 1];
    }


    pSort::dataType *new_data = new pSort::dataType[new_data_size];
    memset(new_data, 0, sizeof(pSort::dataType) * new_data_size);

    // printf("%d: [DEBUG] My new data size will be %d\n", ID, new_data_size);
    // printf("%d: [DEBUG] My offsets are: ", ID);
    // for (int bucket_idx = glob_beg_idx; bucket_idx < glob_end_idx; bucket_idx++)
    // {
    //     printf("%d:%d ", bucket_idx, offsets[bucket_idx - glob_beg_idx]);
    // }
    // printf("\n");

    // Send your local buckets in the wild
    MPI_Request requests[1024];
    int request_count = 0;
    for (int bucket_idx = 0; bucket_idx < num_buckets; bucket_idx++)
    {
        // Find out who is responsible for this particular bucket
        int dest = min(numProcs - 1, (bucket_idx / (num_buckets / numProcs)));
        // printf("%d: [DEBUG] Found rank %d responsible for bucket %d\n", ID, dest, bucket_idx);

        if (local_counts[ID][bucket_idx] > 0)
        {
            if (dest != ID)
            {
                MPI_isend(local_buckets[bucket_idx], local_counts[ID][bucket_idx], mpi_dataType, dest, bucket_idx, &requests[request_count++]);
                // MPI_send(&temp, local_counts[ID][bucket_idx], mpi_dataType, dest, bucket_idx);
                // printf("%d [DEBUG] Sending %d entries from bucket %d to %d\n", ID, local_counts[ID][bucket_idx], bucket_idx, dest);
            }
        }
    }

    // printf("\n%d: [DEBUG] Starting to receive with ndata=%d and digit=%d\n", ID, ndata, digit);

    // Receive all the relevant buckets
    for (int src = 0; src < numProcs; src++)
    {
        for (int bucket_idx = glob_beg_idx; bucket_idx < glob_end_idx; bucket_idx++)
        {
            if (local_counts[src][bucket_idx] > 0)
            {
                if (src == ID)
                {
                    memcpy(&new_data[offsets[bucket_idx - glob_beg_idx]], local_buckets[bucket_idx], sizeof(pSort::dataType) * local_counts[ID][bucket_idx]);
                    offsets[bucket_idx - glob_beg_idx] = (offsets[bucket_idx - glob_beg_idx] + local_counts[ID][bucket_idx]);
                }
                else
                {
                    MPI_irecv(&new_data[offsets[bucket_idx - glob_beg_idx]], local_counts[src][bucket_idx], mpi_dataType, src, bucket_idx, &requests[request_count++]);
                    // MPI_recv(&new_data[offsets[bucket_idx - glob_beg_idx]], local_counts[src][bucket_idx], mpi_dataType, src, bucket_idx);
                    // printf("%d [DEBUG] Receiving %d entries in bucket %d from %d\n", ID, local_counts[src][bucket_idx], bucket_idx, src);
                    offsets[bucket_idx - glob_beg_idx] = (offsets[bucket_idx - glob_beg_idx] + local_counts[src][bucket_idx]);
                }
            }
        }
    }

    // MPI_barrier
    if (request_count > 0)
    {
        // printf("%d: [DEBUG] Start waiting\n", ID);
        assert(MPI_Waitall(request_count, requests, NULL) == MPI_SUCCESS);
        // printf("%d: [DEBUG] Done waiting\n", ID);
    }

    // Run the counting sort for the received buckets again
    serialCountingSort(ID, new_data, new_data_size, b, digit);

    // Free the buckets
    for (int bucket_idx = 0; bucket_idx < num_buckets; bucket_idx++)
    {
        delete[] local_buckets[bucket_idx];
    }
    if (del)
        delete[] data;
    digit += 1;
    return radixsort::parallelRadixSort(ID, numProcs, mpi_dataType, new_data, new_data_size, b, digit, true);
}
