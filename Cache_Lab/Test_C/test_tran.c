#include <stdio.h>
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

void printfMatrix(int M, int N, int A[N][M]) {
    int i, j;
    for (i = 0; i < N; i++){
        for (j = 0; j < M; j++){
            printf("%d ",A[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void printfSmallMatrix(int M, int N, int A[N][M]) {
    int i, j;
    for (i = 0; i < 8; i++){
        for (j = 0; j < 8; j++){
            printf("%d ",A[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}


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
            if (i==0 && j==0)
                printfSmallMatrix(M,N,B);
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
                
                //A 1x4 in A left bottom 
                k = i+4+cnt;
                tmp_5 = A[k][j];
                tmp_6 = A[k][j+1];
                tmp_7 = A[k][j+2];
                tmp_8 = A[k][j+3];

                //first put (A 1x4 in A left bottom) into (B 1x4 in B right top)
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
            if (i==0 && j==0)
                printfSmallMatrix(M,N,B);
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
            if (i==0 && j==0)
                printfSmallMatrix(M,N,B);
            /* Fourth
            * I will transpose 0,8,16,24 set with validBits=0 and bias=4Byte
            * miss=4
            */
            for (tmp_1 = 0; tmp_1 < 4; tmp_1++){
                for (tmp_2 = 0; tmp_2 < tmp_1; tmp_2++){
                    tmp_3 = B[j+tmp_1][i+4+tmp_2];
                    B[j+tmp_1][i+4+tmp_2] = B[j+tmp_2][i+4+tmp_1];
                    B[j+tmp_2][i+4+tmp_1] = tmp_3;
                }
            }
            if (i==0 && j==0)
                printfSmallMatrix(M,N,B);
        }
    }
}
/*  if (i==0 && j==0)
                printfSmallMatrix(M,N,B);
*/

// 这是4*4的优化版本，具体思想是，尽量多地利用整个cache
void solve_64(int M, int N, int A[N][M], int B[M][N])
{
    int i, j,yi,yj;

    for (i = 0; i < N; i += 8) {      // 枚举每八行
        for (j = 0; j < M; j += 8) {  // 枚举每八列
            // 这里用这些临时变量，如果你查看过A和B的地址的话，你会发现A和B的地址差距是64的整数倍（0x40000），
            // 那么 直接赋值的话，在对角线的时候 每一个Load A[i][i]紧跟Store B[i][i],将造成比较多的
            // eviction
            int temp1, temp2, temp3, temp4, temp5, temp6, temp7, temp8, cnt;
            // 1块8*8，我们分成4块来做，每一块4*4
            // 这是 左上，且将左下的块移动到 B中的右上，这样是为了更高效地利用cache
            yi=i;
            yj=j;
            for (cnt = 0; cnt < 4; ++cnt, ++i) {  // 枚举0~8中的每一行，一行八列
                temp1 = A[i][j];  // 这样我们就一次取出了8个元素，我们的A的miss就只有一次了 原始4*4
                                  // 则是两次
                temp2 = A[i][j + 1];
                temp3 = A[i][j + 2];
                temp4 = A[i][j + 3];  // 左上

                temp5 = A[i][j + 4];  // 右上
                temp6 = A[i][j + 5];
                temp7 = A[i][j + 6];
                temp8 = A[i][j + 7];

                B[j][i] = temp1;  // 左上翻转
                B[j + 1][i] = temp2;
                B[j + 2][i] = temp3;
                B[j + 3][i] = temp4;

                B[j][i + 4] = temp5;  //将A中右上 存到B中的右上，这也是全部命中的
                B[j + 1][i + 4] = temp6;
                B[j + 2][i + 4] = temp7;
                B[j + 3][i + 4] = temp8;
            }
            i -= 4;
            if (yi==0 && yj==0)
                printfSmallMatrix(M,N,B);
            // 处理A中左下
            for (cnt = 0; cnt < 4; ++j, ++cnt) {  // 枚举0~8中的每一行，一行八列
                temp1 = A[i + 4][j];
                temp2 = A[i + 5][j];
                temp3 = A[i + 6][j];
                temp4 = A[i + 7][j];  // 拿到左下的元素

                // 因为我们这里本来就要处理 右上，所以这不会带来更多的miss
                temp5 = B[j][i + 4];  // 拿到我们之前赋给B右上的，也就是A右上的元素
                temp6 = B[j][i + 5];
                temp7 = B[j][i + 6];
                temp8 = B[j][i + 7];

                B[j][i + 4] = temp1;  //将左下翻转到B右上
                B[j][i + 5] = temp2;
                B[j][i + 6] = temp3;
                B[j][i + 7] = temp4;

                // 这一步，也不会带来更多的miss因为 i + 4 <= j + 4 <= i + 7 恒成立
                // 所以每次带来的 eviction 和 store B[j][i +
                // 4]只会带来一次MISS，和原来的操作是一样的

                B[j + 4][i] = temp5;  //将原B右上 赋值到 B左下， 这样A右上也就完成了翻转
                B[j + 4][i + 1] = temp6;
                B[j + 4][i + 2] = temp7;
                B[j + 4][i + 3] = temp8;
            }
            j -= 4;
            j += 4;  // 处理第四块 右下
            if (yi==0 && yj==0)
                printfSmallMatrix(M,N,B);
            for (i += 4, cnt = 0; cnt < 4; ++cnt, ++i) {
                temp1 = A[i][j];  // 第四块没有任何改动， 和原来效果是一样的
                temp2 = A[i][j + 1];
                temp3 = A[i][j + 2];
                temp4 = A[i][j + 3];

                B[j][i] = temp1;
                B[j + 1][i] = temp2;
                B[j + 2][i] = temp3;
                B[j + 3][i] = temp4;
            }
            i -= 8, j -= 4;
            if (yi==0 && yj==0)
                printfSmallMatrix(M,N,B);
        }
    }
}



void randMatrix(int M, int N, int A[N][M]) {
    int i, j;
    srand(time(NULL));
    for (i = 0; i < N; i++){
        for (j = 0; j < M; j++){
            // A[i][j] = i+j;  /* The matrix created this way is symmetric */
            A[i][j]=rand()%10;
        }
    }
}

int main(){
    int N=64,M=64;
    int A[N][M],B[M][N];
    randMatrix(N,M,A);
    printfSmallMatrix(M,N,A);
    solve_64(M,N,A,B);
    //printfMatrix(M,N,A);
    //printfMatrix(M,N,B);
}