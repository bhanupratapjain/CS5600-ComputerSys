#include <stdio.h>
#include <unistd.h>

void main() {
	int a = 0;
	while (1) {
		a += 1;
		printf("Number %d\n", a);
		sleep(1);
		}
}
	