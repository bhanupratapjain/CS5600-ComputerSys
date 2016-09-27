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


## Implementation Stratergy

###Checkpointing

1. Register user signal -12 , through contructor in shared lib `libckpt.so`
2. Preload this shared lib in test program `count` through `LD_PRELOAD`
3. When the user signal -12 is called on program `count`, call the signal handler.
4. Inside the signal handler:
..1. Read `proc/self/maps` line by line.
..2. Create structure `header` which holds the addresss locations and permission flags of the read line.
..3. Push `header` for each line to the checkpoint file.
..4. Push the data for the address range to the checkpoint file. 
..5. Read the contex. 		
..6. Check whether the exuction is through `checkpoint` or `restart` through global `checkpoint_flag`. If the execution flow is from `restart` then `exit` else `continue`.
..7. Save contex in checkpoint file.
..8. Save `checkpoint_flag` address to the checkpoint file.
	

###Restart