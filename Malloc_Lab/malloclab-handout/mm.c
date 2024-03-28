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

/* 
 * rounds up to the nearest multiple of ALIGNMENT
 * 工作原理是通过将数&~0x7(二进制为1111....1000)，则数二进制下最后 3 位的值被清除，
 * 数必定是8（二进制下位1000）的倍数. 这个操作一般是向下取8的倍数
*/
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

//int mm_check(void);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
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













