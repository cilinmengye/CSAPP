#include<stdio.h>
#include<stdlib.h>

#define ALIGNMENT 8
#define WSIZE 4
#define DSIZE 8
#define TOCH(p) ((char *)(p))
/*用于获取地址指针指向的内容*/
#define GETP(p) (*(unsigned long *)p)
#define TOVOID(p) ((void *)p)
/*用于存放地址指针指向的内容*/
#define PUTP(p, val) ((GETP(p)) = (unsigned long)(val))
/*获取pred指针*/
#define PREDP(bp) (TOCH(bp))
/*获取succ指针, 一个指针指向的内容占8字节*/
#define SUCCP(bp) (TOCH(bp) + DSIZE)

int main()
{
    void *p1 = malloc(DSIZE);
    void *p2 = malloc(DSIZE*2);
    void *p3 = malloc(DSIZE*3);
    void *bp = malloc(2*DSIZE);
    void *p = p2;
    *(int *)p2 = 123;
    PUTP(PREDP(bp), p);
    PUTP(SUCCP(bp), NULL);
    printf("%lx,%lx,%lx\n",(unsigned long) p2, GETP(PREDP(bp)),GETP(SUCCP(bp)));
    printf("%ld\n",GETP(GETP(PREDP(bp))));
}