#include <stdio.h>

#define MAX_HEAP (20*(1<<20))  /* 20 MB */

/*查看下分配内存的地址*/
int main(){

    static char *mem_start_brk;
    /* allocate the storage we will use to model the available VM */
    if ((mem_start_brk = (char *)malloc(MAX_HEAP)) == NULL) {
	    fprintf(stderr, "mem_init_vm: malloc error\n");
	    exit(1);
    }
    void *p = malloc();
    void *p2 = malloc(2);
    *(unsigned int *)p2 = 1;
    *(unsigned long *)p = (unsigned long *)mem_start_brk;
    printf("%x\n",(unsigned int *)mem_start_brk);
    printf("%x\n",*(unsigned long *)p);
    printf("%d\n",*(unsigned int *)p2);
    printf("%x,%x\n",(unsigned long)p, (unsigned long)p2);
    
}