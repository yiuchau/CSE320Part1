/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sfmm.h"
#include <errno.h>


#ifdef CSE320
#define  cse320(fmt, ...) do{printf("CSE320: %s:%s:%d " fmt, __FILE__,__FUNCTION__, __LINE__, ##__VA_ARGS__);}while(0)
#else
#define cse320(fmt, ...)
#endif

/**
 * If allocater compiled with no flags or -DLIFO provided
 * freelist managed in last-in-first-out manner.
 * If allocater compiled with flag -DADDRESS
 * freelist maintained in increasing address order
 */
#ifdef ADDRESS
#define FL_POLICY 1
#else
#define FL_POLICY 0
#endif

/**
 * If allocator compiled with no flags or -DFIRST provided
 * first placement strategy for locating blocks
 * If allocator compiled with flag -DNEXT
 * next placement strategy is used
 */
#ifdef NEXT
#define BP_POLICY 1
#else
#define BP_POLICY 0
#endif
/**
 * You should store the head of your free list in this variable.
 * Doing so will make it accessible via the extern statement in sfmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */

sf_free_header* freelist_head = NULL;
sf_free_header* nextFitPtr = NULL;
size_t START = 0, END;


#define PAGE_SIZE 4096
#define MIN_BLOCK_SIZE 32
#define WSIZE 8 /* Quad word, pointer, and header/footer size (bytes) */
#define DSIZE 16 /* Long double size (bytes) */



void *find_fit(size_t asize);
void *extend_heap(size_t words);

void* coalesce(void *ptr);
void* addFree(void *bp);
void removeFree(void *bp);
void place(void *bp, size_t size, size_t asize);
void printAll();

void* sf_malloc(size_t size) {
	size_t asize; /* Adjusted block size */
	sf_free_header*bp;

	/* Ignore spurious requests */
	if (size == 0 || size > ((size_t)4 * (1 << 30))){
		errno = EINVAL; //invalid args
		return NULL;
	}

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)
		asize = 2*DSIZE;
	else
		asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1))/ DSIZE);

	/* Search the free list for a fit */
	if((bp = find_fit(asize)) != NULL){
		place(bp, size, asize);
		if(bp == NULL){
			errno = ENOMEM;
			return NULL;
		}
		cse320("block returned at %lx\n", ((size_t)bp + SF_HEADER_SIZE));
		//printAll();

		return (void *)((size_t)bp + SF_HEADER_SIZE);
	}

	/* No fit found. Get more memory and place the block */
	do{
		if((bp = (sf_free_header*)extend_heap(PAGE_SIZE)) == NULL){
			errno = ENOMEM;
			return NULL;
		}
	}while((bp = find_fit(asize)) == NULL);

	place(bp, size, asize);

	if(bp == NULL){
		errno = ENOMEM;
		return NULL;
	}

	cse320("block returned at %lx\n", ((size_t)bp + SF_HEADER_SIZE));
	//printAll();
	return (void *)((size_t)bp + SF_HEADER_SIZE);
}

void sf_free(void *ptr) {
	//check if valid address
	if((size_t)ptr < START || (size_t)ptr > END || ((size_t)ptr % 16 != 0)){
		errno = EFAULT; //bad address
		return;
	}
	// Adjust block header, footer
	// Coalesce (which also adds resultant block to freelist depending on policy)
	sf_free_header* hptr = (sf_free_header*)((size_t)ptr - SF_HEADER_SIZE);
	sf_footer* fptr = (sf_footer*)((size_t)hptr + (hptr->header.block_size << 4) - SF_FOOTER_SIZE);
	hptr->header.alloc = 0;
	fptr->alloc = 0;
	cse320("Freeing block at %lx of size %i\n", (size_t)hptr, hptr->header.block_size << 4);
	coalesce(hptr);
}

void* sf_realloc(void *ptr, size_t size) {
	sf_header* bp = (sf_header*)((size_t)ptr - SF_HEADER_SIZE);
	size_t asize;
	size_t block_size = bp->block_size << 4;

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)
		asize = 2*DSIZE;
	else
		asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1))/ DSIZE);

	if(size == 0){
		errno = EINVAL; //invalid arg
		return NULL;
	}

	if(asize <= block_size - 32){
		//room to create new block
		sf_header* newPtr = (sf_header*)((size_t)bp + asize);
		cse320("Test: newPtr - oldPtr = %li\n", (size_t)newPtr - (size_t)bp);

		bp->requested_size = size;
		bp->block_size = asize >> 4;

		((sf_footer*)((size_t)bp + asize - SF_FOOTER_SIZE))->block_size = asize >> 4;
		((sf_footer*)((size_t)bp + asize - SF_FOOTER_SIZE))->alloc = 1;

		newPtr->block_size = (block_size - asize) >> 4;
		newPtr->alloc = 0;

		((sf_footer*)((size_t)newPtr + block_size - asize - SF_FOOTER_SIZE))->block_size = (block_size - asize) >> 4;
		((sf_footer*)((size_t)newPtr + block_size - asize - SF_FOOTER_SIZE))->alloc = 0;
		addFree(newPtr);
		cse320("Realloc returns smaller block of size %li\n",asize);
		printAll();
		return ptr;
	}

	if(asize > block_size){
		//allocate new block, free old block
		cse320("Realloc creating newer block of size %li\n",asize);
		
		bp = sf_malloc(size);
		//copy data from ptr to bp
		memcpy(bp, ptr, block_size - (2 * SF_HEADER_SIZE));
		sf_free(ptr);
		return bp;
	}

	cse320("No realloc done.");
	errno = ENOMEM;
    return NULL;
}

void* sf_calloc(size_t nmemb, size_t size) {
	sf_free_header* bp = NULL;

	if(nmemb == 0 || size == 0){
		errno = EINVAL;
		return NULL;
	}

	bp = sf_malloc(nmemb * size);

	memset(bp, 0, nmemb * size);

    return bp;
}

void *find_fit(size_t asize) {
	sf_free_header* ptr = NULL;
	if (freelist_head == NULL){
		extend_heap(PAGE_SIZE);
	}

	//find fit depending on policy: FIRST, NEXT
	if(BP_POLICY == 0){
		cse320("FIRSTFIT\n");
		ptr = freelist_head;
		do{
			if((ptr->header.block_size << 4) >= asize){
				cse320("Free block found at size %i for requested size %li\n",(ptr->header.block_size << 4),asize);
				return ptr;
			}

			ptr = ptr->next;

		}while(ptr != NULL);
	}else{
		//NEXT FIT
		cse320("NEXTFIT\n");
		ptr = nextFitPtr;
		do{
			if((ptr->header.block_size << 4) >= asize){
				cse320("Free block found at size %i for requested size %li\n",(ptr->header.block_size << 4),asize);
				nextFitPtr = (ptr->next == NULL) ? freelist_head : ptr->next;
				return ptr;
			}
			if(ptr->next == NULL){
				//loop around from freelist_head
				ptr = freelist_head;
			}
			else
				ptr = ptr->next;
		}while(ptr != nextFitPtr);
	}

	return NULL;

}

void place(void *bp, size_t size, size_t asize) {

	//adjust asize for splinters
	
	//edit header, footer of newly allocated block
	sf_free_header* ptr;
	sf_footer* fptr;
	ptr = (sf_free_header*)bp;

	size_t block_size = ptr->header.block_size;

	ptr->header.alloc = 1;
	ptr->header.requested_size = size;

	if((block_size << 4) >= (asize + 32)){
		//min block splinter reqs met, splinter block
		cse320("Attempt splinter\n");
		sf_free_header* sptr = (sf_free_header*)((size_t)ptr + asize);

		ptr->header.block_size = asize >> 4;
		fptr = (sf_footer*)((size_t)ptr + asize - SF_FOOTER_SIZE);
		fptr->alloc = 1;
		fptr->block_size = asize >> 4;

		//edit header, footer, of splintered block
		sptr->header.alloc = 0;
		sptr->header.block_size = block_size - (asize >> 4);
		fptr = (sf_footer*)((size_t)sptr + (sptr->header.block_size << 4) - SF_FOOTER_SIZE);
		fptr->alloc = 0;
		fptr->block_size = sptr->header.block_size;
		
		//Remove old block and add splintered block to freelist.
		removeFree(ptr);
		addFree(sptr);

		cse320("Splintered block at %lx with size %i\n", (size_t)sptr, (sptr->header.block_size) << 4);
		cse320("Freelist_head at %lx\n", (size_t)freelist_head);
		
	}else{
		//asize changed to size of block
		asize = block_size << 4; //block size in header unchanged

		fptr = (sf_footer*)((size_t)ptr + asize - SF_FOOTER_SIZE);
		fptr->alloc = 1;
		fptr->block_size = block_size;
		//remove block from freelist
		removeFree(bp);
	}
	cse320("Allocating a block of size: %ld, saved as size %d\n", asize, (ptr->header.block_size << 4));
}

void* extend_heap(size_t size){
		sf_free_header* bp = (sf_free_header*)sf_sbrk(PAGE_SIZE);
		if(bp == NULL){
			errno = ENOMEM;
			return NULL;
		}
		sf_footer* fptr;
		//align block to 16 bytes
		bp = (sf_free_header*)((size_t)bp + 8 - ((size_t)bp % DSIZE));
		if(START == 0){
			START = (size_t)bp; 
			END = START + PAGE_SIZE;
		}else
			END = END + PAGE_SIZE;

		cse320("MEMSTART: %lx END: %lx\n", START, END);
		bp->header.alloc = 0;
		bp->header.block_size = ((PAGE_SIZE + 15) >> 4); // Adjust size for alignment, fix start end with alignment padding
		bp->next = NULL;
		bp->prev = NULL;
	
		fptr = (sf_footer*)((size_t)bp + (bp->header.block_size << 4) - SF_FOOTER_SIZE);
		fptr->alloc = 0;
		fptr->block_size = bp->header.block_size;

		cse320("Heap extended: Header: %lx Size: %i\n",(size_t)bp, ((bp->header.block_size) << 4));
		return coalesce(bp);
}

void* coalesce(void *ptr) {
	sf_header *bp, *prev_bp, *next_bp;
	sf_footer *fptr = NULL, *nfptr = NULL;
	size_t prev_size = 0, next_size = 0, prev_alloc = 1, next_alloc = 1;

	bp = (sf_header*)ptr;
	prev_bp = (sf_header*)((size_t)bp - ((size_t)(((sf_footer*)((size_t)bp - SF_HEADER_SIZE))->block_size) << 4 ));
	next_bp = (sf_header*)((size_t)bp + ((size_t)(bp->block_size) << 4));
	if((size_t)prev_bp > START){
		prev_size = (size_t)(prev_bp->block_size);
		prev_alloc = (size_t)(prev_bp->alloc);

	}else{
		cse320("Prevbp is out of bounds.\n");
	}

	if((size_t)next_bp < END){
		next_size = (size_t)(next_bp->block_size);
		next_alloc = (size_t)(next_bp->alloc);
	}else{
		cse320("Nextbp is out of bounds.\n");
	}

	cse320("Coalescing - bp = %lx | prev = %lx | next = %lx\n", (size_t)bp, (size_t)prev_bp, (size_t)next_bp);
	cse320("Coalescing - prev_alloc = %li, size = %li| next_alloc = %li, size = %li\n", prev_alloc, prev_size << 4, next_alloc, next_size << 4);

	fptr = (sf_footer*)((size_t)bp + (bp->block_size << 4) - SF_FOOTER_SIZE);
	nfptr = (sf_footer*)((size_t)next_bp + (next_bp->block_size << 4) - SF_FOOTER_SIZE);

	if(prev_alloc && next_alloc){
		cse320("Coalesce case 1: %lx - %i\n",(size_t)bp, (bp->block_size) << 4);
		return addFree(bp);
	}
	else if (prev_alloc && !next_alloc){
		bp->block_size = bp->block_size + next_size;
		nfptr->block_size = bp->block_size + next_size;
		removeFree(next_bp);
		cse320("Coalesce case 2: %lx - %li\n",(size_t)bp, (bp->block_size + next_size) << 4);
		return addFree(bp);
	}
	else if(!prev_alloc && next_alloc){
		prev_bp->block_size = prev_size + bp->block_size;
		fptr->block_size = prev_size + bp->block_size;
		removeFree(prev_bp);
		cse320("Coalesce case 3: %lx - %li\n",(size_t)prev_bp, (prev_size + bp->block_size) << 4);
		return addFree(prev_bp);
	}
	else{
		prev_bp->block_size = prev_size + bp->block_size + next_size;
		nfptr->block_size = prev_size + bp->block_size + next_size;
		removeFree(prev_bp);
		removeFree(next_bp);
		cse320("Coalesce case 4: %lx - %li\n",(size_t)prev_bp, (prev_size + bp->block_size + next_size) << 4);
		return addFree(prev_bp);
	}

	//printAll();
	errno = ENOMEM;
	return NULL;

}

void removeFree(void *bp){
	//remove block from free-list
	sf_free_header* ptr = (sf_free_header*)bp;

	//if block being removed is free-list head, change head
	if(ptr == freelist_head){
		freelist_head = ptr->next;
	}

	if(ptr->next != NULL){
		if(ptr->prev != NULL)
			ptr->next->prev = ptr->prev;
		else
			ptr->next->prev = NULL;
	}
	if(ptr->prev != NULL){
		if(ptr->next != NULL)
			ptr->prev->next = ptr->next;
		else
			ptr->prev->next = NULL;
	}
}

void* addFree(void *bp){
	sf_free_header* ptr = (sf_free_header*)bp;
	//add block to free list depending on policy
	if(freelist_head != NULL){
		//LIFO
		cse320("LIFO POLICY\n");
		if(FL_POLICY == 0){
			ptr->next = freelist_head;
			freelist_head->prev = ptr;
			freelist_head = ptr;
			ptr->prev = NULL;
			return ptr;
		}else{
			//ADDRESS
			sf_free_header* aptr = freelist_head;
			cse320("ADDRESS POLICY\n");
			while(aptr != NULL){
				if(ptr < aptr){
					cse320("Block to add: %lx Aptr: %lx Freelist_head: %lx\n", (size_t)ptr, (size_t)aptr, (size_t)freelist_head);
					if(aptr == freelist_head){
						aptr->prev = ptr;
						ptr->prev = NULL;
						ptr->next = aptr;
						freelist_head = ptr;
						//printAll();
						return ptr;
					}else{
						ptr->next = aptr;
						ptr->prev = aptr->prev;
						aptr->prev = ptr;
						ptr->prev->next = ptr;
						return ptr;
					}
				}else{
					if(aptr->next == NULL){
						aptr->next = ptr;
						ptr->prev = aptr;
						ptr->next = NULL;
						return ptr;
					}
					aptr = aptr->next;
				}
			}


		}
	}else{
		ptr->next = NULL;
		ptr->prev = NULL;
		freelist_head = ptr;
		nextFitPtr = freelist_head;
		return ptr;
	}
	cse320("Error in adding to freelist\n");
	errno = ENOMEM;
	return NULL;
}

void printAll(){
	cse320("Printing.. \n");
	sf_free_header* ptr = freelist_head;
	while(ptr != NULL){
		cse320("PrintAll: Ptr %lx\n", (size_t)ptr);
		sf_blockprint(ptr);
		ptr = ptr->next;
	}
}
