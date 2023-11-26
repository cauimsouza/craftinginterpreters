/*
Custom implementation of the malloc, free, and realloc functions from the C
standard lib.

To link against this shared library (instead of against the standard C lib), do
the following:
$ gcc -shared -fPIC -o cmalloc.so cmalloc.c
$ LD_PRELOAD=./cmalloc.so ./a.out
*/

#include <unistd.h>

#define HSIZE 4
#define WSIZE 8
#define GROWSIZE (1 << 12) // 4 kB
#define MIN_BLOCK_SIZE 16

#define PACK(size, alloc) ((size) | (alloc))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define GET(p) (*(unsigned int*)(p))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - HSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - WSIZE)

#define PREV_BLK(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - WSIZE))
#define NEXT_BLK(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int __init = 0;
char *prologuep;
char *epiloguep;

int init() {
    if (__init) {
        return 0;
    }
    __init = 1;
    
    char *bp = (char *) sbrk(GROWSIZE);
    if (bp == (void *) -1) {
        return 1;
    }
    
    // Prologue
    bp += WSIZE;
    PUT(HDRP(bp), PACK(WSIZE, 1));
    PUT(FTRP(bp), PACK(WSIZE, 1));
    prologuep = bp;
    
    bp = NEXT_BLK(bp);
    PUT(HDRP(bp), PACK(GROWSIZE - 2 * WSIZE, 0));
    PUT(FTRP(bp), PACK(GROWSIZE - 2 * WSIZE, 0));
    
    // Epilogue
    bp = NEXT_BLK(bp);
    PUT(HDRP(bp), PACK(0, 1));
    epiloguep = bp;
    
    return 0;
}

char* find_fit(size_t asize) {
    for (char *bp = prologuep; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLK(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize) {
            return bp;
        }
    }
    return NULL;
}

void coallesce_with_next(char *bp) {
    size_t aggreg_size = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(NEXT_BLK(bp)));
    PUT(HDRP(bp), PACK(aggreg_size, 0));
    PUT(FTRP(bp), PACK(aggreg_size, 0));
}

char *coallesce(char *bp) {
    char *nextp = NEXT_BLK(bp);
    if (!GET_ALLOC(HDRP(nextp))) {
        coallesce_with_next(bp);
    }
    
    char *prevp = PREV_BLK(bp);
    if (!GET_ALLOC(HDRP(prevp))) {
        bp = prevp;
        coallesce_with_next(bp);
    }
    
    return bp;
}

void *grow_heap(size_t asize) {
    size_t msize = MAX(asize, GROWSIZE);
    
    char *bp = (char *) sbrk(msize);
    if (bp == (void *) -1) {
        return NULL;
    }
    
    bp = epiloguep;
    PUT(HDRP(bp), PACK(msize, 0));
    PUT(FTRP(bp), PACK(msize, 0));
    
    epiloguep = NEXT_BLK(bp);
    PUT(HDRP(epiloguep), PACK(0, 1));
    
    bp = coallesce(bp);
    
    return bp;
}

void place(char *bp, size_t asize) {
    size_t bsize = GET_SIZE(HDRP(bp));
    if (bsize < asize + MIN_BLOCK_SIZE) {
        PUT(HDRP(bp), PACK(bsize, 1));
        PUT(FTRP(bp), PACK(bsize, 1));
        return;
    }
    
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    
    bp = NEXT_BLK(bp);
    PUT(HDRP(bp), PACK(bsize - asize, 0));
    PUT(FTRP(bp), PACK(bsize - asize, 0));
}

size_t adjust_size(size_t size) {
    return WSIZE + WSIZE * ((size + WSIZE - 1) / WSIZE);
}

void *malloc(size_t size) {
    if (init() != 0) {
        return NULL;
    }
    
    if (size == 0) {
        return NULL;
    }
    
    size_t asize = adjust_size(size);
    
    void *bp = find_fit(asize);
    if (bp == NULL) {
        bp = grow_heap(asize);
        if (bp == NULL) {
            return NULL;
        }
    }
    
    place(bp, asize);
    
    return bp;
}

void free(void *ptr) {
    size_t size = GET_SIZE(HDRP(ptr));   
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    
    coallesce(ptr);
}

void *realloc(void *ptr, size_t new_size) {
    if (init() != 0) {
        return NULL;
    }
    
    if (ptr == NULL) {
        return malloc(new_size);
    }
    
    if (GET_SIZE(HDRP(ptr)) >= adjust_size(new_size)) {
        return ptr;
    }
    
    void *new_ptr = malloc(new_size);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    for (size_t i = 0; i < GET_SIZE(HDRP(ptr)) - WSIZE; i++) {
        *((char *) new_ptr + i) = *((char *) ptr + i);
    }
    
    free(ptr);
    
    return new_ptr;
}
