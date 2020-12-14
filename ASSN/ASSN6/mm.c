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

#define BUFFER (1<<7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define W_SIZE 4
#define D_SIZE 8
#define CHUNK_SIZE (1<<12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define PACK(size, alloc)  ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define SET(p, val) (*(unsigned int *)(p) = (val) | GET_TAG(p))
#define SET_NOTAG(p, val) (*(unsigned int *)(p) = (val))

#define SET_TAG(p) (*(unsigned int*) (p) = GET(p) | 0x2)
#define UNSET_TAG(p) (*(unsigned int*) (p) = GET(p) & ~0x2)

#define SIZE_BLOCK(p) (GET(p) & ~0x7)
#define IS_ALLOCATED(p) (GET(p) & 0x1)
#define GET_TAG(p) (GET(p) & 0x2)

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
static void *find_fit(size_t asize);
static void *coalesce(void *bp);

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

    SET_NOTAG(heap_listp, 0);
    SET_NOTAG(heap_listp + (1 * W_SIZE), PACK(D_SIZE, 1));
    SET_NOTAG(heap_listp + (2 * W_SIZE), PACK(D_SIZE, 1));
    SET_NOTAG(heap_listp + (3 * W_SIZE), PACK(0, 1));
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

    if((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNK_SIZE);

    if((bp = extend_heap(extendsize)) == NULL){
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

    UNSET_TAG(PTR_HEADER(PTR_NEXT_BLOCK(ptr)));

    SET(PTR_HEADER(ptr), PACK(size, 0));
    SET(PTR_FOOTER(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *new_ptr = ptr;
    size_t new_size = size;
    int remainder;
    int extendsize;
    int block_buffer;

    if(size == 0) {
        return NULL;
    }

    if(new_size <= D_SIZE) {
        new_size = 2 * D_SIZE;
    } else {
        new_size = D_SIZE * (new_size + (D_SIZE) + (D_SIZE - 1)) / D_SIZE;
    }

    new_size += BUFFER;

    block_buffer = SIZE_BLOCK(PTR_HEADER(ptr)) - new_size;

    if (block_buffer < 0) {
        if(!IS_ALLOCATED(PTR_HEADER(PTR_NEXT_BLOCK(ptr))) || !SIZE_BLOCK(PTR_HEADER(PTR_NEXT_BLOCK(ptr)))) {
            remainder = SIZE_BLOCK(PTR_HEADER(ptr)) + SIZE_BLOCK(PTR_HEADER(PTR_NEXT_BLOCK(ptr))) - new_size;
            if(remainder < 0) {
                extendsize = MAX(-remainder, CHUNK_SIZE);
                if(extend_heap(extendsize) == NULL){
                    return NULL;
                }
                remainder += extendsize;
            }
            SET_NOTAG(PTR_HEADER(ptr), PACK(new_size + remainder, 1));
            SET_NOTAG(PTR_FOOTER(ptr), PACK(new_size + remainder, 1));
        } else {
            new_ptr = mm_malloc(new_size-D_SIZE);
            memmove(new_ptr, ptr, MIN(size, new_size));
            mm_free(ptr);
        }
        block_buffer = SIZE_BLOCK(PTR_HEADER(new_ptr)) - new_size;
    }

    if(block_buffer < 2 * BUFFER){
        SET_TAG(PTR_HEADER(PTR_NEXT_BLOCK(new_ptr)));
    }
    return new_ptr;
}

static void* extend_heap(size_t words) {
    char *bp;
    size_t size;

    size = ALIGN(words);
    if((long)(bp = mem_sbrk(size)) == -1){
        return NULL;
    }

    SET_NOTAG(PTR_HEADER(bp), PACK(size, 0));
    SET_NOTAG(PTR_FOOTER(bp), PACK(size, 0));
    SET_NOTAG(PTR_HEADER(PTR_NEXT_BLOCK(bp)), PACK(0, 1));

    return coalesce(bp);
}

static void* coalesce(void* bp){
    size_t prev_alloc = IS_ALLOCATED(PTR_FOOTER(PTR_PREV_BLOCK(bp)));
    size_t next_alloc = IS_ALLOCATED(PTR_HEADER(PTR_NEXT_BLOCK(bp)));
    size_t size = SIZE_BLOCK(PTR_HEADER(bp));

    if(GET_TAG(PTR_HEADER(PTR_PREV_BLOCK(bp)))){
        prev_alloc = 1;
    }

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

static void *find_fit(size_t asize)
{
    char* oldrover = rover;

    for(; SIZE_BLOCK(PTR_HEADER(rover)) > 0; rover = PTR_NEXT_BLOCK(rover)){
        if((!IS_ALLOCATED(PTR_HEADER(rover)) && (asize <= SIZE_BLOCK(PTR_HEADER(rover)))) && !(GET_TAG(PTR_HEADER(rover)))){
            return rover;
        }
    }

    for (rover = heap_listp; rover < oldrover; rover = PTR_NEXT_BLOCK(rover)){
        if((!IS_ALLOCATED(PTR_HEADER(rover)) && (asize <= SIZE_BLOCK(PTR_HEADER(rover)))) && !(GET_TAG(PTR_HEADER(rover)))){
            return rover;
        }
    }

    return NULL;
}

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
static void place(void *bp, size_t asize)
{
    size_t csize = SIZE_BLOCK(PTR_HEADER(bp));

    if ((csize - asize) >= (2 * D_SIZE)) {
        SET_NOTAG(PTR_HEADER(bp), PACK(asize, 1));
        SET_NOTAG(PTR_FOOTER(bp), PACK(asize, 1));
        bp = PTR_NEXT_BLOCK(bp);
        SET_NOTAG(PTR_HEADER(bp), PACK(csize - asize, 0));
        SET_NOTAG(PTR_FOOTER(bp), PACK(csize - asize, 0));
    }
    else {
        SET_NOTAG(PTR_HEADER(bp), PACK(csize, 1));
        SET_NOTAG(PTR_FOOTER(bp), PACK(csize, 1));
    }
}