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
#define GETCH(p) ((char *)(p))
/*Get the content to fill in the header or footer*/
#define PACK(size, alloc) ((size) | (alloc))
/*传过来的p一般是经过bp处理后得到的指向头部或尾部的指针*/
#define GET_SIZE(p) (GET(p) & (~0x7))
#define GET_ALLOC(p) (GET(p) & (0x1))
/*获取头部指针,bp为当前块有效载荷的指针*/
#define HDRP(bp) (GETCH(bp) - WSIZE)
/*获取尾部指针,bp为当前块有效载荷的指针*/
#define FTRP(bp) (GETCH(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
/*获取上一个块有效载荷的指针,bp为当前块有效载荷的指针*/
#define PREV_BLKP(bp) (GETCH(bp) - GET_SIZE(GETCH(bp) - DSIZE))
/*获取下一个块有效载荷的指针,bp为当前块有效载荷的指针*/
#define NEXT_BLKP(bp) (GETCH(bp) + GET_SIZE(HDRP(bp)))

/*用于获取地址指针指向的内容*/
#define GETP(p) (*(unsigned long *)p)
/*获取pred指针*/
#define PREDP(bp) (GETCH(bp))
/*获取succ指针, 一个指针指向的内容占8字节*/
#define SUCCP(bp) (GETCH(bp) + DSIZE)
/*用于存放地址指针指向的内容*/
#define PUTP(p, val) ((GETP(p)) = (unsigned long *)(val))



/*beginning of free list*/
static void *bk_listhd;
/*tail of free list*/
static void *bk_listft;

//int mm_check(void);

/*执行合并操作，传入的参数为当前块的有效载荷指针,返回合并后的块有效载荷指针*/
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    void *prev_bp = PREV_BLKP(bp);
    void *next_bp = NEXT_BLKP(bp);
    void *tprev;
    void *tnext;
    

    if (prev_alloc && next_alloc){
        return bp;
    } else if (prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        /*先出来处理列表*/
        /*next_bp这个空闲块需要在空闲列表中解开*/
        /*tprev是空闲列表中前一个块的有效载荷指针*/
        tprev = GETP(PREDP(next_bp));
        /*tnext是空闲列表中后一个块的有效载荷指针*/
        tnext = GETP(SUCCP(next_bp));
        /*空闲列表中前一个块的SUCC指针其中指向的内容为tnext,即下一个块的有效载荷地址*/
        PUTP(SUCCP(tprev), tnext);
        PUTP(PREDP(tnext), tprev);
        /*更改下前部和尾部*/
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    /*最后都要重新更新下空闲列表*/

}

/*参数为申请的字节个数*/
static void *extend_heap(size_t byteNum)
{
    size_t vaildNum = ALIGN(byteNum);
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
    PUT(NEXT_BLKP(HDRP(tp)), PACK(0, 1));
    /*直接放到空闲列表的最后*/
    /*让前向指针指向原来bk_listft所指向的地址*/
    PUTP(PRED(tp), bk_listft);
    /*后向指针指向NULL*/
    PUTP(SUCC(tp), NULL);
    /*bk_listft指向这个新块的有效载荷*/
    bk_listft = tp;
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
    bk_listhd = NULL;
    bk_listft = NULL;
    /*first apply for 4 word size*/
    void *tp = mem_sbrk(4*WSIZE);
    if (tp == (void *)-1)
        return -1;
    PUT(tp, 0);
    PUT(tp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(tp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(tp + (3*WSIZE), PACK(0, 1));
    
    if ((tp = extend_heap(CHUNKSIZE)) == NULL)
        return -1;
    /*直接让bk_listhd指向这个新块的有效载荷地址*/
    bk_listhd = tp;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
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













