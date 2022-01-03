#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#define SEMCOUNT 5
#define EATROUND 3

void philosopher(int id, int l, int r) {
    for(int i = 0; i < EATROUND; i++) {
        if(l < r) {
            sem_acquire(l);
            sem_acquire(r);
        }
        else {
            sem_acquire(r);
            sem_acquire(l);
        }
        sleep(30 * id);
        printf(1, "I'm philosopher %d and I'm eating with forks %d and %d\n", id, l, r);
        sleep(500);
        sem_release(l);
        sem_release(r);
        printf(1, "I'm philosopher %d and I'm done eating\n", id);
    }
}

int main(int argc, char *argv[])
{
    for(int i = 0; i < SEMCOUNT; i++)
        sem_init(i, 1);
    
    for(int i = 0; i < SEMCOUNT; i++) {
        int pid = fork();
        
        if(pid == 0) {
            philosopher(i + 1, i, (i + 1) % SEMCOUNT);
            exit();
        }
    }
    
    while (wait());
    exit();
}
