/***************************************************************************
 *   Copyright (C) 2012 - 2024 by Terraneo Federico and Luigi Rucco        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/ 

#include "process_pool.h"
#include <stdexcept>
#include <cstring>
#include <algorithm>
#ifndef TEST_ALLOC
#include "interfaces_private/userspace.h"
#endif //TEST_ALLOC

using namespace std;

#ifdef WITH_PROCESSES

namespace miosix {

///This constant specifies the size of the minimum allocatable block,
///in bits. So for example 10 is 1KB.
static const unsigned int blockBits=10;
///This constant is the the size of the minimum allocatable block, in bytes.
static const unsigned int blockSize=1<<blockBits;

ProcessPool& ProcessPool::instance()
{
    #ifndef TEST_ALLOC
    //These are defined in the linker script
    extern unsigned int _process_pool_start asm("_process_pool_start");
    extern unsigned int _process_pool_end asm("_process_pool_end");
    static ProcessPool pool(&_process_pool_start,
        reinterpret_cast<unsigned int>(&_process_pool_end)-
        reinterpret_cast<unsigned int>(&_process_pool_start));
    return pool;
    #else //TEST_ALLOC
    static ProcessPool pool(reinterpret_cast<unsigned int*>(0x20008000),96*1024);
    return pool;
    #endif //TEST_ALLOC
}
    
pair<unsigned int *, unsigned int> ProcessPool::allocate(unsigned int size)
{
    #ifndef TEST_ALLOC
    miosix::Lock<miosix::FastMutex> l(mutex);
    size=MPUConfiguration::roundSizeForMPU(max(size,blockSize));
    #else //TEST_ALLOC
    //Size adjustment not supported during test_alloc due to missing mpu header
    if((size & (size - 1)) || size<blockSize)
            throw runtime_error("ProcessPool::allocate unsupported size");
    #endif //TEST_ALLOC
    if(size>poolSize) throw bad_alloc();
    
    unsigned int offset=0;
    if(reinterpret_cast<unsigned int>(poolBase) % size)
        offset=size-(reinterpret_cast<unsigned int>(poolBase) % size);
    unsigned int startBit=offset/blockSize;
    unsigned int sizeBit=size/blockSize;

    for(unsigned int i=startBit;i<=poolSize/blockSize;i+=sizeBit)
    {
        bool notEmpty=false;
        for(unsigned int j=0;j<sizeBit;j++)
        {
            if(testBit(i+j)==0) continue;
            notEmpty=true;
            break;
        }
        if(notEmpty) continue;
        
        for(unsigned int j=0;j<sizeBit;j++) setBit(i+j);
        unsigned int *result=poolBase+i*blockSize/sizeof(unsigned int);
        allocatedBlocks[result]=size;
        return make_pair(result,size);
    }
    throw bad_alloc();
}

void ProcessPool::deallocate(unsigned int *ptr)
{
    #ifndef TEST_ALLOC
    miosix::Lock<miosix::FastMutex> l(mutex);
    #endif //TEST_ALLOC
    map<unsigned int*, unsigned int>::iterator it= allocatedBlocks.find(ptr);
    if(it==allocatedBlocks.end())
    #ifndef TEST_ALLOC
        errorHandler(UNEXPECTED);
    #else //TEST_ALLOC
        throw runtime_error("ProcessPool::deallocate corrupted pointer");
    #endif //TEST_ALLOC
    unsigned int size =(it->second)/blockSize;
    unsigned int firstBit=(reinterpret_cast<unsigned int>(ptr)-
                           reinterpret_cast<unsigned int>(poolBase))/blockSize;
    for(unsigned int i=firstBit;i<firstBit+size;i++) clearBit(i);
    allocatedBlocks.erase(it);
}

ProcessPool::ProcessPool(unsigned int *poolBase, unsigned int poolSize)
    : poolBase(poolBase), poolSize(poolSize)
{
    int numBytes=poolSize/blockSize/8;
    bitmap=new unsigned int[numBytes/sizeof(unsigned int)];
    memset(bitmap,0,numBytes);
}

ProcessPool::~ProcessPool()
{
    delete[] bitmap;
}

} //namespace miosix

#ifdef TEST_ALLOC
//g++ -m32 -o pp -DTEST_ALLOC -DWITH_PROCESSES process_pool.cpp && ./pp
int main()
{
    using namespace miosix;
    ProcessPool& pool=ProcessPool::instance();
    while(1)
    {
        cout<<"a<size(exponent)>|d<addr>"<<endl;
        unsigned int param;
        char op;
        string line;
        getline(cin,line);
        stringstream ss(line);
        ss>>op;
        switch(op)
        {
            case 'a':
                ss>>dec>>param;
                try {
                    pool.allocate(1<<param);
                } catch(exception& e) {
                    cout<<typeid(e).name();
                }
                pool.printAllocatedBlocks();
                break;
            case 'd':
                ss>>hex>>param;
                try {
                    pool.deallocate(reinterpret_cast<unsigned int*>(param));
                } catch(exception& e) {
                    cout<<typeid(e).name();
                }
                pool.printAllocatedBlocks();
                break;
            default:
                cout<<"Incorrect option"<<endl;
                break;
        }
    }
}
#endif //TEST_ALLOC

#endif //WITH_PROCESSES
