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
#include <fcntl.h>

int n_procs;
int *pidsVoters;

void handle_siginit(int sig){
    int i;
    for (i = 0; i < n_procs; i++) {
        kill(pidsVoters[i], SIGTERM);
    }

    free(pidsVoters);
    printf("Finishing by signal\n");
    exit(EXIT_SUCCESS);
}

void handle_sigalrm(int sig) {
    int i;
    for (i = 0; i < n_procs; i++) {
        kill(pidsVoters[i], SIGTERM);
    }

    for (i = 0; i < n_procs; i++) {
        waitpid(pidsVoters[i], NULL, 0);
    }

    free(pidsVoters);
    printf("Finishing by alarm\n");
    exit(EXIT_SUCCESS);
}

/*Main program that implementates the principal process.*/
int main(int argc, char *argv[]) {
    int n_secs, i;
    pid_t pidVoter;
    int procFile;
    struct sigaction act1, act2;
    char buffer[20];

    act1.sa_handler = handle_siginit;
    sigemptyset(&(act1.sa_mask));
    act1.sa_flags = 0;

    act2.sa_handler = handle_sigalrm;
    sigemptyset(&(act2.sa_mask));
    act2.sa_flags = 0;

    if (argc != 3) {
        perror("The arguments should be: <N_PROCS> <N_SECS>");
        exit(EXIT_FAILURE);
    }

    n_procs = atoi(argv[1]);
    n_secs = atoi(argv[2]);

    if (sigaction(SIGINT, &act1, NULL) < 0) {
        perror("Error al configurar manejador de señales");
        return EXIT_FAILURE;
    }

    if (sigaction(SIGALRM, &act2, NULL) < 0) {
        perror("Error al configurar manejador de señales");
        return EXIT_FAILURE;
    }

    procFile = open(INFO_SISTEMA, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (procFile == -1) {
        perror("Error abriendo archivo");
        exit(EXIT_FAILURE);
    }

    snprintf(buffer, sizeof(buffer), "%d\n", n_procs);
    write(procFile, buffer, strlen(buffer));

    pidsVoters = (int*) malloc(n_procs*sizeof(pidsVoters[0]));
    if (!pidsVoters) {
        perror("Error alocando memoria");
        close(procFile);
        exit(EXIT_FAILURE);
    }

    /*Create the voters processes*/
    for (i = 0; i < n_procs; i++) {
        pidVoter = fork();

        if (pidVoter == -1) {
            perror("Error creando un proceso Votante");
            close(procFile);
            free(pidsVoters);
            exit(EXIT_FAILURE);
        } else if (pidVoter == 0) {
            signal(SIGINT, SIG_IGN);
            voter(); 
        } else {
            pidsVoters[i] = pidVoter;
            snprintf(buffer, sizeof(buffer), "%d\n", pidsVoters[i]);
            write(procFile, buffer, strlen(buffer));
        }
    }

    close(procFile);

    sleep(1);

    /*Send SIGUSR1 to the voters*/
    for (i = 0; i < n_procs; i++) {
        kill(pidsVoters[i], SIGUSR1);
    }

    alarm(n_secs);
    
    for (i = 0; i < n_procs; i++) {
        waitpid(pidsVoters[i], NULL, 0);
    }

    free(pidsVoters);
}
