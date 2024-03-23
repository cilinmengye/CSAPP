#include<stdio.h>

int main(int argc, char *argv[], char *envp[]){
    int i;

    printf("Environment variables:\n");

    for (i=0; envp[i]!=0; i++){
        printf("envp[%2d]: %s\n",i,envp[i]);
    }

    return 0;
}