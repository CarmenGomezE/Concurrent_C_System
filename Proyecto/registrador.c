#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "monitor.h"
#include "pow.h"
#include "memoria.h"

void registrador(int *pipefd) {
    BloqueMessage b;
    FILE *file; 
    ssize_t bytesLeidos;
    pid_t ppid;
    char filename[64];
    int fd, i;

    close(pipefd[1]);
    pid_t ppid = getppid();
    snprintf(filename, sizeof(filename), "registro_%d.txt", ppid);

    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    while ((bytesLeidos = read(pipefd[0], &b, sizeof(Bloque))) > 0) {
        dprintf(fd, "Id: %d\nWinner: %s\nTarget: %d\nSolution: %d\nVotes: %d\n", b.block.id, b.block.winner_pid, 
                                                                                 b.block.target, b.block.solution, b.block.votes);
        for(i=0; i<MAX_MINERS; i++){
            dprintf();
        }
    } 
    close(pipefd[0]);
    printf("Registrador ha finalizado");
    exit(EXIT_SUCCESS);
}
