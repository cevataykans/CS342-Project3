#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "smemlib.h"

int getRandomInt(int i){
    srand(i);
	int j = rand() % 12;
	return j;
}

int main()
{
    smem_init(32768);

    int failCount = 0;

    int libStatus = smem_open();
    if (libStatus >= 0)
    {
        int* pointers[12] = {NULL};
        int count[12] = {0};
        
        for(int i = 0 ; i < 1000 ; i++){
            
            int index = getRandomInt(i);
            //printf("%d\n", index);
            count[index]++;
            if(pointers[index] == NULL){
                pointers[index] = smem_alloc((index+1)*1024);
                if(pointers[index] == NULL) failCount++;
            }
            else{
                smem_free(pointers[index]);
                pointers[index] = NULL;
            }
        }
       
        for(int i = 0 ; i < 12 ; i++){
            printf("%d\n", count[i]);
        }
    }
    smem_close();
    printf("Fail rate is %f\n", failCount/1000.0);
    return 0;
}
