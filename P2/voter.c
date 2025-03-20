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
#include <fcntl.h>
 
int n_procsV;
int *pidsVotersV;
sem_t *semCandidate;
sem_t *semFile;
 
void read_systemInfo() {
    int i;
    int fd = open(INFO_SISTEMA, O_RDONLY);
    if (fd == -1) {
        perror("Error abriendo el archivo");
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0) {
        perror("Error leyendo n_procsV");
        close(fd);
        exit(EXIT_FAILURE);
    }
    buffer[bytesRead] = '\0';

    char *ptr = buffer;
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
    int i, count = 0, yes = 0, no = 0;
    int fd;
    int votes[n_procsV];
 
    read_systemInfo();
    sem_wait(semCandidate);

    for (i = 0; i < n_procsV; i++) {
        if (pidsVotersV[i] != getpid()) {
            kill(pidsVotersV[i], SIGUSR2);
        }
    }
 
    while (count < n_procsV - 1) {
        usleep(1000);

        sem_wait(semFile);
        fd = open(VOTACION, O_RDONLY);
        if (fd== -1) {
            perror("Error abriendo archivo");
            sem_post(semFile);
            free(pidsVotersV);
            exit(EXIT_FAILURE);
        }

        char buf[256];
        ssize_t bytesRead = read(fd, buf, sizeof(buf) - 1);
        close(fd);

        if (bytesRead < 0) {
            perror("Error leyendo archivo");
            sem_post(semFile);
            free(pidsVotersV);
            exit(EXIT_FAILURE);
        }

        buf[bytesRead] = '\0';
        char *ptr = buf;
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
            sem_post(semFile);
            free(pidsVotersV);
            exit(EXIT_FAILURE);
        }
        close(fd);
        sem_post(semFile);
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

    sem_post(semCandidate);  
}
 
void handle_sigusr2(int argSig) {
    int vote = rand() % 2;

    sem_wait(semFile);
    int fd = open(VOTACION, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        perror("Error abriendo archivo");
        free(pidsVotersV);
        exit(EXIT_FAILURE);
    }

    char buffer[4];
    snprintf(buffer, sizeof(buffer), "%d\n", vote);
    write(fd, buffer, strlen(buffer));
    fsync(fd);
    close(fd);
    
    sem_post(semFile);
}
 
void handle_sigterm(int argSig) {
    sem_close(semFile);
    sem_close(semCandidate);
    free(pidsVotersV);
    exit(EXIT_SUCCESS);
}
 
int voter( ) {
    struct sigaction act1;
    act1.sa_handler = handle_sigusr1;
    sigemptyset(&(act1.sa_mask));
    act1.sa_flags = 0;

    struct sigaction act2;
    act2.sa_handler = handle_sigterm;
    sigemptyset(&(act2.sa_mask));
    act2.sa_flags = 0;
 
    struct sigaction act3;
    act3.sa_handler = handle_sigusr2;
    sigemptyset(&(act3.sa_mask));
    act3.sa_flags = 0;
 
    sigset_t set;
 
    if (sigaction(SIGUSR1, &act1, NULL) < 0) {
        perror("Error al configurar manejador de señales");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGTERM, &act2, NULL) < 0) {
        perror("Error al configurar manejador de señales");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGUSR2, &act3, NULL) < 0) {
        perror("Error al configurar manejador de señales");
        exit(EXIT_FAILURE);
    }
 
    sigfillset(&set); 
    sigdelset(&set, SIGUSR1);
    sigdelset(&set, SIGTERM); 
    sigdelset(&set, SIGUSR2);
    sigprocmask(SIG_BLOCK, &set, NULL);

    sem_unlink(NOMBRE_SEM3);
    semFile = sem_open(NOMBRE_SEM3, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (semFile == SEM_FAILED) {
        perror("Error inicializando semáforo");
        exit(EXIT_FAILURE);
    }
    
    sem_unlink(NOMBRE_SEM2);
    semCandidate = sem_open(NOMBRE_SEM2, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (semCandidate == SEM_FAILED) {
        perror("Error inicializando semáforo");
        exit(EXIT_FAILURE);
    }
 
    while (1) {
        sigsuspend(&set);
    }
         
    exit(EXIT_SUCCESS);
}