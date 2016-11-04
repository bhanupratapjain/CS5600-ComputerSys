//
// Created by Bhanu on 25/10/2016.
//
#ifndef __MALLOC_H__
#define __MALLOC_H__

#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>

#define MAX_BINS 3
#define BLOCK_USED 0
#define BLOCK_AVAILABLE 1
#define BLOCK_SBRK 0
#define BLOCK_MMAP 1

#define DATA_OFFSET offsetof(struct block_t, actual_size)

typedef struct arena_t {
    struct arena_t *next;
    int no_of_threads;
    struct bin_t *bins[MAX_BINS];
    pthread_mutex_t lock;
} arena_t;

typedef struct bin_t {
    uint8_t type;
    struct block_t *blocks_ptr;
    uint8_t allc_req;
//    uint8_t free_req;
//    uint8_t free_blocks;
//    uint8_t used_blocks;
} bin_t;

typedef struct block_t {
    struct block_t *next;
    uint8_t bin_type;
    uint8_t type;
    uint8_t block_status;
    size_t actual_size;
} block_t;


/*
 * System Page Size
 */
static long system_page_size = 4096;
/*
 * Total No of Processors
 */
static int no_of_processors = 1;
/*
 * Thread Arenas
 */
int no_of_arenas = 0;
static arena_t *global_arena_ptr = NULL;
static __thread arena_t *thread_arena_ptr = NULL;
static pthread_mutex_t arena_init_lock = PTHREAD_MUTEX_INITIALIZER;
static int block_size[] = {8, 64, 512};
static char *BinTypeString[] = {"BIN_8", "BIN_64", "BIN_512"};
static char *BlockTypeString[] = {"BLOCK_SBRK", "BLOCK_MMAP"};
static char *BlockStatusString[] = {"BLOCK_USED", "BLOCK_AVAILABLE"};
static size_t MAX_HEAP_SIZE = 512;

//static void *init_malloc_hook (size_t, const void *);
static void *init_malloc();

arena_t *find_available_arena();

void add_arena(arena_t *arena_ptr);

void add_block_to_bin(bin_t *bin_ptr, block_t *block_ptr);

void initialize_bins(arena_t *arent_t_ptr);

arena_t *initialize_arenas();

int get_index(size_t needed);

block_t *get_unused_block(bin_t *bin_ptr);

void remove_block_from_bin(block_t *block_ptr);

void free_block(block_t *block_ptr);

void printArenas();

bin_t *add_new_bin(block_t *block_ptr, int bin_index, const void *sbrk_return_addr);

int add_blocks(size_t mem_size, int bin_index);

block_t *get_block(bin_t *bin_ptr, size_t mem_size, int bin_index);

void *get_new_memory(size_t mem_size);

block_t *create_new_block(size_t size, int bin_index, const void *block_addr);

void *malloc(size_t size);

void *calloc(size_t nmemb, size_t size);

void free(void *ptr);

void *realloc(void *ptr, size_t size);

void fork_prepare();

void fork_parent();

void fork_child();

void acquire_locks();

void release_locks();

static long compute_malloc_stats(arena_t *);

//__malloc_hook = init_malloc_hook;


#endif
