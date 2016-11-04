//
// Created by Bhanu on 25/10/2016.
//
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>

#include "malloc.h"


arena_t *find_available_arena() {
    arena_t *arena_itr = global_arena_ptr;
    arena_t *arena_final_ptr = global_arena_ptr;
    while (arena_itr != NULL) {

        if (arena_itr->no_of_threads < arena_final_ptr->no_of_threads) {
            arena_final_ptr = arena_itr;
        }
        arena_itr = arena_itr->next;
    }
    return arena_final_ptr;
}

void add_arena(arena_t *arena_ptr) {
    if (global_arena_ptr == NULL) {
        global_arena_ptr = arena_ptr;
        return;
    }
    arena_t *arena_itr = global_arena_ptr;
    arena_t *p_arena_itr = NULL;
    while (arena_itr != NULL) {
        p_arena_itr = arena_itr;
        arena_itr = arena_itr->next;
    }
    p_arena_itr->next = arena_ptr;
}

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

void initialize_bins(arena_t *arent_t_ptr) {
    for (int i = 0; i < MAX_BINS; i++) {
        bin_t *bin_t_ptr = (bin_t *) sbrk(sizeof(bin_t));
        bin_t_ptr->type = i;
        //TODO: STAT
        bin_t_ptr->allc_req = 0;
        /*bin_t_ptr->free_req = 0;
        bin_t_ptr->free_blocks = 0;
        bin_t_ptr->used_blocks = 0;*/
        arent_t_ptr->bins[i] = bin_t_ptr;
    }
}

arena_t *initialize_arenas() {
    pthread_mutex_lock(&arena_init_lock); //Arena Init Lock
    if (no_of_arenas == no_of_processors) {
        /*Don' Create New Arena*/
        /*Move Global Pointer to Available arena*/
        arena_t *arena_ptr = find_available_arena();
        arena_ptr->no_of_threads++;
        pthread_mutex_unlock(&arena_init_lock);
        return arena_ptr;
    }
    /*Create New Arena*/
    arena_t *thread_arena_ptr = (arena_t *) sbrk(sizeof(arena_t));
    thread_arena_ptr->next = NULL;
    thread_arena_ptr->no_of_threads = 1;

    /*Add arena to Global Arena Linked List*/
    add_arena(thread_arena_ptr);

    /*Initialzie BINS*/
    initialize_bins(thread_arena_ptr);

    /*Increase the no. of Arenas*/
    no_of_arenas += 1;

    pthread_mutex_unlock(&arena_init_lock);
    return thread_arena_ptr;
}


void *init_malloc() {

    if (pthread_atfork(fork_prepare, fork_parent, fork_child) != 0) {
        errno = ENOMEM;
        return NULL;;
    }
    system_page_size = sysconf(_SC_PAGESIZE);
    no_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
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

void free_block(block_t *block_ptr) {
    if (block_ptr->type == BLOCK_MMAP) {
        size_t mem_size = (size_t) (block_ptr->actual_size + sizeof(block_t));
        munmap(block_ptr, mem_size);
    } else {
        //TODO::
        /*
        bin_t *bin_ptr = thread_arena_ptr->bins[block_ptr->bin_type];
        bin_ptr->free_req++;
        bin_ptr->free_blocks++;
        bin_ptr->used_blocks--;
        */
        /*pthread_mutex_lock(&thread_arena_ptr->lock);*/
        block_ptr->block_status = BLOCK_AVAILABLE;
        /*pthread_mutex_unlock(&thread_arena_ptr->lock);*/
    }
}

void printArenas() {
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
                            "---------------BLOCK [%s] [%p] [%s]----------------\n",
                            BlockStatusString[block_itr->block_status],
                            block_itr, BlockTypeString[block_itr->type]);
                    block_itr = block_itr->next;
                }
            }
        }
        printf("-----------END OF Arena-----------\n");
        arena_itr = arena_itr->next;
    }

}


bin_t *add_new_bin(block_t *block_ptr, int bin_index, const void *sbrk_return_addr) {
    bin_t *bin_ptr = (bin_t *) (sbrk_return_addr);
    bin_ptr->blocks_ptr = block_ptr;
    bin_ptr->type = bin_index;
    return bin_ptr;
}

/* Do a byte by byte copy. Make sure to cast the pointers
   as chars so you copy bytes and nothing larger. Loop through
   the amount of bytes you want:
        metadata_t->size - sizeof(metadata_t)
   */
void *my_memcpy(void *dest, const void *src, size_t num_bytes) {
    char *d = (char *) dest;
    char *s = (char *) src;
    for (int i = 0; i < num_bytes; i++) {
        d[i] = s[i];
    }
    return d;
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

    while (memory_to_allocate > (block_size[0] + sizeof(block_t))) {
        for (int i = 0; i < MAX_BINS; i++) {

            if (memory_to_allocate < (block_size[i] + sizeof(block_t))) {
                break;
            }
            /*Memory allocated to the block*/
            size_t mem_allocated = block_size[i] + sizeof(block_t);

            block_t *block_ptr = (block_t *) new_mem_return_addr;
            block_ptr->type = BLOCK_SBRK;
            block_ptr->next = NULL;
            block_ptr->bin_type = bin_index;
            if (block_ptr->bin_type > 2 || bin_index > 2)
                printf("#################creating block bin_type [%d]\n", block_ptr->bin_type);
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

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    block_t *block_ptr = (block_t *) (ptr - sizeof(block_t));
    free_block(block_ptr);
}

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

void fork_prepare() {
    acquire_locks();
}

void fork_parent() {
    release_locks();
}

void fork_child() {
    release_locks();
}

void acquire_locks() {
    pthread_mutex_lock(&arena_init_lock);
    arena_t *arena_ptr = global_arena_ptr;
    while (arena_ptr) {
        pthread_mutex_lock(&arena_ptr->lock);
        arena_ptr = arena_ptr->next;
    }
}

void release_locks() {
    arena_t *arena_ptr = global_arena_ptr;
    while (arena_ptr) {
        pthread_mutex_unlock(&arena_ptr->lock);
        arena_ptr = arena_ptr->next;
    }
    pthread_mutex_unlock(&arena_init_lock);
}

void malloc_stats() {
    arena_t *arena_itr = global_arena_ptr;
    int arena_count = 1;
    printf("============================MALLOC STATS===============================\n");
    while (arena_itr != NULL) {
        printf("============================Arena Info [%d]=============================\n", arena_count++);
        printf("\t Total Size of Arena        : %ld KB\n", compute_malloc_stats(arena_itr));
        printf("\t Total Number of Bins       : %d\n", MAX_BINS);
        printf("======================================================================\n");
        for (int i = 0; i < MAX_BINS; i++) {
            bin_t *bin_ptr = arena_itr->bins[i];
            int free_count = get_total_free_blocks(bin_ptr);
            int used_count = get_total_used_blocks(bin_ptr);
            printf("============================Bin Info [%s]=============================\n",
                   BinTypeString[bin_ptr->type]);
            //TODO: STAT
            printf("\t Total Allocation Request : %d\n", bin_ptr->allc_req);
            printf("\t Total Free Blocks        : %d\n", free_count);
            printf("\t Total USed Blocks        : %d\n", used_count);
            printf("\t Total Number of Blocks   : %d\n", free_count + used_count);
        }
        printf("======================================================================\n");
        arena_itr = arena_itr->next;
    }
    printf("============================END OF MALLOC STATS=============================\n");
}


static long compute_malloc_stats(arena_t *arena_ptr) {
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
                total += block_ptr->actual_size + sizeof(bin_t);
            }
        }
    }
    return total;
}

int get_total_free_blocks(bin_t *bin_ptr) {
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

int get_total_used_blocks(bin_t *bin_ptr) {
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

__attribute__ ((constructor))
void myconstructor() {
    init_malloc();
}


