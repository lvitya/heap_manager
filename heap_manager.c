#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#define HEAP_SIZE 2048

struct mem_block {
   size_t size; // size in number of mem_blocks
   union {
      struct mem_block* pPrev; // used in free mem_block
      int payload;             // used to get paiload pointer in allocated mem_block
   };
   struct mem_block* pNext;
};

// it is possible to locate heap at certain address in memory if needed
static char heap[HEAP_SIZE];
static struct mem_block* pHead = (struct mem_block*) heap; // first free block
static struct mem_block heap_init = {.size = HEAP_SIZE / sizeof(struct mem_block), .pPrev = NULL, .pNext = NULL};
static bool inited = false;

static void drop(struct mem_block *pBlock){
    if (pBlock->pPrev != NULL) { // if there is previous block
        pBlock->pPrev->pNext = pBlock->pNext;
    }
    if (pBlock->pNext != NULL) { // if there is next block
        pBlock->pNext->pPrev = pBlock->pPrev;
    }
    return;
}

void* my_malloc(size_t size){
    if (size == 0)
        return NULL;

    void* p;

    if (!inited) { // init heap on first call
        *pHead = heap_init;
        inited = true;
    }

    if (pHead->size == 1 && pHead->pNext == NULL) { // if only Head block left available
        return NULL;
    }

    struct mem_block* pCurrent = pHead;
    size_t bytes_for_block = offsetof(struct mem_block, payload) + size; // (space for mem_block->size) + requested_bytes
    // round up bytes_for_block to number of blocks
    size_t allocate_blocks = (bytes_for_block + sizeof(struct mem_block)-1)/sizeof(struct mem_block);
    while (allocate_blocks > pCurrent->size) { // check if size fits in block
        if (pCurrent->pNext != NULL) {
            pCurrent = pCurrent->pNext; // cycle through free blocks
        } else {
            return NULL; // there is no block large enough
        }
    }

    // if remainder free space is possible to track
    if ((pCurrent->size - allocate_blocks) >= 1) {
        struct mem_block* pAllocated;
        // memory is allocated in the block tail, no need to shift block descriptor
        pAllocated = pCurrent + pCurrent->size - allocate_blocks;
        pAllocated->size = allocate_blocks;
        pCurrent->size -= allocate_blocks;

        // pointer to return
        p = &(pAllocated->payload);
    } else { // drop current mem_block from the list
        if (pCurrent == pHead) { // Head can't be dropped in this design
            return NULL;
        }

        p = &(pCurrent->payload); // return whole block
        drop(pCurrent);
    }

    return (void*) p;
}

// insert pNew after pBlock
static void insert_forward(struct mem_block *pBlock, struct mem_block *pNew){
    if (pBlock->pNext != NULL) { // if not list tail
        pBlock->pNext->pPrev = pNew;
    }
    pNew->pNext = pBlock->pNext;
    pBlock->pNext = pNew;
    pNew->pPrev = pBlock;
}

// merge pBlock with next block if possible
static void merge_forward(struct mem_block *pBlock){
    if (pBlock->pNext == NULL) // if it is list tail then there is no block to merge
        return;
    if (pBlock + pBlock->size == pBlock->pNext){ // if next block borders with pBlock
        pBlock->size += pBlock->pNext->size;
        drop(pBlock->pNext);
    }
    return;
}

void my_free(void* p){ // insert current allocated block to free list
    struct mem_block* pFreed;
    pFreed = (struct mem_block *)((char*)p - offsetof(struct mem_block, payload));

    struct mem_block* pCurrent = pHead;
    // search for position to insert by address
    while (pCurrent->pNext != NULL && (char*)p > (char*)pCurrent->pNext) {
        pCurrent = pCurrent->pNext;
    }
    insert_forward(pCurrent, pFreed);

    merge_forward(pFreed); // merge to higher addresses
    merge_forward(pCurrent); // merge to lower addresses
    return;
}

#ifdef _TEST
#include "test.c"
#endif
