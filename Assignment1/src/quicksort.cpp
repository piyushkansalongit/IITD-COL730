#include "quicksort.h"

#include "util.h"

#include <iostream>
#include <assert.h>
#include <math.h>

using namespace util;
using namespace std;

pSort::dataType
quicksort::findGlobalMedian(int ID, int numProcs, MPI_Datatype mpi_dataType, pSort::dataType *data, int ndata, int begin, int end)
{
    /**
     * Find the median of the data
    */

    // Use the below line if you want to propose your median as the global median
    // pSort::dataType median = findLocalMedian(ID, data, ndata);

    // Use the below line if you just want to send a random entry to the root
    pSort::dataType median = *data;

    // printf("%d: [DEBUG] Found local median to be the record: (%d:%s)\n", ID, median.key, median.payload);

    MPI_Status status;
    if (ID == begin)
    {
        /** Gather local medians */
        pSort::dataType medians[numProcs];
        *(medians + (ID - begin)) = median;
        for (int i = begin + 1; i <= end; i++)
            MPI_recv(&medians[i - begin], 1, mpi_dataType, i, TAG_LOCAL_MEDIAN);

        /** Compute global median */
        median = findLocalMedian(ID, medians, numProcs);
        // printf("%d: [DEBUG] Found local median of medians to be the record: (%d:%s)\n", ID, median.key, median.payload);

        /** Broadcast global median */
        for (int i = begin + 1; i <= end; i++)
            MPI_send(&median, 1, mpi_dataType, i, TAG_GLOBAL_MEDIAN);
    }
    else
    {
        MPI_send(&median, 1, mpi_dataType, begin, TAG_LOCAL_MEDIAN);
        MPI_recv(&median, 1, mpi_dataType, begin, TAG_GLOBAL_MEDIAN);
    }

    // printf("%d: [DEBUG] Found global median to be the record: (%d:%s)\n", ID, median.key, median.payload);
    return median;
}

pair<pSort::dataType *, int>
quicksort::redistribute(int ID, int numProcs, pSort::dataType *data, MPI_Datatype mpi_dataType, int left_partition_size, int right_partition_size, int begin, int end, bool del, bool best)
{
    pSort::dataType *free_data = data;

    int debug = 0;
    if (debug >= 1)
    {
        printf("%d: [DEBUG] Entering util::redistribute with parameters: numProcs=%d, left_partition_size=%d, right_partition_size=%d, begin=%d, end=%d\n", ID, numProcs, left_partition_size, right_partition_size, begin, end);
    }
    int redistributionInfo[numProcs + 4 + numProcs];
    int left_group_size = 0, right_group_size = 0, left_prefix_sum = 0, right_prefix_sum = 0;
    MPI_Status status;
    MPI_Request requests[128];
    int request_count = 0;
    bool asynchronous = true;
    if (ID == begin)
    {
        if (debug >= 2)
        {
            printf("%d: [DEBUG] I am the master for redistribution in this round\n", ID);
        }
        /**
         * From each process, collect the number of lesser than and smaller than elements
        */
        int left_partition_sizes[numProcs], right_partition_sizes[numProcs];

        left_partition_sizes[0] = left_partition_size;
        for (int i = 1; i < numProcs; i++)
            MPI_recv(&left_partition_sizes[i], 1, mpi_dataType, begin + i, TAG_LEFT_PARTITION_SIZE);

        right_partition_sizes[0] = right_partition_size;
        for (int i = 1; i < numProcs; i++)
            MPI_recv(&right_partition_sizes[i], 1, mpi_dataType, begin + i, TAG_RIGHT_PARTITION_SIZE);

        if (debug >= 1)
        {
            printf("%d: [DEBUG] LEFT PARTITION SIZES ", ID);
            for (int i = 0; i < numProcs; i++)
                printf("(%d: %d) ", i, left_partition_sizes[i]);
            printf("\n");

            printf("%d: [DEBUG] RIGHT PARTITION SIZES ", ID);
            for (int i = 0; i < numProcs; i++)
                printf("(%d: %d) ", i, right_partition_sizes[i]);
            printf("\n");
        }

        /**
         * Divide the work between left and right group for next iteration
        */
        int left_partition_sum = 0, right_partition_sum = 0, counter = 0;
        for (int i = 0; i < numProcs; i++)
        {
            left_partition_sum += left_partition_sizes[i];
            right_partition_sum += right_partition_sizes[i];
        }
        left_group_size = std::floor((((double)left_partition_sum * numProcs) / (left_partition_sum + right_partition_sum)) + 0.5);
        left_group_size = std::max(1, left_group_size);
        left_group_size = std::min(numProcs-1, left_group_size);
        right_group_size = numProcs - left_group_size;
        redistributionInfo[numProcs] = left_group_size;
        redistributionInfo[numProcs + 1] = right_group_size;

        if (left_group_size > 0)
        {
            for (int i = 0; i < left_group_size - 1; i++)
                redistributionInfo[counter++] = left_partition_sum / left_group_size;
            redistributionInfo[counter++] = left_partition_sum - (left_group_size - 1) * (left_partition_sum / left_group_size);
        }

        if (right_group_size > 0)
        {
            for (int i = 0; i < right_group_size - 1; i++)
                redistributionInfo[counter++] = right_partition_sum / right_group_size;
            redistributionInfo[counter++] = right_partition_sum - (right_group_size - 1) * (right_partition_sum / right_group_size);
        }

        if (debug >= 1)
        {
            printf("%d: [DEBUG] Total left elements = %d assigned to %d processes\n", ID, left_partition_sum, left_group_size);
            printf("%d: [DEBUG] Total right elements = %d assigned to %d processes\n", ID, right_partition_sum, right_group_size);
            printf("%d: [DEBUG] Left group:: ", ID);
            for (int i = 0; i < left_group_size; i++)
                printf("Rank: %d got %d, ", begin + i, redistributionInfo[i]);
            printf("\n");
            printf("%d: [DEBUG] Right group:: ", ID);
            for (int i = 0; i < right_group_size; i++)
                printf("Rank: %d got %d, ", begin + left_group_size + i, redistributionInfo[left_group_size + i]);
            printf("\n");
        }

        /**
         * Calculate the prefix sums for each of the process and send them the complete information
         * to be able to proceed
        */
        left_prefix_sum = left_partition_sizes[0];
        right_prefix_sum = 0;
        for (int i = 0; i < numProcs; i++)
        {
            right_prefix_sum += right_partition_sizes[i];
        }
        memcpy(&redistributionInfo[numProcs + 4], left_partition_sizes, numProcs * sizeof(int));
        bool flag = true;
        for (int i = begin + 1; i <= end; i++)
        {
            left_prefix_sum += left_partition_sizes[i - begin];
            right_prefix_sum -= right_partition_sizes[i - begin - 1];
            redistributionInfo[numProcs + 2] = left_prefix_sum;
            redistributionInfo[numProcs + 3] = right_prefix_sum;
            if (i >= (begin + left_group_size) && flag)
            {
                memcpy(&redistributionInfo[numProcs + 4], right_partition_sizes, numProcs * sizeof(int));
                flag = false;
            }
            MPI_send(redistributionInfo, numProcs + 4 + numProcs, MPI_INT, i, TAG_REDISTRIBUTION_INFO);
        }
        left_prefix_sum = left_partition_sizes[0];
        right_prefix_sum = 0;
        for (int i = 0; i < numProcs; i++)
        {
            right_prefix_sum += right_partition_sizes[i];
        }
        redistributionInfo[numProcs + 2] = left_prefix_sum;
        redistributionInfo[numProcs + 3] = right_prefix_sum;
        memcpy(&redistributionInfo[numProcs + 4], left_partition_sizes, numProcs * sizeof(int));
    }
    else
    {
        MPI_send(&left_partition_size, 1, MPI_INT, begin, TAG_LEFT_PARTITION_SIZE);
        MPI_send(&right_partition_size, 1, MPI_INT, begin, TAG_RIGHT_PARTITION_SIZE);
        MPI_recv(&redistributionInfo, numProcs + 4 + numProcs, MPI_INT, begin, TAG_REDISTRIBUTION_INFO);
        left_group_size = redistributionInfo[numProcs];
        right_group_size = redistributionInfo[numProcs + 1];
        left_prefix_sum = redistributionInfo[numProcs + 2];
        right_prefix_sum = redistributionInfo[numProcs + 3];
    }

    if (debug >= 1)
    {
        printf("%d: [DEBUG] Redistribution information gathered: ", ID);
        for (int i = 0; i < numProcs + 4 + numProcs; i++)
            printf("%d ", redistributionInfo[i]);
        printf("\n");
    }

    pSort::dataType *new_data = new pSort::dataType[redistributionInfo[ID - begin]];
    memset(new_data, 0, sizeof(pSort::dataType) * redistributionInfo[ID - begin]);
    int new_data_offset = 0;
    int pos, forward, start_ind, target;

    /**
     * Start sending the old data
    */
    pos = 0;
    forward = 0;                                       // Iter to keep track of position
    start_ind = left_prefix_sum - left_partition_size; // The final starting index of my left data block in the new data array
    target = left_partition_size;                      // The target number of smaller than records to send
    for (int i = 0; i < left_group_size; i++)
    {
        pos += redistributionInfo[i];

        if (pos < start_ind)
            continue; // Do not send to a process who's not expecting to receive from you

        if (target <= 0)
            break; // Stop if the target is reached

        forward = std::min(pos - start_ind, target); // Send the minimum of target and receiver's capacity

        if (forward == 0) // Do not send or receive empty messages. (These can lead to deadlocks)
            continue;

        if (debug >= 1)
        {
            printf("%d: [DEBUG] Sending %d smaller than records to process %d\n", ID, forward, begin + i);
        }

        if (begin + i != ID)
        {
            MPI_isend(data, forward, mpi_dataType, begin + i, TAG_SMALLER_THAN_RECORDS, &requests[request_count++]);
        }
        else
        {
            memcpy(new_data + new_data_offset, data, forward * sizeof(pSort::dataType));
            new_data_offset += forward;
        }
        data += forward;
        target -= forward;
        start_ind += forward;
    }

    pos = 0;
    forward = 0;
    start_ind = right_prefix_sum - right_partition_size; // The final starting index of my right data in the new data arrays (Reverse)
    target = right_partition_size;                       // The target number of larger than records to send
    for (int i = numProcs - 1; i >= left_group_size; i--)
    {
        pos += redistributionInfo[i];

        if (pos < start_ind)
            continue;

        if (target <= 0)
            break;

        forward = std::min(pos - start_ind, target);

        if (forward == 0)
            continue;

        if (debug >= 1)
        {
            printf("%d: [DEBUG] Sending %d larger than records to process %d\n", ID, forward, begin + i);
        }

        if (begin + i != ID)
        {
            MPI_isend(data, forward, mpi_dataType, begin + i, TAG_GREATER_THAN_RECORDS, &requests[request_count++]);
        }
        else
        {
            memcpy(new_data + new_data_offset, data, forward * sizeof(pSort::dataType));
            new_data_offset += forward;
        }
        data += forward;
        target -= forward;
        start_ind += forward;
    }

    if (debug >= 1)
    {
        printf("%d: [DEBUG] Sent complete data\n", ID);
    }

    /** Start receiving new data */

    pos = 0;
    forward = 0;
    target = redistributionInfo[ID - begin] - new_data_offset; // The target number of records left to be received in the new partition

    if (debug >= 1)
    {
        printf("%d: [DEBUG] I need to receive %d more records\n", ID, target);
    }

    start_ind = new_data_offset; // Calculate the index where you need to begin receiving using the prefix sum //

    if (ID < begin + left_group_size)
    {
        for (int i = begin; i < ID; i++)
        {
            start_ind += redistributionInfo[i - begin];
        }

        for (int i = numProcs + 4; i < numProcs + 4 + numProcs; i++)
        {
            pos += redistributionInfo[i];

            if (pos < start_ind)
                continue;

            if (target <= 0)
                break;

            if ((begin + i - numProcs - 4) != ID)
            {
                forward = std::min(pos - start_ind, target);

                if (forward == 0)
                    continue;

                if (debug >= 1)
                {
                    printf("%d: [DEBUG] Waiting to receive %d records from %d\n", ID, forward, begin + i - numProcs - 4);
                }

                MPI_irecv(new_data + new_data_offset, forward, mpi_dataType, begin + i - numProcs - 4, TAG_SMALLER_THAN_RECORDS, &requests[request_count++]);
                new_data_offset += forward;
                target -= forward;
                start_ind += forward;
            }
        }
    }
    else
    {
        for (int i = begin + numProcs - 1; i > ID; i--)
        {
            start_ind += redistributionInfo[i - begin];
        }
        for (int i = numProcs + 4 + numProcs - 1; i >= numProcs + 4; i--)
        {
            pos += redistributionInfo[i];
            if (pos < start_ind)
                continue;

            if (target <= 0)
                break;

            if ((begin + i - numProcs - 4) != ID)
            {
                forward = std::min(pos - start_ind, target);

                if (forward == 0)
                    continue;

                if (debug >= 1)
                {
                    printf("%d: [DEBUG] Waiting to receive %d records from %d\n", ID, forward, begin + i - numProcs - 4);
                }

                MPI_irecv(new_data + new_data_offset, forward, mpi_dataType, begin + i - numProcs - 4, TAG_GREATER_THAN_RECORDS, &requests[request_count++]);
                new_data_offset += forward;
                target -= forward;
                start_ind += forward;
            }
        }
    }

    /** MPI Barrier */
    MPI_Waitall(request_count, requests, NULL);

    if (del)
        delete[] free_data;

    if (debug >= 1)
    {
        printf("%d: [DEBUG] Received complete data\n", ID);
    }

    if (ID < (begin + left_group_size))
    {
        if (debug >= 2)
        {
            printf("%d: [DEBUG] I'll recurse for numProcs=%d, begin=%d, end=%d\n", ID, left_group_size, begin, begin + left_group_size - 1);
        }
        return quicksort::parallelQuickSort(ID, left_group_size, mpi_dataType, &new_data[0], redistributionInfo[ID - begin], begin, begin + left_group_size - 1, true, best);
    }
    else
    {
        if (debug >= 2)
        {
            printf("%d: [DEBUG] I'll recurse for numProcs=%d, begin=%d, end=%d\n", ID, right_group_size, begin + left_group_size, end);
        }
        return quicksort::parallelQuickSort(ID, right_group_size, mpi_dataType, &new_data[0], redistributionInfo[ID - begin], begin + left_group_size, end, true, best);
    }
}

pair<pSort::dataType *, int>
quicksort::parallelQuickSort(int ID, int numProcs, MPI_Datatype mpi_dataType, pSort::dataType *data, int ndata, int begin, int end, bool del, bool best)
{

    // printf("%d: [DEBUG] Begin Parallel Sort for parameters: numProcs = %d, ndata=%d, begin=%d, end=%d\n", ID, numProcs, ndata, begin, end);
    if (begin == end)
    {
        // printf("%d: [INFO] I've %d records sorted with me\n", ID, ndata);
        if (best)
        {
            serialRadixSort(ID, data, ndata, 4);
        }
        else
        {
            serialQuickSort(ID, data, ndata);
        }

        return pair<pSort::dataType *, int>(data, ndata);
    }

    /**
     * Find global median of the data
    */
    pSort::dataType median = quicksort::findGlobalMedian(ID, numProcs, mpi_dataType, data, ndata, begin, end);

    /**
     * Partition the data around global median
    */
    int left_partition_size, right_partition_size;
    left_partition_size = partition(ID, data, ndata, median);
    right_partition_size = ndata - left_partition_size;
    // printf("%d: [DEBUG] Partitioned the set of %d points into sets of %d and %d\n", ID, ndata, left_partition_size, right_partition_size);

    /**
     * Re-distribute the data around the pivot and recurse
    */
    return quicksort::redistribute(ID, numProcs, data, mpi_dataType, left_partition_size, right_partition_size, begin, end, del, best);
}