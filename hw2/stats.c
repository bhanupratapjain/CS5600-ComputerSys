//
// Created by bhanu on 04/11/2016.
//


#include <stdio.h>
#include <sys/mman.h>


#include "arena.h"
#include "block.h"
#include "bin.h"

static int get_total_free_blocks(bin_t *bin_ptr) {
    block_t *block_itr = bin_ptr->blocks_ptr;
    int count = 0;
    while (block_itr != NULL) {
        if (block_itr->block_status == BLOCK_AVAILABLE) {
            count += 1;
        }
        block_itr = block_itr->next;
    }
    return count;
}

static int get_total_used_blocks(bin_t *bin_ptr) {
    block_t *block_itr = bin_ptr->blocks_ptr;
    int count = 0;
    while (block_itr != NULL) {
        if (block_itr->block_status == BLOCK_USED) {
            count += 1;
        }
        block_itr = block_itr->next;
    }
    return count;
}

void print_arenas() {
    arena_t *arena_itr = global_arena_ptr;
    int arena_count = 1;
    printf("-----------Global Arena [%p]\n", global_arena_ptr);
    while (arena_itr != NULL) {
        printf("-----------Arena %d [%p], threads[%d]-----------\n", arena_count++,
               arena_itr, arena_itr->no_of_threads);

        for (int i = 0; i < MAX_BINS; i++) {
            bin_t *bin_ptr = arena_itr->bins[i];
            if (bin_ptr != NULL) {
                printf("-------------BIN [%s] [%p] ----------------\n",
                       BinTypeString[bin_ptr->type], bin_ptr);
                block_t *block_itr = bin_ptr->blocks_ptr;
                while (block_itr != NULL) {
                    printf(
                            "---------------BLOCK [%d] [%p] [%d]----------------\n",
                            block_itr->block_status,
                            block_itr, block_itr->type);
                    block_itr = block_itr->next;
                }
            }
        }
        printf("-----------END OF Arena-----------\n");
        arena_itr = arena_itr->next;
    }
}

long get_arena_size(arena_t *arena_ptr) {
    if (arena_ptr == NULL) {
        return 0;
    }
    long total = sizeof(arena_t);
    for (int i = 0; i < MAX_BINS; i++) {
        bin_t *bin_ptr = arena_ptr->bins[i];
        if (bin_ptr != NULL) {
            total += sizeof(bin_t);
            block_t *block_ptr = bin_ptr->blocks_ptr;
            if (block_ptr != NULL) {
                total += (block_ptr->actual_size + sizeof(bin_t));
            }
        }
    }
    return total;
}

void malloc_stats() {
    arena_t *arena_itr = global_arena_ptr;
    int arena_count = 1;
    printf("============================MALLOC STATS===============================\n");
    while (arena_itr != NULL) {
        printf("============================Arena [%d]=============================\n", arena_count++);
        printf("\t Total Size of Arena        : %ld KB\n", get_arena_size(arena_itr));
        printf("\t Total Number of Bins       : %d\n", MAX_BINS);
        printf("======================================================================\n");
        for (int i = 0; i < MAX_BINS; i++) {
            bin_t *bin_ptr = arena_itr->bins[i];
            int free_count = get_total_free_blocks(bin_ptr);
            int used_count = get_total_used_blocks(bin_ptr);
            printf("============================Bin [%s]=============================\n",
                   BinTypeString[bin_ptr->type]);
            //TODO: STAT
            printf("\t Total Allocation Request : %d\n", bin_ptr->allc_req);
            printf("\t Total Free Request       : %d\n", bin_ptr->free_req);
            printf("\t Total Free Blocks        : %d\n", free_count);
            printf("\t Total USed Blocks        : %d\n", used_count);
            printf("\t Total Number of Blocks   : %d\n", free_count + used_count);
        }
        printf("======================================================================\n");
        arena_itr = arena_itr->next;
    }
    printf("============================END OF MALLOC STATS=============================\n");
}


