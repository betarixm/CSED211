#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

int main(){
    srand(time(NULL));   // Initialization, should only be called once.
    while(1){
        /* Create some arbitrary values */
        int x = rand();
        int y = rand();
        int z = rand();
        /* Convert to double */
        double dx = (double) x;
        double dy = (double) y;
        double dz = (double) z;
        if ((dx*dy)*dz!=dx*(dy*dz)){
            printf("%f\n", (dx*dy)*dz);
            printf("%f\n", dx*(dy*dz));
            printf("dx: %f, dy: %f, dz: %f\n", dx, dy, dz);
            printf("x: %x, y: %x, z: %x\n", x, y, z);
            break;
        }
    }
    return 0;
}