//
// Created by bhanu on 05/11/2016.
//

#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include "block.h"

void *realloc(void *ptr, size_t size) {
    /*If size==0 and ptr exists the it is the call for free*/
    void *new_mem_add = NULL;
    if (size == 0 && ptr != NULL) {
        free(ptr);
        return NULL;
    } else if (size != 0) {
        new_mem_add = malloc(size);
    }

    /*if ptr is NULL then it the call for malloc*/
    if (ptr == NULL) {
        return new_mem_add;
    }

    /*realloc call*/
    block_t *old_block_ptr = (block_t *) (ptr - sizeof(block_t));

    /* Copy as many bytes as are available from the old block
	             and fit into the new size.  */
    if (size > old_block_ptr->actual_size)
        size = old_block_ptr->actual_size;
    memcpy(new_mem_add, ptr, size);
    free(ptr);
    return new_mem_add;
}
