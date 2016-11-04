# Custom Malloc Library

Custom Malloc Library supporting providing the following api:

```
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void malloc_stats();
```



## Data Structures

##### Arena 
`arenat_t` 
Arena act as the main memory pool for thread. There can be one/more threads per arena. The total number of arenas cannot be greater than the number of processors.


##### Bin *(free list)*
`bin_t`
Bins are the part of arena and act as free list. There are bins only for `sbrk` memory, while `mmap` does not maintain any bin. Current implementation has `8`,`64`,`512` bins. The code can support dynamic bin sizes.

##### Block
`block_t`
Blocks are the part of bins. Actual memory allocation is represented by the block. Both `sbrk` and `mmap` are represented by blocks.

## Strategy

- Initalization is done through constructor, where `pthread_atfork` is hooked along with other inits.
- For every `malloc` call:
    - Create a new arena if the total arenas is less then the number of processors else share.
        - While creating a new arena, initialize the bins.
    - If the requested memory is more than `HEAP_LIMIT` (*512*), then allocated memory through `mmap`
    - Else, request memory from existing `heap` using `sbrk`, thus increasing it.
    - While using `sbrk` memory, look for available blocks in respective bin. If no block is free, request new memory(*`PAGE_SIZE`*) from heap and equally divide this among all the bins of the arena.
    

##### Block Allocation
Whenever there is no available block in `bin`, existing heap is increased by a memory `PAGE` and this new memory is divided in blocks and distributed across all the bin of the respected arena.

##### Thread Safety

It is achieved by two locks

- `global lock`: Used during initialization.
- `arena lock`: Used during arena specific operations.

##### Fork Safety
To get a hold of all the free block in the child thread of all arenas, before fork, we unlock all the arenas and then these arenas are passed the child process. To achieve this we use `pthread_atfork` during init, and inject the following
   
- `fork_prepare` : Acquire all arena locks by parent in parent process space before fork.
- `fork_parent` : Release all arena locks by parent in parent process space after fork.
- `fork_child`: Release all arena locks by child in child process space after fork.




## Know Issues

If we try to overload the block/bin header, the performance is poor. 

### Future Work

- Performance improvements. 
- Implement `hooks`
- Buddy Allocation
