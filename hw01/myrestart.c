#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <ucontext.h>
#include <sys/mman.h>
#include "util.h"

/*Name of the image file*/
char ckpt_image_name[512];

// byte-size value to hold some combination of the above 8 options
unsigned char flag = 0; // all flags/options turned off to start

int unmap_existing_stack(header * stack_header);
int read_existing_stack(header * stack_header);
int get_memory_protection(char * flags);
int get_map_protection(char * flags);
int get_map_protection(char * flags);
int process_checkpoint();
int restore_memory(header * stack_header);

int unmap_existing_stack(header * stack_header) {

	size_t size = stack_header->eAddr - stack_header->sAddr;
	//printf("s%ld size%ld\n", stack_header->sAddr, size);
	if (munmap((void *) stack_header->sAddr, size) < 0) {
		perror("Error in munmap");
		return -1;
	}
	return 0;
}

int read_existing_stack(header * stack_header) {
	FILE *fp_in = NULL;
	size_t bufsize = 512;
	//printf("Reading Memory Maps for the current process.\n");
	fp_in = fopen("/proc/self/maps", "r");
	char * line = NULL;
	if (fp_in == NULL) {
		perror("Error while opening Input Memory Maps");
		return -1;
	}
	while (1) {
		if (feof(fp_in)) {
			//printf("End of file reached\n");
			break;
		}
		getline(&line, &bufsize, fp_in);
		if (strstr(line, "stack") != NULL) {
			if (parse_mem_header(stack_header, line) < 0) {
				perror("Error while parsing stack header");
				if (fclose(fp_in) != 0) {
					perror("Error occurred while closing input file.");
					return -1;
				}
				return -1;
			}
			free(line);
			//print_mem_headers(stack_header);
			break;
		}
	}
	if (fclose(fp_in) != 0) {
		perror("Error occurred while closing input file.");
	}
	//printf("End of reading existing stack.\n");
	return 0;

}

int get_memory_protection(char * flags) {
	//printf("inside get_mem_protec: %s \n",flags);
	int prot = PROT_NONE;
	if (strstr(flags, "r") != NULL) {
		prot |= PROT_READ;
		//printf("read memory\n");
	}
	if (strstr(flags, "w") != NULL) {
		prot |= PROT_WRITE;
		//printf("write memory\n");
	}
	if (strstr(flags, "x") != NULL) {
		prot |= PROT_EXEC;
		//printf("exec memory\n");
	}
	//printf("mem prot%d\n", prot);
	return prot;
}

int get_map_protection(char * flags) {
	//printf("get_map_protection %s \n",flags);
	int prot = MAP_ANONYMOUS | MAP_FIXED;
	if (strstr(flags, "p") != NULL) {
		prot |= MAP_PRIVATE;
		//printf("private memory\n");
	} else {
		prot |= MAP_SHARED;
		//printf("shared memory\n");
	}
	//printf("map prot%d\n", prot);
	return prot;
}

int process_checkpoint() {
	int fd_ckpt;
	int * checkpoint_flag_ptr = NULL;
	ucontext_t context;
	header mem_header = { 0 };
	ssize_t read_bytes;
	void * new_mem = NULL;
	char * perms;
	fd_ckpt = open(ckpt_image_name, O_RDONLY);
	if (fd_ckpt < 0) {
		perror("Error while opening checkpoint file");
		return -1;
	}
	while (1) {
		read_bytes = read(fd_ckpt, &mem_header, sizeof(header));
		//printf("read %ld bytes for header\n", read_bytes);
		if (read_bytes == 0) {
			//printf("End of file.\n");
			break;
		}
		//print_mem_headers(&mem_header);
		if (mem_header.type == HEADER_CONTEXT) {
			//printf("Got context.\n");
			break;
		}
		perms = mem_header.perms;
		size_t mem_size = mem_header.eAddr - mem_header.sAddr;
		new_mem = mmap((void*) mem_header.sAddr, mem_size, PROT_WRITE,
				get_map_protection(perms), -1, 0);

		if (new_mem == MAP_FAILED) {
			perror("Error while getting virtual memory for checkpoint");
			return -1;
		}

		//printf("Data length size %ld\n", mem_size);
		read_bytes = read(fd_ckpt, new_mem, mem_size);
		//printf("read %ld bytes for data\n", read_bytes);
		if (read_bytes != mem_size) {
			//printf("Incorrect Data Length Read.\n");
			return -1;
		}

		if (mprotect(new_mem, mem_size, get_memory_protection(perms)) < 0) {
			perror("Error while setting map protection");
			return -1;
		}

	}
	read_bytes = read(fd_ckpt, &context, sizeof(context));
	//printf("read %ld bytes for data\n", read_bytes);
	if (read_bytes < 0) {
		perror("Can't read context data.");
		return -1;
	}

	/*Reading Checkpoint flag address*/
	//printf("flag::%ld\n",mem_header.fAddr);
	checkpoint_flag_ptr = (int *) mem_header.fAddr;

	//printf("flag pointer:: %p, value::%d \n",checkpoint_flag_ptr,(*checkpoint_flag_ptr));
	(*checkpoint_flag_ptr) = -1;
	//printf("flag pointer:: %p, value::%d \n",checkpoint_flag_ptr,(*checkpoint_flag_ptr));

	if (setcontext(&context) < 0) {
		perror("Unable to set context.");
	}

	//printf("End of process_checkpoint file.\n");
	exit(0);
}

int restore_memory(header * stack_header) {
	//printf("Restoring Memory.\n");
	/*unmap existing stack*/
	if (unmap_existing_stack(stack_header) < 0) {
		perror("Error while unmapping existing stack");
		return -1;
	}
	//printf("unmap done\n");
	/*read and process checkpoint image*/
	if (process_checkpoint() < 0) {
		perror("Error while unmapping existing stack");
		return -1;
	}
	return 0;
}

int restore_checkpoint() {
	/*read old stack*/
	header *existing_stack_header = (header*) malloc(sizeof(header));
	if (read_existing_stack(existing_stack_header) < 0) {
		perror("Error while reading existing stack");
		return -1;
	}
	/*map virtual memory for stack*/
	unsigned char *stack_ptr = (unsigned char *) 0x5300000;
	//printf("%ld\n", sysconf(_SC_PAGE_SIZE));
	if (mmap(stack_ptr, 1000 * sysconf(_SC_PAGE_SIZE),
			PROT_READ | PROT_WRITE | PROT_EXEC,
			MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0) == MAP_FAILED) {
		perror("Error while getting virtual memory for stack");
		return -1;
	}
	//printf("sp:%p\n", stack_ptr);
	stack_ptr += 1000 * sysconf(_SC_PAGE_SIZE);
	asm volatile ("mov %0,%%rsp;" : : "g" (stack_ptr) : "memory");
	/*restore memory from checkpoint*/
	if (restore_memory(existing_stack_header) < 0) {
		perror("Error while restoring checkpoint memory");
		return -1;
	}
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc == 2) {
		//printf("Restoring Checkpoint image %s\n", argv[1]);
		strcpy(ckpt_image_name, argv[1]);
		if (restore_checkpoint() < 0) {
			perror("Error while restoring checkpoint");
		}
	} else if (argc > 2) {
		printf("Too many arguments supplied.\n");
	} else {
		printf("One argument expected.\n");
	}
}
