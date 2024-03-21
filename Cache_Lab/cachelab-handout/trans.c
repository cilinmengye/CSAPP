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

void transpose_submit_32x32(int M, int N, int A[N][M], int B[M][N]){
    int i,j,k,cnt,tmp_1,tmp_2,tmp_3,tmp_4,tmp_5,tmp_6,tmp_7,tmp_8;
    
    for (i = 0; i < 32; i += 8){
        for (j = 0; j < 32; j += 8){
            /*don't make B[j][i]=A[i][j] but use tmp is to deal with an condition
             *which such as B[0][0]=A[0][0],B[1][0]=A[0][1].....
             *when write A[0][0] to B[0][0],cache about A is evcited by B
             *and then I want get A[0][1],A[0][2]...e.t. I need evcited cache about B
            */
           for (cnt = 0; cnt < 8; cnt++){
                k = i + cnt;
                tmp_1 = A[k][j];    tmp_5 = A[k][j+4];
                tmp_2 = A[k][j+1];  tmp_6 = A[k][j+5];
                tmp_3 = A[k][j+2];  tmp_7 = A[k][j+6];
                tmp_4 = A[k][j+3];  tmp_8 = A[k][j+7];

                B[j][k] = tmp_1;    B[j+4][k] = tmp_5;
                B[j+1][k] = tmp_2;  B[j+5][k] = tmp_6;
                B[j+2][k] = tmp_3;  B[j+6][k] = tmp_7;
                B[j+3][k] = tmp_4;  B[j+7][k] = tmp_8;
           }
        }
    }
}


/*64x64 Full score version*/
void transpose_submit_64x64(int M, int N, int A[N][M], int B[M][N]){
    int i,j,cnt,k,tmp_1,tmp_2,tmp_3,tmp_4,tmp_5,tmp_6,tmp_7,tmp_8;
    
    for (i = 0; i < 64; i += 8){
        for (j = 0; j < 64; j += 8){
            /*First:
            * now cache have B in 0,8,16,24 set and validBits is 0(I assume, not true)
            * have A in 1,9,17,25 set, validBits is 0(I assume, not true)
            * miss=4+4=8 in all
            */
            for (cnt = 0; cnt < 4; cnt++){
                k = i+cnt;
                tmp_1 = A[k][j];    tmp_5 = A[k][j+4];
                tmp_2 = A[k][j+1];  tmp_6 = A[k][j+5];
                tmp_3 = A[k][j+2];  tmp_7 = A[k][j+6];
                tmp_4 = A[k][j+3];  tmp_8 = A[k][j+7];

                B[j][k] = tmp_1;    B[j][k+4] = tmp_5;
                B[j+1][k] = tmp_2;  B[j+1][k+4] = tmp_6;
                B[j+2][k] = tmp_3;  B[j+2][k+4] = tmp_7;
                B[j+3][k] = tmp_4;  B[j+3][k+4] = tmp_8;
            }
            /*Second:
            * now cache have B in 0,8,16,24 set and and validBits=0
            * but when I put B 0,8,16,24 set with validBits=0 and bias=4Byte into 
            * B 0,8,16,24 set with validBits=1
            * so B in 0,8,16,24 set and validBits=0 will become more and more less; 
            * This process to B have 4 miss in all 
            * At sametime , I will put A 1,9,17,25 set with validBits=1 and bias=0Byte into
            * B 0,8,16,24 set with validBits=0 and bias=4Byte
            * Last cache have B in 0,8,16,24 set and validBits=1
            * have A in 1,9,17,25 set, validBits=1
            * miss=4+4=8 in all
            */
            for (cnt = 0; cnt < 4; cnt++){
                //B 1x4 in B right top
                k = j+cnt;
                tmp_1 = B[k][i+4]; 
                tmp_2 = B[k][i+5]; 
                tmp_3 = B[k][i+6]; 
                tmp_4 = B[k][i+7]; 
                
                //A 1x4 in A left bottom (Upright)
                k= j+cnt;
                tmp_5 = A[i+4][k];
                tmp_6 = A[i+5][k];
                tmp_7 = A[i+6][k];
                tmp_8 = A[i+7][k];

                //first put (A 1x4 in A left bottom(Upright) ) into (B 1x4 in B right top)
                k = j+cnt;
                B[k][i+4] = tmp_5;
                B[k][i+5] = tmp_6;
                B[k][i+6] = tmp_7;
                B[k][i+7] = tmp_8;

                //then put (B 1x4 in B right top) into (B 1x4 in B left bottom)
                k = j+4+cnt;
                B[k][i] = tmp_1;
                B[k][i+1] = tmp_2;
                B[k][i+2] = tmp_3;
                B[k][i+3] = tmp_4;
            }
            /*Third
            * I will put A in 1,9,17,25 set with validBits=1 and bias=4Byte into
            * B B in 0,8,16,24 set and validBits=1 with bias=4Byte 
            * miss=0 in all
            */
            for (cnt = 0 ; cnt < 4; cnt++){
                k = i+4+cnt;
                tmp_1 = A[k][j+4];
                tmp_2 = A[k][j+5];
                tmp_3 = A[k][j+6];
                tmp_4 = A[k][j+7];

                B[j+4][k] = tmp_1;
                B[j+5][k] = tmp_2;
                B[j+6][k] = tmp_3;
                B[j+7][k] = tmp_4;
            }
        }
    }
}

void transpose_submit_61x67(int M, int N, int A[N][M], int B[M][N]){
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (N == M && N == 32){
        transpose_submit_32x32(M, N, A, B);
    }
    else if (N == M && N == 64){
        transpose_submit_64x64(M, N, A, B);
    }
    else {
        transpose_submit_61x67(M, N, A, B);
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

