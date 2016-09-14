#include <stdio.h>

int main(void) {
	char * name = "bhanu";
	int a = 1;
	name+=1;
	int * b = &a;
	(*b)++;
	printf("%s\n", name);
	printf("%d\n", (*b));
  // printf("Hello World!\n");
  // return 0; 
  }