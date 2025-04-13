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
#include "voter.h"
#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>
 
int n_procsV;
int *pidsVotersV;
sem_t *semFile2, *semCandidate2;
 
void read_systemInfo() {
    /*Read the process pids file*/
    int i, fd;
    char *ptr;
    ssize_t bytesRead;
    char buffer[256];

    fd = open(INFO_SISTEMA, O_RDONLY);
    if (fd == -1) {
        perror("Error abriendo el archivo");
        exit(EXIT_FAILURE);
    }

    bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0) {
        perror("Error leyendo n_procsV");
        close(fd);
        exit(EXIT_FAILURE);
    }
    buffer[bytesRead] = '\0';

    ptr = buffer;
    n_procsV = strtol(ptr, &ptr, 10);
    pidsVotersV = (int*) malloc(n_procsV * sizeof(pidsVotersV[0]));
    if (!pidsVotersV) {
        perror("Error alocando memoria");
        close(fd);
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < n_procsV; i++) {
        pidsVotersV[i] = strtol(ptr, &ptr, 10);
    }

    close(fd);
}
 
void handle_sigusr1(int argSig) {
    /*Code of the SIGUSR1 signal*/
    int i, count = 0, yes = 0, no = 0, fd;
    int votes[n_procsV];
    char buf[256];
    char *ptr ;
    ssize_t bytesRead;
 
    read_systemInfo();
    sem_wait(semCandidate2);

    for (i = 0; i < n_procsV; i++) {
        if (pidsVotersV[i] != getpid()) {
            kill(pidsVotersV[i], SIGUSR2);
        }
    }
 
    while (count < n_procsV - 1) {
        usleep(1000);

        sem_wait(semFile2);
        fd = open(VOTACION, O_RDONLY);
        if (fd== -1) {
            perror("Error abriendo archivo");
            sem_post(semFile2);
            free(pidsVotersV);
            exit(EXIT_FAILURE);
        }

        bytesRead = read(fd, buf, sizeof(buf) - 1);
        close(fd);

        if (bytesRead < 0) {
            perror("Error leyendo archivo");
            sem_post(semFile2);
            free(pidsVotersV);
            exit(EXIT_FAILURE);
        }

        buf[bytesRead] = '\0';
        ptr = buf;
        for (i = 0; i < n_procsV; i++) {
            if (pidsVotersV[i] != getpid()) {
                if (*ptr) {
                    votes[count++] = strtol(ptr, &ptr, 10);
                }
            }
        }

        fd = open(VOTACION, O_WRONLY | O_TRUNC, 0644);
        if (fd == -1) {
            perror("Error vaciando archivo");
            sem_post(semFile2);
            free(pidsVotersV);
            exit(EXIT_FAILURE);
        }
        close(fd);
        sem_post(semFile2);
    }
 
    printf("Candidate %d => [", getpid());
    fflush(stdout);
    for (i = 0; i < count; i++) {
        if (votes[i] == 1) {
            printf(" Y");
            fflush(stdout);
            yes++;
        } else {
            printf(" N");
            fflush(stdout);
            no++;
        }
    }
    printf(" ] => ");
    fflush(stdout);
    if (yes > no) {
        printf("Accepted\n");
        fflush(stdout);
    } else {
        printf("Rejected\n");
        fflush(stdout);
    }

    usleep(250000);

    for (i = 0; i < n_procsV; i++) {
        if (pidsVotersV[i] != getpid()) {
            kill(pidsVotersV[i], SIGUSR1);
        }
    }

    sem_post(semCandidate2);  
}
 
void handle_sigusr2(int argSig) {
    /*Code of the SIGUSR2 signal*/
    int vote, fd;
    char buffer[4];

    vote = rand() % 2;
    sem_wait(semFile2);
    fd = open(VOTACION, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        perror("Error abriendo archivo");
        free(pidsVotersV);
        exit(EXIT_FAILURE);
    }

    snprintf(buffer, sizeof(buffer), "%d\n", vote);
    write(fd, buffer, strlen(buffer));
    fsync(fd);
    close(fd);
    
    sem_post(semFile2);
}
 
void handle_sigterm(int argSig) {
    /*Code of the SIGTERM signal*/
    free(pidsVotersV);
    sem_close(semFile2);
    sem_close(semCandidate2);
    exit(EXIT_SUCCESS);
}
 
int voter( ) {
    struct sigaction act1, act2, act3;
    sigset_t set, old_set;

    /*Create the handler of the SIGUSR1 signal*/
    act1.sa_handler = handle_sigusr1;
    sigemptyset(&(act1.sa_mask));
    act1.sa_flags = 0;

    /*Create the handler of the SIGTERM signal*/
    act2.sa_handler = handle_sigterm;
    sigemptyset(&(act2.sa_mask));
    act2.sa_flags = 0;

    /*Create the handler of the SIGUSR2 signal*/
    act3.sa_handler = handle_sigusr2;
    sigemptyset(&(act3.sa_mask));
    act3.sa_flags = 0;

    /*Create and set the mask*/
    sigemptyset(&set); 
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGTERM);
    sigprocmask(SIG_BLOCK, &set, &old_set); 
 
    /*Configurate the handler of the SIGUSR1 signal*/
    if (sigaction(SIGUSR1, &act1, NULL) < 0) {
        perror("Error al configurar manejador de señales");
        exit(EXIT_FAILURE);
    }

    /*Configurate the handler of the SIGTERM signal*/
    if (sigaction(SIGTERM, &act2, NULL) < 0) {
        perror("Error al configurar manejador de señales");
        exit(EXIT_FAILURE);
    }

    /*Configurate the handler of the SIGUSR2 signal*/
    if (sigaction(SIGUSR2, &act3, NULL) < 0) {
        perror("Error al configurar manejador de señales");
        exit(EXIT_FAILURE);
    }

    /*Open the file for the voters to vote*/
    semFile2= sem_open(SEM_FILE, 0);
    if (semFile2==SEM_FAILED) {
        perror("Error inicializando semáforo");
        exit(EXIT_FAILURE);
    }
    
    /*Open the file to chose the candidate*/
    semCandidate2=sem_open(SEM_CANDIDATE,0);
    if (semCandidate2==SEM_FAILED) {
        perror("Error inicializando semáforo");
        exit(EXIT_FAILURE);
    }

    while (1) {
        sigsuspend(&old_set);
    }
         
    exit(EXIT_SUCCESS);
}