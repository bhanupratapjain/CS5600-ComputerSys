#include "util.h"

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

void print_mem_headers(header * mem_header_ptr) {
	printf("Start Add: %ld\n", mem_header_ptr->sAddr);
	printf("End Add: %ld\n", mem_header_ptr->eAddr);
	printf("Perms: %s\n", mem_header_ptr->perms);
	printf("Type: %s\n", mem_header_ptr->type == HEADER_CONTEXT ? "HEADER_CONTEXT" : "HEADER_MAP");

}

int parse_mem_header(header * mem_header_ptr, char * line) {
	const char CONST_SPACE[2] = " ";
	const char CONST_HYPHEN[2] = "-";
	char *token;
	char *add_range;
	char *add_range_token;


	/*set type to maps*/
	mem_header_ptr->type = HEADER_MAP;

	/* get the address range*/
	token = strtok(line, CONST_SPACE);

	/* copy address range*/
	add_range = token;
	//printf("Address Range %s\n", add_range);

	/*get flags*/
	token = strtok(NULL, CONST_SPACE);
	strcpy(mem_header_ptr->perms,token);
	//printf("Flags %s\n", flags);

	/*parse address range*/
	add_range_token = strtok(add_range, CONST_HYPHEN);
	//printf("start %s\n", add_range_token);
	mem_header_ptr->sAddr = strtol(add_range_token, NULL, 16);

	add_range_token = strtok(NULL, CONST_HYPHEN);
	//printf("end %s\n", add_range_token);
	mem_header_ptr->eAddr = strtol(add_range_token, NULL, 16);

	return 0;

}

char * get_next_line(FILE *fp) {
	char * line = NULL;
	line = (char*) malloc(sizeof(char));
	int i = 0, j = 1;
	char c;
	while (1) {
		if (feof(fp)) {
			printf("End of file reached\n");
			line[i] = '\0'; // at the end append null character to mark end of string
			return line;
		}
		c = fgetc(fp);
		if (c == '\n' ||  c == EOF) {
			line[i] = '\0'; // at the end append null character to mark end of string
			return line;
		} else {
			line = (char*) realloc(line, j * sizeof(char));
			// store read character by making pointer point to c
			line[i] = c;
			i++;
			j++;
		}

	}
}
