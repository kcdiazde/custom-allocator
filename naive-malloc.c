#include <stdio.h> // printf
#include <unistd.h> // sbrk
#include <stdint.h> // int types
#include <assert.h>

void * naiveMalloc(size_t size) {
    const void * k_InvalidResult = (void*) -1;

    void * heapPtr = sbrk(0);
    void * incResult = sbrk(size);
   
    if ((incResult == k_InvalidResult) ||  (heapPtr != incResult)) {
        printf("Ouch!\n");
        return NULL;
    }
    else {
        return heapPtr;
    }
}

int main() {

    size_t sizeToReq = 10;
    uint8_t * arr = NULL;

    arr = (uint8_t *) naiveMalloc(sizeof(uint8_t) * 25);

    assert(arr != NULL);

    for (size_t ii = 0; ii < sizeToReq; ++ii) {
        arr[ii] = ii;
    }

    for (size_t ii = 0; ii < sizeToReq; ++ii) {
        printf("arr[%ld] = %d\n", ii, arr[ii]);
    }


    return 0;
}
