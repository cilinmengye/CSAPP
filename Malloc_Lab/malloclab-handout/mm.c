/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
/* Support Routines: in memlib.c
 * You can invoke the following functions in memlib.c:
 * void *mem_sbrk(int incr): If successful, returns a generic pointer to the first byte of 
 *                           the newly allocated heap area, otherwise returns (void *)-1                         
 * void *mem_heap_lo(void): Returns a generic pointer to the first byte in the heap
 * void *mem_heap_hi(void): Returns a generic pointer to the last byte in the heap.
 * size t mem_heapsize(void): Returns the current size of the heap in bytes.
 * size t mem_pagesize(void): Returns the system’s page size in bytes (4K on Linux systems).
 * 
 * Programming Rules:
 * You are not allowed to define any global or static compound data structures 
 * such as arrays, structs, trees, or lists in your mm.c program. 
 * However, you are allowed to declare global scalar variables such as integers, floats, and pointers in mm.c.
 *
 * You should not invoke any memory-management related library calls or system calls. 
 * This excludes the use of malloc, calloc, free, realloc, sbrk, brk or any variants 
 * of these calls in your code
 * 
 * For consistency with the libc malloc package, which returns blocks aligned on 8-byte boundaries,
 * your allocator must always return pointers that are aligned to 8-byte boundaries. 
 * The driver will enforce this requirement for you.
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "K-ON",
    /* First member's full name */
    "cilinmengye",
    /* First member's email address */
    "834430027@qq.com",
    "cilinmengye",
    "834430027lin@gmail.com"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define WSIZE 4
#define DSIZE 8

/* 
 * rounds up to the nearest multiple of ALIGNMENT
 * 工作原理是通过将数&~0x7(二进制为1111....1000)，则数二进制下最后 3 位的值被清除，
 * 数必定是8（二进制下位1000）的倍数. 这个操作一般是向下取8的倍数
*/
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*Define the size of an application for heap expansion*/
#define CHUNKSIZE (1<<12)
/*
 * Access the content in p in the form of unsigned int *, 
 * because the p we get is initially in the form of void *,
 * and later we will need to use the value in p for calculations, so we need to convert it
 * So this is just a macro defined so that we don't keep writing type conversions repeatedly
*/
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (GET(p) = (val))
#define TOCH(p) ((char *)(p))
/*Get the content to fill in the header or footer*/
#define PACK(size, alloc) ((size) | (alloc))
/*传过来的p一般是经过bp处理后得到的指向头部或尾部的指针*/
#define GET_SIZE(p) (GET(p) & (~0x7))
#define GET_ALLOC(p) (GET(p) & (0x1))
/*获取头部指针,bp为当前块有效载荷的指针*/
#define HDRP(bp) (TOCH(bp) - WSIZE)
/*获取尾部指针,bp为当前块有效载荷的指针*/
#define FTRP(bp) (TOCH(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
/*获取上一个块有效载荷的指针,bp为当前块有效载荷的指针*/
#define PREV_BLKP(bp) (TOCH(bp) - GET_SIZE(TOCH(bp) - DSIZE))
/*获取下一个块有效载荷的指针,bp为当前块有效载荷的指针*/
#define NEXT_BLKP(bp) (TOCH(bp) + GET_SIZE(HDRP(bp)))

/*用于获取地址指针指向的内容*/
#define GETP(p) (*(unsigned long *)p)
#define TOVOID(p) ((void *)p)
/*用于存放地址指针指向的内容*/
#define PUTP(p, val) ((GETP(p)) = (unsigned long)(val))
/*获取pred指针*/
#define PREDP(bp) (TOCH(bp))
/*获取succ指针, 一个指针指向的内容占8字节*/
#define SUCCP(bp) (TOCH(bp) + DSIZE)


/*beginning of free list*/
static void *freelt_hd;
/*tail of free list*/
static void *freelt_ft;

//int mm_check(void);

static void printfBlock(void *bp)
{
    printf("空闲列表中的块%lx: SIZE:%d, ALLOC:%d, PRED:%lx, SUCC:%lx\n",(unsigned long)bp,
            GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)), GETP(PREDP(bp)), GETP(SUCCP(bp)));
}
static void freelt_status()
{
    printf("最前后指针为%lx, %lx", (unsigned long)freelt_hd, (unsigned long)freelt_ft);
    if (freelt_hd == NULL || freelt_ft == NULL){
        printf("空闲列表为空\n");
        return ;
    }
    if (freelt_hd == freelt_ft){
        printf("空闲列表有一个块\n");
        printfBlock(freelt_hd);
        return;
    }
    int cnt = 0;
    printf("整个列表为:\n");
    for (void *i = freelt_hd; i != NULL; i = TOVOID(GETP(SUCCP(i)))){
        printf("%d :", cnt);
        printfBlock(i);
        cnt++;
    }
    printf("整个列表结束\n");
}

/*将有效载荷指针bp代表的空闲块插入空闲列表*/
static void insert_freelt(void *bp)
{
    printf("insert_freelt:success\n");
    void *mv_hd;
    void *mv_ft;
    void *insert_ad;
    void *freelt_pred;
    void *freelt_next;
    size_t size;
    printf("在插入空闲列表之前:\n");
    freelt_status();

    /*说明这个时候空闲列表为空*/
    if (freelt_hd == NULL && freelt_ft == NULL){
        freelt_hd = bp;
        PUTP(PREDP(bp), NULL);
        freelt_ft = bp;
        PUTP(SUCCP(bp), NULL);
        return;
    } else if (freelt_hd == NULL){
        /*这个操作根本就没有链接起来呀喂！*/
        freelt_hd = bp;
        PUTP(PREDP(bp), NULL);
        return;
    } else if (freelt_ft == NULL){
        /*这个操作根本就没有链接起来呀喂！*/
        freelt_ft = bp;
        PUTP(SUCCP(bp), NULL);
        return;
    }

    /*否则双向搜索要插入的空闲列表地方*/
    size = GET_SIZE(HDRP(bp));
    mv_hd = freelt_hd;
    mv_ft = freelt_ft;
    insert_ad = NULL;
    printf("insert_freelt2:success\n");
    printf("insert_freelt2:%lx,%lx,%lx\n",(unsigned long)mv_hd, (unsigned long)mv_ft, (unsigned long)bp);
    printf("insert_freelt: min:%d, max:%d, self:%d\n",GET_SIZE(HDRP(mv_hd)), GET_SIZE(HDRP(mv_ft)), size);
    /*首先排除极端情况*/
    /*当前最小块都比当前块大则直接将块插入空闲列表的最前面*/
    if (GET_SIZE(HDRP(mv_hd)) >= size){
        PUTP(SUCCP(bp), freelt_hd);
        PUTP(PREDP(bp), NULL);
        /*bug5:*/
        PUTP(PREDP(freelt_hd), bp);
        freelt_hd = bp;
        printf("insert_freelt 2.1: %lx,%lx,%lx\n",
                (unsigned long)freelt_hd, (unsigned long)freelt_ft, (unsigned long)bp);
        return;
    }
    /*当前最大块都比当前块小则直接将块插入空闲列表的最后面*/
    if (GET_SIZE(HDRP(mv_ft)) <= size){
        PUTP(PREDP(bp), freelt_ft);
        PUTP(SUCCP(bp), NULL);
        /*bug5:*/
        PUTP(SUCCP(freelt_ft), bp);
        freelt_ft = bp;
        printf("insert_freelt 2.2: %lx,%lx,%lx\n",
                (unsigned long)freelt_hd, (unsigned long)freelt_ft, (unsigned long)bp);
        return;
    }
    printf("insert_freelt3:success\n");
    /*上面对特殊情况的判断保证了，bp一定插入在空闲列表的中间部分*/
    for (; mv_hd != NULL && mv_ft != NULL;){
        printf("insert_freelt3.1:%d, %d, %d\n", GET_SIZE(HDRP(mv_hd)), GET_SIZE(HDRP(mv_ft)), size);
        /*说明要插入当前mv_hd的前面*/
        if (GET_SIZE(HDRP(mv_hd)) >= size){
            insert_ad = mv_hd;
            break;
        }
        /*说明要插入当前mv_ft的后面*/
        if (GET_SIZE(HDRP(mv_ft)) <= size){
            insert_ad = mv_ft;
            break;
        }
        mv_hd = TOVOID(GETP(SUCCP(mv_hd)));
        mv_ft = TOVOID(GETP(PREDP(mv_ft)));
        printf("insert_freelt3.2:%lx, %lx\n", (unsigned long)mv_hd, (unsigned long)mv_ft);
    }
    if (insert_ad == mv_hd){
        /*bp插入在freelt_pred和freelt_next中间*/
        freelt_pred = TOVOID(GETP(PREDP(insert_ad)));
        freelt_next = insert_ad;
        PUTP(SUCCP(bp), freelt_next);
        PUTP(PREDP(bp), freelt_pred);
        PUTP(SUCCP(freelt_pred), bp);
        PUTP(PREDP(freelt_next), bp);
    } else if (insert_ad == mv_ft){
        /*bp插入在freelt_pred和freelt_next中间*/
        freelt_pred = insert_ad;
        freelt_next = TOVOID(GETP(SUCCP(insert_ad)));
        PUTP(SUCCP(bp), freelt_next);
        PUTP(PREDP(bp), freelt_pred);
        PUTP(SUCCP(freelt_pred), bp);
        PUTP(PREDP(freelt_next), bp);
    } else {
        printf("inser_free:what?\n");
        exit(1);
    }
}

/*将bp从空闲列表中解开*/
static void unlock_freelt(void *bp)
{
    void *freelt_pred;
    void *freelt_next;

    /*freelt_pred是空闲列表中前一个块的有效载荷指针*/
    freelt_pred = TOVOID(GETP(PREDP(bp)));
    /*freelt_pred是空闲列表中后一个块的有效载荷指针*/
    freelt_next = TOVOID(GETP(SUCCP(bp)));
    printf("unlock_freelt:success!\n");
    printf("unlock_freelt:%lx,%lx\n",(unsigned long)freelt_pred, (unsigned long)freelt_next);
    /*bug2:注意这里这里两个指针是NULL的情况,为了提高空间的利用率，我这里没有给空闲列表分别在最前和最后加上哨兵*/
    if (freelt_pred != NULL)
        PUTP(SUCCP(freelt_pred), freelt_next);
    else 
        /*改动！未解决这里有问题*/
        freelt_hd = freelt_next; //freelt_hd以前所指向的块被作为分配块了，所以要变为NULL，在coalesce函数中会调用insert_freelt给分配回来
    printf("unlock_freelt2:success!\n");
    if (freelt_next != NULL)
        PUTP(PREDP(freelt_next), freelt_pred);
    else 
        freelt_ft = freelt_pred;
}


/*执行合并操作，传入的参数为当前块的有效载荷指针,返回合并后的块有效载荷指针*/
static void *coalesce(void *bp)
{
    printf("coalesce:success!\n");
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    /*获取堆中的前一块的有效载荷地址指针*/
    void *prev_bp = PREV_BLKP(bp);
    /*获取堆中的后一块的有效载荷地址指针*/
    void *next_bp = NEXT_BLKP(bp);

    printf("coalesce2: 当前堆中前后空闲块情况(0空闲)%d,%d\n", prev_alloc, next_alloc);
    printf("coalesce2:%lx,%lx,%lx,%lx\n",(unsigned long)prev_bp, (unsigned long)next_bp,
                                     (unsigned long)freelt_hd, (unsigned long)freelt_ft);
    printf("coalesce2:success!\n");
    if (prev_alloc && next_alloc){
        ;
    } else if (prev_alloc && !next_alloc){
        /*先出来处理列表*/
        /*next_bp这个空闲块需要在空闲列表中解开*/
        unlock_freelt(next_bp);
        /*更改下前部和尾部*/
        printf("合并: 当前块大小%d,后块大小%d\n", size, GET_SIZE(HDRP(next_bp)));
        size += GET_SIZE(HDRP(next_bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(next_bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc){
        /*先出来处理列表*/
        /*prev_bp这个空闲块需要在空闲列表中解开*/
        unlock_freelt(prev_bp);
        printf("coalesce2.3:success!\n");
        /*更改下前部和尾部*/
        printf("合并: 当前块大小%d,前块大小%d\n", size, GET_SIZE(HDRP(prev_bp)));
        size += GET_SIZE(HDRP(prev_bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(prev_bp), PACK(size, 0));
        /*同时合并后的空闲块有效载荷的地址也发生了变化*/
        bp = prev_bp;
    } else if (!prev_alloc && !next_alloc){
        /*prev_bp这个空闲块需要在空闲列表中解开*/
        unlock_freelt(prev_bp);
        /*next_bp这个空闲块需要在空闲列表中解开*/
        unlock_freelt(next_bp);
        /*更改下前部和尾部*/
        size += GET_SIZE(HDRP(prev_bp)) + GET_SIZE(HDRP(next_bp));
        printf("合并: 当前块大小%d, 后块大小%d, 前块大小%d\n", size, GET_SIZE(HDRP(next_bp)), GET_SIZE(HDRP(prev_bp)));
        PUT(HDRP(prev_bp), PACK(size, 0));
        PUT(FTRP(next_bp), PACK(size, 0));
        /*同时合并后的空闲块有效载荷的地址也发生了变化*/
        bp = prev_bp;
    }
    /*最后都要重新更新下空闲列表*/
    insert_freelt(bp);
    printf("合并完后并且更新空闲列表后\n");
    freelt_status();
    return bp;
}

/*参数为申请的字节个数*/
static void *extend_heap(size_t byteNum)
{
    printf("extend_heap:success!\n");
    size_t vaildNum = ALIGN(byteNum);
    printf("extend_heap:总共新申请堆 %d\n", vaildNum);
    void *tp;
    if ((tp = mem_sbrk(vaildNum)) == NULL)
        return NULL;
    /*
     * 新块添加头部和尾部
     * 需要注意的是size_t是long unsigned int 有64位，
     * 但是我们的GET(p)将p强制类型转换为unsigned int 为32位，所以不用担心内容超出
     * PUT(HDRP(tp), PACK(vaildNum, 0));覆盖了原来的结束块
    */
    PUT(HDRP(tp), PACK(vaildNum, 0));
    PUT(FTRP(tp), PACK(vaildNum, 0));
    /*添加新的结束块*/
    /*
     * bug1:PUT(NEXT_BLKP(HDRP(tp)), PACK(0, 1)); but the right way is follow:
     */
    PUT(HDRP(NEXT_BLKP(tp)), PACK(0, 1));
    /*在coalesce中会调用insert_freelt负责更新空闲列表*/
    printf("extend_heap2:success!\n");
    return coalesce(tp);
}

/* 
 * mm_init - initialize the malloc package.
 * 常规操作，将填充块，序言块和结尾块添加上去。
 * 同时还要申请初始的堆空闲块
 * The return value should be -1 if there was a problem in performing the initialization, 0 otherwise.
 */
int mm_init(void)
{
    printf("mm_init:success!\n");
    freelt_hd = NULL;
    freelt_ft = NULL;
    /*first apply for 4 word size*/
    void *tp = mem_sbrk(4*WSIZE);
    if (tp == (void *)-1)
        return -1;
    PUT(tp, 0);
    PUT(tp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(tp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(tp + (3*WSIZE), PACK(0, 1));
    
    printf("mm_init2:success!\n");
    if ((tp = extend_heap(CHUNKSIZE)) == NULL)
        return -1;
    printf("mm_init3:success!\n");
    freelt_status();
    return 0;
}

/*使用首次适配*/
static void *find_fit(size_t size){
    printf("find_fit:success  %lx,%lx\n", (unsigned long)freelt_hd, (unsigned long)freelt_ft);
    void *mv_hd = freelt_hd;
    void *mv_ft = freelt_ft;
    /*bug4: 可能空闲列表为空，这个时候freelt_hd == NULL,freelt_ft == NULL*/
    if (freelt_hd == NULL || freelt_ft == NULL)
        return NULL;
    printf("find_fit:%d %d\n",GET_SIZE(HDRP(mv_hd)), GET_SIZE(HDRP(mv_ft)));
    /*特殊情况*/
    if (size <= GET_SIZE(HDRP(mv_hd))){
        return mv_hd;
    }
    if (size > GET_SIZE(HDRP(mv_ft))){
        return NULL;
    }
    printf("find_fit2:success\n");
    for (;mv_hd != NULL && mv_ft != NULL;){
        if (GET_SIZE(HDRP(mv_hd)) >= size){
            return mv_hd;
        }
        if (GET_SIZE(HDRP(mv_ft)) == size){
            return mv_ft;
        }
        else if (GET_SIZE(HDRP(mv_ft)) < size){
            return TOVOID(GETP(SUCCP(mv_ft)));
        }
        mv_hd = TOVOID(GETP(SUCCP(mv_hd)));
        mv_ft = TOVOID(GETP(PREDP(mv_ft)));
    }
    return NULL;
}

/*将size大小的内容放到bp所指向的空闲块上*/
static void place(void *bp, size_t size)
{
    printf("place:success!\n");
    size_t csize = GET_SIZE(HDRP(bp));
    
    printf("place:success %d, %d\n", csize, size);
    if ((csize - size) >= (4 * DSIZE)){
        /*对隐式链表的操作*/
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        /*对显式链表的操作*/
        /*bp在显式链表中要拆开了*/
        unlock_freelt(bp);
        printf("place:success in first if one !\n");
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - size, 0));
        PUT(FTRP(bp), PACK(csize - size, 0));
        /*然后这个被分割出来的空闲块就要加入空闲列表了,在coalesce中会调用insert_freelt负责更新空闲列表*/
        coalesce(bp);
        printf("place:success in first if two !\n");
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        /*对显式链表的操作*/
        /*bp在显式链表中要拆开了*/
        unlock_freelt(bp);
    }
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
/*
 * 由于我使用了显示空闲链表，一个指针占用8字节，我有两个（pred,succ），同时还要加上头部和尾部（各4字节）. 
 * 所以空闲块的大小最小为24字节+8字节=32字节
 * 但是分配完后两个指针（pred,succ）就不要了，所以这个时候最小块大小为8字节+8字节=16字节
 * 
 * mm_malloc 函数返回一个指向至少 size 字节大小的分配块的指针。
 * 整个分配的块应该位于堆区域内，并且不应与任何其他已分配的块重叠。
 */
void *mm_malloc(size_t size)
{
    printf("\n");
    printf("\n");
    printf("mm_malloc:success!%d\n", size);
    size_t asize;
    size_t extendsize;
    void *bp;

    if (size == 0)
        return NULL;
    if (size < DSIZE)
        asize = DSIZE;
    else 
        asize = ALIGN(size + SIZE_T_SIZE); /*这里还要加上头部和尾部的总共8个字节*/
    printf("mm_malloc2:%d, %d\n", size, asize);
    if ((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        freelt_status();
        return bp;
    }
    extendsize = asize >= CHUNKSIZE ? asize : CHUNKSIZE;
    if ((bp = extend_heap(extendsize)) == NULL)
        return NULL;
    place(bp, asize);
    freelt_status();
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    printf("\n");
    printf("\n");

    size_t size = GET_SIZE(HDRP(ptr));
    printf("mm_free:success! 释放的大小为:%d \n", size);

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    /*在coalesce中会调用insert_freelt负责更新空闲列表*/
    coalesce(ptr);
    printf("mm_free:success2!\n");
    freelt_status();
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    exit(1);
    /*
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
    **/
}

/*
 * It returns a nonzero value if and only if your heap is consistent.
 * Some examples of what a heap checker might check are:
 * • Is every block in the free list marked as free?
 * • Are there any contiguous free blocks that somehow escaped coalescing?
 * • Is every free block actually in the free list?
 * • Do the pointers in the free list point to valid free blocks?
 * • Do any allocated blocks overlap?
 * • Do the pointers in a heap block point to valid heap addresses? 
*/
//int mm_check(void){
//
//} 













