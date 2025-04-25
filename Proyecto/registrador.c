#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "monitor.h"
#include "pow.h"
#include "memoriaMiner.h"
#include "miner.h"

void registrador(int *pipefd) {
    Bloque b;
    ssize_t bytesLeidos;
    pid_t ppid;
    char filename[64];
    int fd, i;

    close(pipefd[1]);
    ppid = getppid();
    snprintf(filename, sizeof(filename), "registro_%d.txt", ppid);

    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    while ((bytesLeidos = read(pipefd[0], &b, sizeof(Bloque))) > 0) {
        dprintf(fd, "d : %04d\n", b.id);
        dprintf(fd, "Winner : %d\n", b.winner_pid);
        dprintf(fd, "Target : %ld\n", b.target);
        dprintf(fd, "Solution : %ld ( validated )\n", b.solution);
        dprintf(fd, "Votes : %ls/%d\n", b.votes, MAX_MINERS);

        dprintf(fd, "Wallets : ");
        for (i = 0; i < MAX_MINERS; i++) {
            if (b.miner_coins[i] > 0) {
                dprintf(fd, "%d:%02ld ", i, b.miner_coins[i]);
            }
        }
        dprintf(fd, "\n\n");
    } 
    close(pipefd[0]);
    printf("Registrador ha finalizado");
    exit(EXIT_SUCCESS);
}
