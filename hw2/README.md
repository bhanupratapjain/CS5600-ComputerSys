# Custom Malloc Library

Custom Malloc Library supporting providing the following api:

```
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
```


export LD_PRELOAD=/home/bhanupratapjain/CS5600-ComputerSys/hw2/libmalloc.so test1
