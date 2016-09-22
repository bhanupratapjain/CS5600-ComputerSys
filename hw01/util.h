#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>

enum HeaderType {
    HEADER_MAP,
    HEADER_CONTEXT
};

typedef struct {
	long sAddr;
	long eAddr;
	long fAddr;
	char perms[512];
	enum HeaderType type;
} header;

void print_mem_headers(header * mem_header_ptr);

int parse_mem_header(header * mem_header_ptr, char * line);

char * get_next_line(FILE *fp);
#endif
