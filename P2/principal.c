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
#include <pthread.h>
#include <fcntl.h>

int n_procs;
int *pidsVoters;
sem_t *semFile, *semCandidate;

void handle_siginit(int sig){
    /*Code of the SIGINT signal*/
    int i;
    for (i = 0; i < n_procs; i++) {
        kill(pidsVoters[i], SIGTERM);
    }

    for (i = 0; i < n_procs; i++) {
        if (waitpid(pidsVoters[i], NULL, 0) == -1) {
            perror("Error esperando a los procesos Votante");
            free(pidsVoters);
            exit(EXIT_FAILURE);
        }
    }

    free(pidsVoters);
    sem_close(semFile);
    sem_close(semCandidate);
    sem_unlink(SEM_FILE);
    sem_unlink(SEM_CANDIDATE);
    printf("Finishing by signal\n");
    exit(EXIT_SUCCESS);
}

void handle_sigalrm(int sig) {
    /*Code of the SIGALRM signal*/
    int i;
    for (i = 0; i < n_procs; i++) {
        kill(pidsVoters[i], SIGTERM);
    }

    for (i = 0; i < n_procs; i++) {
        waitpid(pidsVoters[i], NULL, 0);
    }

    free(pidsVoters);
    sem_close(semFile);
    sem_close(semCandidate);
    sem_unlink(SEM_FILE);
    sem_unlink(SEM_CANDIDATE);
    printf("Finishing by alarm\n");
    exit(EXIT_SUCCESS);
}

/*Main program that implementates the principal process.*/
int main(int argc, char *argv[]) {
    int n_procs, n_secs, i, procFile;
    int *pidsVoters;
    sem_t *semFile, *semCandidate;
    pid_t pidVoter;
    struct sigaction act1, act2;
    char buffer[20];

    /*Create the handler of the SIGINT signal*/
    act1.sa_handler = handle_siginit;
    sigemptyset(&(act1.sa_mask));
    act1.sa_flags = 0;

    /*Create the handler of the SIGALARM signal*/
    act2.sa_handler = handle_sigalrm;
    sigemptyset(&(act2.sa_mask));
    act2.sa_flags = 0;

    /*Check and establish the recived values*/
    if (argc != 3) {
        perror("The arguments should be: <N_PROCS> <N_SECS>");
        exit(EXIT_FAILURE);
    }
    n_procs = atoi(argv[1]);
    n_secs = atoi(argv[2]);
    if (n_procs <= 0 || n_secs <= 0) {
        perror("The arguments should be positive");
        exit(EXIT_FAILURE);
    }

    /*Configurate the handler of the SIGINT signal*/
    if (sigaction(SIGINT, &act1, NULL) < 0) {
        perror("Error al configurar manejador de señales");
        return EXIT_FAILURE;
    }

     /*Configurate the handler of the SIGALARM signal*/
    if (sigaction(SIGALRM, &act2, NULL) < 0) {
        perror("Error al configurar manejador de señales");
        return EXIT_FAILURE;
    }

    /*Open the file and write the child pids*/
    procFile = open(INFO_SISTEMA, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (procFile == -1) {
        perror("Error abriendo archivo");
        exit(EXIT_FAILURE);
    }
    snprintf(buffer, sizeof(buffer), "%d\n", n_procs);
    write(procFile, buffer, strlen(buffer));

    /*Create the file for the voters to vote*/
    sem_unlink(SEM_FILE);
    semFile = sem_open(SEM_FILE, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (semFile == SEM_FAILED) {
        perror("Error inicializando semáforo de archivo");
        close(procFile);
        exit(EXIT_FAILURE);
    }

    /*Create the file to chose the candidate*/
    sem_unlink(SEM_CANDIDATE); 
    semCandidate = sem_open(SEM_CANDIDATE, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (semCandidate == SEM_FAILED) {
        perror("Error inicializando semáforo de candidato");
        sem_close(semFile);
        sem_unlink(SEM_FILE);
        close(procFile);
        exit(EXIT_FAILURE);
    }

    /*Create an array for the pids of the childs*/
    pidsVoters = (int*) malloc(n_procs*sizeof(pidsVoters[0]));
    if (!pidsVoters) {
        perror("Error alocando memoria");
        close(procFile);
        sem_close(semFile);
        sem_close(semCandidate);
        sem_unlink(SEM_FILE);
        sem_unlink(SEM_CANDIDATE);
        exit(EXIT_FAILURE);
    }

    /*Create the voters processes*/
    for (i = 0; i < n_procs; i++) {
        pidVoter = fork();

        if (pidVoter == -1) {
            perror("Error creando un proceso Votante");
            close(procFile);
            sem_close(semFile);
            sem_close(semCandidate);
            sem_unlink(SEM_FILE);
            sem_unlink(SEM_CANDIDATE);
            free(pidsVoters);
            exit(EXIT_FAILURE);
        } else if (pidVoter == 0) {
            /*Code of the voter processes*/
            signal(SIGINT, SIG_IGN);
            voter(semFile, semCandidate); 
        } else {
            /*Write in the file the pid of the childs*/
            pidsVoters[i] = pidVoter;
            snprintf(buffer, sizeof(buffer), "%d\n", pidsVoters[i]);
            write(procFile, buffer, strlen(buffer));
        }
    }

    /*Close the file*/
    close(procFile);

    sleep(1);

    /*Send SIGUSR1 to the voters*/
    for (i = 0; i < n_procs; i++) {
        kill(pidsVoters[i], SIGUSR1);
    }

    /*Set the alarm*/
    alarm(n_secs);

    for (i = 0; i < n_procs; i++) {
        waitpid(pidsVoters[i], NULL, 0);
    }   


    sem_close(semFile);
    sem_close(semCandidate);
    sem_unlink(SEM_FILE);
    sem_unlink(SEM_CANDIDATE);
    free(pidsVoters);
    exit(EXIT_SUCCESS);
}
