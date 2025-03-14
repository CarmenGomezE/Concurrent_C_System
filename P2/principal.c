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
#include <sys/signal.h>
#include <sys/wait.h>
#include "voter.h"

int n_procs;
pid_t *pidsVoters;


void handle_sigint(int sig){
    fflush(stdout);
    
    int procCounter, status;
    
    for(procCounter=0; procCounter<n_procs; procCounter++){
        kill(pidsVoters[procCounter], SIGTERM);
    }

    /*Wait for all the childs*/
    for(procCounter=0; procCounter<n_procs; procCounter++){
        if (waitpid(pidsVoters[procCounter], &status, 0)==-1) {
            perror("Waitpid failed");
            free(pidsVoters);
            exit(EXIT_FAILURE);
        }
    }

    free(pidsVoters);
    printf("Finishing by signal\n");
    exit(EXIT_SUCCESS);
}

/*Main program that implementates the principal process.*/
int main(int argc, char *argv[]){
    int n_secs, procCounter;
    pid_t pidVoter;
    FILE *procFile;
    struct sigaction act;
    act.sa_handler = handle_sigint;
    sigemptyset(&(act.sa_mask));
    act.sa_flags = 0;

    if(argc!=3){
        perror("The arguments should be: <N_PROCS> <N_SECS>");
        exit(EXIT_FAILURE);
    }

    n_procs= atoi(argv[1]);
    n_secs= atoi(argv[2]);

    if (sigaction(SIGINT, &act, NULL) <0) {
        perror("Error al configurar manejador de señales");
        return EXIT_FAILURE;
    }

    procFile= fopen("ficheroProcesos.txt", "w");

    if(procFile==NULL){
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    fprintf(procFile, "%d\n", n_procs);

    pidsVoters= malloc(n_procs*sizeof(pidsVoters[0]));

    if(pidsVoters==NULL){
        perror("Failed to reserve memory");
        fclose(procFile);
        exit(EXIT_FAILURE);
    }

    /*Create the voters processes*/
    for(procCounter=0; procCounter<n_procs; procCounter++){
        pidVoter=fork();

        if(pidVoter==-1){
            perror("Failed to create voter");
            fclose(procFile);
            free(pidsVoters);
            exit(EXIT_FAILURE);
        }

        if(pidVoter==0){
            signal(SIGINT, SIG_IGN);
            voter(); 
        }
        else{
            pidsVoters[procCounter]=pidVoter;
        }
    }

    sleep(1);


    /*Write the voters PID in a file and send SIGUSR1 to the voters*/
    for(procCounter=0; procCounter<n_procs; procCounter++){
        fprintf(procFile, "%d\n", pidsVoters[procCounter]);
        kill(pidsVoters[procCounter], SIGUSR1);
    }
    fclose(procFile);

    for (procCounter= 0; procCounter < n_procs; procCounter++) {
        waitpid(pidsVoters[procCounter], NULL, 0);
    }

    /*Sistem is active during n_secs seconds*/
    sleep(n_secs);

    exit(EXIT_SUCCESS);
}
