#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <sys/mman.h>
#include <stdbool.h>

// TODO: Detect when no pages avaliable anymore. Each for loop
// TODO: Not use malloc

typedef struct MemNode MemNode_t;
struct MemNode {
    void * pAddr;
    bool isFree;
    size_t size;
    MemNode_t * next;
};

typedef struct MemBlock MemBlock_t;

struct MemBlock {
    void * pHead; 
    MemBlock_t * next;
    MemNode_t headNode;
    size_t totalSize;
    size_t sizeAvail; // TODO: Needed?
};

MemBlock_t headMemBlock = {.pHead = NULL,
                           .next = NULL,
                           .totalSize = 0,
                           .sizeAvail = 0};

MemBlock_t * getNewBlock() {
    MemBlock_t * newBlock = NULL;
    newBlock = (MemBlock_t *) malloc(sizeof(MemBlock_t));
    newBlock->pHead = NULL; 
    newBlock->totalSize = 0;
    newBlock->sizeAvail = 0;
    newBlock->next = NULL;

    return newBlock;
}

void initializeMemBlock() {
    // Create a memBlock to always have 1 extra 
    MemBlock_t * newBlock = getNewBlock();
    headMemBlock.next = newBlock;
}

void printMemBlocksInfo() {
    printf("\n - Memory Blocks hierarchy:\n");

    MemBlock_t * currBlock = &headMemBlock;

    size_t blockNum = 0;
    while(currBlock != NULL) {
        if (currBlock->pHead) {
            printf("\tBlock #%ld\n", blockNum);

            printf("\t\t - pHead = %p\n", currBlock->pHead);
            printf("\t\t - totalSize = 0x%lx\n", currBlock->totalSize);
            printf("\t\t - sizeAvail = 0x%lx\n", currBlock->sizeAvail);
            MemNode_t * currNode = &(currBlock->headNode);

            printf("\t\t Nodes:\n");
            while (currNode != NULL) {
                printf("\t\t\t - Node with pAddr = %p\n", currNode->pAddr);
                printf("\t\t\t - isFree = %s\n", currNode->isFree ? "true" : "false");
                printf("\t\t\t - size = 0x%lx\n", currNode->size);
                printf("\n");
                currNode = currNode->next;
            }
        }
        currBlock = currBlock->next;
        ++blockNum;
    }
}

MemBlock_t * findBlockWithCapacity(size_t sizeRequested) {
    printf("\tWill search for a block to use...\n");
    MemBlock_t * currBlock = &headMemBlock;

    while(currBlock != NULL) {
        if (currBlock->sizeAvail >= sizeRequested) {
            return currBlock;
        }
        currBlock = currBlock->next;
    }

    return NULL;
}

MemBlock_t * findEmptyBlock() {
    MemBlock_t * currBlock = &headMemBlock;

    while (currBlock != NULL) {
        if (currBlock->pHead == NULL) {
            MemBlock_t * newBlock = getNewBlock();
            currBlock->next = newBlock;
            return currBlock;
        }

        currBlock = currBlock->next;
    }
    
    return NULL;
}

size_t getSizeAllignedToPage(size_t sizeRequested) {
    const long k_pageSize = sysconf(_SC_PAGESIZE);
    const size_t numPagesNeeded = (size_t)(sizeRequested / k_pageSize) + 1;
    return numPagesNeeded * k_pageSize;
}

void * allocate(size_t size) {
        return mmap(NULL, 
                    size, 
                    PROT_READ | PROT_WRITE, 
                    MAP_PRIVATE | MAP_ANONYMOUS, 
                    -1, 
                    0
                    );
}

MemNode_t * getNewNode() {
     MemNode_t * newNode = (MemNode_t*) malloc(sizeof(MemNode_t));
     newNode->pAddr = NULL;
     newNode->isFree = true;
     newNode->size = 0;
     newNode->next = NULL;

     return newNode;
}

void initializeNewBlock(void * pAddr, size_t actualSize, size_t sizeRequested) {
    // Find first avalialable block
     MemBlock_t * blockToUse = findEmptyBlock();
     
     // Assign values to block
     blockToUse->pHead = pAddr;
     blockToUse->totalSize = actualSize;
     blockToUse->sizeAvail = actualSize - sizeRequested;

     // Initialize MemNode
     blockToUse->headNode.pAddr = pAddr;
     blockToUse->headNode.isFree = false;
     blockToUse->headNode.size = sizeRequested;

     // New node of free area
     MemNode_t * freeAreaNode = getNewNode();
     freeAreaNode->pAddr = pAddr + sizeRequested;
     freeAreaNode->isFree = true;
     freeAreaNode->size = actualSize - sizeRequested;
     freeAreaNode->next = NULL;

     blockToUse->headNode.next = freeAreaNode;
}

MemNode_t * findAndSetAvailableNode(MemBlock_t * blockToUse, size_t sizeRequested) {
    printf("- findAndSetAvailableNode\n");

    MemNode_t * nodeToUse = &(blockToUse->headNode);

    while (nodeToUse != NULL) {
        if (nodeToUse->isFree && nodeToUse->size >= sizeRequested) {
            printf("\tFound free node, will use\n");
            size_t newFreeSize = nodeToUse->size - sizeRequested;
            nodeToUse->isFree = false;
            nodeToUse->size = sizeRequested;

            if (newFreeSize == 0) {
                break;
            }

            printf("\tArea is longer than needed by %ld\n", newFreeSize);
            if ((nodeToUse->next != NULL) && (nodeToUse->next->isFree)) {
                nodeToUse->next->size += newFreeSize;
                break;
            }

            MemNode_t * freeAreaNode = getNewNode();
            freeAreaNode->pAddr = nodeToUse->pAddr + sizeRequested;
            freeAreaNode->isFree = true;
            freeAreaNode->size = newFreeSize;
            freeAreaNode->next = NULL;

            if (nodeToUse->next == NULL) {
                nodeToUse->next = freeAreaNode;
            }
            else {
                freeAreaNode->next = nodeToUse->next;
                nodeToUse->next = freeAreaNode;
            }
            break;
        }

        nodeToUse = nodeToUse->next;
    }

    if (nodeToUse != NULL) {
        blockToUse->sizeAvail -= sizeRequested;
    }

    return nodeToUse;
}

void * memAlloc(size_t sizeRequested) {
    printf(" - %s\n", __func__);
    // TODO: Check alignment

    void * pAddr = NULL;

    MemBlock_t * blockToUse = findBlockWithCapacity(sizeRequested);

    // Not found, needs a new block
    if (blockToUse == NULL) {
        printf("\tBlock not found, creating...\n");

        const size_t actualSize = getSizeAllignedToPage(sizeRequested);
        printf("\tNeeds block of total size %ld\n", actualSize);

        pAddr = allocate(actualSize);
        initializeNewBlock(pAddr, actualSize, sizeRequested);
    }
    else {
        printf("\t Found block, will assign...\n");
        MemNode_t * nodeToUse = findAndSetAvailableNode(blockToUse, sizeRequested);
        pAddr = nodeToUse->pAddr;
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

    initializeMemBlock();

    size_t size1 = 0x100 / sizeof(uint16_t);
    uint16_t * arr1 = (uint16_t *) memAlloc(sizeof(uint16_t) * size1);
    assert(arr1 != NULL);

    size_t size2 = 0x2000 / sizeof(uint16_t);
    uint16_t * arr2 = (uint16_t *) memAlloc(sizeof(uint16_t) * size2);
    assert(arr2 != NULL);

    size_t size3 = 0x10 / sizeof(uint16_t);
    uint16_t * arr3 = (uint16_t *) memAlloc(sizeof(uint16_t) * size3);
    assert(arr3 != NULL);

    size_t size4 = 0x1000 / sizeof(uint16_t);
    uint16_t * arr4 = (uint16_t *) memAlloc(sizeof(uint16_t) * size4);
    assert(arr4 != NULL);


    printMemBlocksInfo();

    fillArr(arr1, size1);
    fillArr(arr2, size2);
    fillArr(arr3, size3);

    return 0;
}
