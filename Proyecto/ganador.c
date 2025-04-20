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

void ganador(int seconds, int threads) {
    // 1. Preparar memoria compartida para la votación
    sem_wait(sem_winner);
    shared_data->vote_count = 0;
    shared_data->vote_approved = 0;
    
    for (int i = 0; i < shared_data->miner_count; ++i) {
        shared_data->votes[i] = 0;  // Inicializar votos a 0 (sin voto aún)
    }
    shared_data->block_solution = get_block_solution();  // Obtenemos la solución para el bloque

    printf("Memoria compartida preparada para la votación.\n");
    sem_post(sem_winner);

    // 2. Enviar SIGUSR2 a los demás mineros para comenzar la votación
    for (int i = 0; i < shared_data->miner_count; ++i) {
        if (shared_data->miner_pids[i] != getpid()) {
            kill(shared_data->miner_pids[i], SIGUSR2);
        }
    }

    printf("Se ha enviado la señal SIGUSR2 a los demás mineros para iniciar la votación.\n");

    // 3. Esperar activamente a que todos los mineros voten o hasta alcanzar un número máximo de intentos
    int max_attempts = 100;
    int attempts = 0;
    while (shared_data->vote_count < shared_data->miner_count && attempts < max_attempts) {
        usleep(100000);  // Espera activa corta (100ms)
        attempts++;
    }

    // 4. Comprobar el resultado de la votación
    sem_wait(sem_vote);
    if (shared_data->vote_approved) {
        printf("La solución ha sido aprobada. Incrementando monedas para el ganador.\n");
        shared_data->miner_coins[getpid()] += 1;  // Incrementamos las monedas del ganador
    } else {
        printf("La solución fue rechazada.\n");
    }
    sem_post(sem_vote);

    // 5. Enviar el bloque resuelto al monitor
    enviar_bloque_resuelto();  // Función que envía el bloque resuelto al monitor

    // 6. Preparar la siguiente ronda
    shared_data->block_target = get_next_block_target();  // Ajustar el siguiente objetivo para el bloque
    for (int i = 0; i < shared_data->miner_count; ++i) {
        kill(shared_data->miner_pids[i], SIGUSR1);  // Notificar a los mineros que inician la siguiente ronda
    }
    printf("Nueva ronda de minería iniciada.\n");
}
