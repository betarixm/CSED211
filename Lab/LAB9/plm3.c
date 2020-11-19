#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
    pid_t proc = atoi(argv[1]);
    int signal = atoi(argv[2]);

    kill(proc, signal);
    return 0;
}