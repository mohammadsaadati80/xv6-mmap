#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{
    int pid[10];
    for (int i = 0 ; i < 5; i++)
    {
        pid[i] = fork();
        if (pid[i] == 0)
        {
            while(1);
            exit();
        }
    }

    while (wait());
    return 0;
}