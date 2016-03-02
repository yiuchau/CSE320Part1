/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include "sfmm.h"

/**
 * You should store the head of your free list in this variable.
 * Doing so will make it accessible via the extern statement in sfmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */
sf_free_header* freelist_head = NULL;

#define PAGE_SIZE 4096

void* sf_malloc(size_t size) {
    return NULL;
}

void sf_free(void *ptr) {

}

void* sf_realloc(void *ptr, size_t size) {
    return NULL;
}

void* sf_calloc(size_t nmemb, size_t size) {
    return NULL;
}
