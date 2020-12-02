#include <stdio.h>

#define arr 5
#define process 5

void first(int arraySize[], int processSize[]){
  int allocation[process] = {0};

  // implementation
  int block, procSize, *blockSize;
  int n_arraySize[arr];

  for(int i = 0; i < arr; i++){
      n_arraySize[i] = arraySize[i];
  }

  for(int proc = 0; proc < process; proc++){
      block = 0;
      procSize = processSize[proc];
      for(; block < arr; block++){
          blockSize = &(n_arraySize[block]);

          if(procSize <= *blockSize){
              allocation[proc] = block;
              *blockSize -= procSize;
              break;
          }
      }
      if(block == arr){
          printf("Process %d can't allocated. Indicate it with -1.\n", proc);
          allocation[proc] = -1;
      }
  }
  //

  for (int i = 0; i < arr ; i++){
    printf("Process %d, Block %d\n", i, allocation[i]);
  }
}

void best(int arraySize[], int processSize[]){
  int allocation[process] = {0};

  // implementation
  int block, bestBlock, procSize, blockSize;
  int n_arraySize[arr];

  for(int i = 0; i < arr; i++){
      n_arraySize[i] = arraySize[i];
  }

    for(int proc = 0; proc < process; proc++){
        bestBlock = -1;
        procSize = processSize[proc];
        for(block = 0; block < arr; block++){
            blockSize = n_arraySize[block];
            if((procSize <= blockSize) && ((bestBlock == -1) || (blockSize < n_arraySize[bestBlock]))){
                bestBlock = block;
            }
        }

        if(bestBlock == -1){
            printf("Process %d can't allocated. Indicate it with -1.\n", proc);
            allocation[proc] = -1;
        } else {
            allocation[proc] = bestBlock;
            n_arraySize[bestBlock] -= processSize[proc];
        }
    }
  //

  for (int i = 0; i < arr ; i++){
    printf("Process %d, Block %d\n", i, allocation[i]);
  }
}

int main()
{
  int arraySize[] = {30, 50, 40, 10, 20};
  int processSize[] = {41, 4, 24, 15, 19};

  first(arraySize,processSize);
  best(arraySize,processSize);
}

