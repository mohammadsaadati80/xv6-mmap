#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]){
	if(argc < 2){
		printf(2, "You must enter exactly 1 number!\n");
	}
    else
    {
		int saved_ebx, pid = atoi(argv[1]);
		asm volatile(
			"movl %%ebx, %0;"
			"movl %1, %%ebx;" 
			: "=r" (saved_ebx)
			: "r"(pid)
		);
		// printf(1, "User: set_parent_process() called for pid: %d\n" , pid);
		printf(1, "Debugging pid %d\n" , pid);
        printf(1, "Debugger pid %d\n" , getpid());
		printf(1, "Debugger parent %d\n", get_parent_pid());
		asm("movl %0, %%ebx" : : "r"(saved_ebx)); 
		// exit();  	
        set_process_parent();
		
        printf(2, "Parent changed.\n");
		wait();
	}
	printf(2, "Debugger exited\n");
    exit();
} 
