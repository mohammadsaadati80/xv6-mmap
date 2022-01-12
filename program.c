#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]){
    printf(1, "My pid: %d\n", getpid());
    printf(1, "Parent pid: %d\n", get_parent_pid());
    sleep(2000);
    printf(1, "My pid: %d\n", getpid());
    printf(1, "Parent pid: %d\n", get_parent_pid());
    printf(2, "program exited\n");
    exit();
} 
