#include <stdio.h>
#include <unistd.h>

void main() {
	// signal(SIGUSR2, sighandler);

	int a = 0;
	while (1) {
		a += 1;
		printf("new number %d\n", a);
		sleep(1);
		}
}
	