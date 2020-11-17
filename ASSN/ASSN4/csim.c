#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <math.h>
#include "cachelab.h"

typedef struct {
    bool valid;
    unsigned long long tag;
    int lru_count;
} Block;

typedef struct {
    Block *arr_block;
} Set;

typedef struct {
    int num_set;
    int num_block;
    Set *arr_set;
} Cache;

Cache cache = {};
int s = 0, E = 0, b = 0;
int LRU = 0;
int hits = 0, misses = 0, evictions = 0;

void initCache();

void freeCache();

void parseTrace(FILE* file);

void cacheAccess(unsigned long long int addr);

int main(int argc, char *argv[]) {
    char *filename;
    FILE *file;
    for (int opt; (opt = getopt(argc, argv, "s:E:b:t:")) != -1;) {
        switch (opt) {
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                filename = optarg;
                break;
            default:
                return 1;
        }
    }

    if (!s || !E || !b || !filename) {
        return 1;
    }

    if (!(file = fopen(filename, "r"))) {
        return 1;
    }

    initCache();
    parseTrace(file);
    printSummary(hits, misses, evictions);
    freeCache(file);
    fclose(file);

    return 0;
}

void initCache() {
    cache.num_set = (int)pow(2.0, s);
    cache.num_block = E;
    cache.arr_set = malloc(sizeof(Set) * cache.num_set);
    for (int i = 0; i < cache.num_set; ++i) {
        cache.arr_set[i].arr_block = malloc(sizeof(Block) * cache.num_block);
        for(int j = 0; j < E; ++j) {
            cache.arr_set[i].arr_block[j].valid = false;
            cache.arr_set[i].arr_block[j].tag = 0;
            cache.arr_set[i].arr_block[j].lru_count = 0;
        }
    }
}

void freeCache() {
    for (int i = 0; i < cache.num_set; ++i) {
        free(cache.arr_set[i].arr_block);
    }
    free(cache.arr_set);
}

void parseTrace(FILE* file) {
    char opt;
    unsigned long long addr;

    while (fscanf(file, " %c %llx%*c%*d", &opt, &addr) != EOF) {
        printf("%c %llx: ", opt, addr);
        if (opt == 'I') { continue; }
        cacheAccess(addr);
        if (opt == 'M') { cacheAccess(addr); }
    }
}

void cacheAccess(unsigned long long int addr) {
    unsigned int INDEX = (addr >> (unsigned int)b) & ((unsigned int)pow(2, s) - 1);
    unsigned long long TAG = (addr >> ((unsigned int)s + (unsigned int)b));
    int block_idx, eviction_block = 0;
    unsigned long long int eviction_lru = -1;
    bool is_hit = false;
    printf("idx %d, tag %llx \n", INDEX, TAG);
    Set *set = &cache.arr_set[INDEX];

    for(block_idx = 0; block_idx < E; ++block_idx){
        if(TAG == set->arr_block[block_idx].tag && set->arr_block[block_idx].valid){
            is_hit = true;
            break;
        }
    }

    if(is_hit){
        ++hits;
        printf("hit ");
        set->arr_block[block_idx].lru_count = LRU++;
    } else {
        ++misses;
        for(int eviction_idx = 0; eviction_idx < E; ++eviction_idx){
            if(eviction_lru > set->arr_block[eviction_idx].lru_count){
                eviction_block = eviction_idx;
                eviction_lru = set->arr_block[eviction_idx].lru_count;
            }
        }
        if(set->arr_block[eviction_block].valid){
            ++evictions;
        }
        set->arr_block[eviction_block].valid = true;
        set->arr_block[eviction_block].tag = TAG;
        set->arr_block[eviction_block].lru_count = LRU++;
    }
}