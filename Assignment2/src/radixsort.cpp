#include "radixsort.h"
#include "sort.h"
#include <stdio.h>
#include <string.h>
#include <omp.h>
#include <algorithm>
#include <sstream>
#include "util.h"
#include "boost/format.hpp"
using boost::format;

using namespace radixsort;

void radixsort::sort(pSort::dataType *data, int ndata, int nThreads, int b)
{
    printf("[INFO] Number of processors using for radix sort = %d\n", nThreads);
    
    if(nThreads==1)
    {
        util::serialRadixSort(-1, data, ndata);
        return;
    }

    int beg_indices[nThreads];
    int end_indices[nThreads];
    int sizes[nThreads];

    int num_buckets = 1 << b;
    int local_counts[nThreads][num_buckets];

    for (int digit = 0; digit < 32 / b; digit++)
    {
        #pragma omp parallel default(none) shared(data, ndata, b, digit, num_buckets, local_counts, beg_indices, end_indices, sizes, nThreads) num_threads(nThreads)
        {
            // Declarations
            int ID, beg_ind, end_ind, size;

            ID = omp_get_thread_num();

            beg_ind = ID * (ndata / nThreads);
            end_ind = std::min(ndata, (ID + 1) * (ndata / nThreads));
            if (ID == nThreads - 1)
                end_ind += ndata % nThreads;
            size = end_ind - beg_ind;

            beg_indices[ID] = beg_ind;
            end_indices[ID] = end_ind;
            sizes[ID] = size;

            // Determine the counts first
            for(int i=0; i<num_buckets; i++)
                local_counts[ID][i] = 0;
            pSort::dataType *local_buckets[num_buckets];
            for (int i = 0; i < size; i++)
            {
                unsigned int key = data[i + beg_ind].key;
                key = key << (32 / b - 1 - digit) * b;
                key = key >> (32 / b - 1) * b;
                local_counts[ID][key] += 1;
            }

            // Do the memory allocation for the buckets
            for (int bucket_idx = 0; bucket_idx < num_buckets; bucket_idx++)
            {
                local_buckets[bucket_idx] = new pSort::dataType[local_counts[ID][bucket_idx]];
            }

            // Fill the buckets
            for (int i = 0; i < size; i++)
            {
                unsigned int key = data[i + beg_ind].key;
                key = key << (32 / b - 1 - digit) * b;
                key = key >> (32 / b - 1) * b;
                memcpy(local_buckets[key], &data[i + beg_ind], sizeof(pSort::dataType));
                local_buckets[key]++;
            }

            // Reset the pointers to the buckets
            for (int i = 0; i < num_buckets; i++)
            {
                local_buckets[i] -= local_counts[ID][i];
            }

            // std::ostringstream msg;
            // msg << "My local count array is: {";
            // for (int i = 0; i < num_buckets-1; i++)
            // {
            //     msg << format("(%d:%d), ") % i % local_counts[ID][i];
            // }
            // msg << format("(%d:%d)}\n") % (num_buckets-1) % local_counts[ID][num_buckets-1];
            // util::printDebugMsg(ID, msg.str());
    

            #pragma omp barrier

            // Calculate global counts
            int global_counts[num_buckets];
            memset(global_counts, 0, sizeof(global_counts));
            for(int id=0; id<nThreads; id++)
            {
                for(int bucket_idx = 0; bucket_idx < num_buckets; bucket_idx++)
                {
                    global_counts[bucket_idx] += local_counts[id][bucket_idx];
                }
            }

            // Convert global counts to prefix sum
            for(int i=1; i<num_buckets; i++)
            {
                global_counts[i] += global_counts[i-1];
            }

            // Copy your buckets to appropriate place
            for(int bucket_idx=0; bucket_idx<num_buckets; bucket_idx++){
                // Determine how many records come before this local bucket
                int offset = bucket_idx > 0 ? global_counts[bucket_idx-1] : 0;
                for(int id = 0; id < ID; id++)
                {
                    offset += local_counts[id][bucket_idx];
                }
                memcpy(&data[offset], local_buckets[bucket_idx], local_counts[ID][bucket_idx] * sizeof(pSort::dataType));
            }

            // Free the heap memory
            for(int bucket_idx=0; bucket_idx<num_buckets; bucket_idx++)
            {
                delete[] local_buckets[bucket_idx];
            }

        }
    }
}