/**
 * @file mainMiner.c
 * @author Carmen Gómez, María Pozo.
 * @brief Implementation of the main of the miner process.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "pow.h"
#include "miner.h"
#include "registrador.h"
#include "monitor.h"

int main(int argc, char *argv[]){
    int sec, n_threads, status=0;
    pid_t pid;

    if(argc!=3){
        perror("The arguments should be ./miner <N_SECONDS> <N_THREADS>");
        exit(EXIT_FAILURE);
    }

    sec= atoi(argv[1]);
    n_threads= atoi(argv[1]);

    if(sec<=0 || n_threads<=0){
        perror("The arguments should be positive");
        exit(EXIT_FAILURE);
    }

    /** Tubería para mandar al registrador */
    int pipefd[2];  
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    /** Código del registrador */
    if (pid == 0) {
        registrador(pipefd);
        perror("Error en el registrador");
        exit(EXIT_FAILURE);
    } else{
            /** Código del minero */
        miner(pipefd, sec, n_threads);

        waitpid(pid, &status, 0);
        if(!WIFEXITED(status) || WEXITSTATUS(status)!= EXIT_SUCCESS){
            printf("Aviso: el Registrador no terminó correctamente\n");
        }
        miner_cleanup();
    }

    close(pipefd[1]);
    close(pipefd[0]);

    exit(EXIT_SUCCESS);
}


