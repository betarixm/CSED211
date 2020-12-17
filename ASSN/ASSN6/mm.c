/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 *
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

#define PADDING (1u<<7u) // 블럭 패딩
#define TRUE 1
#define FALSE 0

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4u
#define DSIZE 8u
#define CHUNKSIZE (1u<<12u)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define PACK(size, alloc)  (((unsigned int)(size)) | ((unsigned int)(alloc))) // 헤더를 패킹하는 매크로

#define GET(p) (*(unsigned int *)(p)) // 특정 주소를 참조해서 값을 가져오는 매크로

#define PUT(p, val) (*(unsigned int *)(p) = (val)) // 특정 주소를 참조해서 값을 기록하는 매크로
#define PUT_WITH_TAG(p, val) (*(unsigned int *)(p) = (val) | GET_TAG(p)) // 특정 주소를 참조해서 태그와 함께 값을 기록하는 매크로

#define PUT_TAG(p) (*(unsigned int*) (p) = GET(p) | 0x2u) // 특정 주소에 태그와 함께 값을 기록하는 매크로
#define DELETE_TAG(p) (*(unsigned int*) (p) = GET(p) & ~0x2u) // 태그를 제거하는 매크로

#define GET_SIZE(p) (GET(p) & ~0x7u) // 블럭의 크기를 반환하는 매크로
#define GET_ALLOC(p) (GET(p) & 0x1u) // 블럭의 allocation 여부를 반환하는 매크로
#define GET_TAG(p) (GET(p) & 0x2u) // 블럭의 태그를 반환하는 매크로

#define HDRP(bp) ((char *)(bp) - WSIZE) // 블럭의 헤더 주소를 반환하는 매크로
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // 블럭의 푸터 주소를 반환하는 매크로

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) // 블럭의 다음 블럭 주소를 반환하는 매크로
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) // 블럭의 이전 블럭 주소를 반환하는 매크로

#define IS_BLOCK_ALLOCATED(ptr) (GET_ALLOC(HDRP(ptr)) && GET_SIZE(HDRP(ptr))) // 블럭의 할당 여부를 반환하는 매크로

#define RED   "\x1B[31m" // ANSI Escape Code: 색상 지정 - 빨강
#define GRN   "\x1B[32m" // ANSI Escape Code: 색상 지정 - 초록
#define RESET "\x1B[0m"  // ANSI Escape Code: 색상 지정 - 초기화


// 인자로 주어진 크기만큼 mem_sbrk로 힙을 확장하는 함수
void *extend_heap(size_t words);

// 특정 주소에 특정 사이즈의 블럭을 allocate 하는 함수
static void place(void *bp, size_t asize);

// 특정 사이즈가 들어갈 알맞은 위치를 찾아주는 함수
static void *find_fit(size_t asize);

// 인접 free 블럭들을 coalesce 하는 함수
static void *coalesce(void *bp);

// 인접하면서 연속하는 free 블럭들의 크기 합을 반환하는 함수
int extend_heap_recycle(void *ptr, size_t size);

// 힙의 consistency 를 체크하는 디버깅용 함수
static int mm_check();

// DEBUG 모드를 활성화하여 mm_check()를 각 함수 호출마다 실행되도록 하는 지정자
// 아래 주석을 해제하여 활성화할 수 있다.
// #define DEBUG

static char *heap_listp = 0; // 힙의 첫 주소를 가리키는 포인터
static char *current_block; // 현재 작업 중인 블럭을 가리키는 포인터

int num_expected_free_blocks = 0; // [디버깅] 예측되는 free block의 개수
int num_expected_blocks = 0; // [디버깅] 예측되는 block의 개수

/*
 * mm_init - initialize the malloc package.
 * 힙의 가장 처음 부분을 초기화하여 할당할 준비를 한다.
 */
int mm_init(void) {
#ifdef DEBUG
    // 예측되는 블럭의 정보를 초기화한다.
    num_expected_free_blocks = 0; num_expected_blocks = 0;
#endif

    // 우선 힙을 정의하는데 필요한 최소 크기를 mem_sbrk를 통해 얻어온다.
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *) -1) {
        return -1;
    }

    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 헤더
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 푸터
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); // 에필로그 헤더
    heap_listp += (2 * WSIZE); // heap_listp가 블록의 주소를 가리키도록 설정한다.

    current_block = heap_listp; // init된 상태이므로, 현재 블럭을 가장 첫 블럭을 가리키도록 한다.

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    size_t asize; // Adjusted size. Alignment에 맞도록 조정된 사이즈
    size_t extendsize; // 더 늘어나야 하는 사이즈
    char *bp; // 블럭 포인터

    // heap_listp가 설정되지 않은 상태라면, 초기화 되지 않았다는 뜻이므로 초기화를 진행한다.
    if (heap_listp == 0) {
        mm_init();
    }

    // size가 0이라면, malloc을 진행하지 않아도 되므로, 미리 종료하낟.
    if (size == 0) {
        return NULL;
    }

    if (size <= DSIZE) {
        // 할당해야 하는 크기가 DSIZE보다 작다면, 2 * DSIZE로 최소 크기를 맞춰준다.
        asize = 2 * DSIZE;
    } else {
        // 아니라면, 아래와 같은 수식을 통해 alignment를 맞춘다.
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }

    // 우선 힙에 해당 사이즈가 들어갈 공간이 있는지 find_fit을 통해 탐색한다.
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize); // find_fit으로 결과를 찾았다면, place를 통해 해당 위치에 allocate를 진행한다.
#ifdef DEBUG
        num_expected_free_blocks--; // fit 된 위치에 allocate 되었을 때, free block의 개수는 1 줄어드는 것이 예측되므로, 1을 줄여준다.
        mm_check(); // 종료되기 전, 디버깅 함수를 호출한다.
#endif
        return bp;
    }

    // find_fit으로 적절한 공간을 찾지 못하였으므로, extend_heap을 이용해 힙을 늘려준다.
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize)) == NULL) {
        return NULL;
    }
    place(bp, asize); // 공간이 늘어났으므로, place를 통해 해당 위치에 allocate를 진행한다.

#ifdef DEBUG
    mm_check(); // 종료되기 전, 디버깅 함수를 호출한다.
#endif

    return bp;
}

/*
 * mm_free - 블럭을 free하는 역할을 수행한다.
 * 주어진 블럭의 allocate 정보를 삭제하여 적절히 free 한 후에, coalesce를 진행한다.
 */
void mm_free(void *ptr) {
    // 주어진 주소가 적절하지 않은 경우이므로, 바로 종료한다.
    if (ptr == 0) {
        return;
    }

#ifdef DEBUG
    // 블럭이 free 될때, free block의 개수가 1 증가하는 것이 예측되므로, 1 더해준다.
    if(IS_BLOCK_ALLOCATED(ptr)){
        ++num_expected_free_blocks;
    }
#endif

    size_t size = GET_SIZE(HDRP(ptr)); // 할당 해제하고자 하는 블럭의 크기를 구한다.

    // heap_listp가 적절하지 않은 값인 경우, 초기화를 진행한다.
    if (heap_listp == 0) {
        mm_init();
    }

    DELETE_TAG(HDRP(NEXT_BLKP(ptr))); // 태그 삭제

    // allocation 정보 삭제
    PUT_WITH_TAG(HDRP(ptr), PACK(size, 0));
    PUT_WITH_TAG(FTRP(ptr), PACK(size, 0));

    // coalesce 수행
    coalesce(ptr);

#ifdef DEBUG
    // 함수가 종료되기 전 디버깅 함수 호출
    mm_check();
#endif

}

/*
 * mm_realloc - 주어진 블럭을 realloc하는 역할을 수행한다.
 * realloc을 수행할 때, 다음 블럭이 free되어 있다면, 해당 블럭으로 확장을 진행하고,
 * 이전 블럭의 free되어 있다면, 이전 블럭으로 확장을 진행한다.
 * 다음 블럭으로 확장을 진행할 때에는, extend_heap_recycle을 이용하여 혹시 coalesce 가 되지 않은 블럭을 모두 합친다.
 * 만약 이전 블럭과 다음 블럭이 모두 비어있지 않다면, malloc을 다시 수행한다.
 */
void *mm_realloc(void *ptr, size_t size) {
    void *new_ptr = ptr; // reallocation의 결과가 될 주소
    size_t body_size = size; // 블럭 몸체의 크기
    int expandable_size; // 확장할 수 있는 크기
    int extendsize; // 확장 해야할 크기
    int remaining_size; // 확장에 남은 크기
    int is_next_block_allocated = FALSE; // 다음 블럭 할당 여부
    int is_prev_block_allocated = FALSE; // 이전 블럭 할당 여부
    int is_reallocated = FALSE; // 재할당 여부

    // 크기가 0이라면 바로 반환한다.
    if (size == 0) {
        return NULL;
    }

    // malloc을 수행할 때와 같이, 사이즈를 align한다.
    if (body_size <= DSIZE) {
        body_size = 2 * DSIZE;
    } else {
        body_size = DSIZE * ((body_size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }

    body_size += PADDING;

    // 남은 크기를 구한다.
    remaining_size = GET_SIZE(HDRP(ptr)) - body_size;

    if (remaining_size < 0) {
        // 남은 크기가 0보다 작다면, 확장을 진행해야 하므로, 확장을 수행한다.

        is_next_block_allocated = IS_BLOCK_ALLOCATED(NEXT_BLKP(ptr)); // 다음 블럭 할당 여부
        is_prev_block_allocated = IS_BLOCK_ALLOCATED(PREV_BLKP(ptr)); // 이전 블럭 할당 여부

        if (!is_next_block_allocated) {
            // 다음 블럭이 할당되지 않았다면, 다음 블럭으로의 확장 가능성을 검토한다.

            expandable_size = remaining_size; // 우선 남은 크기를 확장 가능한 크기로 지정한다.
#ifdef DEBUG
            // 아래 정보가 extend_heap_recycle에서 수정되기 때문에, 예측되는 블럭 개수 정보를 백업한다.
            int count_free_back = num_expected_free_blocks;
            int count_malloc_back = num_expected_blocks;
#endif
            if (remaining_size < 0) {
                // 남은 크기가 0보다 작다면, extend_heap_recycle 을 통해 더 확장할 수 있는 크기를 가져온다.
                expandable_size += extend_heap_recycle(ptr, -remaining_size);
            }

            if (expandable_size < 0) {
                // 확장 가능한 크기가 0보다 작다면, extend를 수행한다.
#ifdef DEBUG
                // 앞서 백업해둔 값을 복구한다.
                num_expected_blocks = count_malloc_back;
                num_expected_free_blocks = count_free_back;
#endif
                // 최소 청크 크기를 만족하는, 확장할 크기를 결정한다.
                extendsize = MAX(-expandable_size, CHUNKSIZE);

                // extend_heap을 통해 확장을 수행한다.
                if (extend_heap(extendsize) == NULL) {
                    return NULL;
                }
#ifdef DEBUG
                // extend_heap에서 예측 블럭 개수를 증가하지만, realloc에서는 증가하지 않으므로, 1을 빼서 복구한다.
                --num_expected_blocks;
#endif
                // 확장 가능한 크기에 방금 구한 크기를 더한다.
                expandable_size += extendsize;
            }

            // 현재 몸체 크기에 확장 가능한 크기를 더해서 블럭을 확장한다.
            PUT(HDRP(ptr), PACK(body_size + expandable_size, 1));
            PUT(FTRP(ptr), PACK(body_size + expandable_size, 1));
            is_reallocated = TRUE; // 재할당이 완료되었음을 마킹한다.
        }

        if (!is_reallocated && !is_prev_block_allocated) {
            // 재할당이 진행되지 않았고, 이전 블럭이 할당되지 않은 상태라면, 이전 블럭으로의 확장을 시도한다.
            new_ptr = PREV_BLKP(ptr); // 이전 블럭의 주소를 가져온다.

            // 확장 가능한 크기를 검토한다.
            expandable_size = remaining_size + (int)GET_SIZE(HDRP(new_ptr));
            if (expandable_size >= 0) {
                // 확장 가능한 크기가 0 이상이라면, 확장을 진행한다.
#ifdef DEBUG
                // 이때, 예측되는 블럭의 개수를 업데이트 한다.
                --num_expected_blocks;
                --num_expected_free_blocks;
#endif
                // 이전 블럭으로부터 크기를 업데이트하여 재할당을 수행한다.
                PUT(HDRP(new_ptr), PACK(body_size + expandable_size, 1));
                PUT(FTRP(new_ptr), PACK(body_size + expandable_size, 1));
                // 데이터를 이전 포인터로 옮긴다.
                memmove(new_ptr, ptr, MIN(size, body_size));
                is_reallocated = TRUE; // 재할당 되었음을 표시한다.
            }
        }

        if (!is_reallocated) {
            // 최종적으로 재할당이 진행되지 않았다면, 새로 malloc을 진행해서 메모리를 할당받는다.
            new_ptr = mm_malloc(body_size - DSIZE);
            memmove(new_ptr, ptr, MIN(size, body_size)); // 정보 이전
            mm_free(ptr); // 기존 할당 받은 공간은 해제한다.
        }
        remaining_size = GET_SIZE(HDRP(new_ptr)) - body_size; // 남은 크기 업데이트
    }

    if (remaining_size < 2 * PADDING) { // 남은 크기가 2*패딩 보다 작다면 태그를 업데이트 한다.
        PUT_TAG(HDRP(NEXT_BLKP(new_ptr)));
    }

#ifdef DEBUG
    // 함수가 종료되기 전에 디버깅 함수를 호출한다.
    mm_check();
#endif

    // 재할당된 주소를 반환한다.
    return new_ptr;
}

/*
 * extend_heap_recycle - 주어진 주소로부터 연속된 free 블럭을 요구한 크기 이상이 될 때까지 모아서 그 크기를 반환한다.
 */
int extend_heap_recycle(void *ptr, size_t size) {
    int result = 0; // 결과 크기를 담을 변수
    void *cur; // 포인터 커서
    size = ALIGN(size); // 크기를 우선 정렬한다.
    
    // 주어진 포인터로부터 탐색을 진행한다.
    for (cur = NEXT_BLKP(ptr); (!IS_BLOCK_ALLOCATED(cur)) && GET_SIZE(HDRP(cur)) > 0; cur = NEXT_BLKP(cur)) {
        // 탐색을 진행하는 동안, 사이즈를 더한다.
        result += (int)GET_SIZE(HDRP(cur));
#ifdef DEBUG
        // 예측 블럭 정보를 업데이트 한다.
        --num_expected_blocks;
        --num_expected_free_blocks;
#endif
        if (result > size) {
            // 사이즈가 충분해 졌을 때, 반복을 종료한다.
            break;
        }
    }
    
    // 크기를 반환한다.
    return result;

}

/*
 * extend_heap - 주어진 크기만큼 힙을 확장시키는 역할을 수행한다.
 */
void *extend_heap(size_t words) {
    char *bp; // 블럭 포인터
    size_t size; // 사이즈

    size = ALIGN(words); // 주어진 사이즈를 align한다.

    // mem_sbrk로 공간을 확보한다.
    if ((long) (bp = mem_sbrk(size)) == -1) {
        return NULL;
    }

#ifdef DEBUG
    // 블럭 증가가 예측되므로, 증가시켜준다.
    num_expected_blocks++;
#endif

    // 확보된 공간에 헤더와 푸터를 설정한다.
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    // coalesce를 진행한다.
    return coalesce(bp);
}

/*
 * coalesce - 연속된 free 블럭들을 합치는 역할을 수행한다.
 * 이전 블럭과 다음 블럭의 할당 여부에 따라서 크게 4가지로 나뉘어 작동한다.
 */
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || GET_TAG(HDRP(PREV_BLKP(bp))); // 이전 블럭의 할당 여부
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // 이후 블럭의 할당 여부
    size_t size = GET_SIZE(HDRP(bp)); // 현재 블럭의 사이즈

    if (prev_alloc && next_alloc) {
        // 앞 뒤 둘 다 할당되어 있을 경우, 바로 반환한다.
        return bp;
    } else if (prev_alloc && !next_alloc) {
        // 다음 블럭이 할당되어 있지 않다면, 다음 블럭까지 확장을 진행하여 두 블럭을 합친다.
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT_WITH_TAG(HDRP(bp), PACK(size, 0));
        PUT_WITH_TAG(FTRP(bp), PACK(size, 0));
#ifdef DEBUG
        // 이때, 전체 블럭 개수는 1 감소한다.
        --num_expected_blocks;
        --num_expected_free_blocks;
#endif
    } else if (!prev_alloc && next_alloc) {
        // 이전 블럭이 할당되어 있지 않다면, 이전 블럭까지 확장을 진행하여 두 블럭을 합친다.
        size += GET_SIZE((HDRP(PREV_BLKP(bp))));
        PUT_WITH_TAG(FTRP(bp), PACK(size, 0));
        PUT_WITH_TAG(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
#ifdef DEBUG
        // 이때, 전체 블럭 개수는 1 감소한다.
        --num_expected_blocks;
        --num_expected_free_blocks;
#endif
    } else {
        // 이전 블럭과 다음 블럭이 모두 할당되어 있지 않다면, 확장을 진행하여 세 블럭을 합친다.
        size += (GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))));
        PUT_WITH_TAG(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT_WITH_TAG(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
#ifdef DEBUG
        // 이때, 전체 블럭 개수는 2 감소한다.
        num_expected_blocks -= 2;
        num_expected_free_blocks -= 2;
#endif
    }
    if ((current_block > (char *) bp) && (current_block < NEXT_BLKP(bp))) {
        // current 블럭의 위치를 업데이트 해준다.
        current_block = bp;
    }
    return bp;
}

/*
 * find_fit - 주어진 사이즈의 블럭이 들어갈 적절한 위치를 찾아서 반환한다.
 * current_block으로부터 뒤의 공간에 대해서는 first-fit을 통해 위치를 찾고,
 * 앞선 탐색으로부터 찾지 못했을 경우에는 heap의 처음부터 current_block 까지에 대해 best_fit을 수행하여 적절한 위치를 찾는다.
 */
static void *find_fit(size_t asize) {
    char *current_block_backup = current_block; // 현재 블럭의 위치를 미리 백업
    char *best_block = NULL; // 최선의 블럭 포인터

    int size_current, size_best; // 크기 변수

    // 현재 블럭으로부터 탐색을 진행한다.
    for (; GET_SIZE(HDRP(current_block)) > 0; current_block = NEXT_BLKP(current_block)) {
        size_current = GET_SIZE(HDRP(current_block));
        if ((asize <= size_current) && (!GET_ALLOC(HDRP(current_block))) && (!(GET_TAG(HDRP(current_block))))) {
            // 넣고자 하는 크기보다 크고, 할당되지 않은 블럭을 발견했다면, 해당 블럭의 주소를 반환한다.
            return current_block;
        }
    }

    // 처음부터 탐색을 진행한다.
    for (current_block = heap_listp; current_block < current_block_backup; current_block = NEXT_BLKP(current_block)) {
        size_current = GET_SIZE(HDRP(current_block));
        if ((asize <= size_current) && (!GET_ALLOC(HDRP(current_block))) && (!(GET_TAG(HDRP(current_block))))) {
            // 넣고자 하는 크기보다 크고, 할당되지 않은 블럭을 발견했을 때,
            if (best_block == NULL || (size_current < size_best)) {
                // best_block이 NULL이거나, 찾은 크기가 최선 크기보다 작다면 best_block을 업데이트한다.
                best_block = current_block;
                size_best = size_current;
            }
        }
    }

    return best_block;
}

/*
 * place - free 블럭에 주어진 사이즈의 블럭을 할당하는 작업을 수행한다.
 * 만약, 블럭에 공간이 많이 남는다면 split을 수행한다.
 */
static void place(void *bp, size_t asize) {
    size_t ptr_size = GET_SIZE(HDRP(bp)); // 현재 블럭의 크기를 구한다.
    size_t remainder = ptr_size - asize; // 남은 공간의 크기를 구한다.
    if (remainder >= 2 * DSIZE) {
        // 남은 공간이 최소 공간보다 크다면, split한다.
        PUT_WITH_TAG(HDRP(bp), PACK(asize, 1)); // 블럭 헤더
        PUT_WITH_TAG(FTRP(bp), PACK(asize, 1)); // 블럭 푸터
        PUT(HDRP(NEXT_BLKP(bp)), PACK(remainder, 0)); // split 된 블럭의 헤더
        PUT(FTRP(NEXT_BLKP(bp)), PACK(remainder, 0)); // split 된 블럭의 푸터
#ifdef DEBUG
        // 이때, free block이 1개 늘어난다.
        num_expected_blocks++;
        num_expected_free_blocks++;
#endif
    } else {
        // 공간이 충분하지 않으므로, split을 진행하지 않는다.
        PUT_WITH_TAG(HDRP(bp), PACK(ptr_size, 1)); // 블럭 헤더
        PUT_WITH_TAG(FTRP(bp), PACK(ptr_size, 1)); // 블럭 푸터
    }
}

/*
 * mm_check - Heap Consistency Checker
 * return -1 at error. else, return 0.
 * It checks the suggested in "malloclab.pdf"
 */
static int mm_check() {
    /* Below is suggested check list in "malloclab.pdf" */

    char* cursor; // 블럭 탐색을 위한 커서
    int num_free_blocks, num_alloc_blocks, num_coalesce_errs, num_overlap_blocks, num_unaligned_blocks; // 블럭 종류에 따른 개수
    printf("\n[+] DEBUG STATUS\n");

    /*
     * Is every block in the free list marked as free?
     * - 이 malloc 구현에서는 implicit 리스트를 이용하고 있기 때문에, free list가 존재하지 않는다.
     * - 그렇기 때문에, free list를 체크하는 대신 free, malloc, realloc이 불린 횟수와 실제 동적할당 된 힙에서의 allocated 개수를 비교하여 이를 검증할 것이다.
     */
    printf("  - Check the expected heap status and actual heap status is matching... (expecting based on counting function call)\n");
    num_free_blocks = -1, num_alloc_blocks = -1;

    for(cursor = heap_listp; cursor < (char*)mem_heap_hi(); cursor = NEXT_BLKP(cursor)) {
        // 모든 블럭에 대해 탐색을 진행하면서,
        if (GET_ALLOC(HDRP(cursor))) {
            // 할당된 블럭이라면, 할당된 블럭 개수를 늘리고,
            num_alloc_blocks++;
        } else {
            // 그렇지 않은 블럭이라면, free된 블럭 개수를 늘린다.
            num_free_blocks++;
        }
    }

    // 실제 블럭 정보와, 예측되는 블럭 정보를 출력한다.
    printf("    * Actual: All (%d), Allocated (%d), Freed (%d)\n", num_alloc_blocks + num_free_blocks, num_alloc_blocks, num_free_blocks);
    printf("    * Expect: All (%d), Allocated (%d), Freed (%d)\n", num_expected_blocks, num_expected_blocks - num_expected_free_blocks, num_expected_free_blocks);

    if((num_expected_blocks - num_expected_free_blocks) != num_alloc_blocks) {
        // 만약, 예측과 다르다면 경고를 출력한다.
        printf("    " RED "[ ERRO ]" RESET " The number of function call and allocated block is not matched.\n");
        printf("             Expected allocated: %d, Actual allocated: %d\n", num_expected_blocks - num_expected_free_blocks, num_alloc_blocks);
    } else {
        // 아니라면, PASS 했음을 출력한다.
        printf("    " GRN "[ PASS ]" RESET " It seems good :) \n");
    }
    printf("\n");

    /*
     * Are there any contiguous free blocks that somehow escaped coalescing?
     * - coalescing이 모두 적절히 수행되었는지 체크한다.
     * - 2개의 연속된 free 블럭이 있는지 확인하여 체크할 것이다.
     */
    printf("  - Check any contiguous free blocks that somehow escaped coalescing...\n");
    num_coalesce_errs = 0; // coalesce 되지 않은 블럭의 개수

    // 모든 블럭에 대해 탐색을 진행하면서, 연속된 free 블럭이 있다면 블럭의 개수에 1 더한다.
    for(cursor=current_block; GET_SIZE(HDRP(NEXT_BLKP(cursor))) > 0; cursor=NEXT_BLKP(cursor)) {
        if ((!IS_BLOCK_ALLOCATED(cursor)) && (!IS_BLOCK_ALLOCATED(NEXT_BLKP(cursor)))) {
            num_coalesce_errs++;
        }
    }

    for(cursor = NEXT_BLKP(heap_listp); NEXT_BLKP(cursor) < current_block; cursor = NEXT_BLKP(cursor)) {
        if ((!IS_BLOCK_ALLOCATED(cursor)) && (!IS_BLOCK_ALLOCATED(NEXT_BLKP(cursor)))) {
            num_coalesce_errs++;
        }
    }

    if(num_coalesce_errs != 0) {
        // 블럭 개수가 0이 아니라면, 에러를 출력한다.
        printf("    " RED "[ ERRO ]" RESET " The number of contiguous free blocks: %d\n", num_coalesce_errs);
    } else {
        printf("    " GRN "[ PASS ]" RESET " It seems good :) \n");
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

    num_overlap_blocks = 0; // overlap 블럭 개수

    // 모든 블럭에 대해 탐색하면서, overlap 되는 블럭이 있다면, 블럭의 개수를 증가시킨다.
    for(cursor=current_block; GET_SIZE(HDRP(cursor)) > 0; cursor=NEXT_BLKP(cursor)) {
        if (IS_BLOCK_ALLOCATED(NEXT_BLKP(cursor)) && (cursor + GET_SIZE(HDRP(cursor)) > NEXT_BLKP(cursor))) {
            num_overlap_blocks++;
        }
    }

    for(cursor = NEXT_BLKP(heap_listp); cursor < current_block; cursor = NEXT_BLKP(cursor)) {
        if (IS_BLOCK_ALLOCATED(NEXT_BLKP(cursor)) && (cursor + GET_SIZE(HDRP(cursor)) > NEXT_BLKP(cursor))) {
            num_overlap_blocks++;
        }
    }

    if(num_overlap_blocks != 0) {
        // 오버랩된 블럭의 개수가 0이 아니라면, 에러를 출력한다.
        printf("    " RED "[ ERRO ]" RESET " The number of overlapping blocks: %d\n", num_overlap_blocks);
    } else {
        printf("    " GRN "[ PASS ]" RESET " It seems good :) \n");
    }
    printf("\n");

    /*
     * Do the pointers in a heap block point to valid heap addresses?
     * - 모든 힙 블록 포인터가 align 된 주소를 가리키는지 확인한다.
     */
    printf("  - Check the pointers in a heap block point to valid (aligned) heap addresses...\n");
    num_unaligned_blocks = 0; // align 되지 않는 블럭의 개수

    // 모든 블럭에 대해 탐색을 진행하면서, 크기가 align 되었는지 확인한다.
    for(cursor=current_block; GET_SIZE(HDRP(cursor)) > 0; cursor=NEXT_BLKP(cursor)) {
        if (IS_BLOCK_ALLOCATED(NEXT_BLKP(cursor)) && ((NEXT_BLKP(cursor) - cursor)%ALIGNMENT != 0)) {
            num_unaligned_blocks++;
        }
    }

    for(cursor = NEXT_BLKP(heap_listp); cursor < current_block; cursor = NEXT_BLKP(cursor)) {
        if (IS_BLOCK_ALLOCATED(NEXT_BLKP(cursor)) && ((NEXT_BLKP(cursor) - cursor)%ALIGNMENT != 0)) {
            num_unaligned_blocks++;
        }
    }

    if(num_unaligned_blocks++ != 0) {
        // align 되지 않은 블럭의 개수가 0이 아니라면, 에러를 출력한다.
        printf("    " RED "[ ERRO ]" RESET " The number of un-aligned blocks: %d\n", num_unaligned_blocks);
    } else {
        printf("    " GRN "[ PASS ]" RESET " It seems good :) \n");
    }
    printf("\n");
    return 0;
}