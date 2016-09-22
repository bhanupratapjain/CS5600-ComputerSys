#include<stdio.h>
#include<stdlib.h>

void recurs() {
	int i = 1;
		while (1) {
			char * heap = (char *) (malloc(1000 * 1000));
			printf("heap test %d\n", i);
			i++;
			recurs();
		}

}

void main() {
	recurs();
}
