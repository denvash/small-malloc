#ifndef WET2_UTILS_H
#define WET2_UTILS_H

#define NEXT_BYTE (1)
#define PREV_BYTE (-1)
#define ALLOCATION_ERROR ((void*) (-1))

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

static ListOfMallocMetadata listOfBlocks = ListOfMallocMetadata();

void *getData(MallocMetadata *metaData) {
    return (void *) (metaData + NEXT_BYTE);
}

MallocMetadata *getMetaData(void *data) {
    return !data ? nullptr : ((MallocMetadata *) data + PREV_BYTE);
}

#endif
