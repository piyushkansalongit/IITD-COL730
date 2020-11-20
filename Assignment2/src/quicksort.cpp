#include "quicksort.h"
#include <omp.h>
#include <algorithm>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "util.h"
#include "sort.h"
#include "boost/format.hpp"
using boost::format;

using namespace quicksort;

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
            if (data[start].key != pivot.key)
            {
                swap(ID, data, i, start);
                start += 1;
            }
            else
            {
                swap(ID, data, i, start);
                swap(ID, data, i, start + same);
                start += 1;
            }
        }
        else if (data[i].key == pivot.key)
        {
            swap(ID, data, i, start + same);
            same += 1;
        }
        // printdata(ID, data, ndata);
    }
    if (same % 2 == 0)
    {
        // printf("%d: [DEBUG] Returing left partition size: %d\n", ID, start+(same/2));
        return start + (same / 2);
    }
    else
    {
        // printf("%d: [DEBUG] Returing left partition size: %d\n", ID, start+(same+1/2));
        return start + (same + 1) / 2;
    }
}

void __sort(pSort::dataType *data, int ndata, int nThreads, int level, int* free_threads, omp_lock_t* writelock)
{
    int debug = 0;

    if (ndata == 0)
        return;

    if(debug >= 1)
    {
        util::printDebugMsg(level, (format("Number of processors using for quick sort on %d records = %d") % ndata % nThreads).str());
    }

    if (nThreads == 1)
    {
        util::serialRadixSort(-1, data, ndata);
        omp_set_lock(writelock);
        (*free_threads)+=1;
        omp_unset_lock(writelock);
        return;
    }

    int l_parts[nThreads], r_parts[nThreads];
    int grp_beg_ind, grp_end_ind;
    grp_beg_ind = 0;
    grp_end_ind = nThreads - 1;

    #pragma omp parallel default(none) firstprivate(data, ndata, nThreads, grp_beg_ind, grp_end_ind) shared(debug, l_parts, r_parts) num_threads(nThreads)
    {
        int ID = omp_get_thread_num();
        int data_beg_ind, data_end_ind, data_size;
        int l_part_size, r_part_size;

        data_beg_ind = ID * (ndata / nThreads);
        data_end_ind = ID != grp_end_ind ? std::min(ndata, (ID+1) * (ndata / nThreads)) : std::min(ndata, (ID + 1) * (ndata / nThreads)) + ndata % nThreads;
        data_size = data_end_ind - data_beg_ind;
        if (debug >= 2)
        {
            util::printdata(ID, (format("Received %d records:") % data_size).str().c_str(), data + data_beg_ind, data_size);
        }

        /** Partitioning around the global pivot */
        pSort::dataType pivot = *data;
        /** Make sure everyone in this group gets the same pivot */
        #pragma omp barrier
        l_parts[ID] = l_part_size = partition(ID, data + data_beg_ind, data_size, pivot);
        r_parts[ID] = r_part_size = data_size - l_part_size;
        if (debug >= 2)
        {
            util::printdata(ID, (format("Partitioned data around pivot: (%d: %s) into %d and %d ==>") % pivot.key % pivot.payload % l_part_size % r_part_size).str().c_str(), data + data_beg_ind, data_size);
        }

        /** Globally re-shuffle the data using the local partitions */
        pSort::dataType *l_buf, *r_buf;
        int l_offset = 0, r_offset = 0;
        l_buf = new pSort::dataType[l_part_size];
        r_buf = new pSort::dataType[r_part_size];
        memcpy(l_buf, &data[data_beg_ind], l_part_size * sizeof(pSort::dataType));
        memcpy(r_buf, &data[data_beg_ind + l_part_size], r_part_size * sizeof(pSort::dataType));
        
        /** Make sure everyone in this group has copied relevant data blocks in temporary arrays */
        #pragma omp barrier
        for (int id = grp_beg_ind; id <= grp_end_ind; id++)
        {
            if (id < ID)
            {
                l_offset += l_parts[id];
                r_offset += r_parts[id];
            }
            r_offset += l_parts[id];
        }
        if (debug >= 1)
        {
            util::printDebugMsg(ID, (format("Copying %d records at offset %d and %d records at offset %d") % l_part_size % l_offset % r_part_size % r_offset).str());
        }
        memcpy(&data[l_offset], l_buf, l_part_size * sizeof(pSort::dataType));
        memcpy(&data[r_offset], r_buf, r_part_size * sizeof(pSort::dataType));
        delete[] l_buf;
        delete[] r_buf;
    }

    /** Every thread calculates the size of the left and right partition */
    int l_sum = 0, r_sum = 0;
    for (int i = 0; i < nThreads; i++)
    {
        l_sum += l_parts[i];
        r_sum += r_parts[i];
    }

    /** Divide the partition among thread subgroups for next iteration */
    int l_group_size, r_group_size;
    l_group_size = std::floor((((double)l_sum * nThreads) / (l_sum + r_sum)) + 0.5);
    l_group_size = std::max(1, l_group_size);
    l_group_size = std::min(nThreads - 1, l_group_size);
    r_group_size = nThreads - l_group_size;

    util::printDebugMsg(level, (format("Splitting {%d:%d} into {%d:%d} and {%d:%d} for the next iteration") % ndata % nThreads % l_sum % l_group_size % r_sum % r_group_size).str());
    
    int usable_free_threads = 0;
    omp_set_lock(writelock);
    usable_free_threads = *free_threads;
    *free_threads = 0;
    omp_unset_lock(writelock);
    if(usable_free_threads > 0)
        printf("[DEBUG] Found free threads = %d\n", usable_free_threads);

    #pragma omp parallel
    {
        #pragma omp sections nowait
        {
            #pragma omp section
            {
                if(l_sum > r_sum)
                {
                    if(usable_free_threads % 2 == 0)
                        __sort(data, l_sum, l_group_size + usable_free_threads/2, level+1, free_threads, writelock);
                    else
                        __sort(data, l_sum, l_group_size + usable_free_threads/2 + 1, level+1, free_threads, writelock);
                }
                else
                {
                        __sort(data, l_sum, l_group_size + usable_free_threads/2, level+1, free_threads, writelock);
                } 
            }

            #pragma omp section
            {
                if(l_sum > r_sum)
                {
                    __sort(data+l_sum, r_sum, r_group_size + usable_free_threads/2, level+1, free_threads, writelock);
                }
                else
                {
                    if(usable_free_threads % 2 == 0)
                        __sort(data+l_sum, r_sum, r_group_size + usable_free_threads/2, level+1, free_threads, writelock);
                    else
                        __sort(data+l_sum, r_sum, r_group_size + usable_free_threads/2 + 1, level+1, free_threads, writelock);
                }
                
            }
        }
    }
}

void quicksort::sort(pSort::dataType *data, int ndata, int nThreads)
{
    omp_lock_t writelock;
    int free_threads = 0;
    omp_init_lock(&writelock);
    __sort(data, ndata, nThreads, 0, &free_threads, &writelock);
    omp_destroy_lock(&writelock);
}


