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

/*Inicializar la máscara a 0, bloquear todas las señales que se van a usar*/
/*Configurar el manejador*/
/*Semáforo 1 de exclusiçon mutua para entrar en los ficheros*/
/*Semáforo 2 para elegir al candidato*/
/*Inicializar la simulación*/
/*Iniciar votos-> crear procesos hijos que ejecuta voter, usa los dos semáforos, los cierra antes de terminar*/
/*Cada proceso debería leer la estructura y participar
- espera que le llegue la señal(bloqueado)
- lee la red
- intentar ser candidato, le pasa el semaforo y lo intentas bajar
- CANDIDATO: envia la señal a los demás(tiene los pids), espera los votos, finalizo la ronda(libero el semaforo y señal)
- VOTANTE: voto y lo guardo, y sigo hasta que pase algo que diga que pare*/
/*El padre controla la simulación, y escribe la estructura en el fichero y espera las señales*/
/*Configurar la */

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
#include <semaphore.h>

typedef struct {
    pthread_mutex_t semaphoreMutex; 
    sem_t semaphore;      
    int shared_counter;
} Semaphore;

void initializeShemaphore(Semaphore *sh) {
    if (pthread_mutex_init(&sh->semaphoreMutex, NULL) != 0) {
        perror("Error al inicializar el mutex");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&sh->semaphore, 0, 1) != 0) { 
        perror("Error al inicializar el semáforo");
        exit(EXIT_FAILURE);
    }

    sh->shared_counter = 0;
}

void destroyShemaphore(Semaphore *sh) {
    pthread_mutex_destroy(&sh->semaphoreMutex);
    sem_destroy(&sh->semaphore);
}

void handle_siginit(int sig){
    pid_t *pidsVoters;
    int n_procs, procCounter, status;
    FILE *f =NULL; 
    
    f= fopen(INFO_SISTEMA, "r");

    if (!f) {
        perror("Error abriendo el archivo");
        exit(EXIT_FAILURE);
    }

    if(fscanf(f, "%d", &n_procs) != 1) {
        perror("Error leyendo n_procs");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    pidsVoters= (int*) malloc (n_procs*sizeof(pidsVoters[0]));
    if (!pidsVoters) {
        perror("Error alocando memoria");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < n_procs; i++) {
        if(fscanf(f, "%d", &pidsVoters[i]) != 1) {
            perror("Error leyendo pidsVoters");
            free(pidsVoters);
            fclose(f);
            exit(EXIT_FAILURE);
        }
    }
    
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
    int n_procs;
    pid_t *pidsVoters;
    Semaphore sh;
    struct sigaction act;
    act.sa_handler = handle_siginit;
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

    initializeShemaphore(&sh);

    procFile= fopen("ficheroProcesos.txt", "w");

    if(procFile==NULL){
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

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

    destroyShemaphore(&sh);
    exit(EXIT_SUCCESS);
}
