//
// Created by bhanu on 05/11/2016.
//
#include <stdio.h>
#include "block.h"

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    free_block(ptr);
}
