#include <cstdlib>
#include <cmath>
#include <iostream>

void* smalloc(size_t size){
    if(size==0 || size>pow(10,8))
        return NULL;

    void* newBlock=sbrk(size);

    if(newBlock==(void*)(-1))
        return NULL;
    else
        return newBlock;

}
