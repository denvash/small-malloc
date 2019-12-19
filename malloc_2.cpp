#include <iostream>
#include <unistd.h>
#include <cmath>
#include <cstring>
#include <cstddef>

#define ALLOCATION_ERROR ((void*) (-1))

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata *next;
    MallocMetadata *prev;
};

struct ListOfMallocMetadata {
    size_t totalAllocatedBlocks;
    size_t totalAllocatedBytes;
    size_t numberOfBlocksInUse;
    size_t numberOfBytesInUse;
    MallocMetadata *firstBlock;
    MallocMetadata *lastBlock;

    ListOfMallocMetadata() : totalAllocatedBlocks(0), totalAllocatedBytes(0), numberOfBlocksInUse(0),
                             numberOfBytesInUse(0), firstBlock(), lastBlock() {}
};

static ListOfMallocMetadata listOfBlocks = ListOfMallocMetadata();

size_t _num_allocated_blocks() {
    return listOfBlocks.totalAllocatedBlocks;
}

size_t _num_allocated_bytes() {
    return listOfBlocks.totalAllocatedBytes;
}

size_t _num_free_blocks() {
    return _num_allocated_blocks() - listOfBlocks.numberOfBlocksInUse;
}

size_t _num_free_bytes() {
    return _num_allocated_bytes() - listOfBlocks.numberOfBytesInUse;
}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}

size_t _num_meta_data_bytes() {
    return _num_allocated_blocks() * _size_meta_data();
}

void *getData(MallocMetadata *metaData) {
    return (void *) (metaData + 1);
}

MallocMetadata *getMetaData(void *data) {
    return !data ? NULL : ((MallocMetadata *) data - 1);
}

bool sizeInCapacityRange(size_t size) {
    return size > pow(10, 8);
}

void *smalloc(size_t size) {
    if (size == 0 || sizeInCapacityRange(size))
        return NULL;

    // first block allocation
    if (!listOfBlocks.totalAllocatedBlocks) {
        void *firstBlockAddress = sbrk(_size_meta_data() + size);
        if (firstBlockAddress == ALLOCATION_ERROR)
            return NULL;

        auto firstMeta = (MallocMetadata *) firstBlockAddress;
        firstMeta->size = size;
        firstMeta->is_free = false;
        listOfBlocks.totalAllocatedBlocks++;
        listOfBlocks.totalAllocatedBytes = size;
        listOfBlocks.numberOfBlocksInUse++;
        listOfBlocks.numberOfBytesInUse = size;
        listOfBlocks.firstBlock = firstMeta;
        listOfBlocks.lastBlock = firstMeta;
        return getData(firstMeta);
    } else {

        auto currBlock = listOfBlocks.firstBlock;
        MallocMetadata *finalBlock = NULL;

        while (currBlock != NULL) {
            if (size <= currBlock->size && currBlock->is_free) {
                currBlock->is_free = false;
                listOfBlocks.numberOfBlocksInUse++;
                listOfBlocks.numberOfBytesInUse += currBlock->size;
                return getData(currBlock);
            }
            if (!(currBlock->next)) {
                finalBlock = currBlock;
            }
            currBlock = currBlock->next;
        }

        // didn't find proper block,need to allocate
        void *newBlockAddress = sbrk(_size_meta_data() + size);
        if (newBlockAddress == ALLOCATION_ERROR) {
            return NULL;
        }

        auto newMeta = (MallocMetadata *) newBlockAddress;
        newMeta->size = size;
        newMeta->is_free = false;

        newMeta->prev = finalBlock;
        finalBlock->next = newMeta;

        listOfBlocks.totalAllocatedBlocks++;
        listOfBlocks.totalAllocatedBytes += size;
        listOfBlocks.numberOfBlocksInUse++;
        listOfBlocks.numberOfBytesInUse += size;
        listOfBlocks.lastBlock = newMeta;

        return getData(newMeta);
    }
}

void *scalloc(size_t num, size_t size) {
    if (size == 0 || sizeInCapacityRange(size * num))
        return NULL;

    void *newBlockAddress = smalloc(num * size);
    if (!newBlockAddress)
        return NULL;
    memset(newBlockAddress, 0, num * size);
    return newBlockAddress;
}

void sfree(void *p) {
    if (!p) return;

    auto metaData = getMetaData(p);
    if (metaData->is_free) return;

    metaData->is_free = true;
    listOfBlocks.numberOfBlocksInUse--;
    listOfBlocks.numberOfBytesInUse -= metaData->size;
}

void *srealloc(void *oldP, size_t size) {
    if (size == 0 || sizeInCapacityRange(size))
        return NULL;
    if (!oldP)
        return smalloc(size);

    auto oldMetaData = getMetaData(oldP);

    // Reuse
    if (size <= oldMetaData->size)
        return oldP;
    else {
        sfree(oldP);
        void *newBlockAddress = smalloc(size);
        if (!newBlockAddress)
            return NULL;

        memcpy(newBlockAddress, oldP, oldMetaData->size);
        return newBlockAddress;
    }
}
