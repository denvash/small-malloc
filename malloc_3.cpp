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

void* splitBlock(void* blockAdress,size_t leastSignificantSize){
    MallocMetadata* leastSignificantMeta=(MallocMetadata*)blockAdress;
    std::cout<<"size of mallocmetadata:"<<sizeof(MallocMetadata)<<std::endl;

    int mostSignificantSize=leastSignificantMeta->size-leastSignificantSize-sizeof(MallocMetadata);
    leastSignificantMeta->size=leastSignificantSize;
    leastSignificantMeta->is_free=false;
//    void* splittedBlock=(void*)((MallocMetadata*)(leastSignificantMeta+1))+leastSignificantSize);
    void* splittedBlock=(void*)((char*)(leastSignificantMeta+1)+leastSignificantSize);

    MallocMetadata* mostSignificantMeta=(MallocMetadata*)splittedBlock;
    mostSignificantMeta->size=mostSignificantSize;
    mostSignificantMeta->is_free=true;
    mostSignificantMeta->prev=leastSignificantMeta;
    MallocMetadata* tempMeta=leastSignificantMeta->next;
    mostSignificantMeta->next=tempMeta;
    tempMeta->prev=mostSignificantMeta;

    return (void*)(mostSignificantMeta+1);
}
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
//        firstMeta->is_free=false;
        firstMeta->is_free=true;
        listOfBlocks.numberOfBlocksInUse++;
        listOfBlocks.firstBlock=firstMeta;
        listOfBlocks.lastBlock=firstMeta;
        return (void*)(firstMeta+1);
    }else{
        MallocMetadata* currBlock=listOfBlocks.firstBlock;
        MallocMetadata* finalLinkedBlock;

        while(currBlock!=NULL){
            if(currBlock->size>=size && currBlock->is_free)
                std::cout<<"size of mallocmetadata:"<<sizeof(MallocMetadata)<<std::endl;
            int diff=currBlock->size-size-sizeof(MallocMetadata);
//                if(currBlock->size-size-sizeof(MallocMetadata)>=128)
                if(diff>=128)
                    return splitBlock(currBlock,size);
                else
                    return (void*)(currBlock+1);

            if(!(currBlock->next))
                finalLinkedBlock=currBlock;

            currBlock=currBlock->next;
        }

        //didn't find proper block,need to allocate
        void* newBlockAdress=sbrk(sizeof(MallocMetadata)+size);
        if(newBlockAdress==(void*)(-1))
            return NULL;
        MallocMetadata* newMeta=(MallocMetadata*)newBlockAdress;
        newMeta->size=size;
//        newMeta->is_free=false;
        newMeta->is_free=true;
        newMeta->prev=finalLinkedBlock;
        finalLinkedBlock->next=newMeta;
        listOfBlocks.numberOfBlocksInUse++;
        listOfBlocks.lastBlock=newMeta;

        return (void*)(newMeta+1);
    }


}

void* scalloc(size_t num,size_t size){
    if(size==0 || size*num>pow(10,8))
        return NULL;

    void* newBlockAdress=smalloc(num*size);
    if(!newBlockAdress)
        return NULL;
    memset(newBlockAdress,0,num*size);
    return newBlockAdress;

}

void* sfree(void* p){
    return NULL;
}

void* srealloc(void* oldp,size_t size){
    if(size==0 || size>pow(10,8))
        return NULL;
    if(!oldp)
        return smalloc(size);
    if(((MallocMetadata*)oldp-1)->size>=size)
        return oldp;
    else{
        void* newBlockAdress=smalloc(size);
        if(!newBlockAdress)
            return NULL;
        memcpy(newBlockAdress,oldp,((MallocMetadata*)oldp-1)->size);
        sfree(oldp);
        return newBlockAdress;
    }
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