#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]){
	printf(1, "Parent pid: %d\n", get_parent_pid());
	exit();
} 
