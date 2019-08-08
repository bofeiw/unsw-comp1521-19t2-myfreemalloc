// COMP1521 19t2 ... Assignment 2: heap management system

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "myHeap.h"

/** minimum total space for heap */
#define MIN_HEAP 4096
/** minimum amount of space for a free Chunk (excludes Header) */
#define MIN_CHUNK 32


#define ALLOC 0x55555555
#define FREE  0xAAAAAAAA

/// Types:

typedef unsigned int uint;
typedef unsigned char byte;

typedef uintptr_t addr; // an address as a numeric type

/** The header for a chunk. */
typedef struct header {
    uint status;    /**< the chunk's status -- ALLOC or FREE */
    uint size;      /**< number of bytes, including header */
    byte data[];    /**< the chunk's data -- not interesting to us */
} header;

/** The heap's state */
struct heap {
    void *heapMem;     /**< space allocated for Heap */
    uint heapSize;    /**< number of bytes in heapMem */
    void **freeList;    /**< array of pointers to free chunks */
    uint freeElems;   /**< number of elements in freeList[] */
    uint nFree;       /**< number of free chunks */
};


/// Variables:

/** The heap proper. */
static struct heap Heap;


/// Functions:

static addr heapMaxAddr(void);


/** Initialise the Heap. */
int initHeap(int size) {
    // make sure it is a minimum of MIN_HEAP
    if (size < MIN_HEAP) {
        size = MIN_HEAP;
    }

    // round up to a multiple of 4
    size = size + (4 - (size % 4)) % 4;

    // allocate heap memory
    Heap.heapMem = calloc(1, size);
    if (Heap.heapMem == NULL) {
        // cannot allocate memory
        return -1;
    }

    // initialise the heap
    ((header *) Heap.heapMem)->status = FREE;
    ((header *) Heap.heapMem)->size = size;

    // allocate free list
    Heap.freeList = calloc(1, size / MIN_CHUNK * sizeof(void *));
    if (Heap.freeList == NULL) {
        // cannot allocate free list
        return -1;
    }

    // sets the first item in this array to the single free-space chunk
    Heap.freeList[0] = Heap.heapMem;

    // init other variables
    Heap.heapSize = size;
    Heap.freeElems = size / MIN_CHUNK;
    Heap.nFree = 1;

    return 0;
}

/** Release resources associated with the heap. */
void freeHeap(void) {
    free(Heap.heapMem);
    free(Heap.freeList);
}

/** Allocate a chunk of memory large enough to store `size' bytes. */
void *myMalloc(int size) {
    if (size < 1) {
        return NULL;
    }

    // round up to a multiple of 4
    size = size + (4 - (size % 4)) % 4;

    const int minSize = size + sizeof(header);
    int minHeaderIndex = -1;

    // find the minimum eligible chunk
    for (int i = 0; i < Heap.nFree; ++i) {
        header *currHeader = (header *) Heap.freeList[i];
        if (currHeader->size >= minSize && minHeaderIndex < 0) {
            minHeaderIndex = i;
        } else if (minHeaderIndex >= 0 && currHeader->size > ((header *) Heap.freeList[minHeaderIndex])->size) {
            minHeaderIndex = i;
        }
    }

    if (minHeaderIndex < 0) {
        // no such free chunk
        return NULL;
    }

    header *minHeader = (header *) Heap.freeList[minHeaderIndex];
    if (minHeader->size <= minSize + MIN_CHUNK) {
        // allocate the whole chunk
        // TODO implement this case
        fprintf(stderr, "hi\n");
    } else {
        // split into two chunks
        uint nextSize = minHeader->size - minSize;
        minHeader->size = minSize;
        minHeader->status = ALLOC;

        // calculate next free chunk's address
        // minSize is the size of the chunk allocated this time
        // add it to the base address gets the next free chunk
        header *nextFree = (header *)(((addr)minHeader) + minSize);
        nextFree->size = nextSize;
        nextFree->status = FREE;

//        printf("minHeader: %p\n" , minHeader);
//        printf("nextFree: %p\n" , nextFree);
//        printf("size: %d\n" , size);
//        printf("next status: %x\n" , nextFree->status);
//        printf("next status&: %p\n" , &(nextFree->status));
//        printf("minHeader + size: %p\n" , minHeader + size);

        // update free list
        // in this case, only need to change the address at minHeaderIndex to nextFree
        // because there is no deletion and the order of addresses is preserved
        Heap.freeList[minHeaderIndex] = nextFree;

        addr headerAddr = (addr) minHeader;
        addr dataAddr = headerAddr + sizeof(header);
//        printf("(void *) dataAddr: %p\n" , (void *) dataAddr);

        return (void *) dataAddr;
    }
    return NULL;
}

/** Deallocate a chunk of memory. */
void myFree(void *obj) {
    /// TODO ///
}

/** Return the first address beyond the range of the heap. */
static addr heapMaxAddr(void) {
    return (addr) Heap.heapMem + Heap.heapSize;
}

/** Convert a pointer to an offset in the heap. */
int heapOffset(void *obj) {
    addr objAddr = (addr) obj;
    addr heapMin = (addr) Heap.heapMem;
    addr heapMax = heapMaxAddr();
    if (obj == NULL || !(heapMin <= objAddr && objAddr < heapMax))
        return -1;
    else
        return (int) (objAddr - heapMin);
}

/** Dump the contents of the heap (for testing/debugging). */
void dumpHeap(void) {
//    return;
    int onRow = 0;

    // We iterate over the heap, chunk by chunk; we assume that the
    // first chunk is at the first location in the heap, and move along
    // by the size the chunk claims to be.
    addr curr = (addr) Heap.heapMem;
    while (curr < heapMaxAddr()) {
        header *chunk = (header *) curr;

        char stat;
//        fprintf(
//                stdout,
//                "address %p\n",
//                chunk
//        );
        switch (chunk->status) {
            case FREE:
                stat = 'F';
                break;
            case ALLOC:
                stat = 'A';
                break;
            default:
                fprintf(
                        stderr,
                        "myHeap: corrupted heap: chunk status %08x\n",
                        chunk->status
                );
                fprintf(
                        stderr,
                        "address %p\n",
                        chunk
                );
                printf("chunk status&: %p\n" , &(chunk->status));
                exit(1);
        }

        printf(
                "+%05d (%c,%5d)%c",
                heapOffset((void *) curr),
                stat, chunk->size,
                (++onRow % 5 == 0) ? '\n' : ' '
        );

        curr += chunk->size;
    }

    if (onRow % 5 > 0)
        printf("\n");
}
