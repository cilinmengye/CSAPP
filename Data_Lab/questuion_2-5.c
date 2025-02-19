#include <stdio.h>
#include <string.h>

typedef unsigned char *byte_pointer;

void show_bytes(byte_pointer start,size_t len){
    size_t i;
    for (i=0;i<len;i++){
        printf("%.2x",start[i]);
    }
    printf("\n");
}

void show_int(int x){
    show_bytes((byte_pointer)&x,sizeof(int));
}

int main()
{
    int val=0x87654321;
    byte_pointer valp=(byte_pointer) &val;
    show_bytes(valp,1);
    const char *s="abcdef";
    show_bytes((byte_pointer)s,strlen(s));
    unsigned ulength=0;
    int length=0;
    printf("%x,%x\n",ulength-1,length-1);
    return 0;
}
