#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]){
	if(argc < 2)
	{
		printf(2, "You must enter exactly one number!\n");
		exit();
	}
    else
    {
		int saved_ebx, number = atoi(argv[1]);
		asm volatile(
			"movl %%ebx, %0;"
			"movl %1, %%ebx;" 
			: "=r" (saved_ebx)
			: "r"(number)
		);
		printf(1, "User: calculate_sum_of_digits() called for number: %d\n" , number);
		printf(1, "Sum Of Digits %d = %d\n" , number , calculate_sum_of_digits());
		asm("movl %0, %%ebx" : : "r"(saved_ebx)); 
		exit();  	
    }
    exit();
} 
