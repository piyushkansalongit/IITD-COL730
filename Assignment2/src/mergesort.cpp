#include "mergesort.h"
#include <omp.h>
#include <string.h>
#include "util.h"
#include "sort.h"

pSort::dataType* serialMergeAndSelectFirst(int ID, pSort::dataType *data, int ndata, pSort::dataType *buffer, int buffer_size, pSort::dataType *temp)
{
    // printf("%d: [DEBUG] I'll merge and accept the %d smaller values\n", ID, ndata);
    temp = new pSort::dataType[ndata];
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
    return temp;
}

pSort::dataType* serialMergeAndSelectSecond(int ID, pSort::dataType *data, int ndata, pSort::dataType *buffer, int buffer_size, pSort::dataType *temp)
{
    // printf("%d: [DEBUG] I'll merge and accept the %d larger values\n", ID, ndata);
    temp = new pSort::dataType[ndata];
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
    return temp;
}

void __sort(pSort::dataType *data, int ndata, int nThreads, int indices[], int sizes[], bool serial_sort)
{
    // printf("Merging for nThreads=%d\n", nThreads);

    if (ndata == 0)
        return;

    pSort::dataType* temp[nThreads];

    #pragma omp parallel default(shared) num_threads(nThreads)
    {
        #pragma omp single
        {
            if(serial_sort)
            {
                for(int ID=0; ID < nThreads; ID++)
                {
                    #pragma omp task default(shared) firstprivate(ID) depend(out: indices[ID], sizes[ID])
                        util::serialRadixSort(ID, &data[indices[ID]], sizes[ID]);
                }
            }

            for(int ID=0; ID < nThreads; ID++)
            {
                if (ID % 2 == 0 && ID+1 != nThreads){
                    if(serial_sort)
                    {
                        #pragma omp task default(shared) firstprivate(ID) depend(in: indices[ID], indices[ID+1], sizes[ID], sizes[ID+1]) depend(out: temp[ID])
                            temp[ID] = serialMergeAndSelectFirst(ID, &data[indices[ID]], sizes[ID], &data[indices[ID+1]], sizes[ID+1], temp[ID]);
                    }
                    else
                    {
                        #pragma omp task default(shared) firstprivate(ID) depend(out: temp[ID])
                            temp[ID] = serialMergeAndSelectFirst(ID, &data[indices[ID]], sizes[ID], &data[indices[ID+1]], sizes[ID+1], temp[ID]);
                    }
                    
                }
                else if(ID % 2 == 1){
                    if(serial_sort)
                    {
                        #pragma omp task default(shared) firstprivate(ID) depend(in: indices[ID], indices[ID-1], sizes[ID], sizes[ID-1]) depend(out: temp[ID])
                            temp[ID] = serialMergeAndSelectSecond(ID, &data[indices[ID]], sizes[ID], &data[indices[ID-1]], sizes[ID-1], temp[ID]);
                    }
                    else
                    {
                        #pragma omp task default(shared) firstprivate(ID) depend(out: temp[ID])
                            temp[ID] = serialMergeAndSelectSecond(ID, &data[indices[ID]], sizes[ID], &data[indices[ID-1]], sizes[ID-1], temp[ID]);
                    }
                }
            }

            for(int ID=0; ID < nThreads; ID++)
            {
                if(ID % 2 ==0 && ID+1 != nThreads){
                    #pragma omp task default(shared) firstprivate(ID) depend(in: temp[ID], temp[ID+1])
                        memcpy(&data[indices[ID]], temp[ID], sizeof(pSort::dataType) * sizes[ID]);
                }
                else if(ID %2 == 1){
                    #pragma omp task default(shared) firstprivate(ID) depend(in: temp[ID], temp[ID-1])
                        memcpy(&data[indices[ID]], temp[ID], sizeof(pSort::dataType) * sizes[ID]);
                }
            }
            
        }
        
    }
    
    if(nThreads==2)
        return;

    if(nThreads%2==0)
    {
        int _indices[nThreads/2], _sizes[nThreads/2];
        for(int i=0; i<nThreads; i+=2)
        {
            _indices[i/2] = indices[i];
            _sizes[i/2] = sizes[i] + sizes[i+1];
        }
        __sort(data, ndata, nThreads/2, _indices, _sizes, false);
    }
    else
    {
        int _indices[nThreads/2 + 1], _sizes[nThreads/2 + 1];
        for(int i=0; i<nThreads; i+=2)
        {
            _indices[i/2] = indices[i];
            _sizes[i/2] = sizes[i] + sizes[i+1];
        }
        _indices[nThreads/2] = indices[nThreads-1];
        _sizes[nThreads/2] = sizes[nThreads-1];
        __sort(data, ndata, nThreads/2 + 1, _indices, _sizes, false);
    }
    
}

void mergesort::sort(pSort::dataType *data, int ndata, int nThreads)
{
    printf("[INFO] Number of processors using for merge sort = %d\n", nThreads);

    if(nThreads == 1)
    {
        util::serialRadixSort(-1, data, ndata);
        return;
    }

    int indices[nThreads], sizes[nThreads];
    for(int ID=0; ID < nThreads; ID++)
    {
        int data_beg_ind, data_end_ind, data_size;

        data_beg_ind = ID * (ndata / nThreads);
        if(ID != nThreads-1)
            data_end_ind = std::min(ndata, (ID + 1) * (ndata / nThreads));
        else
            data_end_ind = std::min(ndata, (ID + 1) * (ndata / nThreads)) + ndata % nThreads;
        data_size = data_end_ind - data_beg_ind;
        
        indices[ID] = data_beg_ind;
        sizes[ID] = data_size;
    }

    __sort(data, ndata, nThreads, indices, sizes, true);
}