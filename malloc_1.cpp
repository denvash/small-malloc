#include <cmath>
#include <unistd.h>

void *smalloc(size_t size) {
    if (size == 0 || size > pow(10, 8))
        return nullptr;

    void *newBlock = sbrk(size);

    if (newBlock == (void *) (-1))
        return nullptr;
    else
        return newBlock;
}
