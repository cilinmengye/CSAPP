#include <stdio.h>

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
    return 0;
}
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