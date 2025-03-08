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

void handle_sigusr1(int sig){
}

void handle_sigterm(int sig){
    exit(EXIT_SUCCESS);
}


int voter( ){
    struct sigaction act1;
    act1.sa_handler = handle_sigusr1;
    sigemptyset(&(act1.sa_mask));
    act1.sa_flags = 0;

    struct sigaction act2;
    act2.sa_handler = handle_sigterm;
    sigemptyset(&(act2.sa_mask));
    act2.sa_flags = 0;

    signal(SIGINT, SIG_IGN);

    if (sigaction(SIGUSR1, &act1, NULL) <0) {
        perror("Error al configurar manejador de señales");
        return EXIT_FAILURE;
    }
    if (sigaction(SIGTERM, &act2, NULL) <0) {
        perror("Error al configurar manejador de señales");
        return EXIT_FAILURE;
    }
    while(1){
        pause();
    }
        
    exit(EXIT_SUCCESS);
}