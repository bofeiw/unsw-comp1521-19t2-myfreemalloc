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

    // initialise heap memory
    Heap.heapMem = calloc(1, size);
    if (Heap.heapMem == NULL) {
        return -1;
    }
    ((header *) Heap.heapMem)->status = FREE;
    ((header *) Heap.heapMem)->size = size;

    // initialise heap wrapper
    Heap.freeList = calloc(1, size / MIN_CHUNK);
    if (Heap.freeList == NULL) {
        return -1;
    }

    // sets the first item in this array to the single free-space chunk
    Heap.freeList[0] = Heap.heapMem;

    // init other variables
    Heap.heapSize = size;
    Heap.nFree = 0;
    Heap.freeElems = 0;

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

    header* temp = *Heap.freeList;
    header* smallestFree = NULL;
    while (temp->size > 0) {
        printf("size: %d\n", ((header *) temp)->size);
        // if this header is available
        if (temp->status == FREE && temp->size >= size) {
            if (smallestFree == NULL) {
                // first found header
                smallestFree = temp;
            } else if (temp->size < smallestFree->size) {
                // this header is smaller
                smallestFree = temp;
            }
        }
        ++temp;
    }
    printf("smallest size: %d\n", smallestFree->size);

    if (smallestFree == NULL) {
        return NULL;
    }

    int remainingSize = smallestFree->size - size;
    if (remainingSize == 0) {
        // delete from list
    } else {
        header* remaining = ((void *) smallestFree) + size;
        remaining->size = smallestFree->size - size;
        remaining->status = smallestFree->status = FREE;

        smallestFree->size = size;

        int i = 0;
        temp = *Heap.freeList;
        while (((header *) Heap.freeList[i])->size > 0) {
            if ((header *) Heap.freeList[i] == smallestFree) {
                printf("...: %d\n", smallestFree->size);
                Heap.freeList[i] = remaining;
                break;
            }
            ++i;
        }
    }

    smallestFree->status = ALLOC;

    return smallestFree;
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
    int onRow = 0;

    // We iterate over the heap, chunk by chunk; we assume that the
    // first chunk is at the first location in the heap, and move along
    // by the size the chunk claims to be.
    addr curr = (addr) Heap.heapMem;
    while (curr < heapMaxAddr()) {
        header *chunk = (header *) curr;

        char stat;
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
