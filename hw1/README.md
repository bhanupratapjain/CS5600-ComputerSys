# HW01


## Problem
In this assignment, you will build a mini-DMTCP. DMTCP is a widely used software package for checkpoint-restart: http://dmtcp.sourceforge.net. However, you do not need any prior knowledge of DMTCP for this assignment. The software that you produce will be self-contained.

You will write a shared library, `libckpt.so`. Given an arbitrary application in C or C++, you will then run it with this library with the following command-line:

```
LD_PRELOAD=/path/to/libckpt.so ./myprog
```

where `/path/to/libckpt.so` must be an absolute pathname for the `libckpt.so`.

You must provide a Makefile for this project. Here is some advice for writing the code for `libckpt.so`. However, if you prefer a different design, you are welcome to follow that. Your code will only be tested on single-threaded targets. You are not required to support multi-threaded applications. Similarly, you are not required to support shared memory.

(Note that the memory layout of a process is stored in `/proc/PID/maps`. The fields such as rwxp can be read as "read", "write", "execute" and "private". Naturally, rwx refer to the permissions. Private refers to private memory. If you see "s" instead of "p", then it is shared memory.)


## Running Instruction for HW01


- tar -xvf hw1.tar.gz
- cd hw1
- make check (*This will build all required binaries and call `count`. Then this will trigger envent `kill -12` on `count` and finally kill `count`*)
	
	This will generate the following error. Ignore it as it is generated beacuse the original count program was killed. 
	```
	Makefile:9: recipe for target 'check' failed
	make: *** [check] Killed
	```
- make res (*This will call the restart program on the checkpoint image.*)

*(Please note that in order run the make target with no errors we can use only `make check -i` and it will run `count`, save checkpoint image and successfully restart the program.)*

## Implementation Stratergy

###Checkpointing

1. Register user `signal -12` , through contructor in shared lib `libckpt.so`
2. Preload this shared lib in test program `count` through `LD_PRELOAD`
3. When the user `signal -12` is called on program `count`, call the signal handler.
4. Inside the signal handler:
	1. Read `proc/self/maps` line by line.
	2. Skip `vvar`, `vdso`, `vsyscall` and all non readable regions.
	3. Create structure `header` which holds the addresss locations and permission flags of the read line.
	4. Push `header` for each line to the checkpoint file.
	5. Push the data for the address range to the checkpoint file. 
	6. Read the context. 		
	7. Check whether the exuction flow is from `checkpoint` or `restart` by checking global variable `checkpoint_flag`. If the execution flow is from `restart` then `exit` else 		`continue`.
	8. Save context in checkpoint file.
	9. Save `checkpoint_flag` address to the checkpoint file.
	10. Exit

### Restart
1. Copy the chekpoint image name passed in command line arguments to gloabal variable `ckpt_image_name`.
2. Read the old stack and save addressed to `header`
3. Map memory for new temporary stack.
4. Point the `stack pointer` to the new stack.
5. Unmap original stack allocated by the kernel.
6. Read the checkpint image:
	1. Map memory for the address range.
	2. Read data from checkpoint image to the allocated memory.
	3. Set proper memory/map protection to the allocated memory.
	4. Read the context.
	5. Read `checkpoint_flag` and set value to `-1`
	6. Set the read context.
	

###Restart