#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <semaphore.h>
#include "miner.h"
#include "monitor.h"

extern SharedData *shared_data;
extern sem_t *sem_init;
extern sem_t *sem_winner;
extern sem_t *sem_vote;

// Variable global para indicar cuando el minero debe iniciar una nueva ronda
volatile sig_atomic_t start_new_round = 0;

void sigusr2_handler(int signo) {
    // Indica que la votación ha comenzado
    start_new_round = 1;
}

void perdedor() {
    // 1. Esperar la señal SIGUSR2 para iniciar la votación
    signal(SIGUSR2, sigusr2_handler);  // Configura el manejador de SIGUSR2
    while (!start_new_round) {
        pause();  // Espera activa hasta que llegue SIGUSR2
    }

    printf("Minero PID %d ha recibido la señal SIGUSR2 para iniciar la votación.\n", getpid());

    // 2. Verificar la solución obtenida por el ganador y registrar el voto
    int vote = 0;  // 0 significa voto negativo
    if (verify_solution(shared_data->block_solution)) {  // Función que verifica si la solución es correcta
        vote = 1;  // 1 significa voto positivo
    }

    // 3. Registrar el voto en la memoria compartida
    sem_wait(sem_vote);
    shared_data->votes[get_pid_index()] = vote;  // Guardamos el voto de este minero en la lista
    shared_data->vote_count++;
    printf("Minero PID %d ha votado: %d\n", getpid(), vote);
    sem_post(sem_vote);

    // 4. Esperar la señal SIGUSR1 para iniciar la siguiente ronda
    signal(SIGUSR1, sigusr1_handler);  // Configura el manejador de SIGUSR1 para iniciar la siguiente ronda
    while (1) {
        pause();  // Esperar señal SIGUSR1 para iniciar nueva ronda
    }

    printf("Minero PID %d ha comenzado una nueva ronda.\n", getpid());
}
