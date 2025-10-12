#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s seed array_size\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        char *args[] = {"./sequential_min_max", argv[1], argv[2], NULL};
        execvp(args[0], args);

        perror("exec failed");
        exit(1);
    } else {
        wait(NULL);
    }

    return 0;
}
