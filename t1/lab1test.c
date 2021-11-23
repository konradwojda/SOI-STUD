#include </usr/include/stdio.h>
#include </usr/include/lib.h>

int getprocnr(int pid)
{
    message m;
    m.m1_i1 = pid;
    return (_syscall(MM, 78, &m));
}

int main(int argc, char* argv[])
{    
    int proc = atoi(argv[1]);
    int i;
    int res;
    if(argc == 1)
    {
        printf("No arguments");
        return 0;
    }

    for(i = proc; i > proc - 10; i--)
    {
        res = getprocnr(i);
        if(res != -1)
        {
            printf("Pid: %d, index: %d\n", i, res);
        }
        else
        {
            printf("No proccess of pid: %d, error number: &%d\n", i, errno);
        }
    }
    return 0;
}
