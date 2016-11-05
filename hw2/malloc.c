//
// Created by Bhanu on 25/10/2016.
//
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include "arena.h"
#include "block.h"
#include "bin.h"

static int init_malloc() {

    if (pthread_atfork(fork_prepare, fork_parent, fork_child) != 0) {
        errno = ENOMEM;
        return -1;
    }
    system_page_size = sysconf(_SC_PAGESIZE);
    no_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
    return -1;
}


void *malloc(size_t size) {

    block_t *block_ptr = NULL;

    /*Initialize Arenas*/
    if (thread_arena_ptr == NULL) {
        arena_t *arena_ptr = initialize_arenas();
        if (arena_ptr == NULL) {
            errno = ENOMEM;
            return NULL;
        }
        thread_arena_ptr = arena_ptr;
    }

    size_t mem_size = size + sizeof(block_t);

    if (mem_size > MAX_HEAP_SIZE) {
        /*MMAP*/
        block_ptr = (block_t *) get_new_memory(mem_size);
        block_ptr->type = BLOCK_MMAP;
        block_ptr->block_status = BLOCK_USED;
        block_ptr->actual_size = size;
    } else {
        /*SBRK */
        /*Get the bin index based on user requested memory + block size*/
        int bin_index = get_index(mem_size);

        pthread_mutex_lock(&thread_arena_ptr->lock);
        bin_t *bin_ptr = thread_arena_ptr->bins[bin_index];
        //TODO: STAT
        bin_ptr->allc_req++;

        /*Get Block*/
        block_ptr = get_block(bin_ptr, mem_size, bin_index);
        if (block_ptr == NULL) {
            errno = ENOMEM;
            return NULL;
        }

        /*Release arena lock*/
        pthread_mutex_unlock(&thread_arena_ptr->lock);
    }

    /*Free Block Found*/
    return (void *) block_ptr + sizeof(block_t);
}


__attribute__ ((constructor))
void myconstructor() {
    if (init_malloc() < 0) {
        errno = ENOMEM;
    };
}


