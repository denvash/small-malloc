#include <iostream>
#include <unistd.h>
#include <cmath>
#include <cstring>
#include "malloc_2.h"
#include <cstddef>

using namespace std;

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata *next;
    MallocMetadata *prev;

    MallocMetadata(size_t newSize, MallocMetadata **prevData) : size(newSize), is_free(false), next(nullptr),
                                                                prev(*prevData) {}
};

struct ListOfMallocMetadata {
    size_t totalAllocatedBlocks;
    size_t totalAllocatedBytes;
    size_t numberOfFreeBlocks;
    size_t numberOfFreeBytes;
    MallocMetadata *firstBlock;
    MallocMetadata *lastBlock;

    ListOfMallocMetadata() : totalAllocatedBlocks(0), totalAllocatedBytes(0), numberOfFreeBlocks(0),
                             numberOfFreeBytes(0), firstBlock(), lastBlock() {}
};


static ListOfMallocMetadata listOfBlocks;
//ListOfMallocMetadata* listOfBlocks={};

void *splitBlock(void *blockAdress, size_t leastSignificantSize) {
    MallocMetadata *leastSignificantMeta = (MallocMetadata *) blockAdress;
    int mostSignificantSize = leastSignificantMeta->size - leastSignificantSize - sizeof(MallocMetadata);
    leastSignificantMeta->size = leastSignificantSize;
    leastSignificantMeta->is_free = false;

    void *splittedBlock = (void *) ((char *) (leastSignificantMeta + 1) + leastSignificantSize);
    MallocMetadata *mostSignificantMeta = (MallocMetadata *) splittedBlock;
    mostSignificantMeta->size = mostSignificantSize;
    mostSignificantMeta->is_free = true;
    mostSignificantMeta->prev = leastSignificantMeta;

    if (!leastSignificantMeta->next) {
        leastSignificantMeta->next = mostSignificantMeta;
        listOfBlocks.lastBlock = mostSignificantMeta;
    } else {
        MallocMetadata *tempMeta = leastSignificantMeta->next;
        leastSignificantMeta->next = mostSignificantMeta;
        mostSignificantMeta->next = tempMeta;
        tempMeta->prev = mostSignificantMeta;
    }

    return blockAdress;
}

void *smalloc(size_t size) {
    if (size == 0 || size > pow(10, 8))
        return NULL;

//first block allocation
    if (!listOfBlocks.totalAllocatedBlocks) {
        void *firstBlockAdress = sbrk(sizeof(MallocMetadata) + size);
        if (firstBlockAdress == (void *) (-1))
            return NULL;

        MallocMetadata *firstMeta = (MallocMetadata *) firstBlockAdress;
        firstMeta->size = size;
        firstMeta->is_free = false;
        listOfBlocks.totalAllocatedBlocks++;
        listOfBlocks.totalAllocatedBytes = size;
        listOfBlocks.firstBlock = firstMeta;
        listOfBlocks.lastBlock = firstMeta;
        return (void *) (firstMeta + 1);
    } else {
        MallocMetadata *currBlock = listOfBlocks.firstBlock;
        MallocMetadata *finalLinkedBlock;

        while (currBlock != NULL) {
            if (currBlock->size >= size && currBlock->is_free) {
                int diff = currBlock->size - size - sizeof(MallocMetadata);
                if (diff >= 128)
                    return splitBlock(currBlock, size);
                else
                    return (void *) (currBlock + 1);
            }

            if (!(currBlock->next))
                finalLinkedBlock = currBlock;

            currBlock = currBlock->next;
        }

        //didn't find proper block,need to allocate
        void *newBlockAdress = sbrk(sizeof(MallocMetadata) + size);
        if (newBlockAdress == (void *) (-1))
            return NULL;
        MallocMetadata *newMeta = (MallocMetadata *) newBlockAdress;
        newMeta->size = size;
        newMeta->is_free = false;
        newMeta->prev = finalLinkedBlock;
        finalLinkedBlock->next = newMeta;
        listOfBlocks.totalAllocatedBlocks++;
        listOfBlocks.totalAllocatedBytes += size;
        listOfBlocks.lastBlock = newMeta;

        return (void *) (newMeta + 1);
    }


}

void *scalloc(size_t num, size_t size) {
    if (size == 0 || size * num > pow(10, 8))
        return NULL;

    void *newBlockAdress = smalloc(num * size);
    if (!newBlockAdress)
        return NULL;
    memset(newBlockAdress, 0, num * size);
    return newBlockAdress;

}

void sfree(void *p) {
    if (!p)
        return;
    else if (((MallocMetadata *) p - 1)->is_free)
        return;

    //both neighbors are free
    if (((MallocMetadata *) p - 1)->next && (((MallocMetadata *) p - 1)->next)->is_free &&
        ((MallocMetadata *) p - 1)->prev && (((MallocMetadata *) p - 1)->prev)->is_free) {

        MallocMetadata *tempMeta = (((MallocMetadata *) p - 1)->next)->next;

        size_t totalNewFreeBlockSize = (((MallocMetadata *) p - 1)->prev)->size +
                                       ((MallocMetadata *) p - 1)->size +
                                       (((MallocMetadata *) p - 1)->next)->size + 2 * (sizeof(MallocMetadata));
        (((MallocMetadata *) p - 1)->prev)->size = totalNewFreeBlockSize;
        (((MallocMetadata *) p - 1)->prev)->next = tempMeta;

        if (tempMeta)
            tempMeta->prev = (((MallocMetadata *) p - 1)->prev);
        else
            listOfBlocks.lastBlock = (((MallocMetadata *) p - 1)->prev);

        listOfBlocks.totalAllocatedBlocks -= 2;
        listOfBlocks.numberOfFreeBlocks--;
        listOfBlocks.numberOfFreeBytes += ((MallocMetadata *) p - 1)->size + 2 * (sizeof(MallocMetadata));


        //only higher neighbor is free
    } else if (((MallocMetadata *) p - 1)->next && (((MallocMetadata *) p - 1)->next)->is_free) {
        MallocMetadata *tempMeta = (((MallocMetadata *) p - 1)->next)->next;
        size_t totalNewFreeBlockSize = ((MallocMetadata *) p - 1)->size +
                                       (((MallocMetadata *) p - 1)->next)->size + (sizeof(MallocMetadata));

        ((MallocMetadata *) p - 1)->size = totalNewFreeBlockSize;
        ((MallocMetadata *) p - 1)->is_free = true;
        ((MallocMetadata *) p - 1)->next = tempMeta;

        if (tempMeta)
            tempMeta->prev = ((MallocMetadata *) p - 1);
        else
            listOfBlocks.lastBlock = ((MallocMetadata *) p - 1);

        listOfBlocks.totalAllocatedBlocks--;
        listOfBlocks.numberOfFreeBytes += ((MallocMetadata *) p - 1)->size + sizeof(MallocMetadata);


        //only lower neighbor is free
    } else if (((MallocMetadata *) p - 1)->prev && (((MallocMetadata *) p - 1)->prev)->is_free) {
        MallocMetadata *tempMeta = ((MallocMetadata *) p - 1)->next;
        size_t totalNewFreeBlockSize = ((MallocMetadata *) p - 1)->size +
                                       (((MallocMetadata *) p - 1)->prev)->size + (sizeof(MallocMetadata));

        (((MallocMetadata *) p - 1)->prev)->size = totalNewFreeBlockSize;
        (((MallocMetadata *) p - 1)->prev)->next = tempMeta;

        if (tempMeta)
            tempMeta->prev = ((MallocMetadata *) p - 1)->prev;
        else
            listOfBlocks.lastBlock = ((MallocMetadata *) p - 1)->prev;

        listOfBlocks.totalAllocatedBlocks--;
        //no merge with neighbors
    } else {
        ((MallocMetadata *) p - 1)->is_free = true;
    }

    listOfBlocks.numberOfFreeBlocks++;
    listOfBlocks.numberOfFreeBytes += ((MallocMetadata *) p - 1)->size + sizeof(MallocMetadata);


}

void *srealloc(void *oldp, size_t size) {
    if (size == 0 || size > pow(10, 8))
        return NULL;
    if (!oldp)
        return smalloc(size);
    if (((MallocMetadata *) oldp - 1)->size >= size)
        return oldp;
    else {
        sfree(oldp);
        void *newBlockAdress = smalloc(size);
        if (!newBlockAdress)
            return NULL;

        memcpy(newBlockAdress, oldp, ((MallocMetadata *) oldp - 1)->size);

        return newBlockAdress;
    }
}

size_t _num_free_blocks() {
    return listOfBlocks.totalAllocatedBlocks - listOfBlocks.numberOfFreeBlocks;
}

size_t _num_free_bytes() {
    return listOfBlocks.totalAllocatedBytes - listOfBlocks.numberOfFreeBytes;
}

size_t _num_allocated_blocks() {
    return listOfBlocks.totalAllocatedBlocks;
}

size_t _num_allocated_bytes() {
    return listOfBlocks.totalAllocatedBytes;
}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}

size_t _num_meta_data_bytes() {
    return (listOfBlocks.totalAllocatedBlocks) * (_size_meta_data());
}

void printAllMetaBlocks() {
    auto currBlock = listOfBlocks.firstBlock;
    while (currBlock != nullptr) {
        cout << "curr Meta block adress:" << currBlock << endl;
        cout << "curr Meta block size:" << currBlock->size << endl;
        cout << "curr Meta block free?:" << (bool) currBlock->is_free << endl;
        cout << "Number of Total allocted blocks:" << _num_allocated_blocks() << endl;
        cout << "Number of Total allocated bytes:" << _num_allocated_bytes() << endl;
        cout << "Number of free blocks:" << _num_free_blocks() << endl;
        cout << "Number of free bytes:" << _num_free_bytes() << endl;
        cout << "Adress of static list:" << &listOfBlocks << endl;
        cout << "========================" << endl;
        currBlock = currBlock->next;
    }
    cout << "///" << endl;
}