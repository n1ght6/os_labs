#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <seed> <array_size>\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        
        execl("./sequential_min_max", "sequential_min_max", argv[1], argv[2], NULL);
        perror("execl"); 
        exit(1);
    } else {
        
        wait(NULL);
        printf("Child process finished\n");
    }

    return 0;
}
