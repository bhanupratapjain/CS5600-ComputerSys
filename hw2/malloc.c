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

typedef enum {
	BLOCK_SBRK, BLOCK_MMAP
} BlockType;

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
	BlockType type;
	BlockStatus block_status;
	size_t actual_size;
	void * req_addr;
	arena_t * arena_ptr;

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
static char* BinTypeString[] = { "BIN_8", "BIN_64", "BIN_512" };
static char* BlockTypeString[] = { "BLOCK_SBRK", "BLOCK_MMAP" };
static char* BlockStatusString[] = { "BLOCK_USED", "BLOCK_AVAILABLE" };
static size_t MAX_HEAP_SIZE = 512;
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
//	printf("####Inside Add Arena .Global arena [%p]\n", global_arena_ptr);
	if (global_arena_ptr == NULL) {
		global_arena_ptr = arena_ptr;
		return;
	}
	arena_t * arena_itr = global_arena_ptr;
	arena_t * p_arena_itr = NULL;
	while (arena_itr != NULL) {
//		printf("####found arena [%p]\n", arena_itr);
		p_arena_itr = arena_itr;
		arena_itr = arena_itr->next;
	}
	p_arena_itr->next = arena_ptr;
}

void addBlockToBin(bin_t * bin_ptr, block_t * block_ptr) {
	block_t * block_itr = bin_ptr->blocks_ptr;
	block_t * p_block_itr = NULL;
//	printf("Adding Block to Bin\n");
	while (block_itr != NULL) {
		p_block_itr = block_itr;
		block_itr = block_itr->next;
	}
	p_block_itr->next = block_ptr;
}

arena_t * initializeArenas() {
	pthread_mutex_lock(&arena_init_lock); //Arena Init Lock
//	printf("####Inside Init Arena\n");
//	printf("arenas-%d, processors-%d", no_of_arenas, no_of_processors);
	if (no_of_arenas == no_of_processors) {
//		printf("####Existing Arena\n");
		/*Don' Create New Arena*/
		/*Move Global Pointer to Available arena*/
		arena_t * arena_ptr = findAvailableArena();
		pthread_mutex_unlock(&arena_init_lock);
		return arena_ptr;
	}
//	printf("####Creadting new arena\n");
	/*Create New Arena*/
	arena_t * thread_arena_ptr = (arena_t *) sbrk(sizeof(arena_t));
	thread_arena_ptr->next = NULL;
	thread_arena_ptr->status = ARENA_USED;
	thread_arena_ptr->type = ARENA_THREAD;

//	printf("####Adding new arena\n");

	/*Add arena to Global Arena Linked List*/
	addArena(thread_arena_ptr);

//	printf("####New arena added\n");
	/*Increase the no. of Arenas*/
	no_of_arenas += 1;
//	printf("arenas-%d", no_of_arenas);
//	printf("#### Arena Ptr %p", thread_arena_ptr);

	pthread_mutex_unlock(&arena_init_lock);
	return thread_arena_ptr;
}
int intitializeMainArena() {

	system_page_size = sysconf(_SC_PAGESIZE);
	no_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
//	no_of_processors = 1;
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
		memory *= memory;
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

void freeBlock(block_t *block_ptr) {
	printf("inside free block\n");
	block_ptr->block_status = BLOCK_AVAILABLE;
	if (block_ptr->type == BLOCK_MMAP) {
		size_t mem_size = block_ptr->actual_size + sizeof(block_t);
		printf("Deallocating from addr [%p] with size[%zu], actual[%zu]\n",
				block_ptr, mem_size, block_ptr->actual_size);
		munmap(block_ptr, mem_size);
	} else {
		block_ptr->arena_ptr->no_of_threads--;
	}

}

void printArenas() {
//	printf("Printing Arenas\n");
	arena_t * arena_itr = global_arena_ptr;
	int arena_count = 1;
	printf("-----------Global Arena [%p]\n", global_arena_ptr);
	while (arena_itr != NULL) {
		printf("-----------Arena %d [%p]-----------\n", arena_count++,
				arena_itr);

		for (int i = 0; i < MAX_BINS; i++) {
			bin_t * bin_ptr = arena_itr->bins[i];
			if (bin_ptr != NULL) {
				printf("-------------BIN [%s] [%p] ----------------\n",
						BinTypeString[bin_ptr->type], bin_ptr);
				block_t * block_itr = bin_ptr->blocks_ptr;
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

void *calloc(size_t nmemb, size_t size) {

	void * add_ptr = malloc(nmemb * size);

	if (add_ptr == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	block_t * block_ptr = (block_t *) (add_ptr - sizeof(block_t));
	memset(add_ptr, 0, block_ptr->actual_size);

	return add_ptr;
}

void *malloc(size_t size) {

	/*Initialize Main Arena*/
	if (intitializeMainArena() < 0) {
		errno = ENOMEM;
		return NULL;
	}
	//	printf("####Main thread initialized\n");

	block_t * block_ptr = NULL;

	/*MMAP Allocation*/
	if (size > MAX_HEAP_SIZE) {
		size_t mem_size = size + sizeof(block_t);

		void *ret = mmap(0, mem_size, PROT_READ | PROT_WRITE,
				MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		if (ret == MAP_FAILED) {
			errno = ENOMEM;
			return NULL;
		}
		block_ptr = (block_t *) ret;
		block_ptr->actual_size = size;
		block_ptr->type = BLOCK_MMAP;
		block_ptr->block_status = BLOCK_USED;

		printf("%s:%d malloc(%zu): Allocated %zu bytes at %p\n",
		__FILE__, __LINE__, size, mem_size,
				(void *) block_ptr + sizeof(block_t));

	} else {
		/*SBRK Allocation*/
		/*Initialize Arena Pool*/
		arena_t * arena_ptr = initializeArenas();
		//	printf("#### Arena Ptr %p", arena_ptr);
		if (arena_ptr == NULL) {
			errno = ENOMEM;
			return NULL;
		}

		//	printf("####Thread arena initialized\n");

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

			block_ptr = (block_t *) (sbrk_return_addr + sizeof(bin_t));
			block_ptr->next = NULL;
			block_ptr->bin_type = bin_type_array[bin_index];
			block_ptr->block_status = BLOCK_USED;
			block_ptr->actual_size = size;
			block_ptr->req_addr = sbrk_return_addr + sizeof(bin_t)
					+ sizeof(block_t);
			block_ptr->arena_ptr = arena_ptr;
			block_ptr->type = BLOCK_SBRK;

			bin_t * bin_ptr = (bin_t *) (sbrk_return_addr);
			bin_ptr->blocks_ptr = block_ptr;
			bin_ptr->type = bin_type_array[bin_index];

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

				block_ptr = (block_t *) (sbrk_return_addr + sizeof(bin_t));
				block_ptr->next = NULL;
				block_ptr->bin_type = bin_type_array[bin_index];
				block_ptr->block_status = BLOCK_USED;
				block_ptr->actual_size = size;
				block_ptr->req_addr = sbrk_return_addr + sizeof(bin_t)
						+ sizeof(block_t);
				block_ptr->arena_ptr = arena_ptr;
				block_ptr->type = BLOCK_SBRK;
				addBlockToBin(bin_ptr, block_ptr);

			} else {
				/*Free Block Found*/
				block_ptr->block_status = BLOCK_USED;
			}

		}
		//	printf("block addr=%p \n", block_ptr);
		//	printf("aread addr=%p \n", block_ptr->arena_ptr);
		//	printf("bin index=%d\n", bin_index);
		//	printf("bin bin_type_array=%s\n", BinTypeString[bin_type_array[bin_index]]);
		//	printf("bin index=%s\n", bin_type_array[bin_index]);

		printf("%s:%d malloc(%zu): Allocated %zu bytes at %p\n",
		__FILE__, __LINE__, size, block_size[bin_index],
				(void *) block_ptr + sizeof(block_t));
	}

//	printArenas();
	/*Free Block Found*/
	return (void *) block_ptr + sizeof(block_t);

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

	if (ptr == NULL) {
		return;
	}
//	printf("free call\n");
//	printf("raw add %p = \n", ptr);
	block_t * block_ptr = (block_t *) (ptr - sizeof(block_t));
//	printf("Block ptr addre = %p\n", block_ptr);
	printf("%s:%d free(%p): Freeing %zu bytes from %p\n",
	__FILE__, __LINE__, ptr, block_ptr->actual_size, block_ptr);
	freeBlock(block_ptr);
//	if (block_ptr != NULL)
//		printArenas();

//	printf("block free successfull\n");

}

void *realloc(void *ptr, size_t size) {
// Allocate new memory (if needed) and copy the bits from old location to new.

	/*If size==0 and ptr exists the it is the call for free*/
	if (size == 0 && ptr != NULL) {
		free(ptr);
		return NULL;
	}

	void * new_mem_add = malloc(size);

	/*if ptr is NULL then it the call for malloc*/
	if (ptr == NULL) {
		return new_mem_add;
	}

	/*realloc call*/
	block_t * old_block_ptr  = (block_t *)(ptr - sizeof(block_t));
	memcpy(new_mem_add,ptr,old_block_ptr->actual_size);
	free(ptr);
	return new_mem_add;
}

