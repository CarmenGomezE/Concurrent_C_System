#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "monitor.h"
#include "pow.h"

void monitor(int fd1[2], int fd2[2]) {
    long int target, solution;
    char buffer[80];

    read(fd1[0], &target, sizeof(target));
    read(fd1[0], &solution, sizeof(solution));

    if ((int)pow_hash(solution) == (int)target) {
        snprintf(buffer, sizeof(buffer), "SolutionAccepted");
    } else {
        snprintf(buffer, sizeof(buffer), "SolutionRejected");
    }

    write(fd2[1], buffer, strlen(buffer) + 1);

    kill(getpid(), SIGUSR2);

    close(fd1[0]);
    close(fd2[1]);
    exit(EXIT_SUCCESS);
}