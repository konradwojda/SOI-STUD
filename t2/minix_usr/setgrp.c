#include </usr/include/stdio.h>
#include </usr/include/lib.h>

int set_proc_grp(int pid, int group)
{
    message m;
    m.m1_i1 = pid;
    m.m1_i2 = group;
    return _syscall(MM, 79, &m);
}

int main(int argc, char* argv[])
{
    int res, proc, grp;
    if (argc != 3)
    {
        printf("Usage: ./setgrp [INDEX] [GROUP: 1 or 2]\n");
    }
    proc = atoi(argv[1]);
    grp = atoi(argv[2]);
    set_proc_grp(proc, grp);
    return 0;
}