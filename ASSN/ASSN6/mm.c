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
#define ALIGNMENT 8u

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7u)

#define PADDING (1u<<7u)
#define TRUE 1
#define FALSE 0

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4u
#define DSIZE 8u
#define CHUNKSIZE (1u<<12u)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define PACK(size, alloc)  (((unsigned int)(size)) | ((unsigned int)(alloc)))

#define GET(p) (*(unsigned int *)(p))

#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define PUT_WITH_TAG(p, val) (*(unsigned int *)(p) = (val) | GET_TAG(p))

#define PUT_TAG(p) (*(unsigned int*) (p) = GET(p) | 0x2u)
#define DELETE_TAG(p) (*(unsigned int*) (p) = GET(p) & ~0x2u)

#define GET_SIZE(p) (GET(p) & ~0x7u)
#define GET_ALLOC(p) (GET(p) & 0x1u)
#define GET_TAG(p) (GET(p) & 0x2u)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define IS_BLOCK_ALLOCATED(ptr) (GET_ALLOC(HDRP(ptr)) && GET_SIZE(HDRP(ptr)))
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

static int extend_heap_recycle(void *ptr, size_t size);

static char *heap_listp = 0;
static char *current_block;

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *) -1) {
        return -1;
    }

    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);

    current_block = heap_listp;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    size_t asize;
    size_t extendsize;
    char *bp;

    if (heap_listp == 0) {
        mm_init();
    }

    if (size == 0) {
        return NULL;
    }

    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    } else {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }

    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize)) == NULL) {
        return NULL;
    }
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    if (ptr == 0) {
        return;
    }

    size_t size = GET_SIZE(HDRP(ptr));

    if (heap_listp == 0) {
        mm_init();
    }

    DELETE_TAG(HDRP(NEXT_BLKP(ptr)));

    PUT_WITH_TAG(HDRP(ptr), PACK(size, 0));
    PUT_WITH_TAG(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    void *new_ptr = ptr;
    size_t body_size = size;
    int expandable_size;
    int extendsize;
    int remaining_size;
    int is_next_block_allocated, is_prev_block_allocated, is_reallocated = FALSE;

    if (size == 0) {
        return NULL;
    }

    if (body_size <= DSIZE) {
        body_size = 2 * DSIZE;
    } else {
        body_size = DSIZE * ((body_size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }

    body_size += PADDING;

    remaining_size = GET_SIZE(HDRP(ptr)) - body_size;

    if (remaining_size < 0) {
        is_next_block_allocated = IS_BLOCK_ALLOCATED(NEXT_BLKP(ptr));
        is_prev_block_allocated = IS_BLOCK_ALLOCATED(PREV_BLKP(ptr));
        if (!is_next_block_allocated) {
            expandable_size = remaining_size;
            if (remaining_size < 0) {
                expandable_size += extend_heap_recycle(ptr, -remaining_size);
            }
            if (expandable_size < 0) {
                extendsize = MAX(-expandable_size, CHUNKSIZE);
                if (extend_heap(extendsize) == NULL) {
                    return NULL;
                }
                expandable_size += extendsize;
            }
            PUT(HDRP(ptr), PACK(body_size + expandable_size, 1));
            PUT(FTRP(ptr), PACK(body_size + expandable_size, 1));
            is_reallocated = TRUE;
        }

        if (!is_reallocated && !is_prev_block_allocated) {
            new_ptr = PREV_BLKP(ptr);
            expandable_size = remaining_size + (int)GET_SIZE(HDRP(new_ptr));
            if (expandable_size >= 0) {
                PUT(HDRP(new_ptr), PACK(body_size + expandable_size, 1));
                PUT(FTRP(new_ptr), PACK(body_size + expandable_size, 1));
                memcpy(new_ptr, ptr, MIN(size, body_size));
                is_reallocated = TRUE;
            }
        }

        if (!is_reallocated) {
            new_ptr = mm_malloc(body_size - DSIZE);
            memmove(new_ptr, ptr, MIN(size, body_size));
            mm_free(ptr);
        }

        remaining_size = GET_SIZE(HDRP(new_ptr)) - body_size;
    }

    if (remaining_size < 2 * PADDING) {
        PUT_TAG(HDRP(NEXT_BLKP(new_ptr)));
    }

    return new_ptr;
}

static int extend_heap_recycle(void *ptr, size_t size) {
    int result = 0;
    void *cur;
    size = ALIGN(size);

    for (cur = NEXT_BLKP(ptr); (!IS_BLOCK_ALLOCATED(cur)) && GET_SIZE(HDRP(cur)) > 0; cur = NEXT_BLKP(cur)) {
        result += (int)GET_SIZE(HDRP(cur));
        if (result > size) {
            break;
        }
    }

    return result;

}

static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    size = ALIGN(words);
    if ((long) (bp = mem_sbrk(size)) == -1) {
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (GET_TAG(HDRP(PREV_BLKP(bp)))) {
        prev_alloc = 1;
    }

    if (prev_alloc && next_alloc) {
        return bp;
    } else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT_WITH_TAG(HDRP(bp), PACK(size, 0));
        PUT_WITH_TAG(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {
        size += GET_SIZE((HDRP(PREV_BLKP(bp))));
        PUT_WITH_TAG(FTRP(bp), PACK(size, 0));
        PUT_WITH_TAG(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {
        size += (GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))));
        PUT_WITH_TAG(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT_WITH_TAG(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    if ((current_block > (char *) bp) && (current_block < NEXT_BLKP(bp))) {
        current_block = bp;
    }
    return bp;
}

static void *find_fit(size_t asize) {
    char *current_block_backup = current_block;
    char *best_block = NULL;

    int size_current, size_best;
    for (; GET_SIZE(HDRP(current_block)) > 0; current_block = NEXT_BLKP(current_block)) {
        size_current = GET_SIZE(HDRP(current_block));
        if ((asize <= size_current) && (!GET_ALLOC(HDRP(current_block))) && (!(GET_TAG(HDRP(current_block))))) {
            return current_block;
        }
    }

    for (current_block = heap_listp; current_block < current_block_backup; current_block = NEXT_BLKP(current_block)) {
        size_current = GET_SIZE(HDRP(current_block));
        if ((asize <= size_current) && (!GET_ALLOC(HDRP(current_block))) && (!(GET_TAG(HDRP(current_block))))) {
            if (best_block == NULL || (size_current < size_best)) {
                best_block = current_block;
                size_best = size_current;
            }
        }
    }

    return best_block;
}

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
static void place(void *bp, size_t asize) {
    size_t ptr_size = GET_SIZE(HDRP(bp));
    size_t remainder = ptr_size - asize;
    if (remainder >= 2 * DSIZE) {
        /* Split block */
        PUT_WITH_TAG(HDRP(bp), PACK(asize, 1)); /* Block header */
        PUT_WITH_TAG(FTRP(bp), PACK(asize, 1)); /* Block footer */
        PUT(HDRP(NEXT_BLKP(bp)), PACK(remainder, 0)); /* Next header */
        PUT(FTRP(NEXT_BLKP(bp)), PACK(remainder, 0)); /* Next footer */
    } else {
        /* Do not split block */
        PUT_WITH_TAG(HDRP(bp), PACK(ptr_size, 1)); /* Block header */
        PUT_WITH_TAG(FTRP(bp), PACK(ptr_size, 1)); /* Block footer */
    }
}