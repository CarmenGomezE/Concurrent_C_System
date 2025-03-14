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

#define INFO_SISTEMA "ficheroProcesos.txt"
#define VOTACION "ficheroVotacion.txt"

int n_procsV;
int *pidsVotersV;
int is_candidate = 0;

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
    int i, count = 0, yes, no;
    FILE *f;
    int votes[n_procsV];

    printf("El proceso Votante %d ha recibido la señal SIGUSR1.\n", getpid());
    fflush(stdout);
    read_systemInfo();
    printf("El proceso Votante %d ha leído la información del sistema.\n", getpid());
    fflush(stdout);

    while(1) {
        sleep(1);
        if (!is_candidate) {
            is_candidate = 1;
            printf("El proceso Votante %d es el candidato.\n", getpid());
            fflush(stdout);

            for (i = 0; i < n_procsV; i++) {
                if (pidsVotersV[i] != getpid()) {
                    kill(pidsVotersV[i], SIGUSR2);
                }
            }

            while (count < n_procsV) {
                sleep(0.001);
                f = fopen(VOTACION, "r");
                if (!f) {
                    perror("Error abriendo archivo");
                    free(pidsVotersV);
                    exit(EXIT_FAILURE);
                }

                for (i = 0; i < n_procsV; i++) {
                    if (fscanf(f, "%d", &votes[i]) == 1) {
                        count++;
                    }
                }

                fclose(f);
                f = fopen(VOTACION, "w");
                if (!f) {
                    perror ("Error vaciando el archivo de votaciones");
                    free(pidsVotersV);
                    exit(EXIT_FAILURE);
                }
                fclose(f);
            }

            printf("Candidate %d => [", getpid());
            fflush(stdout);
            yes = 0;
            no = 0;
            for (i = 0; i < n_procsV; i++) {
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

            sleep(0.25);

            is_candidate = 0;

            exit(EXIT_SUCCESS);
        }
    }
    
}

void handle_sigusr2(int argSig) {
    int vote = rand() % 2;
    FILE *f = fopen(VOTACION, "a");
    if (!f) {
        perror("Error abriendo archivo");
        free(pidsVotersV);
        exit(EXIT_FAILURE);
    }

    fprintf(f, "%d\n", vote);
    fclose(f);
}

void handle_sigterm(int argSig) {
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

    printf("El proceso Votante %d está listo.\n", getpid());
    fflush(stdout);

    while (1) {
        pause();
    }
        
    exit(EXIT_SUCCESS);
}