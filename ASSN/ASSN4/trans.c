/*
 * Cache Lab: Understanding Cache Memories
 * Part B: Optimizing Matrix Transpose
 *
 * 20190084 권민재
 * mzg00@postech.ac.kr
 */

/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
void blocking(int M, int N, int A[N][M], int B[M][N], int ROW, int COL);
void blocking_diag(int M, int N, int A[N][M], int B[M][N], int ROW, int COL);
void blocking_64(int M, int N, int A[N][M], int B[M][N], int ROW, int COL);
/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    // 행렬 탐색을 위한 임시 변수
    int row, col;

    // 사이즈에 따른 transpose 함수 포인터
    void (*trans_function)(int M, int N, int A[N][M], int B[M][N], int ROW, int COL);

    // 64 x 64 행렬에는 blocking_64 함수를 지정.
    if(M == 64 && N == 64){
        trans_function = blocking_64;
    } else {
        trans_function = blocking;
    }

    // 8x8 부분 행렬로 쪼개서 계산
    for(row = 0; row < N; row+=8){
        for(col = 0; col < M; col+=8){
            trans_function(M, N, A, B, row, col);
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

void blocking(int M, int N, int A[N][M], int B[M][N], int ROW, int COL){
    if(ROW == COL){ // 대각선을 포함한 부분에 대해서는 blocking_diag를 수행한다.
        blocking_diag(M, N, A, B, ROW, COL);
        return;
    }

    // 8x8 부분 행렬에 대해 transpose를 수행하되, M과 N은 넘지 않도록 한다.
    int row, col;
    for(row = COL; row < COL + 8 && row < M; ++row){
        for(col = ROW; col < ROW + 8 && col < N; ++col){
            B[row][col] = A[col][row];
        }
    }
}

void blocking_diag(int M, int N, int A[N][M], int B[M][N], int ROW, int COL){
    int row, col, tmp;
    // 8x8 부분 행렬에 대해 transpose를 수행하되, M과 N은 넘지 않도록 한다.
    for(row = ROW; row < N && row < ROW + 8; ++row){
        tmp = A[row][row]; // 두 인덱스가 같은 곳의 값을 미리 변수에 저장한다.
        for(col = COL; col < M && col < COL + 8; ++col){
            if(row == col){ // 두 인덱스가 같을 때에는 transpose를 수행하지 않는다.
                continue;
            }
            B[col][row] = A[row][col];
        }
        // 저장된 변수로 B를 설정한다.
        B[row][row] = tmp;
    }
}

void blocking_64(int M, int N, int A[N][M], int B[M][N], int ROW, int COL){
    // A의 오른쪽 위 4개의 값을 미리 저장한다.
    int r0 = A[ROW][COL + 4];
    int r1 = A[ROW][COL + 5];
    int r2 = A[ROW][COL + 6];
    int r3 = A[ROW][COL + 7];
    int t0, t1, t2, t3;
    int i = 0;

    // A의 왼쪽 반에 대해 위에서부터 4개씩 전치를 수행한다.
    for(i = 0; i < 8; ++i){
        t0 = A[ROW + i][COL + 0];
        t1 = A[ROW + i][COL + 1];
        t2 = A[ROW + i][COL + 2];
        t3 = A[ROW + i][COL + 3];
        B[COL + 0][ROW + i] = t0;
        B[COL + 1][ROW + i] = t1;
        B[COL + 2][ROW + i] = t2;
        B[COL + 3][ROW + i] = t3;
    }

    // A의 오른쪽 반에 대해 아래에서 부터 4개씩 전치를 수행하되, 제일 위의 줄은 제외된다.
    for(i = 7; i > 0; --i){
        t0 = A[ROW + i][COL + 4];
        t1 = A[ROW + i][COL + 5];
        t2 = A[ROW + i][COL + 6];
        t3 = A[ROW + i][COL + 7];
        B[COL + 4][ROW + i] = t0;
        B[COL + 5][ROW + i] = t1;
        B[COL + 6][ROW + i] = t2;
        B[COL + 7][ROW + i] = t3;
    }

    // 미리 저장해둔 오른쪽 위 4개의 값을 transpose한다.
    B[COL + 4][ROW] = r0;
    B[COL + 5][ROW] = r1;
    B[COL + 6][ROW] = r2;
    B[COL + 7][ROW] = r3;
}
/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

