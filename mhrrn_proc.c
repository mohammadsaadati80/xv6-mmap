#include "types.h"
#include "user.h"
#include "stat.h"
#include "fcntl.h"

int main(int argc, char* argv[]) {
    int pid, priority;

    pid = atoi(argv[1]);
    priority = atoi(argv[2]);

    set_mhrrn_param_process(pid, priority);
    exit();
}
