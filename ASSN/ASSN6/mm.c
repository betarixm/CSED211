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

static int mm_check();

#define DEBUGx

#ifdef DEBUG
#define VERBOSE
int count_free = 0, count_realloc = 0, count_malloc = 0;
#endif

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

#ifdef DEBUG
    ++count_malloc;
#endif

    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    } else {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }

    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
#ifdef DEBUG
        mm_check();
#endif
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize)) == NULL) {
#ifdef DEBUG
        --count_malloc;
#endif
        return NULL;
    }
    place(bp, asize);

#ifdef DEBUG
    mm_check();
#endif

    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    if (ptr == 0) {
        return;
    }

#ifdef DEBUG
    if(IS_BLOCK_ALLOCATED(ptr)){
        ++count_free;
    }
#endif

    size_t size = GET_SIZE(HDRP(ptr));

    if (heap_listp == 0) {
        mm_init();
    }

    DELETE_TAG(HDRP(NEXT_BLKP(ptr)));

    PUT_WITH_TAG(HDRP(ptr), PACK(size, 0));
    PUT_WITH_TAG(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);

#ifdef DEBUG
    mm_check();
#endif

}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {

#ifdef DEBUG
    ++count_realloc;
#endif

    void *new_ptr = ptr;
    size_t body_size = size;
    int expandable_size;
    int extendsize;
    int remaining_size;
    int is_next_block_allocated, is_prev_block_allocated, is_reallocated = FALSE;

    if (size == 0) {
#ifdef DEBUG
        --count_realloc;
#endif
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
#ifdef DEBUG
                    --count_realloc;
#endif
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
                memmove(new_ptr, ptr, MIN(size, body_size));
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

#ifdef DEBUG
    mm_check();
#endif

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
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || GET_TAG(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

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

/*
 * mm_check - Heap Consistency Checker
 * return -1 at error. else, return 0.
 * It checks the suggested in "malloclab.pdf"
 */
static int mm_check() {
    /* Below is suggested check list in "malloclab.pdf" */

    char* cursor;
    int num_free_blocks, num_alloc_blocks, num_coalesce_errs, num_overlap_blocks, num_unaligned_blocks;
    printf("\n[+] DEBUG STATUS\n");

    /*
     * Is every block in the free list marked as free?
     * - 이 malloc 구현에서는 implicit 리스트를 이용하고 있기 때문에, free list가 존재하지 않는다.
     * - 그렇기 때문에, free list를 체크하는 대신 free, malloc, realloc이 불린 횟수와 실제 동적할당 된 힙에서의 allocated 개수를 비교하여 이를 검증할 것이다.
     */
    printf("  - Check the number of function call and allocated block is matching...\n");
    printf("    (If this test print 'PASS' at last debug, it means it's okay.)\n");
    num_free_blocks = 0, num_alloc_blocks = 0;
    for(cursor=current_block; GET_SIZE(HDRP(cursor)) > 0; cursor=NEXT_BLKP(cursor)) {
        if (IS_BLOCK_ALLOCATED(cursor)) {
            num_alloc_blocks++;
        } else {
            num_free_blocks++;
        }
    }

    for(cursor = NEXT_BLKP(heap_listp); cursor < current_block; cursor = NEXT_BLKP(current_block)) {
        if (IS_BLOCK_ALLOCATED(cursor)) {
            num_alloc_blocks++;
        } else {
            num_free_blocks++;
        }
    }

#ifdef VERBOSE
    printf("    * Free Blocks in heap: %d\n", num_free_blocks);
    printf("    * Allocated Blocks in heap: %d\n", num_alloc_blocks);
    printf("    * mm_malloc called: %d\n", count_malloc);
    printf("    * mm_free called: %d\n", count_free);
    printf("    * mm_realloc called: %d\n", count_realloc);
#endif

    if((count_malloc - count_free) != num_alloc_blocks) {
        printf("    [ ERRO ] The number of function call and allocated block is not matched.\n");
        printf("             Expected allocated: %d, Actual allocated: %d\n", count_malloc - count_free, num_alloc_blocks);
    } else {
        printf("    [ PASS ] It seems good :) \n");
    }
    printf("\n");

    /*
     * Are there any contiguous free blocks that somehow escaped coalescing?
     */
    printf("  - Check any contiguous free blocks that somehow escaped coalescing...\n");
    num_coalesce_errs = -1;
    for(cursor=current_block; GET_SIZE(HDRP(cursor)) > 0; cursor=NEXT_BLKP(cursor)) {
        if ((!IS_BLOCK_ALLOCATED(cursor)) && (!IS_BLOCK_ALLOCATED(NEXT_BLKP(cursor)))) {
            num_coalesce_errs++;
        }
    }

    for(cursor = NEXT_BLKP(heap_listp); cursor < current_block; cursor = NEXT_BLKP(current_block)) {
        if ((!IS_BLOCK_ALLOCATED(cursor)) && (!IS_BLOCK_ALLOCATED(NEXT_BLKP(cursor)))) {
            num_coalesce_errs++;
        }
    }

    if(num_coalesce_errs != 0) {
        printf("    [ ERRO ] The number of contiguous free blocks: %d\n", num_coalesce_errs);
    } else {
        printf("    [ PASS ] It seems good :) \n");
    }
    printf("\n");

    /*
     * Is every free block actually in the free list?
     * - Free list를 사용하고 있지 않기 때문에, 해당 검사는 수행하지 않는다.
     */

    /*
     * Do the pointers in the free list point to valid free blocks?
     * - 이 구현은 implicit list를 이용하고 있기 때문에, 해당 검사는 수행하지 않는다.
     */

    /*
     * Do any allocated blocks overlap?
     * - 이 구현은 implicit list를 이용하고 있기 때문에, 모든 블럭 리스트에 대해 겹치는 구간이 있는지 검사할 것 이다.
     */
    printf("  - Check any blocks overlap...\n");

    num_overlap_blocks = 0;
    for(cursor=current_block; GET_SIZE(HDRP(cursor)) > 0; cursor=NEXT_BLKP(cursor)) {
        if (IS_BLOCK_ALLOCATED(NEXT_BLKP(cursor)) && (cursor + GET_SIZE(HDRP(cursor)) > NEXT_BLKP(cursor))) {
            num_overlap_blocks++;
        }
    }

    for(cursor = NEXT_BLKP(heap_listp); cursor < current_block; cursor = NEXT_BLKP(current_block)) {
        if (IS_BLOCK_ALLOCATED(NEXT_BLKP(cursor)) && (cursor + GET_SIZE(HDRP(cursor)) > NEXT_BLKP(cursor))) {
            num_overlap_blocks++;
        }
    }

    if(num_overlap_blocks != 0) {
        printf("    [ ERRO ] The number of overlapping blocks: %d\n", num_overlap_blocks);
    } else {
        printf("    [ PASS ] It seems good :) \n");
    }
    printf("\n");

    /*
     * Do the pointers in a heap block point to valid heap addresses?
     * - 모든 힙 블록 포인터가 align 된 주소를 가리키는지 확인한다.
     */
    printf("  - Check the pointers in a heap block point to valid (aligned) heap addresses...\n");
    num_unaligned_blocks = 0;
    for(cursor=current_block; GET_SIZE(HDRP(cursor)) > 0; cursor=NEXT_BLKP(cursor)) {
        if (IS_BLOCK_ALLOCATED(NEXT_BLKP(cursor)) && ((NEXT_BLKP(cursor) - cursor)%ALIGNMENT != 0)) {
            num_unaligned_blocks++;
        }
    }

    for(cursor = NEXT_BLKP(heap_listp); cursor < current_block; cursor = NEXT_BLKP(current_block)) {
        if (IS_BLOCK_ALLOCATED(NEXT_BLKP(cursor)) && ((NEXT_BLKP(cursor) - cursor)%ALIGNMENT != 0)) {
            num_unaligned_blocks++;
        }
    }

    if(num_unaligned_blocks++ != 0) {
        printf("    [ ERRO ] The number of un-aligned blocks: %d\n", num_unaligned_blocks);
    } else {
        printf("    [ PASS ] It seems good :) \n");
    }
    printf("\n");
    return 0;
}
