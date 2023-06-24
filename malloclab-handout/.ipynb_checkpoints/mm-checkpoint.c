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
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* some useful sizes */
#define WSIZE 4
#define DSIZE 8
#define MINBLOCK 24
#define CHUNKSIZE (1<<12)

/* get maximum value of x and y */
#define MAX(x, y) ((x)>(y)? (x):(y))

/* pack up size and whether allocated */
#define PACK(size, alloc) ((size)|(alloc))

/* get or put value or address(pointer) to an address */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define GETADDR(p)         (*(unsigned int **)(p))
#define PUTADDR(p, addr)   (*(unsigned int **)(p) = (unsigned int *)(addr))

/* get the size of the block */
#define GET_SIZE(p) (GET(p) & ~0x7)

/* whether the block is allocated */
#define GET_ALLOC(p) (GET(p) & 0x1)

/* calculate address of header or footer of block pointed by bp */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* pointer to the memory where stores next/prev pointer */
#define NEXT_PTR(bp) ((char *)(bp))
#define PREV_PTR(bp) ((char *)(bp) + WSIZE)

/* pointer to logical next or prev block */
#define NEXT_FREE_BLKP(bp) ((char *)(*(unsigned int *)(NEXT_PTR(bp))))
#define PREV_FREE_BLKP(bp) ((char *)(*(unsigned int *)(PREV_PTR(bp))))

/* pointer to physical next or prev block */ 
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(HDRP(bp)-WSIZE))

static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void place(void *bp, size_t size);
static void *find_free_block(size_t size);
static void place_freelist(void *bp);
static void remove_freelist(void *bp);
static void insert_freelist(void *bp);

static void *freelist_headp;
static void *init_p;

static void *extend_heap(size_t size){
    size_t asize;
    void *bp;
    
    asize = ALIGN(size);
    
    if((long)(bp = mem_sbrk(asize)) == -1){
        return NULL;
    }
    
    PUT(HDRP(bp), PACK(asize, 0));
    PUT(FTRP(bp), PACK(asize, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    
    return coalesce(bp);
}

static void place_freelist(void *bp){
    void *prev_block = PREV_FREE_BLKP(bp);
    void *next_block = NEXT_FREE_BLKP(bp);
    void *remain_bp = NEXT_BLKP(bp);
    
    PUTADDR(NEXT_PTR(remain_bp), next_block);
    PUTADDR(PREV_PTR(remain_bp), prev_block);
    
    if(prev_block == freelist_headp){
        PUTADDR(freelist_headp, remain_bp);
    }
    else{
        PUTADDR(NEXT_PTR(prev_block), remain_bp);
    }
    
    if(next_block !=NULL){
        PUTADDR(PREV_PTR(next_block), remain_bp);
    }
}
    
static void remove_freelist(void *bp){
    void *prev_block = PREV_FREE_BLKP(bp);
    void *next_block = NEXT_FREE_BLKP(bp);
    
    if(prev_block == freelist_headp){
        PUTADDR(freelist_headp, next_block);
    }
    else{
        PUTADDR(NEXT_PTR(prev_block), next_block);
    }
    
    if(next_block != NULL){
        PUTADDR(PREV_PTR(next_block), prev_block);
    }
}

static void insert_freelist(void *bp){
    if(GETADDR(freelist_headp) == NULL){
        PUTADDR(NEXT_PTR(bp), NULL);
        PUTADDR(PREV_PTR(bp), freelist_headp);
        PUTADDR(freelist_headp, bp);
    }
    else{
        void *tmp=NULL;
        tmp=GETADDR(freelist_headp);
        PUTADDR(NEXT_PTR(bp), tmp);
        PUTADDR(PREV_PTR(bp), freelist_headp);
        PUTADDR(freelist_headp, bp);
        PUTADDR(PREV_PTR(tmp), bp);
    }
}

static void *find_free_block(size_t size){
    void *current_p;
    for(current_p=GETADDR(freelist_headp);current_p!=NULL;current_p=NEXT_FREE_BLKP(current_p)){
        if(GET_SIZE(HDRP(current_p)) >= size){
            return current_p;
        }
    }
    return NULL;
}

static void place(void *bp, size_t size){
    size_t block_size;
    size_t remain_size;
    block_size = GET_SIZE(HDRP(bp));
    remain_size = block_size - size;
    
    if(remain_size >= MINBLOCK){
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        void *remain_bp;
        remain_bp = NEXT_BLKP(bp);
        PUT(HDRP(remain_bp), PACK(remain_size, 0));
        PUT(FTRP(remain_bp), PACK(remain_size, 0));
        place_freelist(bp);
    }
    else{
        PUT(HDRP(bp), PACK(block_size, 1));
        PUT(FTRP(bp), PACK(block_size, 1));
        remove_freelist(bp);
    }
}

static void *coalesce(void *bp){
    void *prev_block = PREV_BLKP(bp);
    void *next_block = NEXT_BLKP(bp);
    int prev_alloc = GET_ALLOC(HDRP(prev_block));
    int next_alloc = GET_ALLOC(HDRP(next_block));
    size_t size = GET_SIZE(HDRP(bp));
    
    if(prev_alloc && next_alloc){
        insert_freelist(bp);
        return bp;
    }
    else if(prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(next_block));
        remove_freelist(next_block);
        insert_freelist(bp);
    }
    else if(!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(prev_block));
        bp = prev_block;
        remove_freelist(bp);
        insert_freelist(bp);
    }
    else{
        size += GET_SIZE(HDRP(next_block)) + GET_SIZE(HDRP(prev_block));
        bp = prev_block;
        remove_freelist(prev_block);
        remove_freelist(next_block);
        insert_freelist(bp);
    }
    
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    return bp;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	if((init_p = mem_sbrk(4*WSIZE)) == (void *)-1){
        return -1;
    }
    PUTADDR(init_p, NULL);
    PUT(init_p + WSIZE, PACK(8, 1));
    PUT(init_p + 2*WSIZE, PACK(8, 1));
    PUT(init_p, PACK(0, 1));
    freelist_headp = init_p;
    PUTADDR(freelist_headp, NULL);
    
    if(extend_heap(CHUNKSIZE) == NULL){
        return -1;
    }
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extend_size;
    void *bp = NULL;
    
    if (size == 0){
        return NULL;
    }
    
    asize = ALIGN(size+2*WSIZE);
    
    if((bp = find_free_block(asize)) != NULL){
        place(bp, asize);
        return bp;
    }
    
    extend_size = MAX(CHUNKSIZE, asize);
    if((bp = extend_heap(extend_size)) == NULL){
        return NULL;
    }
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size;
    size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    size_t old_size = GET_SIZE(HDRP(ptr));
    size_t new_size = ALIGN(size + 2*WSIZE);
    size_t extend_size;
    void *old_ptr = ptr;
    void *new_ptr;
    
    if(ptr == NULL){
        return mm_malloc(size);
    }
    
    if(size == 0){
        mm_free(ptr);
        return NULL;
    }
    
    if(new_size <= old_size){
        if(old_size - new_size >= MINBLOCK){
            place(old_ptr, new_size);
            return old_ptr;
        }
        else{
            return old_ptr;
        }
    }
    else{
        if((new_ptr = find_free_block(new_size)) == NULL){
            extend_size = MAX(CHUNKSIZE, new_size);
            if((new_ptr = extend_heap(extend_size)) == NULL){
                return NULL;
            }
        }
    }
    place(new_ptr, new_size);
    memcpy(new_ptr, old_ptr, old_size-2*WSIZE);
    mm_free(old_ptr);
    return new_ptr;
}
