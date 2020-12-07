

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "smemlib.h"

int main()
{
    int i, ret;
    char *p;

    smem_init(32768);

    ret = smem_open();
    if (ret == -1)
        exit(1);

    p = smem_alloc(1024); // allocate space to hold 1024 characters
    for (i = 0; i < 1024; ++i)
    {
        p[i] = 'a'; // init all chars to ‘a’
        printf("Umm is this working? %d\n", p[i]);
    }
    smem_free(p);

    smem_close();

    return (0);
}
