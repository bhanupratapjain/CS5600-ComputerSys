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
//	printf("####Inside Add Arena .Global arena [%p]\n", global_arena_ptr);
    if (global_arena_ptr == NULL) {
        global_arena_ptr = arena_ptr;
        return;
    }
    arena_t *arena_itr = global_arena_ptr;
    arena_t *p_arena_itr = NULL;
    while (arena_itr != NULL) {
//		printf("####found arena [%p]\n", arena_itr);
        p_arena_itr = arena_itr;
        arena_itr = arena_itr->next;
    }
    p_arena_itr->next = arena_ptr;
}

void add_block_to_bin(bin_t *bin_ptr, block_t *block_ptr) {
    block_t *block_itr = bin_ptr->blocks_ptr;
    block_t *p_block_itr = NULL;
//	printf("Adding Block to Bin\n");

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
        arent_t_ptr->bins[i] = bin_t_ptr;
    }
}

arena_t *initialize_arenas() {
    pthread_mutex_lock(&arena_init_lock); //Arena Init Lock
//	printf("####Inside Init Arena\n");
//	printf("arenas-%d, processors-%d", no_of_arenas, no_of_processors);
    if (no_of_arenas == no_of_processors) {
//		printf("####Existing Arena\n");
        /*Don' Create New Arena*/
        /*Move Global Pointer to Available arena*/
        arena_t *arena_ptr = find_available_arena();
        pthread_mutex_unlock(&arena_init_lock);
        return arena_ptr;
    }
//	printf("####Creadting new arena\n");
    /*Create New Arena*/
    arena_t *thread_arena_ptr = (arena_t *) sbrk(sizeof(arena_t));
    thread_arena_ptr->next = NULL;
    thread_arena_ptr->type = ARENA_THREAD;

    /*Add arena to Global Arena Linked List*/
    add_arena(thread_arena_ptr);

    /*Initialzie BINS*/
    initialize_bins(thread_arena_ptr);

//	printf("####New arena added\n");
    /*Increase the no. of Arenas*/
    no_of_arenas += 1;
//	printf("arenas-%d", no_of_arenas);
//	printf("#### Arena Ptr %p", thread_arena_ptr);

    pthread_mutex_unlock(&arena_init_lock);
    return thread_arena_ptr;
}

int initialize_main_arena() {

    system_page_size = sysconf(_SC_PAGESIZE);
    no_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
    /*arena_t main_thread_arena = { 0 };

     main_thread_arena.next = NULL;
     main_thread_arena.status = ARENA_USED;
     main_thread_arena.type = ARENA_MAIN;

     block_t * block_ptr = NULL;
     block_ptr = (block_t *) sbrk(SYSTEM_PAGE_SIZE);
     if (block_ptr == NULL) {
     errno = ENOMEM;
     return -1;
     }
     block_ptr->next = NULL;
     block_ptr->bin_type = BIN_8;

     //	bin_t = (bin_t *) ((char*) block_ptr + sizeof(block_t));
     bin_t bin = { 0 };
     bin.blocks_ptr = block_ptr;
     bin.type = BIN_8;

     main_thread_arena.bins[0] = &bin;*/

    return 0;
}

int get_index(size_t needed) {
//    printf("size to allocate [%zu]\n", needed);
    int index = 0;
    int memory = 8; //Min. Memory Allocated
    while (memory < needed) {
        if (index == 3) {
            break;//Case for memory greater than 512, goes to MMAP bin at index 3
        }
        memory *= 8;
        index++;
    }
    return index;
}

block_t *get_unused_block(bin_t *bin_ptr) {

    /*Do NOt Search for MMAP Blocks*/
//    if (bin_ptr->type == BIN_MMAP) {
//        return NULL;
//    }

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
//    printf("inside free block\n");
    pthread_mutex_lock(&thread_arena_ptr->lock);
    thread_arena_ptr->no_of_threads--;
    if (block_ptr->type == BLOCK_MMAP) {
        size_t mem_size = block_ptr->actual_size + sizeof(block_t);
//        printf("Deallocating from addr [%p] with size[%zu], actual[%zu]\n",
//               block_ptr, mem_size, block_ptr->actual_size);
        remove_block_from_bin(block_ptr);
        munmap(block_ptr, mem_size);
    } else {
        block_ptr->block_status = BLOCK_AVAILABLE;
    }
    pthread_mutex_unlock(&thread_arena_ptr->lock);
}

void printArenas() {
//	printf("Printing Arenas\n");
    arena_t *arena_itr = global_arena_ptr;
    int arena_count = 1;
    printf("-----------Global Arena [%p]\n", global_arena_ptr);
    while (arena_itr != NULL) {
        printf("-----------Arena %d [%p]-----------\n", arena_count++,
               arena_itr);

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

//    printf("Adding blocks with req size[%zu], bin_index[%d]\n", mem_size, bin_index);
    /*Get New Memory*/
    void *new_mem_return_addr = get_new_memory(mem_size, bin_index);

//    printf("got memory @ [%p]\n", new_mem_return_addr);

    if (new_mem_return_addr == NULL) {
        errno = ENOMEM;
        return -1;
    }

    /*Allocate New Memory to BINS*/
    if (bin_index == BIN_MMAP) {
//        printf("Allocating MMAP Memory\n");
        /*Allocate MMAP Memory*/
        block_t *block_ptr = (block_t *) new_mem_return_addr;
        block_ptr->type = BLOCK_MMAP;
        block_ptr->next = NULL;
        block_ptr->bin_type = bin_index;
        block_ptr->block_status = BLOCK_AVAILABLE;

        bin_t *bin_ptr = thread_arena_ptr->bins[BIN_MMAP];
        add_block_to_bin(bin_ptr, block_ptr);

    } else {
        /*Allocate SBRK Memory*/
        /*Divide memory among all the Bins*/
        /*Total Memory Size to allocate to Bins */
//        printf("Allocating SBRK Memory\n");
        int memory_to_allocate = system_page_size;

        while (memory_to_allocate > (block_size[0] + sizeof(block_t))) {
            for (int i = 0; i < MAX_BINS - 1; i++) {

                if (memory_to_allocate < (block_size[i] + sizeof(block_t))) {
                    break;
                }

//                printf("memory_to_allocate [%zu]\n", memory_to_allocate);

//                printf("Creating Block for BIN - %s\n", BinTypeString[i]);
                /*Memory allocated to the block*/
                size_t mem_allocated = block_size[i] + sizeof(block_t);

                block_t *block_ptr = (block_t *) new_mem_return_addr;
                block_ptr->type = BLOCK_SBRK;
                block_ptr->next = NULL;
                block_ptr->bin_type = bin_index;
                block_ptr->block_status = BLOCK_AVAILABLE;

                bin_t *bin_ptr = thread_arena_ptr->bins[i];
                add_block_to_bin(bin_ptr, block_ptr);

                new_mem_return_addr += mem_allocated;
                memory_to_allocate -= mem_allocated;

            }

        }
//        printArenas();
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
    block_ptr->block_status = BLOCK_USED;
    block_ptr->actual_size = (mem_size - sizeof(block_t));
    return block_ptr;
}

void *malloc(size_t size) {
//    printf("malloc size[%zu], thread[%lu]\n", size, pthread_self());
//    printf("arena[%zu],bin[%zu],block[%zu]\n", sizeof(arena_t), sizeof(bin_t), sizeof(block_t));
    /*Initialize Main Arena*/
    if (initialize_main_arena() < 0) {
        errno = ENOMEM;
        return NULL;
    }
    //	printf("####Main thread initialized\n");

    block_t *block_ptr = NULL;

    /*Initialize Arenas*/
    if (thread_arena_ptr == NULL) {
        arena_t *arena_ptr = initialize_arenas();
        //	printf("#### Arena Ptr %p", arena_ptr);
        if (arena_ptr == NULL) {
            errno = ENOMEM;
            return NULL;
        }
        thread_arena_ptr = arena_ptr;
    }

    size_t mem_size = size + sizeof(block_t);

    /*Get the bin index based on user requested memory + block size*/
    int bin_index = get_index(mem_size);

//    printf("bin index[%d]\n", bin_index);
    pthread_mutex_lock(&thread_arena_ptr->lock);

    /*Increase thread count for arena*/
    thread_arena_ptr->no_of_threads++;

    bin_t *bin_ptr = thread_arena_ptr->bins[bin_index];


    /*Get Block*/
    block_ptr = get_block(bin_ptr, mem_size, bin_index);
    if (block_ptr == NULL) {
        errno = ENOMEM;
        return NULL;
    }

//    printf("%s:%d malloc(%zu): Allocated %zu bytes at %p\n",
//           __FILE__, __LINE__, size, mem_size,
//           (void *) block_ptr + sizeof(block_t));

    //	printf("block addr=%p \n", block_ptr);
    //	printf("aread addr=%p \n", block_ptr->thread_arena_ptr);
    //	printf("bin index=%d\n", bin_index);
    //	printf("bin bin_type_array=%s\n", BinTypeString[bin_type_array[bin_index]]);
    //	printf("bin index=%s\n", bin_type_array[bin_index]);

    /*Release arena lock*/
    pthread_mutex_unlock(&thread_arena_ptr->lock);

//    printArenas();
    /*Free Block Found*/
    return (void *) block_ptr + sizeof(block_t);
}

void *get_new_memory(size_t mem_size, int bin_index) {
    void *new_mem_return_addr = NULL;
    if (bin_index == BIN_MMAP) {
        /*Get Memory From MMAP*/
        new_mem_return_addr = mmap(0, mem_size, PROT_READ | PROT_WRITE,
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

block_t *create_new_block(size_t size, int bin_index, const void *block_addr) {
    block_t *block_ptr = (block_t *) block_addr;
    block_ptr->next = NULL;
    block_ptr->bin_type = bin_index;
    block_ptr->block_status = BLOCK_USED;
    block_ptr->actual_size = size;
    block_ptr->type = (bin_index == BIN_MMAP) ? BLOCK_MMAP : BLOCK_SBRK;
    return block_ptr;
}


void free(void *ptr) {
//    printf("free ptr[%p],[%lu]\n", ptr, pthread_self());
    if (ptr == NULL) {
        return;
    }
//	printf("free call\n");
//	printf("raw add %p = \n", ptr);
    block_t *block_ptr = (block_t *) (ptr - sizeof(block_t));
//	printf("Block ptr addre = %p\n", block_ptr);
//    printf("%s:%d free(%p): Freeing %zu bytes from %p\n",
//           __FILE__, __LINE__, ptr, block_ptr->actual_size, block_ptr);
    free_block(block_ptr);
//    printArenas();

//	printf("block free successfull\n");

}

void *realloc(void *ptr, size_t size) {
// Allocate new memory (if needed) and copy the bits from old location to new.
//    printf("***realloc pointer[%p], size[%zu], thread[%lu]\n", ptr, size, pthread_self());
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
//    printf("***doing memset, thread[%lu]\n", pthread_self());
    pthread_mutex_lock(&thread_arena_ptr->lock);
    block_t *old_block_ptr = (block_t *) (ptr - sizeof(block_t));
//    printf("***copying mem[%zu], thread[%lu]\n", old_block_ptr->actual_size, pthread_self());
    /* Copy as many bytes as are available from the old block
	             and fit into the new size.  */
    if (size > old_block_ptr->actual_size)
        size = old_block_ptr->actual_size;
    memcpy(new_mem_add, ptr, size);
    pthread_mutex_unlock(&thread_arena_ptr->lock);
    free(ptr);
    return new_mem_add;
}

