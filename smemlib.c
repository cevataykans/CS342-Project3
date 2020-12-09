
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
#include "smemlib.h"
// TODO: remove all printfs

// Define a name for your shared memory; you can give any name that start with a slash character; it will be like a filename.
char sharedMemoryName[] = "/sharedSegmentName";

// Define semaphore(s)
// I think one is enoguh?

// Define your stuctures and variables.
int shm_fd;
int processCount = 0;
int sharedMemorySize = -1;
int memoryUsed = -1; // indicates if the allocation is made for the first time, this will insert first header!

void *(*allocationAlgo)(int, void *);

const int ALLOCATION_LENGTH_OFFSET = 13;
const int ALLOCATION_USED_OFFSET = 12;
const int ALLOCATION_PID_OFFSET = 8;
const int ALLOCATION_PREV_OFFSET = 4;
const int HEADER_SIZE = 17;

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
    signed char c = -1;
    int a = 5;
    printf("Pointer size for void* is: %ld\n", sizeof(void *));
    printf("Pointer size for 5 is: %ld\n", sizeof(a));
    pid_t b = getpid();
    printf("Pointer size for pidt is: %ld\n", sizeof(b));
    printf("Pointer size for signed char is: %ld\n", sizeof(c));

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
        if (errno == EEXIST) // WE CAN ACTUALLY CALL THE smem_remove here? ask ibrahim hocas
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

    if (shm_fd >= 0)
    {
        printf("Shared memory opened, truncating...\n");
        int truncateRes = ftruncate(shm_fd, segmentsize);
        if (truncateRes < 0)
        {
            printf("Truncate failed...\n");
            return -1;
        }
        sharedMemorySize = segmentsize;
        allocationAlgo = &smem_firstFit;
    }
    printf("Cur pid size is: %ld\n", sizeof(getpid()));
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
    sharedMemorySize = -1;
    memoryUsed = -1;

    return shm_unlink(sharedMemoryName) >= 0 ? 0 : -1;
}

int smem_open()
{
    //TODO protect with semephore the whole function

    if (sharedMemorySize < 0)
    {
        printf("MEM CANNOT SMEM_OPEN FAIL. Library is not initialized!\n");
        return -1;
    }

    // limit the usage of the library!
    if (processCount < MAX_PROCESS_COUNT)
    {
        // find the available spot!
        for (int i = 0; i < MAX_PROCESS_COUNT; i++)
        {
            if (usedData[i] < 0)
            {
                // map the memory
                processData[i].processID = getpid();
                processData[i].ptr = mmap(NULL, sharedMemorySize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

                if (processData[i].ptr != MAP_FAILED)
                {
                    // allocate this slot to the requesting process
                    usedData[i] = 1;
                    processCount++;

                    printf("Library successfuly mapped process to the shared memory\n");
                    return 0;
                }
                printf("Library COULD NOT mapped process to the shared memory\n");
                return -1;
            }
        }
    }
    printf("ERROR, MORE THAN 10 process!\n");
    return -1;
}

void *smem_alloc(int size)
{
    //TODO protect with semephore the whole function

    // get the memory ptr of the process
    pid_t requestingProcessID = getpid();
    void *processMemoryPtr = NULL;
    // check if process id is valid
    for (int i = 0; i < MAX_PROCESS_COUNT; i++)
    {
        if (usedData[i] > 0 && processData[i].processID == requestingProcessID)
        {
            processMemoryPtr = processData[i].ptr;
        }
    }
    if (processMemoryPtr == NULL) //check permission
    {
        printf("You do not have permission to allocate memory!\n");
        return NULL;
    }

    // can allocate a multiple of 8 bytes
    if (size % 8 != 0)
    {
        size = ((size / 8) + 1) * 8;
    }

    //If memory is not used before, initialize the first header.
    if (memoryUsed < 0)
    {
        *((int *)processMemoryPtr) = -1;
        *((int *)processMemoryPtr + ALLOCATION_PREV_OFFSET) = -1;
        memoryUsed = 1;
    }

    //return smem_firstFit(size, processMemoryPtr);
    return allocationAlgo(size, processMemoryPtr);
}

void smem_free(void *p)
{
    //TODO protect with semephore the whole function

    // get the memory ptr of the process
    pid_t requestingProcessID = getpid();
    void *processMemoryPtr = NULL;
    // check if process id is valid
    for (int i = 0; i < MAX_PROCESS_COUNT; i++)
    {
        if (processData[i].processID == requestingProcessID)
        {
            processMemoryPtr = processData[i].ptr;
        }
    }
    if (processMemoryPtr == NULL) //check permission
    {
        printf("You do not have permission to allocate memory!\n");
        return;
    }

    // scenario 1 - at the end of the list
    void *headPtr = p - HEADER_SIZE;

    if (((headPtr - processMemoryPtr) + HEADER_SIZE + *((int *)(headPtr + ALLOCATION_LENGTH_OFFSET))) == sharedMemorySize || *((int *)(headPtr + *((int *)(headPtr)))) < 0)
    {
        *((int *)(headPtr)) = -1;
        return;
    }

    // scneraio 2 - at the beginning of the list
    if (headPtr - processMemoryPtr == 0)
    {
        *((char *)(headPtr + ALLOCATION_USED_OFFSET)) = -1;
        void *nextHeaderPtr = headPtr + *((int *)(headPtr));
        if (*((int *)(nextHeaderPtr)) > 0 && *((char *)(nextHeaderPtr + ALLOCATION_USED_OFFSET)) == -1) // next ptr is alos empty, merge them
        {
            *((int *)(headPtr)) = *((int *)(headPtr)) + *((int *)(nextHeaderPtr));
        }
        return;
    }

    // scenario 3 - in the middle
    void *prevHeaderPtr = headPtr - *((int *)(headPtr + ALLOCATION_PREV_OFFSET));
    void *nextHeaderPtr = headPtr + *((int *)(headPtr));

    char isPrevUsed = *((char *)(prevHeaderPtr + ALLOCATION_USED_OFFSET));
    char isNextUsed = *((char *)(nextHeaderPtr + ALLOCATION_USED_OFFSET));
    if (isPrevUsed < 0 && isNextUsed < 0) //      Scenario 3.1 both prior and next are empty
    {
        void *usedNextPtr = nextHeaderPtr + *((int *)(nextHeaderPtr));
        *((int *)(prevHeaderPtr)) = usedNextPtr - prevHeaderPtr;
        *((int *)(usedNextPtr + ALLOCATION_PREV_OFFSET)) = usedNextPtr - prevHeaderPtr;
    }
    else if (isNextUsed < 0) //      Scenario 3.2 only next is empty
    {
        void *usedNextPtr = nextHeaderPtr + *((int *)(nextHeaderPtr));
        *((char *)(headPtr + ALLOCATION_USED_OFFSET)) = -1;
        *((int *)(headPtr)) = usedNextPtr - headPtr;
        *((int *)(usedNextPtr + ALLOCATION_PREV_OFFSET)) = usedNextPtr - headPtr;
    }
    else if (isPrevUsed < 0) //      Scenario 3.3 only prior is empty
    {
        *((int *)(prevHeaderPtr)) = nextHeaderPtr - prevHeaderPtr;
        *((int *)(nextHeaderPtr + ALLOCATION_PREV_OFFSET)) = nextHeaderPtr - prevHeaderPtr;
    }
    else //      Scenario 3.4 they are full
    {
        *((char *)(headPtr + ALLOCATION_USED_OFFSET)) = -1;
    }
}

int smem_close()
{
    //TODO protect with semephores the whole function

    if (sharedMemorySize < 0)
    {
        printf("MEM CANNOT SMEM_OPEN FAIL. Library is not initialized!\n");
        return -1;
    }

    // find the process
    pid_t processIDToClose = getpid();
    for (int i = 0; i < MAX_PROCESS_COUNT; i++)
    {
        if (usedData[i] > 0 && processData[i].processID == processIDToClose)
        {
            //TODO deallocate every memory used by the process

            //TODO unmap the memory
            int unmapRes = munmap(processData[i].ptr, sharedMemorySize);

            //TODO check unmap result
            if (unmapRes >= 0)
            {
                usedData[i] = -1;
                processCount--;

                printf("Library successfuly UNMAPPED process\n");
                return 0;
            }
            printf("Library COULD NOT unmap\n");
            return -1;
        }
    }
    printf("Requesting process is not using the library, denided service!\n");
    return -1;
}

//

//

//

/// Custom functions

void *smem_firstFit(int size, void *shmPtr)
{
    //find a suitable spot
    void *foundAddress = NULL;
    void *curMemPointer = shmPtr;
    void *prevAddress = NULL;
    while (*((int *)curMemPointer) > 0)
    {
        // current in between hole can be allocated to the requesting process
        if (*((char *)(curMemPointer + ALLOCATION_USED_OFFSET)) < 0 &&
            *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET)) >= size)
        {
            foundAddress = curMemPointer + HEADER_SIZE;
            int remSizeInHole = *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET)) - size;

            *((char *)(curMemPointer + ALLOCATION_USED_OFFSET)) = 1;
            *((pid_t *)(curMemPointer + ALLOCATION_PID_OFFSET)) = getpid();
            *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET)) = size;
            *((int *)(curMemPointer + ALLOCATION_PREV_OFFSET)) = curMemPointer - prevAddress;

            // There is enough space left for a small hole, handle connection
            if (remSizeInHole > HEADER_SIZE)
            {
                // connect handle
                void *newHoleAddress = foundAddress + size;
                *((int *)(newHoleAddress)) = curMemPointer + *((int *)(curMemPointer)) - newHoleAddress;
                *((int *)(curMemPointer)) = newHoleAddress - curMemPointer;
                *((int *)(newHoleAddress + ALLOCATION_PREV_OFFSET)) = newHoleAddress - curMemPointer;

                void *nextAddressPtr = newHoleAddress + *((int *)(newHoleAddress));
                *((int *)(nextAddressPtr + ALLOCATION_PREV_OFFSET)) = nextAddressPtr - newHoleAddress;

                //set settings of remaining hole
                *((char *)(newHoleAddress + ALLOCATION_USED_OFFSET)) = -1;
                *((int *)(newHoleAddress + ALLOCATION_LENGTH_OFFSET)) = remSizeInHole;
            }
            return foundAddress;
        }
        prevAddress = curMemPointer;
        curMemPointer += *((int *)(curMemPointer));
    }

    //a hole could not be found, allocate at the end of tail if there is memory
    if (sharedMemorySize - (curMemPointer - shmPtr) > HEADER_SIZE + size)
    {
        // allocate memory
        foundAddress = curMemPointer + HEADER_SIZE;
        *((char *)(curMemPointer + ALLOCATION_USED_OFFSET)) = 1;
        *((pid_t *)(curMemPointer + ALLOCATION_PID_OFFSET)) = getpid();
        *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET)) = size;
        *((int *)(curMemPointer + ALLOCATION_PREV_OFFSET)) = curMemPointer - prevAddress;

        // create new end tail
        void *tailHoleAddress = foundAddress + size;
        *((int *)(tailHoleAddress)) = -1;
        *((int *)(tailHoleAddress + ALLOCATION_PREV_OFFSET)) = tailHoleAddress - curMemPointer;
        *((int *)(curMemPointer)) = tailHoleAddress - curMemPointer;
    }
    return foundAddress;
}

void *smem_bestFit(int size, void *shmPtr)
{
    return (NULL);
}

void *smem_worstFit(int size, void *shmPtr)
{
    return (NULL);
}