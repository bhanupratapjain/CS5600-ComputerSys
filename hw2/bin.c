//
// Created by bhanu on 04/11/2016.
//
#include <stdio.h>
#include "bin.h"
char *BinTypeString[] = {"BIN_8", "BIN_64", "BIN_512"};


bin_t *add_new_bin(block_t *block_ptr, int bin_index, const void *sbrk_return_addr) {
    bin_t *bin_ptr = (bin_t *) (sbrk_return_addr);
    bin_ptr->blocks_ptr = block_ptr;
    bin_ptr->type = bin_index;
    return bin_ptr;
}

void initialize_bins(arena_t *arent_t_ptr) {
    for (int i = 0; i < MAX_BINS; i++) {
        bin_t *bin_t_ptr = (bin_t *) sbrk(sizeof(bin_t));
        bin_t_ptr->type = i;
        //TODO: STAT
        bin_t_ptr->allc_req = 0;
        bin_t_ptr->free_req = 0;
        /*bin_t_ptr->free_blocks = 0;
        bin_t_ptr->used_blocks = 0;*/
        arent_t_ptr->bins[i] = bin_t_ptr;
    }
}
