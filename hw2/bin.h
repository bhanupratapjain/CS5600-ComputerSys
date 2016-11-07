//
// Created by bhanu on 05/11/2016.
//

#ifndef BIN_H
#define BIN_H

#include "arena.h"
#include "block.h"


extern char *BinTypeString[];

typedef struct bin_t {
    uint8_t type;
    struct block_t *blocks_ptr;
    uint8_t allc_req;
    uint8_t free_req;
//    uint8_t free_blocks;
//    uint8_t used_blocks;
} bin_t;

void initialize_bins(struct arena_t *arent_t_ptr);

bin_t *add_new_bin(struct block_t *block_ptr, int bin_index, const void *sbrk_return_addr);

#endif //BIN_H
