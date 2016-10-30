#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>

#define  MAX_BINS 3

typedef enum {
	ARENA_USED, ARENA_AVAILABLE
} ArenaStatus;

typedef enum {
	BLOCK_USED, BLOCK_AVAILABLE
} BlockStatus;

typedef enum {
	ARENA_MAIN, ARENA_THREAD
} ArenaType;

typedef enum {
	BIN_8, BIN_64, BIN_512
} BinType;

typedef struct MallocHeader {
	size_t size;
} MallocHeader;

typedef struct {
	struct arena_t * next;
	ArenaStatus status;
	ArenaType type;
	int main_thread_arena;
	int no_of_threads;
	struct bin_t * bins[MAX_BINS];
	pthread_mutex_t lock;
} arena_t;

typedef struct {
	BinType type;
	struct block_t * blocks_ptr;
} bin_t;

typedef struct {
	struct block_t * next;
	BinType bin_type;
	BlockStatus block_status;
	size_t actual_size;
	void * req_addr;

} block_t;

arena_t * initializeArenas();
int intitializeMainArena();

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
static __thread arena_t * global_arena_ptr = NULL;
static pthread_mutex_t arena_init_lock = PTHREAD_MUTEX_INITIALIZER;
static BinType bin_type_array[] = { BIN_8, BIN_64, BIN_512 };
static int block_size[] = { 8, 64, 512 };

arena_t * findAvailableArena() {
	arena_t * arena_itr = global_arena_ptr;
	arena_t * arena_final_ptr = global_arena_ptr;
	while (arena_itr != NULL) {

		if (arena_itr->no_of_threads < arena_final_ptr->no_of_threads) {
			arena_final_ptr = arena_itr;
		}
		arena_itr = arena_itr->next;
	}
	return arena_final_ptr;
}

void addArena(arena_t * arena_ptr) {
	arena_t * arena_itr = global_arena_ptr;
	while (arena_itr != NULL) {
		arena_itr = arena_itr->next;
	}
	global_arena_ptr = arena_ptr;
}

void addBlockToBin(bin_t * bin_ptr, block_t * block_ptr) {
	block_t * block_itr = bin_ptr->blocks_ptr;
	while (block_itr != NULL) {
		block_itr = block_itr->next;
	}
	block_itr->next = block_ptr;
}

arena_t * initializeArenas() {
//	pthread_mutex_lock(&arena_init_lock); //Arena Init Lock
	if (no_of_arenas == no_of_processors) {
		/*Don' Create New Arena*/
		/*Move Global Pointer to Available arena*/
		arena_t * arena_ptr = findAvailableArena();
		pthread_mutex_unlock(&arena_init_lock);
		return arena_ptr;
	}
	/*Create New Arena*/
	arena_t thread_arena = { 0 };
	thread_arena.next = NULL;
	thread_arena.status = ARENA_USED;
	thread_arena.type = ARENA_THREAD;

	/*Add arena to Global Arena Linked List*/
	addArena(&thread_arena);
	/*Increase the no. of Arenas*/
	no_of_arenas++;
//	pthread_mutex_unlock(&arena_init_lock);
	return &thread_arena;
}
int intitializeMainArena() {

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
	int index = 0;
	int memory = 8; //Min. Memory Allocated
	while (memory < needed) {
		memory *= 2;
		index++;
	}
	return index;
}

block_t * get_unused_block(bin_t * bin_ptr) {

	block_t * block_itr = bin_ptr->blocks_ptr;

	while (block_itr != NULL) {
		if (block_itr->block_status == BLOCK_AVAILABLE) {
			return block_itr;
		}
		block_itr = block_itr->next;
	}

	/*No Free Block Found*/
	return NULL;

}

void *calloc(size_t nmemb, size_t size) {
	return NULL;
}

void *malloc(size_t size) {

	/*Initialize Main Arena*/
	if (intitializeMainArena() < 0) {
		errno = ENOMEM;
		return NULL;
	}

	printf("####Main thread initialized\n");

	/*Initialize Arena Pool*/
	arena_t * arena_ptr = initializeArenas();
	if (arena_ptr == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	printf("####Thread arena initialized\n");


	block_t * block_ptr = NULL;
	/*Increase thread count for arena*/
	arena_ptr->no_of_threads++;

	/*Get the bin index based on user requested memory*/
	int bin_index = get_index(size);

	if (arena_ptr->bins[bin_index] == NULL) {
		/*Bin not present, create new bin and request for block*/
		size_t mem_size = block_size[bin_index] + sizeof(bin_t)
				+ sizeof(block_t);

		void * sbrk_return_addr = sbrk(mem_size);
		if (sbrk_return_addr == NULL) {
			errno = ENOMEM;
			return NULL;
		}

		block_ptr = (block_t *) (sbrk_return_addr + sizeof(bin_t)
				+ sizeof(block_t));
		block_ptr->next = NULL;
		block_ptr->bin_type = bin_type_array[bin_index];
		block_ptr->block_status = BLOCK_USED;
		block_ptr->actual_size = size;
		block_ptr->req_addr = sbrk_return_addr + sizeof(bin_t)
				+ sizeof(block_t);

		bin_t * bin_ptr = (bin_t *) (sbrk_return_addr);
		bin_t bin = { 0 };
		bin.blocks_ptr = block_ptr;
		bin.type = BIN_8;

		/*Add bin to arena*/
		arena_ptr->bins[bin_index] = bin_ptr;

	} else {
		/*Found bin, search for block*/
		bin_t * bin_ptr = arena_ptr->bins[bin_index];

		block_ptr = get_unused_block(arena_ptr->bins[bin_index]);
		if (block_ptr == NULL) {
			/*No Free Block found. Create New Block*/
			size_t mem_size = block_size[bin_index] + sizeof(block_t);

			void * sbrk_return_addr = sbrk(mem_size);
			if (sbrk_return_addr == NULL) {
				errno = ENOMEM;
				return NULL;
			}

			block_ptr = (block_t *) (sbrk_return_addr + sizeof(bin_t)
					+ sizeof(block_t));
			block_ptr->next = NULL;
			block_ptr->bin_type = bin_type_array[bin_index];
			block_ptr->block_status = BLOCK_USED;
			block_ptr->actual_size = size;
			block_ptr->req_addr = sbrk_return_addr + sizeof(bin_t)
					+ sizeof(block_t);

			addBlockToBin(bin_ptr, block_ptr);

		}
		/*Free Block Found*/

	}

	printf("%s:%d malloc(%zu): Allocated %zu bytes at %p\n",
	__FILE__, __LINE__, size, block_size[bin_index],
			block_ptr + sizeof(block_t));

	/*Free Block Found*/
	return block_ptr + sizeof(block_t);

	/*// TODO: Validate size.
	 size_t allocSize = size + sizeof(MallocHeader);

	 void *ret = mmap(0, allocSize, PROT_READ | PROT_WRITE,
	 MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	 assert(ret != MAP_FAILED);

	 printf("%s:%d malloc(%zu): Allocated %zu bytes at %p\n",
	 __FILE__, __LINE__, size, allocSize, ret);

	 MallocHeader *hdr = (MallocHeader*) ret;
	 hdr->size = allocSize;

	 return ret + sizeof(MallocHeader);*/
}

void free(void *ptr) {
	MallocHeader *hdr = ptr - sizeof(MallocHeader);
	printf("%s:%d free(%p): Freeing %zu bytes from %p\n",
	__FILE__, __LINE__, ptr, hdr->size, hdr);
	munmap(hdr, hdr->size);
}

void *realloc(void *ptr, size_t size) {
// Allocate new memory (if needed) and copy the bits from old location to new.

	return NULL;
}

