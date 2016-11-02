#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <ucontext.h>

int main(void) {
	//	char * name = "bhanu";
	//	int a = 1;
	//	name+=1;
	//	int * b = &a;
	//	(*b)++;
	//	printf("%s\n", name);
	//	printf("%d\n", (*b));
	//  // printf("Hello World!\n");
	//  // return 0;

	ucontext_t p_context;
	int fd_out_1;

	printf("Dumping Context\n");
	if (getcontext(&p_context) < 0) {
		printf("Get Context Failed\n");
		perror("Error while getting context.");
		return -1;
	} else {
		printf("Got Context %lu\n", sizeof(p_context));
	}
	fd_out_1 = open("myckpt1", O_CREAT | O_WRONLY | O_APPEND | O_TRUNC,
			S_IRUSR | S_IWUSR | S_IXUSR);
	if (write(fd_out_1, &p_context, sizeof(p_context)) < 0) {
		perror("Error while writing context.");
		return -1;
	}
	printf("Context Dumped Successfully\n");
	return 0;
}
