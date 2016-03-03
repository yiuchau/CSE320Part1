/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include "sfmm.h"

/**
 * If allocater compiled with no flags or -DLIFO provided
 * freelist managed in last-in-first-out manner.
 * If allocater compiled with flag -DADDRESS
 * freelist maintained in increasing address order
 */
#ifdef ADDRESS
#define FL_POLICY 0
#else
#define FL_POLICY 1
#endif

/**
 * If allocator compiled with no flags or -DFIRST provided
 * first placement strategy for locating blocks
 * If allocator compiled with flag -DNEXT
 * next placement strategy is used
 */
#ifdef NEXT
#define BP_POLICY 0
#else
#define BP_POLICY 1
#endif
/**
 * You should store the head of your free list in this variable.
 * Doing so will make it accessible via the extern statement in sfmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */

sf_free_header* freelist_head = NULL;


#define PAGE_SIZE 4096
#define WSIZE 8 /* Quad word, pointer, and header/footer size (bytes) */
#define DSIZE 16 /* Long double size (bytes) */

void *find_fit(size_t asize);
void *extend_heap(size_t words);
void place(void *bp, size_t size, size_t asize);

void* sf_malloc(size_t size) {
	size_t asize; /* Adjusted block size */
	char *bp;
	size_t extendsize; /* Amount to extend heap if no fit */

	/* Ignore spurious requests */
	if (size == 0)
		return NULL;

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)
		asize = 2*DSIZE;
	else
		asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1))/ DSIZE);

	/* Search the free list for a fit */
	if((bp = find_fit(asize)) != NULL){
		place(bp, size, asize);
		printf("block returned at %lx\n", ((size_t)bp + SF_HEADER_SIZE));
		return (void *)((size_t)bp + SF_HEADER_SIZE);
	}

	/* No fit found. Get more memory and place the block */
	extendsize = (asize > PAGE_SIZE ? asize : PAGE_SIZE);
	if((bp = extend_heap(extendsize/WSIZE)) == NULL)
		return NULL;
	place(bp, size, asize);
	printf("block returned at %lx\n", ((size_t)bp + SF_HEADER_SIZE));
	return (void *)((size_t)bp + SF_HEADER_SIZE);
}

void sf_free(void *ptr) {

}

void* sf_realloc(void *ptr, size_t size) {
    return NULL;
}

void* sf_calloc(size_t nmemb, size_t size) {
    return NULL;
}

void *find_fit(size_t asize) {
	sf_free_header* ptr = NULL;
	if (freelist_head == NULL){
		sf_free_header* hptr = (sf_free_header*)sf_sbrk(0);
		sf_footer* fptr;
		freelist_head = (sf_free_header*)((size_t)hptr + 8 - ((size_t)hptr % DSIZE)); 
		printf("freelist_head at %lx\n", (size_t)freelist_head);
		freelist_head->header.alloc = 0;
		freelist_head->header.block_size = ((PAGE_SIZE + 15) >> 4);
		printf("freelist_header block size saved as = %d\n", freelist_head->header.block_size);

		fptr = (sf_footer*)((size_t)freelist_head + (freelist_head->header.block_size << 4) - SF_FOOTER_SIZE);
		fptr->alloc = 0;
		fptr->block_size = freelist_head->header.block_size;


		printf("freelist_head footer at %lx\n", (size_t)fptr);

		freelist_head->next = NULL;
		freelist_head->prev = NULL;
	}

	ptr = freelist_head;

	do{
		if(ptr->header.block_size >= asize){
			return ptr;
		}

		ptr = ptr->next;

	}while((size_t)ptr < ((size_t)sf_sbrk(0) + PAGE_SIZE));

	return NULL;

}

void place(void *bp, size_t size, size_t asize) {
	
	//edit header, footer of newly allocated block
	sf_free_header* ptr;
	sf_footer* fptr;
	ptr = (sf_free_header*)bp;
	ptr->header.alloc = 1;
	ptr->header.block_size = asize >> 4;
	ptr->header.requested_size = size;
	fptr = (sf_footer*)((size_t)ptr + (ptr->header.block_size << 4) - SF_FOOTER_SIZE);
	fptr->alloc = 1;
	fptr->block_size = ptr->header.block_size;
	printf("Allocating a block of size: %ld, saved as %d\n", asize, ptr->header.block_size);
	//splinter if neccessary
	//remove from free-list
}

void* extend_heap(size_t words){
	return NULL;
}
