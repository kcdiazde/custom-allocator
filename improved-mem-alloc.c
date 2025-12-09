#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <sys/mman.h>
#include <stdbool.h>

// TODO: Detect when no pages avaliable anymore. Each for loop
// TODO: Not use malloc

#define MAX_NUM_BLOCKS 10

typedef struct MemNode MemNode_t;

struct MemNode {
    void * pAddr;
    bool isFree;
    size_t size;
    MemNode_t * next;
};

typedef struct {
    void * pHead; 
    size_t totalSize;
    size_t sizeAvail; // TODO: Needed?
    MemNode_t headNode;
} MemBlock_t;

MemBlock_t memBlocks[MAX_NUM_BLOCKS] = {};

// To allow "dynamic" number of blocks
void initializeMemBlocks() {
    for (size_t blockNum = 0; blockNum < MAX_NUM_BLOCKS; ++blockNum) {
        memBlocks[blockNum].pHead= NULL;
        memBlocks[blockNum].totalSize = 0;
        memBlocks[blockNum].sizeAvail = 0;
        memBlocks[blockNum].headNode.pAddr = NULL;
        memBlocks[blockNum].headNode.isFree = true;
        memBlocks[blockNum].headNode.size = 0;
    }
}

void * memAlloc(size_t sizeRequested) {
    printf(" - %s\n", __func__);
    // TODO: Check alignment

    void * pAddr = NULL;

    // Look for avaliable memblock
    size_t blockToUse = MAX_NUM_BLOCKS;
    printf("\tWill search for a block to use...\n");
    for (size_t blockNum = 0; blockNum < MAX_NUM_BLOCKS; ++blockNum) {
        if (memBlocks[blockNum].sizeAvail >= sizeRequested) {
            blockToUse = blockNum;        
        }
    }

    // Not found, needs a new block
    if (blockToUse == MAX_NUM_BLOCKS) {
        printf("\tBlock not found, creating...\n");

        const long k_pageSize = sysconf(_SC_PAGESIZE);
        const size_t numPagesNeeded = (size_t)(sizeRequested / k_pageSize) + 1;
        const size_t actualSize = numPagesNeeded * k_pageSize;

        printf("\tNeeds %ld pages of total size %ld\n", numPagesNeeded, actualSize);

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
            if (memBlocks[blockNum].pHead== NULL) {
                memBlocks[blockNum].pHead = pAddr;
                memBlocks[blockNum].totalSize = actualSize;
                memBlocks[blockNum].sizeAvail = actualSize - sizeRequested;

                // Initialize MemNode
                memBlocks[blockNum].headNode.pAddr = pAddr;
                memBlocks[blockNum].headNode.isFree = false;
                memBlocks[blockNum].headNode.size = sizeRequested;

                // New node of free area
                MemNode_t * freeAreaNode = (MemNode_t*) malloc(sizeof(MemNode_t));
                freeAreaNode->pAddr = pAddr + sizeRequested;
                freeAreaNode->isFree = true;
                freeAreaNode->size = actualSize - sizeRequested;

                memBlocks[blockNum].headNode.next = freeAreaNode;
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

    arr1 = (uint16_t *) memAlloc(sizeof(uint16_t) * sizeToReq);
    assert(arr1 != NULL);

    fillArr(arr1, sizeToReq);

    uint32_t value = checkArr(arr1, 10);

    printf("readValue = %d\n", value);


    return 0;
}
