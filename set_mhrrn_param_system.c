#include "types.h"
#include "user.h"
#include "stat.h"
#include "fcntl.h"

int main(int argc, char* argv[]) {
    int priority;

    priority = atoi(argv[1]);

    set_mhrrn_param_system(priority);
    exit();
}
