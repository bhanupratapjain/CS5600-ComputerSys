# XV6


## PROCESSES

1. > Where is the data structure for the process table?

    The data structure for process table is in `proc.h` at line 10.
    ```
     struct {
       struct spinlock lock;
       struct proc proc[NPROC];
     } ptable;
    ```

2. > When there is a context switch from one process to another, where are the values of the registers of the old process saved?

    - File: `proc.h`
    - Line: 44
    ```
      33 //PAGEBREAK: 17
      34 // Saved registers for kernel context switches.
      35 // Don't need to save all the segment registers (%cs, etc),
      36 // because they are constant across kernel contexts.
      37 // Don't need to save %eax, %ecx, %edx, because the
      38 // x86 convention is that the caller has saved them.
      39 // Contexts are stored at the bottom of the stack they
      40 // describe; the stack pointer is the address of the context.
      41 // The layout of the context matches the layout of the stack in swtch.S
      42 // at the "Switch stacks" comment. Switch doesn't save eip explicitly,
      43 // but it is on the stack and allocproc() manipulates it.
      44 struct context {
      45   uint edi;
      46   uint esi;
      47   uint ebx;
      48   uint ebp;
      49   uint eip;
      50 };
    ```

3. > What are the possible states of a process? Also, give a brief phrase describing the purpose of each state.

    The possible states of the process are: 
    - **`UNUSED`**: A new process that is being created but not used. `Allocproc` scans the table for a process with state `UNUSED`. When it finds an unused process, `allocproc` sets the state to `EMBRYO` to mark it as used and gives the process a unique pid. 
    - **`EMBRYO`**: New process that is currently being created. 
    - **`SLEEPING`**: Blocked for I/O
    - **`RUNNABLE`**: Ready to run.
    - **`RUNNING`**: Currently executing.
    - **`ZOMBIE`**:  When a child exits, it does not die immediately. Instead, it switches to the ZOMBIE process state until the parent calls wait tolearn of the exit. 
    
4. > What is the function that does a context switch between two processes?

    `swtch` from `swtch.s` 

5. > Explain how the context switch function works.

    `swtch` starts by loading its arguments off the stack into the registers `%eax` and `%edx`. Then `swtch` pushes the register state, creating a context structure on the current stack. 
    ```
      movl 4(%esp), %eax
      movl 8(%esp), %edx
    ```

    Saving only the callee-save registers, `swtch` pushes the first four `%ebp, %ebx, %esi, %ebp`.
 
        pushl %ebp
        pushl %ebx
        pushl %esi
        pushl %edi
     
    Having saved the old context, swtch is ready to restore the new one. It moves the pointer to the new context into the stack pointer. The new stack has the same form as the old one that `swtch` just left—the new stack was the old one in a previous call to swtch—so swtch can invert the sequence to restore the new context. 
     
        movl %esp, (%eax)
        movl %edx, %esp
     
    It pops the values for `%edi, %esi, %ebx, and %ebp` and then returns. Because `swtch` has changed the stack pointer, the values restored and the instruction address returned to are the ones from the new context.
    
        popl %edi
        popl %esi
        popl %ebx
        popl %ebp
        ret

6. >What function calls the context switch function, and explain in detail what the calling function does.

    `swtch` is called by `scheduler(void)` in `proc.c`.  Basically when are scheduling a new process, first we fetch the process from the process table and then switch to the process to run. 
    
    `swtch` is also called in `sched(void)` in `proc.c`. Whenever we are changing the state fo the process which is already schedules, then we used `sched`. For example in `yeild` we are yielding the current process and giving up cpu, the `sched` does a context switch using `swtch` between the current process being yielded and the next scheduled process. 


## PROCESS STARTUP
1. >Suppose a new program is going to start. This requires a call to the system call, exec(). On what lines does the operating system create the first call frame, to be used by the user process's main()?

    `copyout` at line 65 in `exec.c` created the first call frame.

2. >The first call frame must have local variables argc and argv. Where is the value of argv found in the exec() call?

    `argv` is taken as a second argument in `exec()` call. 

3. >On what lines does the function create the process table entry for the new process?

    `exec` allocates a new page table without user mapping with `setupkvm` at line 32(`exec.c`), allocates memory for each segment with `allocuvm` at line 44, and loads each segment into memory with `loaduvm` at line 46.

## SYSTEM CALLS 
1. > The file grep.c makes a call to 'open()'. The definition of 'open()' is inside 'usys.S'. It makes use of the macro 'SYSCALL'. Note that a macro, '$SYS_ ## name', will expand to the concatenation of 'SYS_' and the value of the macro parameter, "name". The assembly instruction 'int' is the interrupt instruction in x86 assembly. The 'int' assembly instruction takes as an argument an integer, 'T_SYSCALL'. The code in usys.S is complex because it uses the C preprocessor. But, roughly, SYSCALL(open) will expand to the assembly code in lines 4 though 9 of usys.S, where the (x86 assembly) instruction: "movl $SYS_ ## name, %eax" expands to: "movl $SYS_open, %eax" The value of SYS_open can be found in the include file, "syscall.h". The instruction: "int $T_SYSCALL" uses information from "traps.h". The "int" instruction is an "interrupt" instruction. It interrupts the kernel at the address for interrupt number 64 (found in traps.). If you do "grep SYS_open /course/cs5600sp16/resources/unix-xv6-source/* it will lead you to: /course/cs5600sp16/resources/unix-xv6-source/syscall.c That will define the "syscalls" array, which is used by the function "syscall". Finally, here is the question: Give the full details of how a call to 'open()' in grep.c will call the function 'sys_open()' in sysfile.c, inside the operating system kernel.

    - `open()` in `grep.c` will call the macro `SYSCALL` in `usys.S`. Here 
        - Value 15 (for `open` as defined in `syscall.h`) is pushed into register `eax`. The `eax` value will be used by the kernel to identify the correct system call. 
        - A trap is generated by `int` to enable a switch from user space to kernel space. It takes `$T_SYSCALL` argument defined in `traps.h` (value 64) and specifies that this is a trap for system call. 
    - With the trap in place, the kernel handles the trap with, `trap()` in `trap.c` and will verify the trap with `$T_SYSCALL` and call `syscall()`
    - `syscall()` in `syscall.c` will now determine the right system call based on `eax` value and `syscalls` array and call `sys_open` corresponding to `SYS_open` - 15.
    - Finally `sys_open()` from `sysfile.c` is executed. 



## FILES AND FILE DESCRIPTORS
1. > The function 'sys_open()' returns a file descriptor 'fd'. To do this, it opens a new file (new i-node) with 'filealloc()', and it allocates a new file descriptor with 'fdalloc()'. Where is the file descriptor allocated? Also, you will see that the file descriptor is one entry in an array. What is the algorithm used to choose which entry in the array to use for the new file descriptor? [ Comment: The name 'NOFILE' means "file number". "No." is sometimes used as an abbreviation for the word "number". ]

    The file descriptor is allocated in the `ofile` array in the `proc` structure. The file descriptors are allocated linearly by linear search and look for the first emply slot.. We perform a linear search for `ofile` array with a max file count of 16 defined by `NOFILE`

2. > As you saw above, the file descriptor turned out to be an index in an array. What is the name of the array for which the file descriptor is an index? Also, what is the type of one entry in that array.

    The name if the array is `ofile` . Entries are of type `struct file`

3. > The type that you saw in the above question is what I was calling a "file handle" (with an offset into the file, etc.). What is the name of the field that holds the offset into the file? We saw it in the function 'sys_open()'.

    The name of the field is `f->off`

4. > Remember when we mentioned a call to 'filealloc()' above? Since the return value of 'filealloc()' is only a file handle, we need to initialize it. Presumably, we will initialize it with a file offset of 0. What is the line number in 'sys_open()' where we initialize the file offset to 0?

    Line no. 315 in `sysfile.c`

5. > The file handle type was initialized to 'FD_INODE'. What are the other types that it could have been initialized to?

    `enum { FD_NONE, FD_PIPE, FD_INODE } type;`

6. > Suppose a file handle had been initialized to FD_PIPE. Find the 'struct' that hold sthe information about a pipe. For each field in that struct, Explain briefly (just a phrase) the purpose of that field.

        struct pipe {
          struct spinlock lock; // spinlock to provide mutual exclusion
          char data[PIPESIZE]; // Data to process 
          uint nread;     // number of bytes read
          uint nwrite;    // number of bytes written
          int readopen;   // read fd is still open
          int writeopen;  // write fd is still open
        };


7. > By examining the function 'sys_dup()', you can discover how a system call to 'dup()' will manipulate both a file descriptor and a "file handle". Describe what it does in each of the two cases.
    
    `sys_dup()` calls `argfd` to obtain the file corresponding to the system call and then calls `fdalloc` to assign it an additional file descriptor. If both are successful, it calls `filedup` to increase the reference count of the file returned by `argfd`. Thus for file descriptor it creates a duplicated file descriptor for the existing file handle. For file handle, it increases the reference count of the existing file handle. 
