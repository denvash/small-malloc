#include <iostream>
#include <unistd.h>
#include <cmath>
#include <cstring>
#include "malloc_2.h"
#include "MallocMetaData.h"
#include <cstddef>

using namespace std;

void *splitBlock(void *blockAddress, size_t leastSignificantSize) {
    auto leastSignificantMeta = (MallocMetadata *) blockAddress;
    size_t mostSignificantSize = leastSignificantMeta->size - leastSignificantSize - _size_meta_data();

    leastSignificantMeta->size = leastSignificantSize;
    leastSignificantMeta->is_free = false;

    void *splittedBlock = (void *) ((char *) (leastSignificantMeta + 1) + leastSignificantSize);
    auto mostSignificantMeta = (MallocMetadata *) splittedBlock;

    mostSignificantMeta->size = mostSignificantSize;
    mostSignificantMeta->is_free = true;
    mostSignificantMeta->prev = leastSignificantMeta;

    if (!leastSignificantMeta->next) {
        leastSignificantMeta->next = mostSignificantMeta;
        listOfBlocks.lastBlock = mostSignificantMeta;
    } else {
        auto tempMeta = leastSignificantMeta->next;
        leastSignificantMeta->next = mostSignificantMeta;
        mostSignificantMeta->next = tempMeta;
        tempMeta->prev = mostSignificantMeta;
    }
    return blockAddress;
}

bool isWildernessBlockExists(size_t requestedSize) {
    auto last = listOfBlocks.lastBlock;
    auto first = listOfBlocks.firstBlock;

    while (first != last) {
        if (first->is_free && requestedSize <= first->size)
            return false;

        first = first->next;
    }

    return last->is_free;
}

void *smalloc(size_t size) {
    if (size == 0 || size > pow(10, 8))
        return nullptr;

    // first block allocation
    if (!listOfBlocks.totalAllocatedBlocks) {
        void *firstBlockAddress = sbrk(_size_meta_data() + size);
        if (firstBlockAddress == ALLOCATION_ERROR)
            return nullptr;

        auto firstMeta = (MallocMetadata *) firstBlockAddress;
        firstMeta->size = size;
        firstMeta->is_free = false;
        listOfBlocks.totalAllocatedBlocks++;
        listOfBlocks.totalAllocatedBytes = size;
        listOfBlocks.firstBlock = firstMeta;
        listOfBlocks.lastBlock = firstMeta;
        return getData(firstMeta);

    } else {
        if (isWildernessBlockExists(size)) {
            auto lastBlock = listOfBlocks.lastBlock;
            if (lastBlock->size < size) {
                lastBlock->size = size;
            }
            // TODO: not sure if need to call sbrk()
            return getData(lastBlock);
        }

        auto currBlock = listOfBlocks.firstBlock;
        MallocMetadata *finalLinkedBlock;

        while (currBlock != nullptr) {
            if (currBlock->size >= size && currBlock->is_free) {
                size_t diff = currBlock->size - size - _size_meta_data();
                if (diff >= 128)
                    return splitBlock(currBlock, size);
                else
                    return getData(currBlock);
            }

            if (!(currBlock->next))
                finalLinkedBlock = currBlock;

            currBlock = currBlock->next;
        }

        // didn't find proper block,need to allocate
        void *newBlockAddress = sbrk(_size_meta_data() + size);
        if (newBlockAddress == ALLOCATION_ERROR)
            return nullptr;

        auto newMeta = (MallocMetadata *) newBlockAddress;
        newMeta->size = size;
        newMeta->is_free = false;

        newMeta->prev = finalLinkedBlock;
        finalLinkedBlock->next = newMeta;

        listOfBlocks.totalAllocatedBlocks++;
        listOfBlocks.totalAllocatedBytes += size;
        listOfBlocks.lastBlock = newMeta;

        return getData(newMeta);
    }


}

void *scalloc(size_t num, size_t size) {
    if (size == 0 || size * num > pow(10, 8))
        return nullptr;

    void *newBlockAddress = smalloc(num * size);

    if (!newBlockAddress)
        return nullptr;

    memset(newBlockAddress, 0, num * size);
    return newBlockAddress;
}

void sfree(void *p) {
    if (!p)
        return;

    auto metaData = getMetaData(p);

    if (metaData->is_free)
        return;

    // both neighbors are free
    if (metaData->next && (metaData->next)->is_free &&
        metaData->prev && metaData->prev->is_free) {

        auto *tempMeta = (metaData->next)->next;

        size_t totalNewFreeBlockSize = (metaData->prev)->size +
                                       (metaData->size +
                                        (metaData->next)->size + 2 * _size_meta_data());

        (metaData->prev)->size = totalNewFreeBlockSize;
        (metaData->prev)->next = tempMeta;

        if (tempMeta)
            tempMeta->prev = metaData->prev;
        else
            listOfBlocks.lastBlock = (metaData->prev);

        listOfBlocks.totalAllocatedBlocks -= 2;
        listOfBlocks.numberOfFreeBlocks--;
        listOfBlocks.numberOfFreeBytes += metaData->size + 2 * _size_meta_data();


        //only higher neighbor is free
    } else if (metaData->next && (metaData->next)->is_free) {
        auto tempMeta = (metaData->next)->next;
        size_t totalNewFreeBlockSize = (metaData->size +
                                        (metaData->next)->size + _size_meta_data());

        metaData->size = totalNewFreeBlockSize;
        metaData->is_free = true;
        metaData->next = tempMeta;

        if (tempMeta)
            tempMeta->prev = metaData;
        else
            listOfBlocks.lastBlock = metaData;

        listOfBlocks.totalAllocatedBlocks--;
        listOfBlocks.numberOfFreeBytes += metaData->size + _size_meta_data();


        //only lower neighbor is free
    } else if (metaData->prev && (metaData->prev)->is_free) {
        auto tempMeta = metaData->next;
        size_t totalNewFreeBlockSize = metaData->size +
                                       (metaData->prev)->size + _size_meta_data();

        (metaData->prev)->size = totalNewFreeBlockSize;
        (metaData->prev)->next = tempMeta;

        if (tempMeta)
            tempMeta->prev = metaData->prev;
        else
            listOfBlocks.lastBlock = metaData->prev;

        listOfBlocks.totalAllocatedBlocks--;
        //no merge with neighbors
    } else {
        metaData->is_free = true;
    }

    listOfBlocks.numberOfFreeBlocks++;
    listOfBlocks.numberOfFreeBytes += metaData->size + _size_meta_data();
}

void *srealloc(void *oldp, size_t size) {
    if (size == 0 || size > pow(10, 8))
        return nullptr;

    if (!oldp)
        return smalloc(size);

    auto oldMetaData = getMetaData(oldp);

    if (oldMetaData->size >= size)
        return oldp;
    else {
        sfree(oldp);
        void *newBlockAddress = smalloc(size);
        if (!newBlockAddress)
            return nullptr;

        memcpy(newBlockAddress, oldp, oldMetaData->size);
        return newBlockAddress;
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
        cout << "curr Meta block address:" << currBlock << endl;
        cout << "curr Meta block size:" << currBlock->size << endl;
        cout << "curr Meta block free?:" << (bool) currBlock->is_free << endl;
        cout << "Number of Total allocated blocks:" << _num_allocated_blocks() << endl;
        cout << "Number of Total allocated bytes:" << _num_allocated_bytes() << endl;
        cout << "Number of free blocks:" << _num_free_blocks() << endl;
        cout << "Number of free bytes:" << _num_free_bytes() << endl;
        cout << "Address of static list:" << &listOfBlocks << endl;
        cout << "========================" << endl;
        currBlock = currBlock->next;
    }
    cout << "///" << endl;
}