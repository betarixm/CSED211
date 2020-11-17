/*
 * Cache Lab: Understanding Cache Memories
 * Part A: Writing a Cache Simulator
 *
 * 20190084 권민재
 * mzg00@postech.ac.kr
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <math.h>
#include "cachelab.h"

#define TRUE 1
#define FALSE 0

// Block: 캐시 블럭을 나타내기 위한 구조체/타입
typedef struct {
    int valid; // 블럭의 Valid
    unsigned long long tag; // 블럭의 Tag
    int lruCount; // 블럭의 LRU 카운터 값
} Block;

// Set: 캐시의 set를 나타내기 위한 구조체/타입
typedef struct {
    Block *blockList; // 블럭의 배열
} Set;

// Cache: 캐시를 나타내기 위한 구조체 / 타입
typedef struct {
    int numSet; // 캐시의 set 크기
    int numLine; // 각 set의 라인 수
    Set *setList; // set의 배열
} Cache;

// 전역 변수 (해당 과제에 전역 변수 제한 없음)
Cache cache = {}; // 시뮬레이션을 진행할 캐시
int s = 0, E = 0, b = 0; // 외부로부터 입력받은 s, E, b 저장
int LRU = 0; // 전역 LRU 카운터
int hits = 0, misses = 0, evictions = 0; // hit, miss, eviction 결과값이 담길 변수

// 캐시 시뮬레이션에 필요한 공간을 동적 할당하는 함수
void initCache();

// 캐시 시뮬레이션에 동적 할당된 메모리를 정리하는 함수
void freeCache();

// 캐시를 시뮬레이션 하는 함수
void simulate(FILE *file);

// 어떤 주소를 가지고 캐시에 접근하는 함수
void cacheAccess(unsigned long long int addr);

int main(int argc, char *argv[]) {
    // 파일 입출력 관련 변수 선언
    char *filename;
    FILE *file;

    // 외부 인자 파싱
    for (int opt; (opt = getopt(argc, argv, "s:E:b:t:")) != -1;) {
        switch (opt) {
            case 's': // s 저장
                s = atoi(optarg);
                break;
            case 'E': // e 저장
                E = atoi(optarg);
                break;
            case 'b': // b 저장
                b = atoi(optarg);
                break;
            case 't': // 파일 이름 저장
                filename = optarg;
                break;
            default: // 지정된 인자가 아니라면 1 반환 및 종료. (에러)
                return 1;
        }
    }

    // s, E, b, 파일 이름이 적절하게 설정되지 않았다면, 1 반환 및 종료. (에러)
    if (!s || !E || !b || !filename) {
        return 1;
    }

    // 파일을 여는데 실패했다면, 1 반환 및 종료. (에러)
    if (!(file = fopen(filename, "r"))) {
        return 1;
    }

    // 캐시 초기화. 필요한 공간 동적 할당
    initCache();

    // 캐시 시뮬레이션 수행
    simulate(file);

    // 시뮬레이션 결과 출력
    printSummary(hits, misses, evictions);

    // 동적할당된 메모리 정리 및 파일 닫기
    freeCache();
    fclose(file);

    return 0;
}

// 캐시 시뮬레이션에 필요한 공간을 동적 할당하는 함수
void initCache() {
    cache.numSet = (int) pow(2.0, s);
    cache.numLine = E;
    cache.setList = malloc(sizeof(Set) * cache.numSet);
    for (int i = 0; i < cache.numSet; ++i) {
        cache.setList[i].blockList = malloc(sizeof(Block) * cache.numLine);
        for (int j = 0; j < E; ++j) {
            cache.setList[i].blockList[j].valid = FALSE;
            cache.setList[i].blockList[j].tag = 0;
            cache.setList[i].blockList[j].lruCount = 0;
        }
    }
}

// 캐시 시뮬레이션에 동적 할당된 메모리를 정리하는 함수
void freeCache() {
    for (int i = 0; i < cache.numSet; ++i) {
        free(cache.setList[i].blockList);
    }
    free(cache.setList);
}

// 캐시를 시뮬레이션 하는 함수
void simulate(FILE *file) {
    char opt;
    unsigned long long addr;

    while (fscanf(file, " %c %llx%*c%*d", &opt, &addr) != EOF) {
        printf("%c %llx: ", opt, addr);
        if (opt == 'I') { continue; }
        cacheAccess(addr);
        if (opt == 'M') { cacheAccess(addr); }
    }
}

// 어떤 주소를 가지고 캐시에 접근하는 함수
void cacheAccess(unsigned long long int addr) {
    // addr의 인덱스. b 만큼 오른쪽으로 쉬프트하고, 이를 associativity 에서 1 뺀 값과 and 하면 구할 수 있다.
    unsigned int INDEX = (addr >> (unsigned int) b) & ((unsigned int) pow(2, s) - 1);

    // addr의 태그. addr을 s+b 만큼 오른쪽으로 쉬프트하면 구할 수 있다.
    unsigned long long TAG = (addr >> ((unsigned int) s + (unsigned int) b));

    int blockIdx; // 블럭 탐색을 위한 임시 변수
    int evictionBlock = 0; // eviction할 블럭의 인덱스
    unsigned long long int evictionLru = -1; // eviction 할 블럭을 결정하는 LRU 지표

    int isHit = FALSE; // hit 여부를 나타내는 변수
    Set *set = &cache.setList[INDEX]; // addr으로 캐시에 접근할 때, 접근해야 할 캐시의 set.

    // set의 블럭들을 탐색하면서, 태그가 같은 valid한 블럭을 찾았다면, hit가 발생했음을 기록한다.
    for (blockIdx = 0; blockIdx < E; ++blockIdx) {
        if (TAG == set->blockList[blockIdx].tag && set->blockList[blockIdx].valid) {
            isHit = TRUE;
            break;
        }
    }

    if (isHit) { // Hit가 발생했다면,
        set->blockList[blockIdx].lruCount = LRU; // 해당 블럭에 전역 LRU 카운터를 설정한다.
        ++hits; // hits를 1 증가시킨다.
    } else { // Hit가 발생하지 않았다면,
        // 블럭들을 탐색하면서, set에서 lruCount의 값이 가장 작은 블럭을 찾는다.
        for (blockIdx = 0; blockIdx < E; ++blockIdx) {
            if (evictionLru > set->blockList[blockIdx].lruCount) {
                evictionBlock = blockIdx;
                evictionLru = set->blockList[blockIdx].lruCount;
            }
        }

        // lruCount가 가장 작은 블럭이 valid 하다면, eviction이 일어나야 하므로, evictions를 1 증가시킨다.
        if (set->blockList[evictionBlock].valid) {
            ++evictions;
        }

        // lruCount가 가장 작은 블럭의 값을 초기화 한다.
        set->blockList[evictionBlock].valid = TRUE;
        set->blockList[evictionBlock].tag = TAG;
        set->blockList[evictionBlock].lruCount = LRU;

        ++misses; // misses를 1 증가시킨다.
    }

    ++LRU; // LRU를 1 증가시킨다.
}