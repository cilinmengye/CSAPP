#include<stdio.h>

void test(int *p){
    p = 2;
}

int main()
{
    int *p = 1;
    test(p);
    printf("%d\n",p);
}