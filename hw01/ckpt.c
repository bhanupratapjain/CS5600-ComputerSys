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

/*Flag to check if the execution is from the checkpoint*/
int checkpoint_flag = 0;

/**
 * Processes the incoming file pointer and writes it into the output pointer.
 * @param fp_in
 * @param fd_out
 * @param mem_header
 * @return
 */
int process_mem_headers(FILE *fp_in, int fd_out, header * mem_header);

/**
 * Write retry for failing write operation.
 * @param fd
 * @param buf
 * @param count
 * @return
 */
ssize_t write_with_retry(int fd, const void *buf, size_t count);
/***
 * Singnal Handler
 * @param signum
 */

void sig_handler(int signum);
/***
 * Closes file handlers
 * @param fp_in
 * @param fd_out
 * @return
 */
int close_file_handlers(FILE *fp_in, int fd_out);

ssize_t write_with_retry(int fd, const void *buf, size_t count) {
	const char *ptr = (const char *) buf;
	//printf("given buff%p\n", buf);
	size_t num_written = 0;
	do {
		//printf("buff:%p size:%ld \n", ptr + num_written, count - num_written);
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
	size_t data_size_written = write(fd, (void *) mem_header_ptr->sAddr,
			data_size);
	if (data_size != data_size_written) {
		perror("Error while writing memory header.");
		return -1;
	}
	//printf("data size:%ld, data_size_written:%ld\n", data_size,
	//		data_size_written);
	return 0;
}

int process_mem_headers(FILE *fp_in, int fd_out, header * mem_header) {
	//printf("process_mem_headers\n");
	size_t bufsize = 512;
	char * line = NULL;
	while (1) {

		/*Clearing Memory Header*/
		memset(mem_header, 0, sizeof(header));
		if (feof(fp_in)) {
			//printf("End of file reached\n");
			break;
		}
		getline(&line, &bufsize, fp_in);
		//printf("line %s\n", line);

		/*Skipping [vvar] memory */
		if (strstr(line, "vvar") != NULL) {
			//printf("Skipping memory [vvar]\n");
			continue;
		}
		/*Skipping [vdso] memory */
		if (strstr(line, "vdso") != NULL) {
			//printf("Skipping memory [vdso]\n");
			continue;
		}
		/*Skipping [vsyscall] memory */
		if (strstr(line, "vsyscall") != NULL) {
			//printf("Skipping memory [vsyscall]\n");
			continue;
		}

		/*Parsing Maps line to header data structure*/
		if (parse_mem_header(mem_header, line) < 0) {
			perror("Error while parsing memory header.");
			return -1;
		}

		//print_mem_headers(mem_header);
		/*Skipping Non Readable Memory*/
		if (strstr(mem_header->perms, "r") == NULL) {
			//printf("Memory Not readable. Skipping\n");
			continue;
		}

		/*Writing memory Header with Data to Chekpoint File*/
		if (write_mem_headers(fd_out, mem_header) < 0) {
			perror("Error while writing memory to checkpoint.");
			return -1;
		}
	}
	return 0;
}

int dump_context(int fd_out, ucontext_t * p_context_ptr) {
	header * context_header = (header*) malloc(sizeof(header));
	context_header->type = HEADER_CONTEXT;
	context_header->fAddr = (long) &checkpoint_flag;

	//printf("Dumping Context\n");

	if (write(fd_out, context_header, sizeof(header)) < 0) {
		perror("Error while writing context header.");
		return -1;
	}
	if (write(fd_out, p_context_ptr, sizeof(ucontext_t)) < 0) {
		perror("Error while writing context data.");
		return -1;
	}
	//printf("Context Dumped Successfully\n");
	return 0;
}

int close_file_handlers(FILE *fp_in, int fd_out) {
	if (fclose(fp_in) != 0) {
		perror("Error occurred while closing input file.");
		return -1;
	}
	if (close(fd_out) != 0) {
		perror("Error occurred while closing output file.");
		return -1;
	}
	return 0;
}

void sig_handler(int signum) {
	printf("Caught signal %d. Checkpoint created with name 'myckpt'.\n", signum);
	FILE *fp_in = NULL;
	int fd_out;
	ucontext_t p_context;
	header mem_header = { 0 };
	//printf("Reading Memory Maps for the current porcess.\n");
	fp_in = fopen("/proc/self/maps", "r");
	fd_out = open("myckpt", O_WRONLY | O_CREAT | O_TRUNC,
			S_IRWXU | S_IRGRP | S_IROTH);
	if (fp_in == NULL) {
		perror("Error while opening Input Memory Maps");
		exit(-1);
	}
	if (fd_out < 0) {
		perror("Error while opening output checkpoint file");
		exit(-1);
	}
	//printf("Checkpoint flag::%p and value::%d\n", &checkpoint_flag,
	//		checkpoint_flag);
	if (process_mem_headers(fp_in, fd_out, &mem_header) < 0) {
		perror("Error while dumping memory");
		close_file_handlers(fp_in, fd_out);
		exit(-1);
	}
	if (getcontext(&p_context) < 0) {
		printf("Get Context Failed\n");
		perror("Error while getting context.");
		close_file_handlers(fp_in, fd_out);
		exit(-1);
	}
	if (checkpoint_flag < 0) {
		//printf("got back from restart\n");
		//printf("Checkpoint flag::%p and value::%d\n", &checkpoint_flag,
		//		checkpoint_flag);
		return;
	}
	if (dump_context(fd_out, &p_context) < 0) {
		perror("Error while dumping context");
		close_file_handlers(fp_in, fd_out);
		exit(-1);
	}
	//printf("Back to Handler\n");
	close_file_handlers(fp_in, fd_out);
}
__attribute__ ((constructor))
void myconstructor() {
	signal(SIGUSR2, sig_handler);
}
