#include <cassert>
#include "../malloc_2.h"

void assertAll(size_t e1, size_t e2, size_t e3, size_t e4, size_t e5, size_t e6) {
    assert(_num_free_blocks() == e1);
    assert(_num_free_bytes() == e2);
    assert(_num_allocated_blocks() == e3);
    assert(_num_allocated_bytes() == e4);
    assert(_num_meta_data_bytes() == e5);
    assert(_size_meta_data() == e6);
}

int main() {

    size_t mSz = _size_meta_data();

    void *nullPointer = smalloc(0);

    assert(nullptr == nullPointer);

    assertAll(0, 0, 0, 0, 0, mSz);
    void *p = smalloc(8);
    assertAll(0, 0, 1, 8, mSz, mSz);
    smalloc(5);
    assertAll(0, 0, 2, 13, 2 * mSz, mSz);
    smalloc(2);
    assertAll(0, 0, 3, 15, 3 * mSz, mSz);
    void *p2 = smalloc(342346);
    assertAll(0, 0, 4, 342361, 4 * mSz, mSz);

    // TODO: CONTINUE FROM HERE
    sfree(p2);
    assertAll(1, 342346, 4, 342361, 4 * mSz, mSz);
    p2 = smalloc(3);
    assertAll(0, 0, 4, 342361, 4 * mSz, mSz);
    srealloc(p2, 2);
    assertAll(0, 0, 4, 342361, 4 * mSz, mSz);
    srealloc(p, 20);
    assertAll(1, 8, 5, 342381, 5 * mSz, mSz);
    smalloc(123);
    assertAll(1, 8, 6, 342504, 6 * mSz, mSz);
    smalloc(8);
    assertAll(0, 0, 6, 342504, 6 * mSz, mSz);
    smalloc(10000000000);
    assertAll(0, 0, 6, 342504, 6 * mSz, mSz);
    p = smalloc(50);
    ((int *) p)[4] = 2321423;
    sfree(p);
    int *a = (int *) scalloc(10, 4);
    assertAll(0, 0, 7, 342554, 7 * mSz, mSz);
    for (int i = 0; i < 10; ++i)
        assert(a[i] == 0);


    return 0;
}