#include "util.h"
#include <vector>
#include <stdio.h>
#include <string.h>
#include <sstream>

namespace util
{

void printDebugMsg(int ID, std::string message)
{
    printf("\n%d: [DEBUG] %s\n", ID, message.c_str());
}

void printdata(int ID, const char* message, pSort::dataType *data, int ndata)
{
    std::ostringstream buffer; 
    if(ndata == 0) return;
    buffer << format("%d: [DEBUG] %s ") % ID % message;
    for (int i = 0; i < ndata - 1; i++)
    {
        buffer << format("(%d: ") % data[i].key;
        for (int j = 0; j < LOADSIZE; j++)
            buffer << format("%c") % data[i].payload[j];
        buffer << ("), ");
    }
    buffer << format("(%d: ") % data[ndata - 1].key;
    for (int j = 0; j < LOADSIZE; j++)
        buffer << format("%c") % data[ndata - 1].payload[j];
    buffer << (")");
    printf("\n%s\n", buffer.str().c_str());
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
            memcpy(&data[index++], &buckets[i][j], sizeof(pSort::dataType));
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
}