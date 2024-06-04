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
    if(N == 32) {
        int i, j;
        for (j = 0; j < M; j+=8) {
            for (i = 0; i < N; i++) {
                if(i/8 == j/8) {
                    int a = A[i][j];
                    int b = A[i][j+1];
                    int c = A[i][j+2];
                    int d = A[i][j+3];
                    int e = A[i][j+4];
                    int f = A[i][j+5];
                    int g = A[i][j+6];
                    int h = A[i][j+7];

                    B[j][i]   = a;
                    B[j+1][i] = b;
                    B[j+2][i] = c;
                    B[j+3][i] = d;
                    B[j+4][i] = e;
                    B[j+5][i] = f;
                    B[j+6][i] = g;
                    B[j+7][i] = h;
                } else {
                    for(int k = 0; k < 8; ++k) {
                        B[j+k][i] = A[i][j+k];
                    }
                }
            }
        }
    } else if(N == 64) {
        int i, j;
        int a, b, c, d, e, f, g, h;
        for (j = 0; j < M; j+=8) {
            for (i = 0; i < N; i+=8) {
                for(int k = 0; k < 4; ++k) {
                    a = A[i+k][j];
                    b = A[i+k][j+1];
                    c = A[i+k][j+2];
                    d = A[i+k][j+3];
                    e = A[i+k][j+4];
                    f = A[i+k][j+5];
                    g = A[i+k][j+6];
                    h = A[i+k][j+7];

                    B[j]  [i+k]   = a;
                    B[j+1][i+k]   = b;
                    B[j+2][i+k]   = c;
                    B[j+3][i+k]   = d;
                    B[j]  [i+k+4] = e;
                    B[j+1][i+k+4] = f;
                    B[j+2][i+k+4] = g;
                    B[j+3][i+k+4] = h;
                }

                for(int k = 0; k < 4; ++k) {
                    a = A[i+4][j+k];
                    b = A[i+5][j+k];
                    c = A[i+6][j+k];
                    d = A[i+7][j+k];

                    e = B[j+k][i+4];
                    f = B[j+k][i+5];
                    g = B[j+k][i+6];
                    h = B[j+k][i+7];

                    B[j+k][i+4] = a;
                    B[j+k][i+5] = b;
                    B[j+k][i+6] = c;
                    B[j+k][i+7] = d;

                    B[j+k+4][i]   = e;
                    B[j+k+4][i+1] = f;
                    B[j+k+4][i+2] = g;
                    B[j+k+4][i+3] = h;
                }

                for(int k =0; k < 4; k+=2) {
                    a = A[i+4+k][j+4];
                    b = A[i+4+k][j+5];
                    c = A[i+4+k][j+6];
                    d = A[i+4+k][j+7];
                    e = A[i+5+k][j+4];
                    f = A[i+5+k][j+5];
                    g = A[i+5+k][j+6];
                    h = A[i+5+k][j+7];

                    B[j+4][i+4+k] = a;
                    B[j+5][i+4+k] = b;
                    B[j+6][i+4+k] = c;
                    B[j+7][i+4+k] = d;
                    B[j+4][i+5+k] = e;
                    B[j+5][i+5+k] = f;
                    B[j+6][i+5+k] = g;
                    B[j+7][i+5+k] = h;
                }
            }
        }
    } else {
        #define BLOCK_SIZE 17
        for (int i = 0; i < N; i += BLOCK_SIZE) {
            for (int j = 0; j < M; j += BLOCK_SIZE) {
                for(int k = i; k < N && k < i + BLOCK_SIZE ; ++k) {
                    for(int l = j; l < M && l < j + BLOCK_SIZE ; ++l) {
                        B[l][k] = A[k][l];
                    }
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

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
    // registerTransFunction(trans, trans_desc); 

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

