//
// Created by bhanu on 04/11/2016.
//
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include "block.h"


int block_size[] = {8, 64, 512};
char *BlockTypeString[] = {"BLOCK_SBRK", "BLOCK_MMAP"};
char *BlockStatusString[] = {"BLOCK_USED", "BLOCK_AVAILABLE"};
size_t MAX_HEAP_SIZE = 512;
long system_page_size = 4096;

void add_block_to_bin(bin_t *bin_ptr, block_t *block_ptr) {
    block_t *block_itr = bin_ptr->blocks_ptr;
    block_t *p_block_itr = NULL;

    if (block_itr == NULL) {
        bin_ptr->blocks_ptr = block_ptr;
        return;
    }
    while (block_itr != NULL) {
        p_block_itr = block_itr;
        block_itr = block_itr->next;
    }
    p_block_itr->next = block_ptr;
}

int get_index(size_t needed) {
    int index = 0;
    int memory = 8; //Min. Memory Allocated
    while (memory < needed) {
        memory *= 8;
        index++;
    }
    return index;
}

block_t *get_unused_block(bin_t *bin_ptr) {

    /*Do NOt Search if there are no Free Blocks*/
    //TODO: STAT
    /*if (bin_ptr->free_blocks == 0) {
        return NULL;
    }*/

    block_t *block_itr = bin_ptr->blocks_ptr;

    /*If there is no Block in the Bin*/
    if (block_itr == NULL) {
        return NULL;
    }

    /*Search for Blocks in Bin*/
    while (block_itr != NULL) {
        if (block_itr->block_status == BLOCK_AVAILABLE) {
            return block_itr;
        }
        block_itr = block_itr->next;
    }
    /*No Free Block Found*/
    return NULL;
}

void remove_block_from_bin(block_t *block_ptr) {
    bin_t *bin_ptr = thread_arena_ptr->bins[block_ptr->bin_type];
    block_t *block_itr = bin_ptr->blocks_ptr;
    block_t *p_block_itr = NULL;

    /*Size ==1*/
    if (block_itr->next == NULL) {
        bin_ptr->blocks_ptr = NULL;
        return;
    }

    /*Head Case*/
    if (block_itr == block_ptr) {
        bin_ptr->blocks_ptr = block_itr->next;
        return;
    }

    while (block_itr != NULL) {
        if (block_itr == block_ptr) {
            p_block_itr->next = block_itr->next;
            break;
        }
        p_block_itr = block_itr;
        block_itr = block_itr->next;
    }
}


int add_blocks(size_t mem_size, int bin_index) {

    /*Get New Memory*/
    void *new_mem_return_addr = get_new_memory(mem_size);

    if (new_mem_return_addr == NULL) {
        errno = ENOMEM;
        return -1;
    }

    /*Allocate SBRK Memory*/
    /*Divide memory among all the Bins*/
    /*Total Memory Size to allocate to Bins */
    int memory_to_allocate = system_page_size;

    while (memory_to_allocate > (block_size[0])) {
        for (int i = 0; i < MAX_BINS; i++) {

            if (memory_to_allocate < (block_size[i] + sizeof(block_t))) {
                break;
            }
            /*Memory allocated to the block*/
            size_t mem_allocated = block_size[i] + sizeof(block_t);

            block_t *block_ptr = (block_t *) new_mem_return_addr;
            block_ptr->type = BLOCK_SBRK;
            block_ptr->next = NULL;
            block_ptr->bin_type = i;
            block_ptr->block_status = BLOCK_AVAILABLE;

            bin_t *bin_ptr = thread_arena_ptr->bins[i];
            //TODO: STAT
            /*bin_ptr->free_blocks++;*/
            add_block_to_bin(bin_ptr, block_ptr);

            new_mem_return_addr += mem_allocated;
            memory_to_allocate -= mem_allocated;
        }
    }
    return 0;
}


block_t *get_block(bin_t *bin_ptr, size_t mem_size, int bin_index) {
    block_t *block_ptr = NULL;
    /*STEP-1 :: Find Blocks*/
    block_ptr = get_unused_block(bin_ptr);
    if (block_ptr == NULL) {
        /*STEP-2 :: Create Blocks*/
        if (add_blocks(mem_size, bin_index) < 0) {
            errno = ENOMEM;
            return NULL;
        }
        /*STEP-3 :: Find Block*/
        block_ptr = get_unused_block(bin_ptr);
    }
    //TODO: STAT
    /*bin_ptr->free_blocks--;*/
    //TODO: STAT
    /*bin_ptr->used_blocks++;*/
    block_ptr->block_status = BLOCK_USED;
    block_ptr->actual_size = (mem_size - sizeof(block_t));
    return block_ptr;
}


void free_block(void *ptr) {

    block_t *block_ptr = (block_t *) (ptr - sizeof(block_t));

    /*if (block_ptr->bin_type > 2) {
        printf("ptr[%p], bintype[%d], status[%s],type[%s], size[%zu]\n", block_ptr, block_ptr->bin_type,
               BlockTypeString[block_ptr->type], BlockStatusString[block_ptr->block_status], block_ptr->actual_size);

    }*/

    if (block_ptr->type == BLOCK_MMAP) {
        size_t mem_size = (size_t) (block_ptr->actual_size + sizeof(block_t));
        munmap(block_ptr, mem_size);
    } else {

        if (check_addr(ptr) < 0) {
            return;
        }
        bin_t *bin_ptr = thread_arena_ptr->bins[block_ptr->bin_type];
        bin_ptr->free_req++;
        /*
        bin_t *bin_ptr = thread_arena_ptr->bins[block_ptr->bin_type];
        bin_ptr->free_req++;
        bin_ptr->free_blocks++;
        bin_ptr->used_blocks--;
        */
        //TODO::
        /*pthread_mutex_lock(&thread_arena_ptr->lock);*/
        block_ptr->block_status = BLOCK_AVAILABLE;
        /*pthread_mutex_unlock(&thread_arena_ptr->lock);*/
    }
}

int check_valid_block(block_t *block_ptr) {
    if (thread_arena_ptr == NULL) {
        return -1;
    }
    if (block_ptr != NULL && 0 <= block_ptr->bin_type <= 2)
        return 0;
    return -1;

}

void *get_new_memory(size_t mem_size) {
    void *new_mem_return_addr = NULL;
    if (mem_size > MAX_HEAP_SIZE) {
        /*Get Memory From MMAP*/
        new_mem_return_addr = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                                   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (new_mem_return_addr == MAP_FAILED) {
            errno = ENOMEM;
            return NULL;
        }

    } else {
        /*Get Memory From SBRK*/
        new_mem_return_addr = sbrk(system_page_size);
        if (new_mem_return_addr == NULL) {
            errno = ENOMEM;
            return NULL;
        }
    }
    return new_mem_return_addr;
}
