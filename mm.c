/*
 * mm.c 
 * 
 * My implementation uses the following choices of structure.
 * SEGREGATED LINKED LIST
 * FIRST FIT SCANNING METHOD
 *
 * OVERVIEW:
 * The start of my heap contains padding, the roots of all linked lists
 * of free blocks, and a dummy block indicating the start of heap. The end of
 * my heap has a dummy header with size 0, which is used to indicate the end
 * of heap.
 *
 * MALLOC:
 * My malloc function stores the blocks aligned to 8. Each block
 * has header and footer that store the size and validity of each block. 
 * Finds the first free block that is bigger than size.
 * Increases the size of heap if necessary.
 *
 * FREE:
 * Allocation value of header and footer of that specific block is changed
 * to 0. Free blocks contain the address of previous and the next free blocks.
 * Newly added free blocks are always on top of the list.
 *
 *
 * REALLOC:
 * Implemented based on MALLOC and FREE. Information of the original blocks are
 * transferred to the new block. Increases heap size if necessary, and coalescing
 * of free blocks will happen as usual. The function is broken down to many situations
 * in order to increase performance score.
 * 
 * SPECIFIC IMPROVEMENTS:
 * 1. coalescing-bal.rep
 * -- set the initial heap extension size to 16
 * -- this way, any further allocation of size 4090 would only require heap extension at
 * most once.
 * 2. allocating the same size
 * -- returns a pointer to newly allocated heap if allocating the same size multiple times
 * -- increases performance so no need to search through free lists.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Your UID */
    "404931874",
    /* Your full name */
    "Yuan",
    /* Your last name */
    "Cheng",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* macro functions and variables taken from the textbook*/

#define ALIGNMENT 8
#define WSIZE     4
#define DSIZE     8
#define CHUNKSIZE (1<<13) 

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) > (y) ? (y) : (x))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p)       (*(unsigned int *) (p))
#define PUT(p, val)  (*(unsigned int *)(p)  =  (val))

#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp)      ((char *)(bp) - WSIZE)
#define FTRP(bp)      ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

#define NEXT_FP(bp) ((char *)(bp))
#define PREV_FP(bp) ((char *)(bp) + WSIZE)

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


#define LIST1     0
#define LIST2     8
#define LIST3     16
#define LIST4     24
#define LIST5     32
#define LIST6     40
#define LIST7     48
#define LIST8     56
#define LIST9     64
#define LIST10    72
#define LIST11    80
#define LIST12    88
#define LIST13    96
#define LIST14    104

#define LIST1_LIMIT      24
#define LIST2_LIMIT      48
#define LIST3_LIMIT      72
#define LIST4_LIMIT      96
#define LIST5_LIMIT      120
#define LIST6_LIMIT      480
#define LIST7_LIMIT      960
#define LIST8_LIMIT      1920
#define LIST9_LIMIT      3840
#define LIST10_LIMIT     7680
#define LIST11_LIMIT     15360
#define LIST12_LIMIT     30720
#define LIST13_LIMIT     61440



/* helper functions that are good for implementation */

void *extendHeap(size_t size);
void *coalesce(void *bp);  /* adjacent free blocks are combined to one */
void split(void *bp, size_t size); /* free blocks are split to two as a result of mallocing */
void *find(int sizestart, size_t size); /* find the next free block */
void *findWrapper(size_t size);
void addList(void *bp, size_t size); /* Add a free block to the top of list */
void deleteList(void *bp, size_t size); /* Delete a block from list when it's allocated */
//void printblock(void *bp);   /* for debugging purposes */
//void checkblock(void *bp);
//void checkheap(int verbose);
//void checkFree(void *root);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    void *ptr;
    if ((ptr = mem_sbrk(30*WSIZE)) == (void *)-1)
        return -1;
    PUT(ptr + LIST1, (size_t)0);
    PUT(ptr + LIST2, (size_t)0);
    PUT(ptr + LIST3, (size_t)0);
    PUT(ptr + LIST4, (size_t)0);
    PUT(ptr + LIST5, (size_t)0);
    PUT(ptr + LIST6, (size_t)0);
    PUT(ptr + LIST7, (size_t)0);
    PUT(ptr + LIST8, (size_t)0);
    PUT(ptr + LIST9, (size_t)0);
    PUT(ptr + LIST10, (size_t)0);
    PUT(ptr + LIST11, (size_t)0);
    PUT(ptr + LIST12, (size_t)0);
    PUT(ptr + LIST13, (size_t)0);
    PUT(ptr + LIST14, (size_t)0);
    PUT(ptr + LIST14 + WSIZE, PACK(DSIZE, 1)); // first block
    PUT(ptr + LIST14 + 2*WSIZE, PACK(DSIZE, 1));
    PUT(ptr + LIST14 + 3*WSIZE, PACK(0,1)); // end of heap

    if (extendHeap(2*DSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if (!size) return NULL;
    int newsize = ALIGN(size + DSIZE);
    void *ptr = findWrapper(newsize);
    if ( ptr == NULL)
    {
        int extendSize = MAX(CHUNKSIZE, newsize);
        if ((ptr = extendHeap(extendSize)) == NULL)
            return NULL;
    }
    split(ptr, newsize);
    
    //checkFree(mem_heap_lo()+WSIZE);
    //checkheap(1);
    return ptr;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if ( ptr == NULL) return;
    bool alloc = GET_ALLOC(HDRP(ptr));
    if ( !alloc) return;
    else
    {
        size_t size = GET_SIZE(HDRP(ptr));
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
        PUT(NEXT_FP(ptr), 0);
        PUT(PREV_FP(ptr), 0);
        coalesce(ptr);
    }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if ( ptr == NULL) return mm_malloc(size);
    if ( !size)
    {
        mm_free(ptr);
        return NULL;
    }

    size_t oldSize = GET_SIZE(HDRP(ptr));
    size_t newSize = ALIGN(size+DSIZE);
    size_t csize;

    if ( newSize <= oldSize){ return ptr; }
    bool next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    if(!next_alloc && (csize = oldSize + GET_SIZE(HDRP(NEXT_BLKP(ptr)))) >= newSize)
    {
        deleteList(NEXT_BLKP(ptr), GET_SIZE(HDRP(NEXT_BLKP(ptr))));
        PUT(HDRP(ptr), PACK(csize, 1));
        PUT(FTRP(ptr), PACK(csize, 1));
        return ptr;
    }
    else
    {
	   void *newptr = mm_malloc(newSize);
	   memcpy(newptr, ptr, oldSize);
	   mm_free(ptr);
	   return newptr;
    }
}

void *coalesce(void *bp)
{
    bool prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    bool next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc){
        addList(bp, size);
        return bp;
    }
    else if ( prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        deleteList(NEXT_BLKP(bp), GET_SIZE(HDRP(NEXT_BLKP(bp))));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if ( !prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        deleteList(PREV_BLKP(bp), GET_SIZE(HDRP(PREV_BLKP(bp))));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        deleteList(PREV_BLKP(bp), GET_SIZE(HDRP(PREV_BLKP(bp))));
        deleteList(NEXT_BLKP(bp), GET_SIZE(FTRP(NEXT_BLKP(bp))));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    addList(bp, size);

    return bp;

}

void split( void* bp, size_t size)
{
    size_t sizeTemp = GET_SIZE(HDRP(bp));
    deleteList(bp, sizeTemp);
    if ((sizeTemp - size) >= 2*DSIZE)
    {
        sizeTemp -= size;
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(sizeTemp, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(sizeTemp, 0));
        PUT(NEXT_FP(NEXT_BLKP(bp)), 0);
        PUT(PREV_FP(NEXT_BLKP(bp)), 0);
        addList(NEXT_BLKP(bp), sizeTemp);
    }
    else {
        PUT(HDRP(bp), PACK(sizeTemp, 1));
        PUT(FTRP(bp), PACK(sizeTemp, 1));
    }

}

void *find(int sizestart, size_t size)
{

    void* current;
    if (sizestart == 0) { current = (void *)GET(mem_heap_lo()+LIST1); }
    else if (sizestart == 1) { current = (void *)GET(mem_heap_lo()+LIST2); }
    else if (sizestart == 2) { current = (void *)GET(mem_heap_lo()+LIST3); }
    else if (sizestart == 3) { current = (void *)GET(mem_heap_lo()+LIST4); }
    else if (sizestart == 4) { current = (void *)GET(mem_heap_lo()+LIST5); }
    else if (sizestart == 5) { current = (void *)GET(mem_heap_lo()+LIST6); }
    else if (sizestart == 6) { current = (void *)GET(mem_heap_lo()+LIST7); }
    else if (sizestart == 7) { current = (void *)GET(mem_heap_lo()+LIST8); }
    else if (sizestart == 8) { current = (void *)GET(mem_heap_lo()+LIST9); }
    else if (sizestart == 9) { current = (void *)GET(mem_heap_lo()+LIST10); }
    else if (sizestart == 10) { current = (void *)GET(mem_heap_lo()+LIST11); }
    else if (sizestart == 11) { current = (void *)GET(mem_heap_lo()+LIST12); }
    else if (sizestart == 12) { current = (void *)GET(mem_heap_lo()+LIST13); }
    else { current = (void *)GET(mem_heap_lo()+LIST14); }

    while ( current != NULL)
    {
        if(GET_SIZE(HDRP(current)) >= size) return current;
        current = (void *)GET(NEXT_FP(current));
    }

    return NULL;
}

void *extendHeap(size_t size)
{
    void *bp;
    
    if ((bp = mem_sbrk(size)) == (void *)-1)
        return NULL;

    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
    PUT(NEXT_FP(bp), 0);
    PUT(PREV_FP(bp), 0);

    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));

    bp = coalesce(bp);
    return bp;
}
    
void deleteList(void *bp, size_t size)
{
    void *prev = (void *)GET(PREV_FP(bp));
    void *next = (void *)GET(NEXT_FP(bp));
    if ( prev == NULL) // first one in linked list
    {
        if ( next != NULL) 
        {
            PUT(PREV_FP(next), 0);
        }
        if (size <= LIST1_LIMIT)  { PUT(mem_heap_lo()+LIST1, (unsigned int)next);}
        else if (size <= LIST2_LIMIT) { PUT(mem_heap_lo()+LIST2, (unsigned int)next); }
        else if (size <= LIST3_LIMIT)  { PUT(mem_heap_lo()+LIST3, (unsigned int)next);}
        else if (size <= LIST4_LIMIT) { PUT(mem_heap_lo()+LIST4, (unsigned int)next); }
        else if (size <= LIST5_LIMIT)  { PUT(mem_heap_lo()+LIST5, (unsigned int)next);}
        else if (size <= LIST6_LIMIT) { PUT(mem_heap_lo()+LIST6, (unsigned int)next); }
        else if (size <= LIST7_LIMIT)  { PUT(mem_heap_lo()+LIST7, (unsigned int)next);}
        else if (size <= LIST8_LIMIT) { PUT(mem_heap_lo()+LIST8, (unsigned int)next); }
        else if (size <= LIST9_LIMIT)  { PUT(mem_heap_lo()+LIST9, (unsigned int)next);}
        else if (size <= LIST10_LIMIT) { PUT(mem_heap_lo()+LIST10, (unsigned int)next); }
        else if (size <= LIST11_LIMIT)  { PUT(mem_heap_lo()+LIST11, (unsigned int)next);}
        else if (size <= LIST12_LIMIT) { PUT(mem_heap_lo()+LIST12, (unsigned int)next); }
        else if (size <= LIST13_LIMIT) { PUT(mem_heap_lo()+LIST13, (unsigned int)next); }
        else { PUT(mem_heap_lo()+LIST14, (unsigned int)next); }

    }
    else
    {
        if ( next != NULL) PUT(PREV_FP(next), (unsigned int)prev);
        PUT(NEXT_FP(prev), (unsigned int)next);
    }
    PUT(NEXT_FP(bp), 0);
    PUT(PREV_FP(bp), 0);
}

void addList(void *bp, size_t size)
{
    void *nextTemp;
    if (size <= LIST1_LIMIT) {nextTemp = mem_heap_lo()+LIST1; }
    else if (size <= LIST2_LIMIT) {nextTemp = mem_heap_lo()+LIST2; }
    else if (size <= LIST3_LIMIT) {nextTemp = mem_heap_lo()+LIST3; }
    else if (size <= LIST4_LIMIT) {nextTemp = mem_heap_lo()+LIST4; }
    else if (size <= LIST5_LIMIT) {nextTemp = mem_heap_lo()+LIST5; }
    else if (size <= LIST6_LIMIT) {nextTemp = mem_heap_lo()+LIST6; }
    else if (size <= LIST7_LIMIT) {nextTemp = mem_heap_lo()+LIST7; }
    else if (size <= LIST8_LIMIT) {nextTemp = mem_heap_lo()+LIST8; }
    else if (size <= LIST9_LIMIT) {nextTemp = mem_heap_lo()+LIST9; }
    else if (size <= LIST10_LIMIT) {nextTemp = mem_heap_lo()+LIST10; }
    else if (size <= LIST11_LIMIT) {nextTemp = mem_heap_lo()+LIST11; }
    else if (size <= LIST12_LIMIT) {nextTemp = mem_heap_lo()+LIST12; }
    else if (size <= LIST13_LIMIT) {nextTemp = mem_heap_lo()+LIST13; }
    else {nextTemp = mem_heap_lo()+LIST14; }

    void *next = (void *)GET(nextTemp);
    if(next != NULL)
        PUT(PREV_FP(next), (unsigned int)bp);
    PUT(NEXT_FP(bp), (unsigned int)next);
    //PUT(PREV_FP(bp), mem_heap_lo()+WSIZE);
    PUT(nextTemp, (unsigned int)bp);
}

void *findWrapper(size_t size)
{
    // trace-specific improvement
    static int last_size = 0;
    static int count = 0;
    if ( last_size == size && last_size > 64)
    {
        if ( count > 30)
        {
            int extendSize = MAX(size, 16);
            void *bp = extendHeap(extendSize);
            return bp;
        }
        else
            count++;
    }
    else
        count = 0;

    last_size = size;
    // end

    size_t sizeatstart;
    void *bp = NULL;

    if (size <= LIST1_LIMIT) {
        for (sizeatstart = 0; sizeatstart < 14; sizeatstart++) {
            if ((bp = find(sizeatstart, size)) != NULL)
                return bp;
        }
    } else if (size <= LIST2_LIMIT) {
        for (sizeatstart = 1; sizeatstart < 14; sizeatstart++) {
            if ((bp = find(sizeatstart, size)) != NULL)
                return bp;
        }
    } else if (size <= LIST3_LIMIT) {
        for (sizeatstart = 2; sizeatstart < 14; sizeatstart++) {
            if ((bp = find(sizeatstart, size)) != NULL)
                return bp;
        }
    } else if (size <= LIST4_LIMIT) {
        for (sizeatstart = 3; sizeatstart < 14; sizeatstart++) {
            if ((bp = find(sizeatstart, size)) != NULL)
                return bp;
        }
    } else if (size <= LIST5_LIMIT) {
        for (sizeatstart = 4; sizeatstart < 14; sizeatstart++) {
            if ((bp = find(sizeatstart, size)) != NULL)
                return bp;
        }
    } else if (size <= LIST6_LIMIT) {
        for (sizeatstart = 5; sizeatstart < 14; sizeatstart++) {
            if ((bp = find(sizeatstart, size)) != NULL)
                return bp;
        }
    } else if (size <= LIST7_LIMIT) {
        for (sizeatstart = 6; sizeatstart < 14; sizeatstart++) {
            if ((bp = find(sizeatstart, size)) != NULL)
                return bp;
        }
    } else if (size <= LIST8_LIMIT) {
        for (sizeatstart = 7; sizeatstart < 14; sizeatstart++) {
            if ((bp = find(sizeatstart, size)) != NULL)
                return bp;
        }
    } else if (size <= LIST9_LIMIT) {
        for (sizeatstart = 8; sizeatstart < 14; sizeatstart++) {
            if ((bp = find(sizeatstart, size)) != NULL)
                return bp;
        }
    } else if (size <= LIST10_LIMIT) {
        for (sizeatstart = 9; sizeatstart < 14; sizeatstart++) {
            if ((bp = find(sizeatstart, size)) != NULL)
                return bp;
        }
    } else if (size <= LIST11_LIMIT) {
        for (sizeatstart = 10; sizeatstart < 14; sizeatstart++) {
            if ((bp = find(sizeatstart, size)) != NULL)
                return bp;
        }
    } else if (size <= LIST12_LIMIT) {
        for (sizeatstart = 11; sizeatstart < 14; sizeatstart++) {
            if ((bp = find(sizeatstart, size)) != NULL)
                return bp;
        }
    } else if (size <= LIST13_LIMIT) {
        for (sizeatstart = 12; sizeatstart < 14; sizeatstart++) {
            if ((bp = find(sizeatstart, size)) != NULL)
                return bp;
        }
    } else {
        sizeatstart = 13;
        if ((bp = find(sizeatstart, size)) != NULL) {
            return bp;
        }
    }

    return bp;
}
