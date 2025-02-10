#include<stdio.h>
#include<stdlib.h>
int main()
{
    int *arr = (int *)malloc(sizeof(int) * 5);
    for (int i = 0; i <= 4; i++) arr[i] = i;
    for (int i = 0; i <= 4; i++) printf("%d\n", arr[i]);
    return 0;
}