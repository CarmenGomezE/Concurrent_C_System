/**
 * @file registrador.h
 * @author Carmen Gómez, María Pozo.
 * @brief Implementation of the Resgitrador program.
 * @version 2.0
 * @date 2024-02-01
 *
 * @copyright Copyright (c) 2022
 *
 */
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

/**
 * @brief Process Registrador
 * @param pipefd Pipe file descriptor
 */
void registrador(int *pipefd) {
    Block b;
    ssize_t bytesLeidos;
    pid_t ppid;
    char filename[64];
    int fd, i;

    ppid = getppid();
    snprintf(filename, sizeof(filename), "registro_%d.txt", ppid);

    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    while ((bytesLeidos = read(pipefd[0], &b, sizeof(Block))) > 0) {
        dprintf(fd, "Id : %04d\n", b.id_block);
        dprintf(fd, "Winner : %d\n", b.winner_pid);
        dprintf(fd, "Target : %ld\n", b.target);
        if(b.validated==1){
            dprintf(fd, "Solution : %ld ( validated )\n", b.solution);
        } else{
        dprintf(fd, "Solution : %ld ( rejected )\n", b.solution);
        }
        dprintf(fd, "Votes : %d/%d\n", b.favor_votes, b.total_votes);

        dprintf(fd, "Wallets : ");
        for (i = 0; i < b.num_wallets; i++) {            
            dprintf(fd, "%d:%02d ", b.wallets[i].pid, b.wallets[i].coins);
        }
        dprintf(fd, "\n\n");
    } 
    exit(EXIT_SUCCESS);
}
