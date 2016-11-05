//
// Created by Bhanu on 02/11/2016.
//
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#include "block.h"

void *calloc(size_t nmemb, size_t size) {
    void *add_ptr = malloc(nmemb * size);
    if (add_ptr == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    block_t *block_ptr = (block_t *) (add_ptr - sizeof(block_t));
    memset(add_ptr, 0, block_ptr->actual_size);
    return add_ptr;
}
