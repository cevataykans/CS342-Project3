
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

// Define a name for your shared memory; you can give any name that start with a slash character; it will be like a filename.
char sharedMemoryName[] = "/sharedSegmentName";

// Define semaphore(s)
// I think one is enoguh?

// Define your stuctures and variables.
int shm_fd;
int processCount = 0;

const int MAX_SEGMENT_SIZE = (1 << 20) * 4;
const int MIN_SEGMENT_SIZE = (1 << 10) * 32;
const int MAX_PROCESS_COUNT = 10;

struct ProcessDatum
{
    pid_t processID;
    void *ptr;
};
struct ProcessDatum processData[10];
signed char usedData[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

int smem_init(int segmentsize)
{
    printf("Max segment size => %d and min segment size => %d\n", MAX_SEGMENT_SIZE, MIN_SEGMENT_SIZE);
    // validate segment size
    if (segmentsize < MIN_SEGMENT_SIZE || segmentsize > MAX_SEGMENT_SIZE)
    {
        printf("Error! your allocation must be between 2^15 (32 KB) and 2^22 (4 MB)\n");
        return -1;
    }

    // check for power of 2!
    int powerChecker = 1;
    while (powerChecker < segmentsize)
    {
        powerChecker = powerChecker << 1;
    }
    if (powerChecker != segmentsize)
    {
        printf("The given segment size must be a power of 2!");
        return -1;
    }

    printf("Smem init called\n"); // remove all printfs when you are submitting to us.
    shm_fd = shm_open(sharedMemoryName, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_fd < 0)
    {
        if (errno == EEXIST)
        {
            printf("Shared memory already exists, calling unlink to reallocate memory!\n");
            shm_unlink(sharedMemoryName);
            shm_fd = shm_open(sharedMemoryName, O_CREAT | O_EXCL | O_RDWR, 0666);
        }
        else
        {
            printf("Opening shared memory failed...\n");
            return -1;
        }
    }

    if (shm_fd > 0)
    {
        printf("Shared memory opened, truncating...\n");
        int truncateRes = ftruncate(shm_fd, segmentsize);
        if (truncateRes < 0)
        {
            printf("Truncate failed...\n");
            return -1;
        }
    }
    return shm_fd >= 0 ? 0 : -1;
}

int smem_remove()
{
    // also reset process count, semephores, any data, make everything reset! should be reubsale once called init!
    processCount = 0;
    for (int i = 0; i < MAX_PROCESS_COUNT; i++)
    {
        usedData[i] = -1;
    }

    return shm_unlink(sharedMemoryName) >= 0 ? 0 : -1;
}

int smem_open()
{
    //protect with semephore the whole function

    // limit the usage of the library!
    if (processCount < MAX_PROCESS_COUNT)
    {
        // allocate this slot to the requesting process
        usedData[processCount] = 1;
        processCount++;

        // map the memory

        return 0;
    }
    return -1;
}

void *smem_alloc(int size)
{
    // protect with semephore the whole function

    // can allocate a minumum of 8 bytes
    if (size < 8)
    {
        size = 8;
    }

    return (NULL);
}

void smem_free(void *p)
{
    // protect with semephore the whole function

    // deallacote space pointed by the pointer p
}

int smem_close()
{
    // protect with semephores the whole function

    // deallocate memory of procecss from the shared segment!

    return (0);
}
