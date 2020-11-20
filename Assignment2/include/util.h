#ifndef UTIL_H
#define UTIL_H
#include "sort.h"
#include <string>
#include "boost/format.hpp"
using boost::format;

namespace util
{
    void printDebugMsg(int ID, std::string message);
    void printdata(int ID, const char* message, pSort::dataType *data, int ndata);
    pSort::dataType *serialRadixSort(int ID, pSort::dataType *data, int ndata, int b=8);
}
#endif
