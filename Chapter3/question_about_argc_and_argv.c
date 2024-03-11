#include<stdio.h>

int main(int argc,char* argv[]){
    //char *p="1Hello";
    //printf("%s,%x\n",p,*p);
    printf("%d,%s,%x\n",argc,argv[1],*argv[1]);
    return 0;
}