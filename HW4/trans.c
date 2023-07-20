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
    // s = 5, E = 1, b = 5 --> 32bytes per cache line
    // 32 bytes per cache line == 8 integers per cache line

	// Total 11 Variables for Transpose (Limit: 12)
	int i, j, k = 0;
	int x1, x2, x3, x4, x5, x6, x7, x8 = 0;

    // Case 1 : M = 32, N = 32
    // 1 ROW = 32 integers --> 4 cache lines
    // Cache can contain 8 rows
    // Use Block Size of 8 
    if(M == 32 && N == 32){
		for(k = 0; k < N; k += 8){
			for(j = 0; j < M; j += 8){
				for(i = 0; i < 8; i++){
					x1 = A[k+i][j];
					x2 = A[k+i][j+1];
					x3 = A[k+i][j+2];
					x4 = A[k+i][j+3];
					x5 = A[k+i][j+4];
					x6 = A[k+i][j+5];
					x7 = A[k+i][j+6];
					x8 = A[k+i][j+7];
					B[j][k+i] = x1;
					B[j+1][k+i] = x2;
					B[j+2][k+i] = x3;
					B[j+3][k+i] = x4;
					B[j+4][k+i] = x5;
					B[j+5][k+i] = x6;
					B[j+6][k+i] = x7;
					B[j+7][k+i] = x8;
				}
			}
		}
    }

    // Case 2 : M = 64, N = 64
    // 1 Row = 64 integers --> 8 cache lines
    // Cache can only contain 4 rows --> Conflict
    // Use Block Size of 4
    else if(M == 64 && N == 64){
		for(k = 0; k < N; k += 4){
			for(j = 0; j < M; j += 4){
				for(i = 0; i < 4; i++){
					x1 = A[k+i][j];
					x2 = A[k+i][j+1];
					x3 = A[k+i][j+2];
					x4 = A[k+i][j+3];
					B[j][k+i] = x1;
					B[j+1][k+i] = x2;
					B[j+2][k+i] = x3;
					B[j+3][k+i] = x4;
				}
			}
		}	
	}
    // Case 3 : M = 61, N = 67
    // 4의 배수인 60 * 60 + 1 * 67 + 7 * 60
    else if(M == 61 && N == 67){
		// 60 * 60
		for(k = 0; k < 60; k += 4){
			for(j = 0; j < 60; j += 4){
				for(i = 0; i < 4; i++){
					x1 = A[k+i][j];
					x2 = A[k+i][j+1];
					x3 = A[k+i][j+2];
					x4 = A[k+i][j+3];

					B[j][k+i] = x1;
					B[j+1][k+i] = x2;
					B[j+2][k+i] = x3;
					B[j+3][k+i] = x4;
				}
			}
		}
		// 1 * 67
		for(i = 0; i < 67; i++){
			x1 = A[i][60];
			B[60][i] = x1;
		}
		// 7 * 60
		for(j = 0; j < 60; j += 4){
			for(i = 0; i < 7; i++){
				x1 = A[60+i][j];
				x2 = A[60+i][j+1];
				x3 = A[60+i][j+2];
				x4 = A[60+i][j+3];
				
				B[j][60+i] = x1;
				B[j+1][60+i] = x2;
				B[j+2][60+i] = x3;
				B[j+3][60+i] = x4;
			}
		}
    }

    else{
        return;
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

