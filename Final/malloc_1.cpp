#include <cmath>
#include <unistd.h>

#define ALLOCATION_ERROR ((void*) (-1))

void *smalloc(size_t size) {
    if (size == 0 || size > pow(10, 8))
        return nullptr;

    void *newBlock = sbrk(size);

    if (newBlock == ALLOCATION_ERROR)
        return nullptr;
    else
        return newBlock;
}
