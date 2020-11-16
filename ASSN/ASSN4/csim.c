#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include "cachelab.h"

typedef struct {
    bool valid;
    int tag;
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

void cacheAccess(int addr);

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

    if (!(file = fopen(optarg, "r"))) {
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
    cache.num_set = 2 << s;
    cache.num_block = E;
    cache.arr_set = malloc(sizeof(Set) * cache.num_set);
    for (int i = 0; i < cache.num_set; ++i) {
        cache.arr_set[i].arr_block = calloc(sizeof(Block), cache.num_block);
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
    int addr;

    while (fscanf(file, " %c %x%*c%*d", &opt, &addr) != EOF) {
        if (opt == 'I') { continue; }
        cacheAccess(addr);
        if (opt == 'M') { cacheAccess(addr); }
    }
}

void cacheAccess(int addr) {
    int INDEX = (addr >> b) & (0x7fffffff >> (31 - s));
    int TAG = (addr >> (s + b));
    int idx_block = 0;
    int eviction_lru = -1;
    int eviction_block = 0;

    Set *set = &cache.arr_set[INDEX];

    for(idx_block = 0;; ++idx_block){
        if(idx_block >= E) {
            ++misses;
            for(int ia = 0; ia < E; ++ia){
                if(eviction_lru > set->arr_block[ia].lru_count){
                    eviction_block = ia;
                    eviction_lru = set->arr_block[ia].lru_count;
                }
            }
            if(set->arr_block[eviction_block].valid){
                ++evictions;
            }
            set->arr_block[eviction_block].valid = true;
            set->arr_block[eviction_block].tag = TAG;
            set->arr_block[eviction_block].lru_count = ++LRU;
            return;
        }
        if(TAG == set->arr_block[idx_block].tag && set->arr_block[idx_block].valid){
            break;
        }
    }

    ++hits;
    set->arr_block[idx_block].lru_count = ++LRU;
}