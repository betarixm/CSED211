#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

int main(){
    for (int x=0; x < 100000; x++){
        int i = 0;
        for(; (*((char*)&(x)+1)) > i; ++i);
        if (i == 18){
            printf("%d", x);
            break;
        }
    }

}