#include <iostream>
#include <unistd.h>
#include <cmath>
#include <stdio.h>
#include <string.h>
#include "malloc_2.h"
#include <stddef.h>




struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;

    MallocMetadata(size_t newSize,MallocMetadata** prevData):size(newSize),is_free(false),next(NULL),prev(*prevData){}
};

struct ListOfMallocMetadata{
    int numberOfBlocksInUse;
    MallocMetadata* firstBlock;
    MallocMetadata* lastBlock;

    ListOfMallocMetadata():numberOfBlocksInUse(0){}
};


//ListOfMallocMetadata* listOfBlocks=(ListOfMallocMetadata*)sbrk(_size_meta_data());
//ListOfMallocMetadata* listOfBlocks;
static ListOfMallocMetadata listOfBlocks;


void* smalloc(size_t size){
    if(size==0 || size>pow(10,8))
        return NULL;

//first block allocation
    if(!listOfBlocks.numberOfBlocksInUse){
       void* firstBlockAdress=sbrk(sizeof(MallocMetadata)+size);
       if(firstBlockAdress==(void*)(-1))
           return NULL;

        MallocMetadata* firstMeta=(MallocMetadata*)firstBlockAdress;
        firstMeta->size=size;
        firstMeta->is_free=false;
        listOfBlocks.numberOfBlocksInUse++;
        listOfBlocks.firstBlock=firstMeta;
        listOfBlocks.lastBlock=firstMeta;
        return (void*)(firstMeta+1);
    }else{
    MallocMetadata* currBlock=listOfBlocks.firstBlock;

    while(currBlock!=NULL){
        if(currBlock->size==size && currBlock->is_free)
            return (void*)(currBlock+1);

        currBlock=currBlock->next;
    }

    //didn't find proper block,need to allocate
        void* newBlockAdress=sbrk(sizeof(MallocMetadata)+size);
        if(newBlockAdress==(void*)(-1))
            return NULL;
        MallocMetadata* newMeta=(MallocMetadata*)newBlockAdress;
        newMeta->size=size;
        newMeta->is_free=false;
        listOfBlocks.numberOfBlocksInUse++;
        listOfBlocks.lastBlock=newMeta;

        return (void*)(newMeta+1);
    }


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