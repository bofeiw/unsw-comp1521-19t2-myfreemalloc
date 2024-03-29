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

/** comparison function for sorting freelist in */
static int chunkCmp(const void *chunk1, const void *chunk2) {
    return *((void **)chunk1) > *((void **)chunk2) ? 1 : -1;
}

/** Allocate a chunk of memory large enough to store `size' bytes. */
void *myMalloc(int size) {
    if (size < 1) {
        return NULL;
    }

    // round up to a multiple of 4
    size = size + (4 - (size % 4)) % 4;

    // minimum size required for allocation
    const int minSize = size + sizeof(header);

    // find the minimum eligible chunk
    int minHeaderIndex = -1;
    for (int i = 0; i < Heap.nFree; ++i) {
        header *currHeader = (header *) Heap.freeList[i];
        if (minHeaderIndex < 0 && currHeader->size >= minSize) {
            minHeaderIndex = i;
        } else if (minHeaderIndex >= 0 && currHeader->size >= minSize &&
                   currHeader->size < ((header *) Heap.freeList[minHeaderIndex])->size) {
            minHeaderIndex = i;
        }
    }

    if (minHeaderIndex < 0) {
        // no free chunk suited for this size
        return NULL;
    }

    // allocate memory, minHeader is the header of allocated memory
    header *minHeader = (header *) Heap.freeList[minHeaderIndex];
    if (minHeader->size <= minSize + MIN_CHUNK) {
        // allocate the whole chunk
        minHeader->status = ALLOC;
        Heap.freeList[minHeaderIndex] = Heap.freeList[--(Heap.nFree)];
        qsort(Heap.freeList, Heap.nFree, sizeof(void *), chunkCmp);
    } else {
        // split into two chunks
        // mark the lower chunk to be allocated
        uint nextSize = minHeader->size - minSize;
        minHeader->size = minSize;
        minHeader->status = ALLOC;

        // calculate next free chunk's address
        // minSize is the size of the chunk allocated this time
        // add it to the base address gets the next free chunk
        header *nextFree = (header *) (((addr) minHeader) + minSize);
        nextFree->size = nextSize;
        nextFree->status = FREE;

        // update free list
        // in this case, only need to change the address at minHeaderIndex to nextFree
        // because there is no deletion and the order of addresses is preserved
        Heap.freeList[minHeaderIndex] = nextFree;
    }

    // calculate and return the usable address of the chunk
    addr headerAddr = (addr) minHeader;
    addr dataAddr = headerAddr + sizeof(header);
    return (void *) dataAddr;
}

/** print and exit when attempting to free unallocated chunk **/
static void exitInvalidFree() {
    fprintf(stderr, "Attempt to free unallocated chunk\n");
    exit(1);
}

/** find the index of the chunk in the freelist, -1 if not exist **/
static int indexOfChunk(const header *chunk) {
    for (int i = 0; i < Heap.nFree; ++i) {
        if (Heap.freeList[i] == chunk) {
            return i;
        }
    }
    return -1;
}

/** free chunk and possibly merge prev chunk and next chunk if they are free **/
static void myFreeChunk(header *prevChunk, header *chunk, header *nextChunk) {
    if (!chunk || chunk->status == FREE) {
        // obj does not represent an allocated chunk in the heap
        exitInvalidFree();
        return;
    }

    // try to merge adjacent free chunks
    int prevFree = prevChunk != NULL && prevChunk->status == FREE;
    int nextFree = nextChunk != NULL && nextChunk->status == FREE;
    if (prevFree && nextFree) {
        // join all three chunks
        prevChunk->status = FREE;
        prevChunk->size += chunk->size + nextChunk->size;
        int nextChunkIndex = indexOfChunk(nextChunk);
        --(Heap.nFree);
        Heap.freeList[nextChunkIndex] = Heap.freeList[Heap.nFree];
    } else if (prevFree) {
        // join with previous chunk
        prevChunk->status = FREE;
        prevChunk->size += chunk->size;
    } else if (nextFree) {
        // join with next chunk
        chunk->status = FREE;
        chunk->size += nextChunk->size;
        Heap.freeList[indexOfChunk(nextChunk)] = chunk;
    } else {
        // free only the chunk
        chunk->status = FREE;
        Heap.freeList[Heap.nFree++] = chunk;
    }

    // make sure the addresses are sorted in increasing order
    qsort(Heap.freeList, Heap.nFree, sizeof(void *), chunkCmp);
}

/** Deallocate a chunk of memory. */
void myFree(void *obj) {
    if (obj == NULL) {
        // If a null pointer is passed as argument, no action occurs.
        // https://linux.die.net/man/3/free
        // https://webcms3.cse.unsw.edu.au/COMP1521/19T2/forums/2732637
        return;
    }

    // find the locations of obj, and record its previous location
    header *prevAddr = NULL;
    header *nextAddr = NULL;
    addr curr = (addr) Heap.heapMem;
    while (curr < heapMaxAddr()) {
        header *chunk = (header *) curr;

        if (&(chunk->data) == obj) {
            // found the obj chunk, calculate next chunk address
            nextAddr = (curr + chunk->size) < heapMaxAddr() ? (header *) (curr + chunk->size) : NULL;

            // free and merge the chunks
            myFreeChunk(prevAddr, chunk, nextAddr);
            return;
        }

        prevAddr = chunk;
        curr += chunk->size;
    }

    // obj not exist
    exitInvalidFree();
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
