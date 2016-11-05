//
// Created by bhanu on 05/11/2016.
//
#include <stdio.h>
#include "block.h"

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    block_t *block_ptr = (block_t *) (ptr - sizeof(block_t));
    free_block(block_ptr);
}
