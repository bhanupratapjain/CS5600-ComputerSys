#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <ucontext.h>

const unsigned char IS_MAP = 0x01; // hex for 0000 0001
const unsigned char IS_CONTEXT = 0x02; // hex for 0000 0010

// byte-size value to hold some combination of the above 8 options
unsigned char flag = 0; // all flags/options turned off to start

typedef struct {
	long sAddr;
	long eAddr;
	char * perms;
	unsigned char flags;

} header;

void parse_mem_header(header * mem_header_ptr, char * line) {
	const char CONST_SPACE[2] = " ";
	const char CONST_HYPHEN[2] = "-";
	char *token;
	char *add_range;
	char *flags;
	char *add_range_token;

//	if(strstr(line,"")!=NULL)

	/*set type to maps*/
	mem_header_ptr->flags = flag |= IS_MAP;

	/* get the address range*/
	token = strtok(line, CONST_SPACE);
	/* copy address range*/
	add_range = malloc(sizeof(char) * strlen(token));
	strcpy(add_range, token);
//	printf("Address Range %s\n", add_range);

	/*get flags*/
	token = strtok(NULL, CONST_SPACE);
	/*copy flags*/
	flags = token;
	printf("Flags %s\n", flags);
	mem_header_ptr->perms = flags;

	/*parse address range*/
	add_range_token = strtok(add_range, CONST_HYPHEN);
	printf("start %s\n", add_range_token);
	mem_header_ptr->sAddr = strtol(add_range_token, NULL, 16);

	add_range_token = strtok(NULL, CONST_HYPHEN);
	printf("end %s\n", add_range_token);
	mem_header_ptr->eAddr = strtol(add_range_token, NULL, 16);

}

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
			mem_header_ptrs[k] = (header*) malloc(sizeof(header));
			parse_mem_header(mem_header_ptrs[k], line);
			printf("End of Line\n");
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

void print_mem_headers(header * mem_header_ptr) {
	printf("Start Add: %ld\n", mem_header_ptr->sAddr);
	printf("End Add: %ld\n", mem_header_ptr->eAddr);
	printf("Perms: %s\n", mem_header_ptr->perms);
	printf("Flags: %u\n", mem_header_ptr->flags);

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

void write_mem_headers(int fd, header * mem_header_ptr) {
	write(fd, (void *) mem_header_ptr, sizeof(header));
	write(fd, (void *) mem_header_ptr->sAddr,
			mem_header_ptr->eAddr - mem_header_ptr->sAddr);
}

void process_mem_headers(int fd_out, header * mem_header_ptrs[]) {
	int index = 0;
	while (1) {
		if (mem_header_ptrs[index] == NULL) {
			printf("end of structure\n");
			break;
		}
		print_mem_headers(mem_header_ptrs[index]);
		write_mem_headers(fd_out, mem_header_ptrs[index]);
		index++;
	}
}

int dump_context(int fd_out) {
	ucontext_t p_context;
	printf("Dumping Context\n");
	if (getcontext(&p_context) < 0) {
		printf("Get Context Failed\n");
		perror("Error while getting context.");
		return -1;
	} else {
		printf("Got Context %lu\n", sizeof(p_context));
	}
	if (write(fd_out, &p_context, sizeof(ucontext_t)) < 0) {
		perror("Error while writing  context.");
		return -1;
	}
	printf("Context Dumped Successfully\n");
	return 0;
}
void sig_handler(int signum) {
	printf("Caught signal %d\n", signum);
	FILE *fp_in = NULL;
	int fd_out;
	header * mem_header_ptrs[100];
	printf("Reading Memory Maps for the current porcess.\n");
	fp_in = fopen("/proc/self/maps", "r");
	fd_out = open("myckpt", O_CREAT | O_WRONLY | O_APPEND | O_TRUNC,
			S_IRUSR | S_IWUSR | S_IXUSR);

	if (fp_in == NULL) {
		perror("Error while opening Input Memory Maps");
		goto END;
	}

	if (fd_out < 0) {
		perror("Error while opening output checkpoint file");
		goto END;
	}

	create_mem_headers(fp_in, mem_header_ptrs);
	process_mem_headers(fd_out, mem_header_ptrs);
	if (dump_context(fd_out) < 0) {
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
