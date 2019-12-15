#include <cassert>
#include "../malloc_3.h"

struct TestData {
    size_t freeBlocks;
    size_t freeBytes;
    size_t allocBlocks;
    size_t allocBytes;

    size_t metaBytes() { return metaSize * allocBlocks; };
    size_t metaSize;

};

void assertAll(TestData &data) {
    TestData realData;
    realData.freeBlocks = _num_free_blocks();
    realData.freeBytes = _num_free_bytes();
    realData.allocBlocks = _num_allocated_blocks();
    realData.allocBytes = _num_allocated_bytes();
    realData.metaSize = _size_meta_data();
    assert(realData.freeBlocks == data.freeBlocks);
    assert(realData.freeBytes == data.freeBytes);
    assert(realData.allocBlocks == data.allocBlocks);
    assert(realData.allocBytes == data.allocBytes);
    assert(realData.metaSize == data.metaSize);
}

int main() {

    TestData data;
    data.metaSize = _size_meta_data();
    data.freeBlocks = 0;
    data.freeBytes = 0;
    data.allocBlocks = 0;
    data.allocBytes = 0;

//    size_t mSz = _size_meta_data();

    //basic smalloc
    //allocate 0
    smalloc(0);
    assertAll(data);
    //allocate 1000000000000
    smalloc(100000000000);
    assertAll(data);
    //allocate 14 and use all 14 bytes
    char *c1 = (char *) smalloc(14);
    data.allocBytes += 14;
    data.allocBlocks += 1;
    assertAll(data);
    for (int i = 0; i < 14; ++i)
        c1[i] = 30;

    //basic free
    //free the previous 14 bytes

    // TODO: CONTINUE FROM HERE
    sfree(c1);
    data.freeBlocks += 1;
    data.freeBytes += 14;
    assertAll(data);

    //basic scalloc
    //allocate 12 bytes and see if they are reset to 0 (since we are reusing the ones from smalloc)
    int *arr = (int *) scalloc(3, 4);
    data.freeBlocks -= 1;
    data.freeBytes -= 14;
    assertAll(data);
    for (int i = 0; i < 3; ++i)
        assert(arr[i] == 0);

    //basic realloc
    //there is no basic realloc. will be tested in advance realloc.

    //challenge 1
    //allocate 1000, free and allocate 500 to see if it splits(should)
    void *p = smalloc(1000);
    data.allocBlocks += 1;
    data.allocBytes += 1000;
    assertAll(data);

    sfree(p);
    data.freeBlocks += 1;
    data.freeBytes += 1000;
    assertAll(data);

    smalloc(500);
    data.freeBytes -= 500 + data.metaSize;
    data.allocBlocks += 1;
    data.allocBytes -= data.metaSize;
    assertAll(data);

    //allocate 400 to clean up
    smalloc(400);
    data.freeBlocks -= 1;
    data.freeBytes -= 500 - data.metaSize;
    assertAll(data);

    //allocate 1000, free and allocate 900 to see it it splits(should not)
    p = smalloc(1000);
    data.allocBlocks += 1;
    data.allocBytes += 1000;
    assertAll(data);

    sfree(p);
    data.freeBlocks += 1;
    data.freeBytes += 1000;
    assertAll(data);

    smalloc(900);
    data.freeBlocks -= 1;
    data.freeBytes -= 1000;
    assertAll(data);

    //challenge 2
    //allocate 500,1000,500
    void *p1;
    void *p2;
    void *p3;

    p1 = smalloc(500);
    p2 = smalloc(1000);
    p3 = smalloc(500);

    data.allocBlocks += 3;
    data.allocBytes += 2000;
    assertAll(data);

    //free middle,right
    sfree(p2);
    sfree(p3);
    data.freeBlocks += 1;
    data.freeBytes += 1500 + data.metaSize;
    data.allocBlocks -= 1;
    data.allocBytes += data.metaSize;
    assertAll(data);

    //allocate 1400
    p2 = smalloc(1400);
    data.freeBlocks -= 1;
    data.freeBytes = 0;
    assertAll(data);

    //free 1400,500
    sfree(p2);
    sfree(p1);
    data.freeBlocks += 1;
    data.freeBytes += 2000 + 2 * data.metaSize;
    data.allocBlocks -= 1;
    data.allocBytes += data.metaSize;
    assertAll(data);

    //allocate 2000
    smalloc(2000);
    data.freeBlocks -= 1;
    data.freeBytes = 0;

    //allocate 100,100,200
    p1 = smalloc(100);
    p2 = smalloc(100);
    p3 = smalloc(200);
    data.allocBlocks += 3;
    data.allocBytes += 400;

    //free left,right,middle in that order
    sfree(p1);
    sfree(p3);
    sfree(p2);
    data.freeBlocks += 1;
    data.freeBytes += 400 + 2 * data.metaSize;
    data.allocBlocks -= 2;
    data.allocBytes += 2 * data.metaSize;
    assertAll(data);

    //allocate 400 to seal up the gap from challenge 2
    smalloc(400);
    data.freeBlocks -= 1;
    data.freeBytes = 0;
    assertAll(data);

    //challenge 3
    //allocate 200
    p = smalloc(200);
    data.allocBlocks += 1;
    data.allocBytes += 200;
    assertAll(data);

    //free 200
    sfree(p);
    data.freeBlocks += 1;
    data.freeBytes += 200;
    assertAll(data);

    //allocate 300
    smalloc(300);
    data.freeBlocks -= 1;
    data.freeBytes = 0;
    data.allocBytes += 100;
    assertAll(data);

    //challenge 4
    //allocate 300000
    p = smalloc(300000);
    data.allocBlocks += 1;
    data.allocBytes += 300000;
    assertAll(data);

    //free 300000
    sfree(p);
    data.allocBlocks -= 1;
    data.allocBytes -= 300000;
    assertAll(data);

    //advanced realloc
    //realloc to smaller size
    p = smalloc(1000);
    data.allocBlocks += 1;
    data.allocBytes += 1000;

    p = srealloc(p, 900);
    assertAll(data);

    //realloc to original size
    p = srealloc(p, 1000);
    assertAll(data);

    //realloc larger size
    p = srealloc(p, 1200);
    data.allocBytes += 200;
    assertAll(data);

    //allocate 100,100,100,10
    p1 = smalloc(100);
    p2 = smalloc(100);
    p3 = smalloc(100);
    smalloc(10);
    data.allocBlocks += 4;
    data.allocBytes += 310;
    assertAll(data);

    //free left,right
    sfree(p1);
    sfree(p3);
    data.freeBlocks += 2;
    data.freeBytes += 200;
    assertAll(data);

    //realloc middle to 150
    p3 = srealloc(p2, 150);
    data.freeBlocks -= 1;
    data.freeBytes -= 100;
    data.allocBytes += data.metaSize;
    data.allocBlocks -= 1;
    assertAll(data);
    //make sure we merged with the next block (which means our start remains the same)
    assert(p3 == p2);

    //realloc to 180
    p2 = srealloc(p2, 180);
    assertAll(data);

    //realloc to 250
    p2 = srealloc(p2, 250);
    data.freeBlocks -= 1;
    data.freeBytes -= 100;
    data.allocBytes += data.metaSize;
    data.allocBlocks -= 1;
    assertAll(data);
    assert(p1 == p2);

    //allocate 100,100,100
    p1 = smalloc(100);
    p2 = smalloc(100);
    p3 = smalloc(100);
    data.allocBlocks += 3;
    data.allocBytes += 300;
    assertAll(data);

    //free left,right
    sfree(p1);
    sfree(p3);
    data.freeBlocks += 2;
    data.freeBytes += 200;
    assertAll(data);

    //realloc middle to 335 //should be merged
    srealloc(p2, 335);
    data.freeBlocks -= 2;
    data.freeBytes = 0;
    data.allocBlocks -= 2;
    data.allocBytes += data.metaSize * 2;
    assertAll(data);

    //allocate 20
    p2 = smalloc(20);
    data.allocBlocks += 1;
    data.allocBytes += 20;
    assertAll(data);

    //allocate 10
    p1 = smalloc(10);
    data.allocBlocks += 1;
    data.allocBytes += 10;
    assertAll(data);

    //free 10
    sfree(p1);
    data.freeBlocks += 1;
    data.freeBytes += 10;
    assertAll(data);

    //realloc to 4000 (should free current, and extend 10 to 4000)
    p2 = srealloc(p2, 4000);
    data.allocBytes += 3990;
    data.freeBytes += 10;
    assertAll(data);

    //allocate 900
    p = smalloc(900);
    data.allocBlocks += 1;
    data.allocBytes += 900;
    assertAll(data);

    //allocate 20 to cover up the hold
    smalloc(20);
    data.freeBlocks -= 1;
    data.freeBytes -= 20;
    assertAll(data);

    //allocate 10
    smalloc(10);
    data.allocBlocks += 1;
    data.allocBytes += 10;
    assertAll(data);

    //allocate 400
    p2 = smalloc(400);
    data.allocBlocks += 1;
    data.allocBytes += 400;
    assertAll(data);

    //allocate 10
    smalloc(10);
    data.allocBlocks += 1;
    data.allocBytes += 10;
    assertAll(data);

    //free 900
    sfree(p);
    data.freeBlocks += 1;
    data.freeBytes += 900;
    assertAll(data);

    //realloc 400 to 600 (will take 900, and split)
    srealloc(p2, 600);
    data.freeBlocks += 1;
    data.freeBytes += 400 - 600 - data.metaSize;
    data.allocBlocks += 1;
    data.allocBytes -= data.metaSize;
    assertAll(data);

    //allocate 300000
    p = smalloc(300000);
    data.allocBlocks += 1;
    data.allocBytes += 300000;
    ((char *) p)[100000] = 42;
    assertAll(data);

    //realloc to 140000
    p = srealloc(p, 140000);
    data.allocBytes -= 160000;
    assert(((char *) p)[100000] == 42);
    assertAll(data);

    //realloc to 280000
    p = srealloc(p, 280000);
    data.allocBytes += 140000;
    assert(((char *) p)[100000] == 42);
    assertAll(data);

    //realloc to 480000
    p = srealloc(p, 480000);
    data.allocBytes += 200000;
    assert(((char *) p)[100000] == 42);
    assertAll(data);


    return 0;
}