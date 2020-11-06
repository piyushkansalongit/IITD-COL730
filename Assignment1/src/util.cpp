#include "util.h"
#include <queue>
using namespace std;

auto comparator = [](pSort::dataType a, pSort::dataType b) {
    return a.key > b.key ? true : false;
};

namespace util
{
    void printdata(int ID, pSort::dataType *data, int ndata)
    {
        if(ndata == 0) return;
        printf("\n%d: [DEBUG] Data array is ", ID);
        for (int i = 0; i < ndata - 1; i++)
        {
            printf("(%x: ", data[i].key);
            for (int j = 0; j < LOADSIZE; j++)
                printf("%c", data[i].payload[j]);
            printf("), ");
        }
        printf("(%x: ", data[ndata - 1].key);
        for (int j = 0; j < LOADSIZE; j++)
            printf("%c", data[ndata - 1].payload[j]);
        printf(")\n\n");
    }

    void MPI_send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag)
    {
        assert(MPI_Send(buf, count, datatype, dest, tag, MPI_COMM_WORLD) == MPI_SUCCESS);
    }

    void MPI_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag)
    {
        MPI_Status status;
        assert(MPI_Recv(buf, count, datatype, source, tag, MPI_COMM_WORLD, &status) == MPI_SUCCESS);
    }

    void MPI_isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Request *request)
    {
        assert(MPI_Isend(buf, count, datatype, dest, tag, MPI_COMM_WORLD, request) == MPI_SUCCESS);
    }

    void MPI_irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Request *request)
    {
        assert(MPI_Irecv(buf, count, datatype, source, tag, MPI_COMM_WORLD, request) == MPI_SUCCESS);
    }

    void MPI_bcast(void *buf, int count, MPI_Datatype datatype, int root){
        assert(MPI_Bcast(buf, count, datatype, root, MPI_COMM_WORLD)==MPI_SUCCESS);
    }

    void swap(int ID, pSort::dataType *data, int from, int to)
    {
        if (from == to)
            return;
        size_t size = sizeof(pSort::dataType);
        char temp[size];
        memcpy(temp, (data + from), size);
        memcpy((data + from), (data + to), size);
        memcpy((data + to), temp, size);
    }

    int partition(int ID, pSort::dataType *data, int ndata, pSort::dataType pivot)
    {
        int start = 0;
        int same = 0;
        for (int i = 0; i < ndata; i++)
        {
            // printf("Data: (%d: %s) and pivot: (%d: %s)\n", data[i].key, data[i].payload, pivot.key, pivot.payload);
            if (data[i].key < pivot.key)
            {
                if(data[start].key != pivot.key){
                    swap(ID, data, i, start);
                    start += 1;
                }
                else
                {
                    swap(ID, data, i, start);
                    swap(ID, data, i, start+same);
                    start += 1;
                }
            }
            else if(data[i].key == pivot.key)
            {
                swap(ID, data, i, start+same);
                same += 1;
            }
            // printdata(ID, data, ndata);
        }
        if(same%2==0){
            // printf("%d: [DEBUG] Returing left partition size: %d\n", ID, start+(same/2));
            return start + (same/2);
        }
        else{
            // printf("%d: [DEBUG] Returing left partition size: %d\n", ID, start+(same+1/2));
            return start + (same+1)/2;
        }
    }

    int
    findGroupMedian(int ID, pSort::dataType *data, int start, int end)
    {
        /**
         * We need to return the index of the kth min element in the original group.
         * Hence, 0 <= k < size
        */

        int size = end - start;
        int k = (size % 2 == 0 ? size / 2 : (size + 1) / 2);

        // Construct a min heap
        priority_queue<pSort::dataType, vector<pSort::dataType>, decltype(comparator)> min_heap(comparator);

        // Push all the element copies
        for (int i = start; i < end; i++)
        {
            min_heap.push(*(data + i));
        }

        // Pop k times to find the median
        pSort::dataType temp;
        for (int i = 0; i < k; i++)
        {
            temp = min_heap.top();
            min_heap.pop();
        }

        // Return the index of the median
        for (int i = start; i < end; i++)
        {
            if (temp.key == (data + i)->key)
                return i;
        }

        // You can't be reaching here
        assert(false);
    }

    void
    findMedianProposal(int ID, pSort::dataType *data, int start, int end)
    {
        if (end <= start + 1)
            return;

        for (int i = start; i < end; i += 5)
        {
            /**
             * Find the index of the median in this group
            */
            int index = findGroupMedian(ID, data, i, std::min(end, i + 5));
            // printf("%d: [DEBUG] Median index for start: %d and end: %d found to be %d\n", ID, i, min(end, i+5),  index);

            /**
             * Push the data element at the index to the beginning of the array
            */
            swap(ID, data, index, i / 5);
        }
        return findMedianProposal(ID, data, start, end / 5 + 1);
    }

    pSort::dataType
    findLocalMedian(int ID, pSort::dataType *data, int ndata, int remaining)
    {
        // printf("%d: [DEBUG] Entering to find local median with ndata=%d and remaining=%d\n", ID, ndata, remaining);
        /**
         * To efficiently find the median of a given array in O(n) without sorting it,
         * we will be implementing "Hoare's Select Algorithm".
         * More details on the same can be found here:
         * https://ocw.mit.edu/courses/electrical-engineering-and-computer-science/6-046j-design-and-analysis-of-algorithms-spring-2012/lecture-notes/MIT6_046JS12_lec01.pdf
        */

        /**
         * Divide the n items into groups of 5 and find the median for each of the group.
         * The below function puts the median in front of the array
        */
        findMedianProposal(ID, data, 0, ndata);
        // printf("%d: [DEBUG] Proposing local median to be (%d: %s)\n", ID, data[0].key, data[0].payload);

        /**
         * Partition around the above median proposal and get the number of elements <= pivot
         * It will never return k = 0 since the proposed median is from the array itself.
        */
        pSort::dataType proposed_median;
        memcpy(&proposed_median, data, sizeof(pSort::dataType));

        int k = partition(ID, data, ndata, proposed_median);
        
        if (k == remaining || k == ndata)
            return *data;
   
        else if (k < remaining)
        {
            // printf("[DEBUG] Increasing start index by %d and available records to: %d\n", k, ndata-k);
            return findLocalMedian(ID, data + k, ndata - k, remaining - k);
        }
        else
        {
            // printf("[DEBUG] Keeping start index start same and decreasing available records to: %d\n", k);
            return findLocalMedian(ID, data, k, remaining);
        }
    }

    pSort::dataType
    findLocalMedian(int ID, pSort::dataType *data, int ndata)
    {
        if (ndata % 2 == 0)
        {
            return findLocalMedian(ID, data, ndata, ndata / 2);
        }
        else
        {
            return findLocalMedian(ID, data, ndata, (ndata + 1) / 2);
        }
    }

    pSort::dataType*
    serialQuickSort(int ID, pSort::dataType *data, int ndata)
    {
        if (ndata <= 1)
            return data;

        // printdata(ID, data, ndata);

        // Use this line if you want to want to partition around median
        // pSort::dataType median = findLocalMedian(ID, data, ndata);

        // Use this line if you want to partition around random
        pSort::dataType median = *data;

        int left_partition_size = partition(ID, data, ndata, median);

        // printf("%d: [DEBUG] Found the median to be (%d: %s) with left partition size %d\n", ID, median.key, median.payload, left_partition_size);

        // printdata(ID, data, ndata);

        serialQuickSort(ID, data, left_partition_size - 1);
        serialQuickSort(ID, data + left_partition_size, ndata - left_partition_size);
        return data;
    }

    pSort::dataType*
    serialCountingSort(int ID, pSort::dataType *data, int ndata, int b, int digit)
    {
        /**
         * b : The 32 bit key should be split in b number of digits
         * digit: The least significant index of the digit
        */

        int num_buckets = 1 << b;
        std::vector<pSort::dataType> buckets[num_buckets];
        // printf("%d: [DEBUG] Entering serial counting sort for b=%d, digit=%d and num buckets = %d\n", ID, b, digit, num_buckets);

        // Place each of the record in the linked list corresponding to it's bucket
        for(int i=0; i<ndata; i++){
            // All we care about is the bits from digit*b to (digit+1)*b
            unsigned int key = data[i].key;
            key = key << (32/b - 1 - digit)*b;
            key = key >> (32/b - 1)*b;
            // printf("%d: %d\n", i, key);
            buckets[key].push_back(data[i]);
        }

        // Merge all the lists back
        // printf("%d: [DEBUG] Merging back...\n", ID);
        int index = 0;
        for(int i=0; i<num_buckets; i++)
        {
            for(int j=0; j<buckets[i].size(); j++)
            {
                mempcpy(&data[index++], &buckets[i][j], sizeof(pSort::dataType));
            }
        }
        return data;
    }


    pSort::dataType*
    serialRadixSort(int ID, pSort::dataType *data, int ndata, int b)
    {
        /** Divide the key subspace into buckets of bit length b
         * and now do a counting sort for 32/b iterations */
        for(int i=0; i<(32/b); i++)
        {
            // printf("%d: [DEBUG] On iteration %d of serial radix sort\n", ID, i);
            serialCountingSort(ID, data, ndata, b, i);
        }
        return data;
    }

} // namespace util
