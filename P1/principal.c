/**
 * @file principal.c
 * @author Carmen Gómez, María Pozo.
 * @brief Implementation of the principal process that executes the program.
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
#include "miner.h"
#include "monitor.h"

/*Main program that implementates the principal process.*/
int main(int argc, char *argv[]){
    int rounds, n_threads, status;
    long int target;
    pid_t pidMinero;
    int fd1[2];
    int fd2[2];
    int pipe_status1, pipe_status2;

    if(argc!=4){
        perror("The arguments should be: <target> <rounds> <n_theads>");
        return EXIT_FAILURE;
    }

    target= strtol(argv[1], NULL,10);
    rounds= atoi(argv[2]);
    n_threads= atoi(argv[3]);

    if(rounds<0 || n_threads<0 || target<0){
        perror("The arguments should be positive.");
        return EXIT_FAILURE;
    }

    /*Creation of the first pipe: MINER ------> MONITOR.*/
    pipe_status1 = pipe(fd1);
    if (pipe_status1 == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
     /*Creation of the second pipe: MONITOR ------> MINER.*/
    pipe_status2 = pipe(fd2);
    if (pipe_status2 == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    /*Principal process creates Miner process.*/
    pidMinero= fork();
    if(pidMinero<0){
        perror("fork");
        exit(EXIT_FAILURE);
    }

    /*Code of the Miner process.*/
    if(pidMinero==0){
        miner(target, rounds, n_threads, fd1, fd2);
    }
    /* Close of the write end of miner in the first pipe.*/
    close(fd1[1]);
    /* Close of the read end of miner in the second pipe.*/
    close(fd2[0]);

    /*Waits for the Miner process to end and prints its end code.*/
    if (waitpid(pidMinero, &status, 0) > 0) {
        if (WIFEXITED(status)) {
            printf("Miner exited with status %d\n", WEXITSTATUS(status));
        } else {
            printf("Miner exited unexpectedly\n");
        }
    } else {
        perror("waitpid (Miner)");
    }
    /*Ends.*/
    exit(EXIT_SUCCESS);
}
