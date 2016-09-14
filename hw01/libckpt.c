#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

typedef struct
{
	long sAddr;
	long eAddr;
	char * cLine;
} header;

void laodMemHeaders(FILE *fp, header * mem_header_ptrs[]) {
	char * line = NULL;
	line = (char*)malloc(sizeof(char));
	int i = 0, j = 1, k = 0;
	char c;
	while (1)
	{
		if ( feof(fp) )
		{
			printf("End of file reached\n");
			free(line); // important step the pointer declared must be made free
			break;
		}
		c = fgetc(fp);

		if (c == '\n') {
			line[i] = '\0'; // at the end append null character to mark end of string
			mem_header_ptrs[k] = (header*)malloc(sizeof(header));
			mem_header_ptrs[k]->cLine = line;
			// strcpy (mem_header_ptrs[k]->cLine, line);
			mem_header_ptrs[k]->sAddr = 1212;
			printf("End of Line\n");
			// printf("%s\n", line);
			line = NULL;
			i = 0;
			j = 1;
			k++;
			// line
		} else {
			line = (char*)realloc(line, j * sizeof(char));
			// store read character by making pointer point to c
			line[i] = c;
			i++;
			j++;
		}

	}
}

void printMemHeaders(header * mem_header_ptrs[]) {
	int count = 0;
	while (1) {
		if (mem_header_ptrs[count] == NULL) {
			printf("end of structure\n");
			break;
		}
		printf("%s\n", mem_header_ptrs[count]->cLine);
		printf("%ld\n", mem_header_ptrs[count]->sAddr);
		count++;
	}
}

void clearMemHeaders( header * mem_header_ptrs[]) {
	int count = 0;
	while (1) {
		if (mem_header_ptrs[count] == NULL) {
			printf("end of structure\n");
			break;
		}
		free(mem_header_ptrs[count]);
		count++;
	}
}

void sigHandler(int signum)
{
	printf("Caught signal %d\n", signum);
	FILE *fp = NULL;
	header * mem_header_ptrs[40];
	// header * mem_header_add = malloc(sizeof(header);
	printf("Reading Memory Maps for the current porcess.\n");
	fp = fopen("/proc/self/maps", "r");

	if (fp == NULL) {
		perror("Error");
		goto END;
	}

	laodMemHeaders(fp, mem_header_ptrs);
	printMemHeaders(mem_header_ptrs);
	// clearMemHeaders(mem_header_ptrs);


	// free(line); // important step the pointer declared must be made free
	// free(line); // important step the pointer declared must be made free
	printf("Back to Handler\n");

	if (fclose(fp) != 0) {
		printf("error occured\n");
		perror("Error");
		goto END;
	}

END:
	printf("End of Signal Handler\n");
}
__attribute__ ((constructor))
void myconstructor() {
	signal(SIGUSR2, sigHandler);
}

// char[] getLine(fp) {
// 	char[]

// }

