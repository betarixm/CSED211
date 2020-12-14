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

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define W_SIZE 4
#define D_SIZE 8
#define CHUNK_SIZE (1<<12)

#define MAX(x, y) ((x) > (y)? (x) : (y))

#define PACK(size, alloc)  ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define SET(p, val) (*(unsigned int *)(p) = (val))

#define SIZE_BLOCK(p) (GET(p) & ~0x7)
#define IS_ALLOCATED(p) (GET(p) & 0x1)

#define PTR_HEADER(bp) ((char *)(bp) - W_SIZE)
#define PTR_FOOTER(bp) ((char *)(bp) + SIZE_BLOCK(PTR_HEADER(bp)) - D_SIZE)

#define PTR_NEXT_BLOCK(bp) ((char *)(bp) + SIZE_BLOCK(((char *)(bp) - W_SIZE)))
#define PTR_PREV_BLOCK(bp) ((char *)(bp) - SIZE_BLOCK(((char *)(bp) - D_SIZE)))

team_t team = {
        /* Team name */
        "Gwon Minjae",
        "Gwon Minjae",
        "20190084",
        "",
        ""
};


static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *fit(size_t asize);
static void *coalesce(void *bp);
static void* best_fit(size_t asize);
static void* next_fit(size_t asize);
static void* first_fit(size_t asize);
static char *heap_listp = 0;
static char *rover;

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((heap_listp = mem_sbrk(4 * W_SIZE)) == (void*)-1){
        return -1;
    }

    SET(heap_listp, 0);
    SET(heap_listp + (1 * W_SIZE), PACK(D_SIZE, 1));
    SET(heap_listp + (2 * W_SIZE), PACK(D_SIZE, 1));
    SET(heap_listp + (3 * W_SIZE), PACK(0, 1));
    heap_listp += (2 * W_SIZE);

    rover = heap_listp;

    if(extend_heap(CHUNK_SIZE / W_SIZE) == NULL){
        return -1;
    }
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char* bp;

    if(heap_listp == 0) {
        mm_init();
    }

    if(size == 0){
        return NULL;
    }

    if(size <= D_SIZE){
        asize = 2 * D_SIZE;
    } else {
        asize = D_SIZE * ((size + (D_SIZE) + (D_SIZE - 1)) / D_SIZE);
    }

    if((bp = fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNK_SIZE);

    if((bp = extend_heap(extendsize / W_SIZE)) == NULL){
        return NULL;
    }
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if(ptr == 0){
        return;
    }

    size_t size = SIZE_BLOCK(PTR_HEADER(ptr));

    if(heap_listp == 0) {
        mm_init();
    }

    SET(PTR_HEADER(ptr), PACK(size, 0));
    SET(PTR_FOOTER(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
        mm_free(ptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(ptr == NULL) {
        return mm_malloc(size);
    }

    newptr = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if(!newptr) {
        return 0;
    }

    /* Copy the old data. */
    oldsize = SIZE_BLOCK(PTR_HEADER(ptr));
    if(size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    mm_free(ptr);

    return newptr;
}

static void* extend_heap(size_t words) {
    char *bp;
    size_t size;

    size = (words % 2) ? ((words + 1) * W_SIZE) : words * W_SIZE;
    if((long)(bp = mem_sbrk(size)) == -1){
        return NULL;
    }

    SET(PTR_HEADER(bp), PACK(size, 0));
    SET(PTR_FOOTER(bp), PACK(size, 0));
    SET(PTR_HEADER(PTR_NEXT_BLOCK(bp)), PACK(0, 1));

    return coalesce(bp);
}

static void* coalesce(void* bp){
    size_t prev_alloc = IS_ALLOCATED(PTR_FOOTER(PTR_PREV_BLOCK(bp)));
    size_t next_alloc = IS_ALLOCATED(PTR_HEADER(PTR_NEXT_BLOCK(bp)));
    size_t size = SIZE_BLOCK(PTR_HEADER(bp));

    if(prev_alloc && next_alloc) {
        return bp;
    } else if (prev_alloc && !next_alloc) {
        size += SIZE_BLOCK(PTR_HEADER(PTR_NEXT_BLOCK(bp)));
        SET(PTR_HEADER(bp), PACK(size, 0));
        SET(PTR_FOOTER(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {
        size += SIZE_BLOCK((PTR_HEADER(PTR_PREV_BLOCK(bp))));
        SET(PTR_FOOTER(bp), PACK(size, 0));
        SET(PTR_HEADER(PTR_PREV_BLOCK(bp)), PACK(size, 0));
        bp = PTR_PREV_BLOCK(bp);
    } else {
        size += (SIZE_BLOCK(PTR_HEADER(PTR_PREV_BLOCK(bp))) + SIZE_BLOCK(PTR_FOOTER(PTR_NEXT_BLOCK(bp))));
        SET(PTR_HEADER(PTR_PREV_BLOCK(bp)), PACK(size, 0));
        SET(PTR_FOOTER(PTR_NEXT_BLOCK(bp)), PACK(size, 0));
        bp = PTR_PREV_BLOCK(bp);
    }
    if((rover > (char*)bp) && (rover < PTR_NEXT_BLOCK(bp))){
        rover = bp;
    }
    return bp;
}

static void *fit(size_t asize)
{
    return next_fit(asize);
}

static void* next_fit(size_t asize) {
    /* Next fit search */
    char *oldrover = rover;
    /* Search from the rover to the end of list */
    for ( ; SIZE_BLOCK(PTR_HEADER(rover)) > 0; rover = PTR_NEXT_BLOCK(rover))
        if (!IS_ALLOCATED(PTR_HEADER(rover)) && (asize <= SIZE_BLOCK(PTR_HEADER(rover))))
            return rover;

    /* search from start of list to old rover */
    for (rover = heap_listp; rover < oldrover; rover = PTR_NEXT_BLOCK(rover))
        if (!IS_ALLOCATED(PTR_HEADER(rover)) && (asize <= SIZE_BLOCK(PTR_HEADER(rover))))
            return rover;

    return NULL;  /* no fit found */
}

static void* best_fit(size_t asize) {
    void *bp;
    void *best = NULL;

    for (bp = heap_listp; SIZE_BLOCK(PTR_HEADER(bp)) > 0; bp = PTR_NEXT_BLOCK(bp)) {
        if (!IS_ALLOCATED(PTR_HEADER(bp)) && (asize <= SIZE_BLOCK(PTR_HEADER(bp)))) {
            if(best == NULL){
                best = bp;
            } else {
                if(SIZE_BLOCK(PTR_HEADER(bp)) < SIZE_BLOCK(PTR_HEADER(best))) {
                    best = bp;
                }
            }
        }
    }
    return best; /* No fit */
}

static void* first_fit(size_t asize) {
    void *bp;

    for (bp = heap_listp; SIZE_BLOCK(PTR_HEADER(bp)) > 0; bp = PTR_NEXT_BLOCK(bp)) {
        if (!IS_ALLOCATED(PTR_HEADER(bp)) && (asize <= SIZE_BLOCK(PTR_HEADER(bp)))) {
            return bp;
        }
    }
    return NULL; /* No fit */
}

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
static void place(void *bp, size_t asize)
{
    size_t csize = SIZE_BLOCK(PTR_HEADER(bp));

    if ((csize - asize) >= (2 * D_SIZE)) {
        SET(PTR_HEADER(bp), PACK(asize, 1));
        SET(PTR_FOOTER(bp), PACK(asize, 1));
        bp = PTR_NEXT_BLOCK(bp);
        SET(PTR_HEADER(bp), PACK(csize - asize, 0));
        SET(PTR_FOOTER(bp), PACK(csize - asize, 0));
    }
    else {
        SET(PTR_HEADER(bp), PACK(csize, 1));
        SET(PTR_FOOTER(bp), PACK(csize, 1));
    }
}