#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
     size_t size1 = 7;
     size_t size2 = 50;
     void *mem1 = malloc(size1);
     void *mem2 = malloc(size2);
     printf("Successfully malloc'd %zu bytes at addr %p\n", size1, mem1);
     printf("Successfully malloc'd %zu bytes at addr %p\n", size2, mem2);
     assert(mem1 != NULL);
     assert(mem2 != NULL);
     free(mem1);
     free(mem2);
     printf("Successfully free'd %zu bytes from addr %p\n", size1, mem1);
     printf("Successfully free'd %zu bytes from addr %p\n", size2, mem2);
     mem1 = malloc(size1);
     printf("Successfully malloc'd %zu bytes at addr %p\n", size1, mem1);
     return 0;

    /*size_t size1 = 513;
    size_t size2 = 600;
    void *mem1 = malloc(size1);
    printf("Successfully malloc'd %zu bytes at addr %p\n", size1, mem1);
    assert(mem1 != NULL);
    mem1 = realloc(mem1, size2);
    printf("Successfully reallocat'd %zu bytes from addr %p\n", size2, mem1);
	free(mem1);
    */printf("Successfully free'd %zu bytes from addr %p\n", size1, mem1);

}
