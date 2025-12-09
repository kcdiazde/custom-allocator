#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <sys/mman.h>

// TODO: Detect when no pages avaliable anymore. Each for loop

#define MAX_NUM_BLOCKS 10

struct MemBlock;

struct MemBlock {
    void * pAddr; 
    size_t totalSize;
    size_t sizeAvail;
};

struct MemBlock memBlocks[MAX_NUM_BLOCKS] = {};

// To allow "dynamic" number of blocks
void initializeMemBlocks() {
    
    for (size_t blockNum = 0; blockNum < MAX_NUM_BLOCKS; ++blockNum) {
        memBlocks[blockNum].pAddr = NULL;
        memBlocks[blockNum].sizeAvail = 0;
    }
}

void * llMalloc(size_t size) {

    void * pAddr = NULL;

    // Look for avaliable memblock
    size_t blockToUse = MAX_NUM_BLOCKS;
    printf("Will search for a block to use...\n");
    for (size_t blockNum = 0; blockNum < MAX_NUM_BLOCKS; ++blockNum) {
        if (memBlocks[blockNum].sizeAvail >= size) {
            blockToUse = blockNum;        
        }
    }

    // Not found, needs a new block
    if (blockToUse == MAX_NUM_BLOCKS) {
        printf("Block not found, creating...\n");

        const long k_pageSize = sysconf(_SC_PAGESIZE);
        const size_t numPagesNeeded = (size_t)(size / k_pageSize) + 1;
        const size_t actualSize = numPagesNeeded * k_pageSize;

        printf("Needs %ld pages of total size %ld\n", numPagesNeeded, actualSize);

        pAddr = mmap(
              NULL, 
              actualSize, 
              PROT_READ | PROT_WRITE, 
              MAP_PRIVATE | MAP_ANONYMOUS, 
              -1, 
              0
        );

        // Find first avalialable block
        for (size_t blockNum = 0; blockNum < MAX_NUM_BLOCKS; ++blockNum) {
            if (memBlocks[blockNum].pAddr == NULL) {
                memBlocks[blockNum].pAddr = pAddr;
                memBlocks[blockNum].totalSize = actualSize;
                memBlocks[blockNum].sizeAvail = actualSize - size;
            }
        }
    }
    else {
        // TODO 
    }


    return pAddr;
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

    initializeMemBlocks();

    size_t sizeToReq = 10;
    uint16_t * arr1 = NULL;

    arr1 = (uint16_t *) llMalloc(sizeof(uint16_t) * sizeToReq);
    assert(arr1 != NULL);

    fillArr(arr1, sizeToReq);

    uint32_t value = checkArr(arr1, 10);

    printf("readValue = %d\n", value);


    return 0;
}
