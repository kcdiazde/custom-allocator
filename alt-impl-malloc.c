#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>

struct MemBlock;

struct MemBlock {
     
};

void * llMalloc(size_t size) {
    const void * k_InvalidResult = (void*) -1;

    void * heapPtr = sbrk(0);
    void * incResult = sbrk(size);
   
    if (incResult == k_InvalidResult) {
        return NULL;
    }
    else {
        assert (heapPtr == incResult);
        return heapPtr;
    }
}

void fillArr(uint16_t * arr, size_t size) {
    for (size_t ii = 0; ii < size; ++ii) {
        arr[ii] = ii;
    }
}

uint32_t checkArr(uint16_t * arr, size_t size) {
    uint32_t readValue = 0;
    for (size_t ii = 0; ii < size; ++ii) {
        readValue += arr[ii];
    }

    return readValue;
}

int main() {

    size_t sizeToReq = 10;
    uint16_t * arr1 = NULL;
    uint16_t * arr2 = NULL;

    arr1 = (uint16_t *) llMalloc(sizeof(uint16_t) * sizeToReq);
    assert(arr1 != NULL);

    arr2 = (uint16_t *) llMalloc(sizeof(uint16_t) * sizeToReq);
    assert(arr2 != NULL);

    fillArr(arr1, sizeToReq);
    fillArr(arr2, sizeToReq);

    uint32_t value = checkArr(arr1, 20);

    printf("readValue = %d\n", value);


    return 0;
}
