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
    FILE *f = fopen(INFO_SISTEMA, "r");
    if (!f) {
        perror("Error abriendo el archivo");
        exit(EXIT_FAILURE);
    }
 
    if(fscanf(f, "%d", &n_procsV) != 1) {
        perror("Error leyendo n_procsV");
        fclose(f);
        exit(EXIT_FAILURE);
    }
 
    pidsVotersV = (int*) malloc (n_procsV*sizeof(pidsVotersV[0]));
    if (!pidsVotersV) {
        perror("Error alocando memoria");
        fclose(f);
        exit(EXIT_FAILURE);
    }
 
    for (i = 0; i < n_procsV; i++) {
        if(fscanf(f, "%d", &pidsVotersV[i]) != 1) {
            perror("Error leyendo pidsVotersV");
            free(pidsVotersV);
            fclose(f);
            exit(EXIT_FAILURE);
        }
    }
 
    fclose(f);
}
 
void handle_sigusr1(int argSig) {
    int i, count = 0, yes = 0, no = 0;
    FILE *f;
    int votes[n_procsV];
 
    printf("El proceso Votante %d ha recibido la señal SIGUSR1.\n", getpid());
    fflush(stdout);
    read_systemInfo();
    printf("El proceso Votante %d ha leído la información del sistema.\n", getpid());
    fflush(stdout);

    sem_wait(semCandidate);
    
    printf("El proceso Votante %d es el Candidato.\n", getpid());
    fflush(stdout);

    for (i = 0; i < n_procsV; i++) {
        if (pidsVotersV[i] != getpid()) {
            kill(pidsVotersV[i], SIGUSR2);
        }
    }
 
    while (count < n_procsV - 1) {
        usleep(1000);

        sem_wait(semFile);
 
        f = fopen(VOTACION, "r");
        if (!f) {
            perror("Error abriendo archivo");
            sem_post(semFile);
            free(pidsVotersV);
            exit(EXIT_FAILURE);
        }
 
        for (i = 0; i < n_procsV; i++) {
            if (pidsVotersV[i] != getpid()) {
                if (fscanf(f, "%d", &votes[i]) == 1) {
                    count++;
                }
            }
        }
 
        fclose(f);
        if(!(f = fopen(VOTACION, "w"))) {
            perror("Error vaciando archivo");
            sem_post(semFile);
            free(pidsVotersV);
            exit(EXIT_FAILURE);
        }
        fclose(f);
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
    printf("El proceso Votante %d ha recibido la señal SIGUSR2.\n", getpid());

    sem_wait(semFile);
    FILE *f = fopen(VOTACION, "a");
    if (!f) {
        perror("Error abriendo archivo");
        free(pidsVotersV);
        exit(EXIT_FAILURE);
    }
 
    fprintf(f, "%d\n", vote);
    fclose(f);
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
 
    sigset_t mask;
 
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
 
    sigemptyset(&mask);
    sigprocmask(SIG_BLOCK, &mask, NULL);

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
 
    printf("El proceso Votante %d está listo.\n", getpid());
    fflush(stdout);
 
    while (1) {
        sigsuspend(&mask);
    }
         
    exit(EXIT_SUCCESS);
}