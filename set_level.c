#include "types.h"
#include "user.h"
#include "stat.h"
#include "fcntl.h"

int main(int argc, char* argv[]) {
    int pid, level;

    pid = atoi(argv[1]);
    level = atoi(argv[2]);

    set_level(pid, level);
    printf(1, "Process %d level changed to %d.\n", pid, level);
    exit();
}
