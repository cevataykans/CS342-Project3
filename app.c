

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "smemlib.h"

int main()
{
    int i, ret;
    char *p;
    int *t;
    char *anotherP;

    smem_init(32768);

    ret = smem_open();
    if (ret == -1)
        exit(1);

    p = smem_alloc(1024);            // allocate space to hold 1024 characters
    t = smem_alloc(2 * sizeof(int)); // allocate space to hold 2 integers
    anotherP = smem_alloc(1);        // allocate space to hold one char

    for (i = 0; i < 1024; ++i)
    {
        p[i] = ('a' + i) % 'a'; // init all chars to ‘a’
    }

    t[0] = 8;
    t[1] = p[1023];
    *anotherP = 'a';

    for (i = 0; i < 1024; ++i)
    {
        printf("Value at index at index %d is => %d\n", i, p[i]);
    }
    //smem_free(p);
    printf("Value of first int is %d and second int is %d\n", t[0], t[1]);
    //smem_free(t);
    printf("Another Char pointer value is: %d\n", *anotherP);
    //smem_free(anotherP);

    smem_close();

    return (0);
}
