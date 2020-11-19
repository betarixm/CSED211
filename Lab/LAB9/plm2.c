#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    char* execve_argv[]={"/bin/ls", "-al", NULL};
    char* execve_envn[] = {NULL};
    execve(execve_argv[0], execve_argv, execve_envn);
    return 0;
}