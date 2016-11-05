//
// Created by bhanu on 05/11/2016.
//

#ifndef BLOCK_H
#define BLOCK_H

#define BLOCK_USED 0
#define BLOCK_AVAILABLE 1
#define BLOCK_SBRK 0
#define BLOCK_MMAP 1

#include <stdint.h>
#include "bin.h"


typedef struct block_t {
    struct block_t *next;
    uint8_t bin_type;
    uint8_t type;
    uint8_t block_status;
    size_t actual_size;
} block_t;

extern int block_size[];
extern char *BlockTypeString[];
extern char *BlockStatusString[];
extern size_t MAX_HEAP_SIZE;
extern long system_page_size;

int get_index(size_t needed);

block_t *get_unused_block(struct bin_t *bin_ptr);

void remove_block_from_bin(block_t *block_ptr);

void add_block_to_bin(struct bin_t *bin_ptr, struct block_t *block_ptr);

block_t *create_new_block(size_t size, int bin_index, const void *block_addr);

int add_blocks(size_t mem_size, int bin_index);

block_t *get_block(struct bin_t *bin_ptr, size_t mem_size, int bin_index);

void free_block(block_t *block_ptr);

void *get_new_memory(size_t mem_size);
#endif //BLOCK_H
