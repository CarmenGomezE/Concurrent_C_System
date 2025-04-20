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
#include "monitor.h"

void create_logger_process() {
    pid_t pid = fork();
    if (pid == 0) {
        execl("./logger", "logger", NULL);
        perror("Error ejecutando logger");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]){
    int sec, n_threads;

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

    create_logger_process();
    start_miner(sec, n_threads);

    exit(EXIT_SUCCESS);
}


