/**
 * @file voter.c
 * @author Carmen Gómez, María Pozo.
 * @brief Implementation of the voters process.
 * @version 2.0
 * @date 2025-03-07
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

void handle_sigusr1(int argSignal){
}

void handle_sigterm(int argSignal){
    exit(EXIT_SUCCESS);
}


int voter( ){
    if (signal(SIGUSR1, handle_sigusr1) == SIG_ERR) {
        perror("Error al configurar manejador de señales");
        return EXIT_FAILURE;
    }
    if (signal(SIGTERM, handle_sigterm) == SIG_ERR) {
        perror("Error al configurar manejador de señales");
        return EXIT_FAILURE;
    }
    while(1){
        pause();
    }
        
    exit(EXIT_SUCCESS);
}