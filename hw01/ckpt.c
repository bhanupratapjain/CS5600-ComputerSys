#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <ucontext.h>
#include "util.h"
#include <sys/resource.h>

int checkpoint_flag = 0;

// byte-size value to hold some combination of the above 8 options
int process_mem_headers(FILE *fp_in, int fd_out, header * mem_header);

void create_mem_headers(FILE *fp, header * mem_header_ptrs[]) {
	char * line = NULL;
	line = (char*) malloc(sizeof(char));
	int i = 0, j = 1, k = 0;
	char c;
	while (1) {
		if (feof(fp)) {
			printf("End of file reached\n");
			free(line); // important step the pointer declared must be made free
			break;
		}
		c = fgetc(fp);

		if (c == '\n') {
			line[i] = '\0'; // at the end append null character to mark end of string

			if (strstr(line, "vvar") != NULL) {
				printf("Skipping memory [vvar]\n");
				goto END_OF_LINE;
			}
			if (strstr(line, "vdso") != NULL) {
				printf("Skipping memory [vdso]\n");
				goto END_OF_LINE;
			}
			if (strstr(line, "vsyscall") != NULL) {
				printf("Skipping memory [vsyscall]\n");
				goto END_OF_LINE;
			}

			mem_header_ptrs[k] = (header*) malloc(sizeof(header));
			parse_mem_header(mem_header_ptrs[k], line);

			END_OF_LINE: printf("End of Line\n");
			line = NULL;
			i = 0;
			j = 1;
			k++;
			// line
		} else {
			line = (char*) realloc(line, j * sizeof(char));
			// store read character by making pointer point to c
			line[i] = c;
			i++;
			j++;
		}

	}
}

void clearMemHeaders(header * mem_header_ptrs[]) {
	int index = 0;
	while (1) {
		if (mem_header_ptrs[index] == NULL) {
			printf("end of structure\n");
			break;
		}
		free(mem_header_ptrs[index]);
		index++;
	}
}

void print_rlimit(struct rlimit *r, const char *name) {
	int64_t cur; /* Soft limit */
	int64_t max; /* Hard limit */
	cur = r->rlim_cur;
	max = r->rlim_max;
	printf("RLIMIT_%s :rlim_cur => %#lx, :rlim_max => %#lx\n", name, cur, max);
}

ssize_t write_with_retry(int fd, const void *buf, size_t count) {
	const char *ptr = (const char *) buf;
	printf("given buff%p\n", buf);
	size_t num_written = 0;
	do {
		printf("buff:%p size:%ld \n", ptr + num_written, count - num_written);
		ssize_t rc = write(fd, ptr + num_written, count - num_written);
		if (rc == -1) {
			if (errno == EINTR || errno == EAGAIN) {
				continue;
			} else {
				perror("Error in write retry");
				return rc;
			}
		} else if (rc == 0)
			break;
		else
			// else rc > 0
			num_written += rc;
	} while (num_written < count);
	return num_written;
}

int write_mem_headers(int fd, header * mem_header_ptr) {
	if (write(fd, (void *) mem_header_ptr, sizeof(header)) < 0) {
		perror("Error while writing memory header.");
		return -1;
	}
	size_t data_size = mem_header_ptr->eAddr - mem_header_ptr->sAddr;
	size_t data_size_written = write_with_retry(fd,
			(void *) mem_header_ptr->sAddr, data_size);
	printf("data size:%ld, data_size_written:%ld\n", data_size,
			data_size_written);
	return 0;
}

int process_mem_headers(FILE *fp_in, int fd_out, header * mem_header) {
	printf("process_mem_headers\n");
	size_t bufsize = 512;
	char * line = NULL;
	while (1) {
		if (feof(fp_in)) {
			printf("End of file reached\n");
			break;
		}
		getline(&line, &bufsize, fp_in);
		printf("line %s\n", line);

//		if (strstr(line, "heap") != NULL) {
//			printf("Skipping memory [heap]\n");
//			continue;
//		}
		if (strstr(line, "vvar") != NULL) {
			printf("Skipping memory [vvar]\n");
			goto CLEAR_HEADER;
		}
		if (strstr(line, "vdso") != NULL) {
			printf("Skipping memory [vdso]\n");
			goto CLEAR_HEADER;
		}
		if (strstr(line, "vsyscall") != NULL) {
			printf("Skipping memory [vsyscall]\n");
			goto CLEAR_HEADER;
		}

		if (parse_mem_header(mem_header, line) < 0) {
			perror("Error while parsing memory header.");
			return -1;
		}

		print_mem_headers(mem_header);
		if (strstr(mem_header->perms, "r") == NULL) {
			printf("Memory Not readable. Skipping\n");
			goto CLEAR_HEADER;
		}

		if (write_mem_headers(fd_out, mem_header) < 0) {
			perror("Error while writing memory to checkpoint.");
			return -1;
		}

		CLEAR_HEADER: memset(mem_header, 0, sizeof(header));
	}
	return 0;
}

int dump_context(int fd_out, ucontext_t * p_context_ptr) {
	header * context_header = (header*) malloc(sizeof(header));
	context_header->type = HEADER_CONTEXT;
	context_header->sAddr = 00000001;
	context_header->eAddr = 00000001;
	context_header->fAddr = (long) &checkpoint_flag;

	printf("Dumping Context\n");

	if (write(fd_out, context_header, sizeof(header)) < 0) {
		perror("Error while writing context header.");
		return -1;
	}

	if (write(fd_out, p_context_ptr, sizeof(ucontext_t)) < 0) {
		perror("Error while writing context data.");
		return -1;
	}
	printf("Context Dumped Successfully\n");
	return 0;
}

void sig_handler(int signum) {
	printf("Caught signal %d\n", signum);
	FILE *fp_in = NULL;
	int fd_out;
	ucontext_t p_context;
	header mem_header = { 0 };
	printf("Reading Memory Maps for the current porcess.\n");
	fp_in = fopen("/proc/self/maps", "r");
	fd_out = open("myckpt", O_WRONLY | O_CREAT | O_TRUNC,
			S_IRWXU | S_IRGRP | S_IROTH);

	if (fp_in == NULL) {
		perror("Error while opening Input Memory Maps");
		goto END;
	}

	if (fd_out < 0) {
		perror("Error while opening output checkpoint file");
		goto END;
	}

//	create_mem_headers(fp_in, mem_header_ptrs);
	printf("Checkpoint flag::%p and value::%d\n", &checkpoint_flag,
			checkpoint_flag);
	if (process_mem_headers(fp_in, fd_out, &mem_header) < 0) {
		perror("Error while dumping memory");
		goto FILE_CLOSE;
	}
	if (getcontext(&p_context) < 0) {
		printf("Get Context Failed\n");
		perror("Error while getting context.");
		goto FILE_CLOSE;
	}
	if (checkpoint_flag < 0) {
//		printf("got back from restart\n");
//		printf("Checkpoint flag::%p and value::%d\n", &checkpoint_flag,
//				checkpoint_flag);
		return;
	}
	if (dump_context(fd_out, &p_context) < 0) {
		perror("Error while dumping context");
		goto FILE_CLOSE;
	}
	printf("Back to Handler\n");
	FILE_CLOSE: if (fclose(fp_in) != 0) {
		perror("Error occurred while closing input file.");
		goto END;
	}
	if (close(fd_out) != 0) {
		perror("Error occurred while closing output file.");
		goto END;
	}

	END: printf("End of Signal Handler\n");
}
__attribute__ ((constructor))
void myconstructor() {
	signal(SIGUSR2, sig_handler);
}
