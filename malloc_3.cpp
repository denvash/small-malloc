#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
#include <cmath>
#include <cstring>
#include <cstddef>

#define ALLOCATION_ERROR ((void*) (-1))
#define MMAP_THRESHOLD 131072
#define MUNMAP_FAILURE -1

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

static ListOfMallocMetadata listOfBlocks = ListOfMallocMetadata();
MallocMetadata *listOfMMAP = nullptr;

size_t _num_allocated_blocks() {
    return listOfBlocks.totalAllocatedBlocks;
}

size_t _num_allocated_bytes() {
    return listOfBlocks.totalAllocatedBytes;
}

size_t _num_free_blocks() {
    return listOfBlocks.numberOfFreeBlocks;
}

size_t _num_free_bytes() {
    return listOfBlocks.numberOfFreeBytes;

}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}

size_t _num_meta_data_bytes() {
    return _num_allocated_blocks() * (_size_meta_data());
}

bool sizeInCapacityRange(size_t size) {
    return size > pow(10, 8);
}

void printAllMetaBlocks() {
    auto currBlock = listOfBlocks.firstBlock;
    cout << "---HEAP blocks---" << endl;
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
    cout << "---MMAP blocks---" << endl;
    currBlock = listOfMMAP;
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

void *getData(MallocMetadata *metaData) {
    return (void *) (metaData + 1);
}

MallocMetadata *getMetaData(void *data) {
    return !data ? nullptr : ((MallocMetadata *) data - 1);
}

void removeListItem(MallocMetadata *meta) {
    auto curr = listOfMMAP;

    while (curr != meta && curr != nullptr) {
        curr = curr->next;
    }

    if (curr == nullptr) {
        return;
    }

    auto prev = curr->prev;
    auto next = curr->next;

    // Empty list
    if (!prev && !next) {
        listOfMMAP = nullptr;
        return;
    }

        // Remove from list start
    else if (!prev) {
        next->prev = nullptr;
    }

        // Remove from list end
    else if (!next) {
        prev->next = nullptr;
    }

        // Remove from list inner
    else {
        prev->next = next;
        next->prev = prev;
    }
}

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
    listOfBlocks.totalAllocatedBlocks++;
    listOfBlocks.numberOfFreeBytes -= (leastSignificantSize + _size_meta_data());
    listOfBlocks.totalAllocatedBytes -= _size_meta_data();
    return blockAddress;
}

bool isWildernessBlockExists(size_t requestedSize) {
    auto first = listOfBlocks.firstBlock;
    if (!first)
        return false;
    auto last = listOfBlocks.lastBlock;

    while (first != last) {
        if (first->is_free && requestedSize <= first->size)
            return false;
        first = first->next;
    }

    return last->is_free;
}

void *smalloc(size_t size) {
    if (size == 0 || sizeInCapacityRange(size))
        return nullptr;
    bool isMMAP = size >= MMAP_THRESHOLD;
    // first block allocation
    if ((!listOfBlocks.firstBlock && !isMMAP) || (!listOfMMAP && isMMAP)) {
        void *firstBlockAddress =
                isMMAP ?
                mmap(0, _size_meta_data() + size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0) :
                sbrk(_size_meta_data() + size);

        if (firstBlockAddress == ALLOCATION_ERROR)
            return nullptr;

        auto firstMeta = (MallocMetadata *) firstBlockAddress;
        firstMeta->size = size;
        firstMeta->is_free = false;
        listOfBlocks.totalAllocatedBlocks++;
        listOfBlocks.totalAllocatedBytes += size;
        if (!isMMAP) {
            listOfBlocks.firstBlock = firstMeta;
            listOfBlocks.lastBlock = firstMeta;
        } else
            listOfMMAP = firstMeta;

        return getData(firstMeta);
    } else {
        if (!isMMAP && isWildernessBlockExists(size)) {
            auto lastBlock = listOfBlocks.lastBlock;
            auto isEnoughMemory = size <= lastBlock->size;

            // need exactly size_t type for sbrk
            size_t bytesDiff = abs(long(size - lastBlock->size));

            if (isEnoughMemory) {
                int diff = lastBlock->size - size - _size_meta_data();
                if (diff >= 128) {
                    // numberOfFreeBytes & totalAllocatedBlocks called within splitBlock
                    return splitBlock(lastBlock, size);
                }
            } else {
                lastBlock->size = size;
                if (sbrk(bytesDiff) == ALLOCATION_ERROR) {
                    return nullptr;
                }
                listOfBlocks.totalAllocatedBytes += bytesDiff;
            }

            listOfBlocks.numberOfFreeBlocks--;
            lastBlock->is_free = false;

            auto newBytes = lastBlock->size + (isEnoughMemory ? 0 : bytesDiff);
            auto prevBytes = listOfBlocks.numberOfFreeBytes;
            listOfBlocks.numberOfFreeBytes = prevBytes < newBytes ? 0 : prevBytes - newBytes;
            return getData(lastBlock);
        }

        auto currBlock = isMMAP ? listOfMMAP : listOfBlocks.firstBlock;
        MallocMetadata *finalLinkedBlock;

        while (currBlock != nullptr) {
            if (size <= currBlock->size && currBlock->is_free) {

                if (!isMMAP) {
                    // Keep the sign, don't use size_t, use int instead
                    int diff = currBlock->size - size - _size_meta_data();
                    if (diff >= 128) // update of listOfBlock is inside split block
                        return splitBlock(currBlock, size);
                }
                currBlock->is_free = false;
                listOfBlocks.numberOfFreeBlocks--;
                listOfBlocks.numberOfFreeBytes -= currBlock->size;
                return getData(currBlock);
            }

            if (!(currBlock->next))
                finalLinkedBlock = currBlock;

            currBlock = currBlock->next;
        }

        // didn't find proper block,need to allocate
        void *newBlockAddress =
                isMMAP ?
                mmap(0, _size_meta_data() + size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0) :
                sbrk(_size_meta_data() + size);

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
    if (size == 0 || sizeInCapacityRange(size * num))
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

    auto isMMAP = metaData->size >= MMAP_THRESHOLD;

    if (isMMAP) {
        // listOfMMAP can't be null
        removeListItem(metaData);

        listOfBlocks.totalAllocatedBytes -= metaData->size;
        listOfBlocks.totalAllocatedBlocks--;

        // Free memory
        // Can fail (return -1), no way to handle
        munmap(metaData, metaData->size + _size_meta_data());
    } else // both neighbors are free
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
        listOfBlocks.totalAllocatedBytes += 2 * _size_meta_data();
        listOfBlocks.numberOfFreeBlocks--;
        listOfBlocks.numberOfFreeBytes += metaData->size + 2 * _size_meta_data();


        //only higher neighbor is free
    } else if (metaData->next && (metaData->next)->is_free) {
        auto tempMeta = (metaData->next)->next;
        size_t totalNewFreeBlockSize = (metaData->size +
                                        (metaData->next)->size + _size_meta_data());

        metaData->is_free = true;
        metaData->next = tempMeta;

        if (tempMeta)
            tempMeta->prev = metaData;
        else
            listOfBlocks.lastBlock = metaData;

        listOfBlocks.totalAllocatedBlocks--;
        listOfBlocks.totalAllocatedBytes += _size_meta_data();
        listOfBlocks.numberOfFreeBytes += metaData->size + _size_meta_data();
        metaData->size = totalNewFreeBlockSize;

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
        listOfBlocks.totalAllocatedBytes += _size_meta_data();
        listOfBlocks.numberOfFreeBytes += metaData->size + _size_meta_data();
        //no merge with neighbors
    } else {
        metaData->is_free = true;
        listOfBlocks.numberOfFreeBlocks++;
        listOfBlocks.numberOfFreeBytes += metaData->size;
    }
}

void *srealloc(void *oldp, size_t size) {
    if (size == 0 || sizeInCapacityRange(size))
        return nullptr;

    if (!oldp)
        return smalloc(size);

    auto oldMetaData = getMetaData(oldp);
    bool isMMAP = oldMetaData->size >= MMAP_THRESHOLD;

    if (oldMetaData->size >= size) {
        if (!isMMAP) {
            int diff = oldMetaData->size - size - _size_meta_data();
            if (diff >= 128) {
                // numberOfFreeBytes & totalAllocatedBlocks called within splitBlock
                return splitBlock(oldMetaData, size);
            }
        }

        if (oldMetaData->is_free) {
            oldMetaData->is_free = false;
            listOfBlocks.numberOfFreeBlocks--;
            listOfBlocks.numberOfFreeBytes -= oldMetaData->size;
        }
        return oldp;

    } else {
        if (!isMMAP) {
            auto lastBlock = listOfBlocks.lastBlock;

            if (oldMetaData == lastBlock) {
                if (listOfBlocks.numberOfFreeBlocks && oldMetaData->is_free)  {
                    listOfBlocks.numberOfFreeBlocks--;
                }

                auto newBytes = lastBlock->size;
                auto prevBytes = listOfBlocks.numberOfFreeBytes;
                listOfBlocks.numberOfFreeBytes = prevBytes < newBytes ? 0 : prevBytes - newBytes;

                size_t diffBytes = abs(long(size - lastBlock->size));
                listOfBlocks.totalAllocatedBytes += diffBytes;
                lastBlock->size = size;
                if (sbrk(diffBytes) == ALLOCATION_ERROR)
                    return nullptr;
                return getData(lastBlock);
            } else {
                auto higherMeta = oldMetaData->next;
                auto lowerMeta = oldMetaData->prev;
                // try to merge with higher address
                if (higherMeta && (oldMetaData->size + higherMeta->size) >= size) {
                    auto nextHigherMeta = higherMeta->next;
                    oldMetaData->next = nextHigherMeta;
                    oldMetaData->size = oldMetaData->size + higherMeta->size + _size_meta_data();
                    listOfBlocks.numberOfFreeBytes -= higherMeta->size;
                    listOfBlocks.numberOfFreeBlocks--;
                    listOfBlocks.totalAllocatedBytes += _size_meta_data();
                    listOfBlocks.totalAllocatedBlocks--;

                    if (nextHigherMeta)
                        nextHigherMeta->prev = oldMetaData;
                    else
                        listOfBlocks.lastBlock = oldMetaData;

                    return oldp;

                } //try to merge with lower address
                else if (lowerMeta && (oldMetaData->size + lowerMeta->size) >= size) {
                    auto nextHigherMeta = oldMetaData->next;
                    lowerMeta->next = nextHigherMeta;
                    if(lowerMeta->is_free)
                        listOfBlocks.numberOfFreeBytes -= lowerMeta->size;
                    listOfBlocks.numberOfFreeBlocks--;
                    listOfBlocks.totalAllocatedBytes += _size_meta_data();
                    listOfBlocks.totalAllocatedBlocks--;

                    if (nextHigherMeta)
                        nextHigherMeta->prev = lowerMeta;
                    else
                        listOfBlocks.lastBlock = lowerMeta;

                    lowerMeta->size += oldMetaData->size + _size_meta_data();
                    lowerMeta->is_free = false;
                    return getData(lowerMeta);


                } //try to merge with both neighbors
                else if (lowerMeta && higherMeta &&
                         (lowerMeta->size + oldMetaData->size + higherMeta->size+2*_size_meta_data()) >= size) {
                    auto nextHigherMeta = higherMeta->next;
                    lowerMeta->next = nextHigherMeta;
                    lowerMeta->size += oldMetaData->size + higherMeta->size + 2 * (_size_meta_data());
                    listOfBlocks.numberOfFreeBytes -= (oldMetaData->size + higherMeta->size);
                    listOfBlocks.numberOfFreeBlocks -= 2;
                    listOfBlocks.totalAllocatedBlocks -= 2;
                    listOfBlocks.totalAllocatedBytes += 2 * (_size_meta_data());

                    if (nextHigherMeta)
                        nextHigherMeta->prev = lowerMeta;
                    else
                        listOfBlocks.lastBlock = lowerMeta;

                    lowerMeta->is_free = false;
                    return getData(lowerMeta);
                }
            }
        }

        // need to find free fitting block or allocate with sbrk
        void *newBlockAddress = smalloc(size);
        if (!newBlockAddress)
            return nullptr;
        sfree(oldp);

        memcpy(newBlockAddress, oldp, oldMetaData->size);
        return newBlockAddress;

    }
}
