#include <cstdlib>
#include <cmath>
#include <iostream>

void* smalloc(size_t size){
    if(size==0 || size>pow(10,8))
        return NULL;
    void* currPB=sbrk(0);
    if(sbrk(size)!=currPB)
        return NULL;

    std::cout << "allocated: "<< size << std::endl;
}
