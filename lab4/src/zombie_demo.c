#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        printf("Child: exiting, pid=%d\n", getpid());
        _exit(0);
    } else {
        printf("Parent: child pid=%d, parent pid=%d\n", pid, getpid());
        printf("Parent: sleeping 30s — проверьте другой терминал на наличие зомби\n");
        sleep(30);

        int status;
        pid_t w = waitpid(pid, &status, 0);
        if (w > 0) {
            printf("Parent: reaped child %d\n", w);
        } else {
            perror("waitpid");
        }

        return 0;
    }
}
