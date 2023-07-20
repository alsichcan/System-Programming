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

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*********************************************************
 * Basic constants and macros for manipulating the free list.
 * From Textbook Chapter 9 p.830
 ********************************************************/
/* Basic constants and macros */
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// Static Variable & Functions
static char* heap_listp;
static char* prev_bp;
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0); /* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); /* Epilogue header */
    heap_listp += (2*WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;

    prev_bp = (char *)heap_listp;
    return 0;
}

/* 
 * extend_heap : Extends the heap with a new free block
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}



/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize; /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;
    
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;
    
    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        prev_bp = bp;
        return bp;
    }
    
    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    prev_bp = bp;
    return bp;
}

/* 
 * find_fit - Perform next_fit search of the implicit free list.
 */
static void *find_fit(size_t asize)
{
    char *bp = prev_bp;

    // Serach from the next block of previously serached block, and allocate if found
    for (bp = NEXT_BLKP(bp); GET_SIZE(HDRP(bp)) != 0; bp = NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            return bp;
        }
    }

    // If not found before, serach from the heap_listp whether there is any newly freed block bigger than asize
    bp = heap_listp;
    while(bp < prev_bp){
        bp = NEXT_BLKP(bp);
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            return bp;
        }
    }

    return NULL;
}

/* 
 * place : Place the requested block at the beginning of the free block,
 *         splitting only if the size of the remainder would equal or 
 *         exceed the minimum block size.
 */
static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));

    if((csize - asize) >= (2*DSIZE)){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
    }
    else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * coalesce -  merges adjacent free blocks using the boundary-tags coalescing technique 
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    /* Case 1 : Prev, Next block allocated */
    if (prev_alloc && next_alloc) { 
        prev_bp = bp;
        return bp;
    }
    /* Case 2 : Prev allocated, Next block unallocated */
    else if (prev_alloc && !next_alloc) { 
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }
    /* Case 3 : Prev unallocated, Next block allocated */
    else if (!prev_alloc && next_alloc) { 
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    /* Case 4 : Prev, Next unallocated */
    else { 
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
        GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    prev_bp = bp;
    return bp;
}

/*
 * mm_realloc - Reallocate memory depending on the old_size and re_size
 */
void *mm_realloc(void *bp, size_t size)
{
    size_t old_size = GET_SIZE(HDRP(bp));
    size_t re_size = size + 2 * WSIZE;

    // 기존보다 메모리가 같거나 작아지는 경우 - header / footer 조정 후 return
    if(re_size <= old_size){
        // PUT(HDRP(bp), PACK(re_size, 1));
        // PUT(FTRP(bp), PACK(re_size, 1));

        // // realloc으로 인해 새롭게 freed가 될 block 처리
        // void* freed_bp = NEXT_BLKP(bp);
        // size_t freed_size = old_size - re_size;
        // PUT(HDRP(freed_bp), PACK(freed_size, 0));
        // PUT(FTRP(freed_bp), PACK(freed_size, 0));
        
        // // freed_bp 뒤에 free block이 있을 경우를 대비해 coalesce
        // coalesce(freed_bp); 
        
        return bp;
    }
    // 기존보다 더 큰 메모리 할당이 필요한 경우
    else{
        // Case 1. 현재 bp 뒤에 더 필요한 메모리만큼의 free block이 존재할 경우
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
        size_t add_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        if(!next_alloc && (re_size <= (old_size + add_size))){
            PUT(HDRP(bp), PACK(old_size + add_size, 1));
            PUT(FTRP(bp), PACK(old_size + add_size, 1));

            // // realloc으로 인해 새롭게 freed가 될 block 처리
            // void* freed_bp = NEXT_BLKP(bp);
            // size_t freed_size = old_size + add_size - re_size;
            // PUT(HDRP(freed_bp), PACK(freed_size, 0));
            // PUT(FTRP(freed_bp), PACK(freed_size, 0));
            // coalesce(freed_bp);

            return bp;
        }else{
        // Case 2. 현재 bp 뒤에 더 필요한 메모리만큼의 free block이 존재하지 않을 경우
            void *new_bp = mm_malloc(re_size);
            memcpy(new_bp, bp, re_size);
            mm_free(bp);
            return new_bp;
        }
    }
}
