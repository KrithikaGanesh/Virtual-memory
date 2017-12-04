#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "harness.h"

#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>
void check_for_multiple_processes();
void check_malloc();
int main(int argc, char ** argv) {
    check_malloc();
    check_for_multiple_processes();



    return 0;
}

void check_for_multiple_processes()
{
    printf("Multiple processes test\n");
    init_petmem();
    if (fork()==0){
        printf("Process 1: Child\n");
        char * buf ;
        buf= pet_malloc(8096);
        printf("Allocated 1 page at %p\n", buf);
        pet_dump();
        buf[50] = 'O';
        buf[51] = 'S';
        printf("%s\n", (char *)(buf + 50));
        pet_free(buf);

    }
    else{
        printf("Process 2: Parent\n");
        char * buf2 ;
        buf2= pet_malloc(8096);
        printf("Allocated 1 page at %p\n", buf2);
        pet_dump();
        buf2[3] = 'S';
        buf2[4] = 'E';
        printf("%s\n", (char *)(buf2 + 3));
        pet_free(buf2);
    }

    printf("--------------------------------------------\n");

}
void check_malloc()
{
    printf("Simple Malloc test\n");
    //small block
    init_petmem();
    double * a = pet_malloc(10*sizeof(double));
    if(a==NULL)
    {
        printf("Malloc does not wor");
    }
    else{
        a[0]=56.6;
        a[9]=100.1;
        printf("%f %f\n",a[0],a[9]);
        printf("Malloc works\n");
        pet_free(a);
    }
    printf("--------------------------------------------\n");
    printf("Malloc NULL test\n");
    char *b;
    if ((b = pet_malloc(0))!=NULL)
    {
        printf("Malloc returns non NULL pointer: FAIL\n");
    }
    else
    {
        printf("Malloc returns NULL pointer:PASS\n");
    }


}
