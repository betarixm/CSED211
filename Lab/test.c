//
// Created by 민재 on 2020/09/09.
//

#include <stdio.h>
#include <assert.h>

unsigned rotate_left(unsigned x, int n) {
    unsigned w = sizeof(unsigned) << 3;
    return (x << n) | (x >> (w - n));
}

int main(){
    assert(rotate_left(0x12345678, 0) == 0x12345678);
    return 0;
}