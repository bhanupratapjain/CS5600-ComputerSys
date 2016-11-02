//
// Created by Bhanu on 02/11/2016.
//
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>

#include "malloc.h"


void *calloc(size_t nmemb, size_t size) {
//    printf("calloc nmemb[%zu],size[%zu], thread[%lu]\n", nmemb, size, pthread_self());

    void *add_ptr = malloc(nmemb * size);

    if (add_ptr == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    //pthread_mutex_lock(&thread_arena_ptr->lock);
    block_t *block_ptr = (block_t *) (add_ptr - sizeof(block_t));
    memset(add_ptr, 0, block_ptr->actual_size);
    //pthread_mutex_unlock(&thread_arena_ptr->lock);
    return add_ptr;
}
