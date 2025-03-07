/**
 * @file principal.c
 * @author Carmen Gómez, María Pozo.
 * @brief Implementation of the principal process.
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
#include "voter.h"

int n_procs;
pid_t *pidsVoters;


void handle_siginit(int argSignal){
    int procCounter;
    
    for(procCounter=0; procCounter<n_procs; procCounter++){
        kill(pidsVoters[procCounter], SIGTERM);
    }
}

/*Main program that implementates the principal process.*/
int main(int argc, char *argv[]){
    int n_secs, procCounter, status;
    pid_t pidVoter;
    FILE *procFile;

    if(argc!=3){
        perror("The arguments should be: <N_PROCS> <N_SECS>");
        return EXIT_FAILURE;
    }

    n_procs= atoi(argv[1]);
    n_secs= atoi(argv[2]);

    if(n_procs<0 || n_secs<0){
        return EXIT_FAILURE;
    }

    if (signal(SIGINT, handle_siginit) == SIG_ERR) {
        perror("Error al configurar manejador de señales");
        return EXIT_FAILURE;
    }

    procFile= fopen("ficheroProcesos.txt", "w");

    if(procFile==NULL){
        perror("Failed to open file");
        return EXIT_FAILURE;
    }

    pidsVoters= malloc(n_procs*sizeof(pidsVoters[0]));

    if(pidsVoters==NULL){
        perror("Failed to reserve memory");
        return EXIT_FAILURE;
    }

    /*Create the voters processes*/
    for(procCounter=0; procCounter<n_procs; procCounter++){
        pidVoter=fork();

        if(pidVoter==-1){
            perror("Failed to create voter");
            exit(EXIT_FAILURE);
        }

        if(pidVoter==0){
            voter(); 
            perror("Error al ejecutar el proceso votante");
            exit(EXIT_FAILURE);
        }
        else{
            pidsVoters[procCounter]=pidVoter;
        }
    }

    /*Write the voters PID in a file*/
    for(procCounter=0; procCounter<n_procs; procCounter++){
        fprintf(procFile, "%d", pidsVoters[procCounter]);
    }

    /*Sistem is active during n_secs seconds*/
    sleep(n_secs);
    printf("Ha pasado el tiempo\n");

    /*Wait for all the childs*/
    for(procCounter=0; procCounter<n_procs; procCounter++){
        if (waitpid(pidsVoters[procCounter], &status, 0) >= 0) {
            perror("waitpid (Child process)");
        }
    }

    free(pidsVoters);
    printf("Finishing by signal");
    exit(EXIT_SUCCESS);
}
