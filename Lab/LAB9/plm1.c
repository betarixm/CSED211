#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    pid_t pid;

    int x;
    x = 0;

    pid = fork();
    if(pid != 0){
        x = 1;
        printf("parent PID : %ld, x : %d ,\n", getpid(), x);
    } else {
        x = 2;
        printf("child PID : %ld, x : %d\n", getpid(), x);
    }

    return 0;
}