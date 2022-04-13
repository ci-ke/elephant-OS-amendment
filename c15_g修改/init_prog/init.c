//init.c:

#include "stdint.h"
#include "stdio.h"
#include "syscall.h"
/* init进程 */
int main(void)
{
    uint32_t ret_pid = fork();
    if (ret_pid)
    {
        printf("i am father, my pid is %d, child pid is %d\n", getpid(), ret_pid);
    }
    else
    {
        printf("i am child, my pid is %d, ret pid is %d\n", getpid(), ret_pid);
    }
    while (1)
        ;
}