three requirement: 
1. multiplexing
2. isolation
3. interaction

an application in user mode try to execute a privelidged instruction, cpu doesn't execute it, switch to supervisor mode so that supervisor-code can terminate the application
an application that wants to invoke a kernel function(e.g., the read system call) must transition to the kernel
cpu provides a special instruction that switches the CPU from user mode to supervisor mode and enters the kernel at an entry point specified by kernel

ecall -> validate argument of system call -> is allowed ? -> execute it or not

pagetable map a virtual address to a physical address

ecall -> raise the hardware priviledge level -> change the program counter to a kernel-defined entry point -> switch to kernal stack -> execute the instruction that implement the system call -> switch to user stack -> return calling sret instruction(a instruction lower the hardware privilege)

# trace
1. user trace, ecall
2. a7 means the num of syscall, argument in a0 a1 if exist
3. mask mean target syscall
4. fork copy mask to child process

# sysinfo
1. traversal freelist
2. traversal proc array, check p->state
3. writeout to user space copyout(p->pagetable, out_addr, (char*) &info, sizeof(info)
