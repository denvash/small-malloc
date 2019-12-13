#include <iostream>
#include <unistd.h>
#include <cmath>
#include <stdio.h>
#include <string.h>
#include "malloc_2.h"


struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;

    MallocMetadata(size_t newSize,MallocMetadata** prevData):size(newSize),is_free(false),next(NULL),prev(*prevData){}
};

struct ListOfMallocMetadata{
    int numberOfBlocksInUse;
    MallocMetadata** firstBlock;
    MallocMetadata** lastBlock;

    ListOfMallocMetadata():numberOfBlocksInUse(0){}
};


//ListOfMallocMetadata* listOfBlocks=(ListOfMallocMetadata*)sbrk(_size_meta_data());
//ListOfMallocMetadata* listOfBlocks;
static ListOfMallocMetadata listOfBlocks;


void* smalloc(size_t size){
    if(size==0 || size>pow(10,8))
        return NULL;

}

void* scalloc(size_t num,size_t size){

}

void* sfree(void* p){

}

void* srealloc(void* oldp,size_t size){

}

size_t _num_free_blocks(){

}

size_t _num_free_bytes(){

}

size_t _num_allocated_blocks(){

}

size_t _num_allocated_bytes(){

}

size_t _size_meta_data(){
return sizeof(MallocMetadata);
}

size_t _num_meta_data_bytes(){
    return (listOfBlocks.numberOfBlocksInUse)*(_size_meta_data());
}