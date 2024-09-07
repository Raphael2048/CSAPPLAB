/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


#define WSIZE      4
#define DSIZE      8
#define CHUNKSIZE (1<<12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p)      (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p) = (unsigned int)(val))

// read size and allocated fields
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

// given block ptr, get header and footer
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// given block ptr, get next and previous blocks
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// given free block ptr, get next and previous free blocks offset
#define PRED_PTR(bp) (char *)(bp)
#define SUCC_PTR(bp) ((char *)(bp) + WSIZE)

#define PRED(bp) (char*)(*((unsigned int*)bp))
#define SUCC(bp) (char*)(*((unsigned int*)(bp) + 1))

#define DEBUG 0

static char* heap_listp;
static char* free_block_listp;

static void *coalesce(void *bp)
{

    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc) {
        return bp;
    }
    else if(prev_alloc && !next_alloc) {
        char* next_succ = SUCC(NEXT_BLKP(bp));
        if(next_succ) {
            PUT(PRED_PTR(next_succ), bp);
        }
        PUT(SUCC_PTR(bp), next_succ);

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if(!prev_alloc && next_alloc) {

        char * succ = SUCC(bp);
        if(succ) {
            PUT(PRED_PTR(succ), PREV_BLKP(bp));
        }
        PUT(SUCC_PTR(PREV_BLKP(bp)), succ);

        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else
    {
        char* next_succ = SUCC(NEXT_BLKP(bp));
        if(next_succ) {
            PUT(PRED_PTR(next_succ), PREV_BLKP(bp));
        }
        PUT(SUCC_PTR(PREV_BLKP(bp)), next_succ);

        size +=  GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}

static void* extend_heap(size_t words)
{
    char *bp;
    size_t size;
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    if(free_block_listp == NULL) {
        free_block_listp = bp;
        PUT(PRED_PTR(bp), NULL);
        PUT(SUCC_PTR(bp), NULL);
    } else {
        for(void*p = free_block_listp;; p = SUCC(p)) {
            if(SUCC(p) == NULL) {
                PUT(SUCC_PTR(p), bp);
                PUT(PRED_PTR(bp), p);
                PUT(SUCC_PTR(bp), NULL);
                break;
            }
        }
    }

    return coalesce(bp);
}

static void print_info()
{
#if DEBUG
    if(free_block_listp != NULL) {
        if(PRED(free_block_listp)) {
            abort();
        }
    }
    for(void *bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        printf("FROM:%x, SIZE:%x/%d, MALLOC:%d\n", (int)bp, GET_SIZE(HDRP(bp)), GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)));
    }
    printf("=================\n");

     for(void *bp = free_block_listp; bp != NULL; bp = SUCC(bp)) {
        printf("FROM:%x, SIZE:%x/%d, MALLOC:%d\n", (int)bp, GET_SIZE(HDRP(bp)), GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)));
    }
    printf("~~~~~~~~~~~~~~~~~~\n~~~~~~~~~~~~~~~~~~\n");
#endif
}

static void *find_fit(size_t asize)
{
    // print_info();
    // for(void *bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    //     if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
    //         return bp;
    //     }
    // }

    for(void * bp = free_block_listp; bp; bp = SUCC(bp)) {
        if (asize <= GET_SIZE(HDRP(bp))) {
            return bp;
        }
    }
    return NULL;
}

static void* place(void * bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    if((csize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
    } 
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));

        if(PRED(bp) != NULL) {
            PUT(SUCC_PTR(PRED(bp)), SUCC(bp));
        } else {
            free_block_listp = SUCC(bp);
        }

        if(SUCC(bp) != NULL) {
            PUT(PRED_PTR(SUCC(bp)), PRED(bp));
        }
    }
    return bp;
}


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((heap_listp = mem_sbrk(4 * WSIZE)) == (void*)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE), PACK(DSIZE, 1));

    heap_listp += (2*WSIZE);

    free_block_listp = NULL;

    print_info();

    if(extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }

    print_info();

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{

    print_info();
    size_t asize;
    size_t extendsize;
    char *bp;

    if(size == 0) {
        return NULL;
    }

    if(size <= DSIZE)
        asize = 2 * DSIZE;
    else 
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    if ((bp = find_fit(asize)) != NULL) {
        bp = place(bp, asize);
        return bp;
    }

    // printf("FIT FAILURE !\n");

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    bp = place(bp, asize);

    print_info();
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    print_info();
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    if(free_block_listp == NULL) {
        free_block_listp = bp;
        PUT(SUCC_PTR(bp), NULL);
        PUT(PRED_PTR(bp), NULL);
    } else {
        if(free_block_listp > bp) {
            PUT(PRED_PTR(bp), NULL);
            PUT(SUCC_PTR(bp), free_block_listp);
            PUT(PRED_PTR(free_block_listp), bp);
            free_block_listp = bp;
        } else {
            for(void *p = PREV_BLKP(bp);; p = PREV_BLKP(p)) {
                if(GET_ALLOC(FTRP(p)) == 0) {
                    PUT(SUCC_PTR(bp), SUCC(p));
                    if(SUCC(p)) {
                        PUT(PRED_PTR(SUCC(p)), bp);
                    }
                    PUT(SUCC_PTR(p), bp);
                    PUT(PRED_PTR(bp), p);
                    break;
                }
            }
        }
    }

    print_info();

    coalesce(bp);

    print_info();
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{

    if(ptr == NULL) {
        return malloc(size);
    } else if(size == 0) {
        mm_free(ptr);
        return NULL;
    } else {
        size_t oldsize = GET_SIZE(HDRP(ptr)) - DSIZE;
        if(oldsize >= size) {
            return ptr;
        } else {
            for(void* bp = NEXT_BLKP(ptr); GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
                if(GET_ALLOC(HDRP(bp))) break;
                oldsize += GET_SIZE(HDRP(bp));
                if(oldsize >= size) {
                    oldsize += DSIZE;
                    PUT(HDRP(ptr), PACK(oldsize, 1));
                    PUT(FTRP(ptr), PACK(oldsize, 1));

                    if(free_block_listp > ptr) {
                        free_block_listp = SUCC(bp);
                        if(free_block_listp != NULL) {
                            PUT(PRED_PTR(free_block_listp), NULL);
                        }
                    } else {
                        for(void* pp = PREV_BLKP(ptr);; pp = PREV_BLKP(pp)) {
                            if(!GET_ALLOC(HDRP(pp))) {
                                PUT(SUCC_PTR(pp), SUCC(bp));
                                if(SUCC(bp)) {
                                    PUT(PRED_PTR(SUCC(bp)), pp);
                                }
                                break;
                            }
                        }
                    }
                    return ptr;
                }
            }
            void* newptr = mm_malloc(size);
            if(newptr == NULL) return NULL;
            size_t copySize = GET_SIZE(HDRP(ptr)) - DSIZE;
            if (size < copySize)
            copySize = size;
            memcpy(newptr, ptr, copySize);
            mm_free(ptr);
            return newptr;
        }
    }
}














