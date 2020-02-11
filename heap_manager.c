#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#define HEAP_SIZE 2048

// it is possible to locate heap at certain address in memory if needed
static char heap[HEAP_SIZE];
static struct mem_block* pHead = (struct mem_block*) heap; // first free block

struct mem_block {
   size_t size; // size in number of mem_blocks
   union {
      struct mem_block* pPrev; // used in free mem_block
      int payload;             // used to get paiload pointer in allocated mem_block
   };
   struct mem_block* pNext;
};

/* struct allocated_mem_block {
   size_t size; // size in number of mem_blocks
   int payload[];
}; */

void* my_malloc(size_t size){
    if (size == 0)
        return NULL;

    static bool inited = false;
    void* p;

    if (!inited) { // init heap on first call
        pHead->size = HEAP_SIZE / sizeof(struct mem_block);
        pHead->pPrev = NULL;
        pHead->pNext = NULL;
        inited = true;
    }

    if (pHead->size == 1 && pHead->pNext == NULL) { // if only Head block left
        return NULL;
    }
    /*1if (pHead == NULL) { // no more free blocks
        return NULL;
    }1*/

    struct mem_block* pCurrent = pHead;
    size_t bytes_for_block = offsetof(struct mem_block, payload) + size; // (space for mem_block->size) + requested_bytes
    // round up bytes_for_block to number of blocks
    size_t allocate_blocks = (bytes_for_block + sizeof(struct mem_block)-1)/sizeof(struct mem_block);
    while (allocate_blocks > pCurrent->size) { // check if size fits in block
        if (pCurrent->pNext != NULL) { // cycle through free blocks
            pCurrent = pCurrent->pNext;
        } else {
            return NULL; // there is no block large enough
        }
    }

    // if remainder free space is possible to track
    if ((pCurrent->size - allocate_blocks) >= 1) {
        struct mem_block* pAllocated;
        // memory is allocated in the block tail
        pAllocated = pCurrent + pCurrent->size - allocate_blocks;
        pAllocated->size = allocate_blocks;
        pCurrent->size -= allocate_blocks;

        // pointer to return
        p = &(pAllocated->payload);
    } else { // drop current mem_block from the list
        if (pCurrent == pHead) { // but don't drop Head
            return NULL;
        }

        p = &(pCurrent->payload); // return whole block
        if (pCurrent->pPrev != NULL) { // if there is previous free block
            pCurrent->pPrev->pNext = pCurrent->pNext;
        }
        if (pCurrent->pNext != NULL) { // if there is next free block
            pCurrent->pNext->pPrev = pCurrent->pPrev;
        }
        /*1 if (pCurrent->pPrev == NULL && pCurrent->pNext == NULL) { // drop last free block
            pHead = NULL;
        }1*/
    }

    return (void*) p;
}

void my_free(void* p){ // insert current allocated block to free list
    struct mem_block* pFreed;
    pFreed = (struct mem_block *)((char*)p - offsetof(struct mem_block, payload));

    /*1if (pHead == NULL) { // free list has been exhausted
        // set pHead to the block
        pHead = pFreed;
        pHead->pNext = NULL;
        pHead->pPrev = NULL;
        return;
    }1*/

    // insert block sorted by address
    struct mem_block* pCurrent = pHead;
    while ((char*)p > (char*)pCurrent) {
        pCurrent = pCurrent->pNext;
    }
    // insert after pCurrent
    pCurrent->pNext->pPrev = pFreed;
    pFreed->pNext = pCurrent->pNext;
    pCurrent->pNext = pFreed;
    pFreed->pPrev = pCurrent;
    return;
}

//#ifdef _TEST
#include "test.c"
//#endif
