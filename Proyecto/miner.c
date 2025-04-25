/**
 * @file miner.c
 * @brief Minero con sincronización por memoria compartida y minería real con hilos.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <errno.h>

#include "miner.h"
#include "monitor.h"
#include "pow.h"
#include "memoriaMiner.h"

/* ==== Definiciones globales ==== */

static SharedData *shared_data;
static sem_t *sem_initial;
static sem_t *sem_winner;
static sem_t *sem_vote;
static volatile sig_atomic_t start_mining = 0;
static volatile sig_atomic_t stop_requested = 0;
static volatile sig_atomic_t is_winner = 0;

/* ==== Estructura para los hilos ==== */
typedef struct minerParams {
    int start;
    int end;
    long int target;
    long int *solution;
    int *solution_flag;
} MinerParams;

/* ==== Manejadores de señales ==== */

void sigusr1_handler(int signo) {
    (void)signo;
    start_mining = 1;
}

void sigusr2_handler(int signo) {
    (void)signo;
    stop_requested = 1;
}

void *miner_thread(void *arg) {
    MinerParams *params = (MinerParams*)arg;

    for (int i = params->start; i < params->end; i++) {
        if (*(params->solution_flag) || stop_requested) return NULL;

        if ((int)pow_hash((long int)i) == params->target) {
            *(params->solution) = (long int)i;
            *(params->solution_flag) = 1;
            break;
        }
    }
    return NULL;
}

void ganador( int *pipefd) {
    int max_attempts = 100, i=0, attempts = 0;
    Bloque b;

    printf("[GANADOR] PID %d se proclama ganador.\n", getpid());

    sem_wait(sem_vote);
    shared_data->vote_count = 0;
    shared_data->vote_approved = 0;

    for (i = 0; i < shared_data->miner_count; i++) {
        shared_data->current_block.votes[i] = 0;
    }

    shared_data->current_block.solution = shared_data->block_target;
    shared_data->current_block.valid = 1;
    sem_post(sem_vote);

    for (i = 0; i < shared_data->miner_count; i++) {
        if (shared_data->miner_pids[i] != getpid()) {
            kill(shared_data->miner_pids[i], SIGUSR2);
        }
    }

    max_attempts = 100;
    attempts = 0;
    while (shared_data->vote_count < shared_data->miner_count - 1 && attempts < max_attempts) {
        usleep(100000); 
        attempts++;
    }

    printf("Votos recibidos: %d/%d\n", shared_data->vote_count, shared_data->miner_count - 1);

    b = shared_data->current_block; 

    if (shared_data->vote_approved >= (shared_data->miner_count - 1) / 2) {
        printf("Bloque aprobado. Se otorga recompensa.\n");
        for (i = 0; i < shared_data->miner_count; i++) {
            if (shared_data->miner_pids[i] == getpid()) {
                b.miner_coins[i]++;
                break;
            }
        }
    } else {
        printf("Bloque rechazado.\n");
    }

    /** AQUÍ ENVÍA EL BLOQUE AL MONITOR */
    

    /** AQUÍ ENVÍA EL BLOQUE AL REGISTRADOR */
    close(pipefd[0]);
    if (write(pipefd[1], &b, sizeof(b)) == -1) {
        perror("write to pipe");
        exit(EXIT_FAILURE);
    }
    close(pipefd[1]);
    printf("Bloque enviado al Registrador a través de la tubería.\n");


    shared_data->block_target = shared_data->current_block.solution;
    for (i = 0; i < shared_data->miner_count; i++) {
        kill(shared_data->miner_pids[i], SIGUSR1);
    }

    start_mining = 0;
}

void perdedor(int *pipefd) {
    int voto = 0;
    Bloque b;

    printf("[PERDEDOR] PID %d esperando SIGUSR2 para votar...\n", getpid());
    while (!stop_requested) pause();

    printf("PID %d recibido SIGUSR2. Evaluando bloque...\n", getpid());

    voto = 0;
    if (shared_data->current_block.valid &&
        pow_hash(shared_data->current_block.solution) == shared_data->block_target) {
        voto = 1;
    }

    sem_wait(sem_vote);
    shared_data->vote_count++;
    if (voto) shared_data->vote_approved++;
    sem_post(sem_vote);

    printf("PID %d ha votado %s\n", getpid(), voto ? "AFIRMATIVO" : "NEGATIVO");

    b = shared_data->current_block; 

    close(pipefd[0]);
    if (write(pipefd[1], &b, sizeof(b)) == -1) {
        perror("write to pipe");
        exit(EXIT_FAILURE);
    }
    close(pipefd[1]);
    printf("Bloque enviado al Registrador a través de la tubería.\n");


    start_mining = 0;
    stop_requested = 0;
    while (!start_mining) pause();
}

void msg_send_termination() {
    /** AQUÍ ENVÍA EL MENSAGE DE FINALIZACIÓN */

    printf("Mensaje de finalización enviado al monitor.\n");
}

void miner_cleanup() {
    if (shared_data != NULL) {
        for (int i = 0; i < shared_data->miner_count; i++) {
            if (shared_data->miner_pids[i] == getpid()) {
                shared_data->miner_pids[i] = 0; 
                shared_data->miner_count--;
                break;
            }
        }

        if (shared_data->miner_count == 0) {
            munmap(shared_data, sizeof(SharedData));
            shm_unlink(SHM_NAME);

            sem_close(sem_initial);
            sem_close(sem_winner);
            sem_close(sem_vote);
            sem_unlink(SEM_INIT_NAME);
            sem_unlink(SEM_WINNER_NAME);
            sem_unlink(SEM_VOTE_NAME);

            msg_send_termination();
        }
    }
}

/* ==== Lógica principal del minero ==== */
void miner(int *pipefd, int seconds, int n_threads) {
    int first_miner, i;
    long int target = shared_data->block_target;
    long int solution = 0;
    int shm_fd;
    int solution_flag = 0;
    int range = POW_LIMIT / n_threads;

    pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
    MinerParams *params = malloc(n_threads * sizeof(MinerParams));

    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);

    /* Abrir o crear memoria compartida */
    shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    first_miner = (ftruncate(shm_fd, sizeof(SharedData)) == 0);

    shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    /* Crear/abrir semáforo de inicialización */
    sem_initial = sem_open(SEM_INIT_NAME, O_CREAT, 0666, 1);
    if (sem_initial == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    sem_winner = sem_open(SEM_WINNER_NAME, O_CREAT, 0666, 1);
    if (sem_winner == SEM_FAILED) {
        perror("sem_open winner");
        exit(EXIT_FAILURE);
    }

    sem_vote = sem_open(SEM_VOTE_NAME, O_CREAT, 0666, 1); 
    if (sem_vote == SEM_FAILED) {
        perror("sem_open vote");
        exit(EXIT_FAILURE);
    }

    sem_wait(sem_initial);
    /** Si es el primer minero inicializa */
    if (first_miner && !shared_data->initialized) {
        shared_data->initialized = 1;
        shared_data->miner_count = 0;
        shared_data->block_target = BLOCK_TARGET;
        shared_data->vote_count = 0;
        shared_data->vote_approved = 0;
        for(i=0; i<MAX_MINERS; i++){
            shared_data->miner_pids[i]=0;
        }
        printf("Primer minero ha inicializado el sistema.\n");
    } 
    /** Si no es el primer minero espera a que se inicialice */
    else {
        while (!shared_data->initialized) {
        usleep(100000);
    }
    }

    if (shared_data->miner_count >= MAX_MINERS) {
        fprintf(stderr, "Sistema lleno. No se puede registrar el minero.\n");
        sem_post(sem_initial);
        exit(EXIT_FAILURE);
    }

    shared_data->miner_pids[shared_data->miner_count++] = getpid();
    printf("Minero registrado: PID %d\n", getpid());

    /** Si es el primero, arranca la ronda */
    if (first_miner) {
        for (i = 0; i < shared_data->miner_count; ++i) {
            kill(shared_data->miner_pids[i], SIGUSR1);
        }
    }

    sem_post(sem_initial);

    /* Esperar a que empiece la ronda */
    while (!start_mining) pause();

    printf("Minero PID %d comenzando a minar con %d hilos durante %d segundos...\n", getpid(), n_threads, seconds);

    for (int i = 0; i < n_threads; i++) {
        params[i].start = i * range;
        params[i].end = (i + 1) * range;
        params[i].target = target;
        params[i].solution = &solution;
        params[i].solution_flag = &solution_flag;

        if (pthread_create(&threads[i], NULL, miner_thread, &params[i])) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    if (solution_flag) {
        if (sem_trywait(sem_winner) == 0) {
            is_winner = 1;
            shared_data->current_block.solution = solution;
            shared_data->current_block.target = target;
            shared_data->current_block.valid = 1;
            printf("[GANADOR] Minero PID %d proclama solución: %ld\n", getpid(), solution);

            for (i = 0; i < shared_data->miner_count; i++) {
                if (shared_data->miner_pids[i] != getpid()) {
                    kill(shared_data->miner_pids[i], SIGUSR2);
                }
            }
        } else {
            printf("[PERDEDOR] Otro proceso ya proclamó solución. PID %d espera votación.\n", getpid());
        }
    } else {
        printf("[PERDEDOR] No encontró solución. PID %d espera votación.\n", getpid());
    }

    if (is_winner) {
        ganador( pipefd);  
    } else {
        perdedor(pipefd); 
    }

    miner_cleanup();
    free(threads);
    free(params);
    printf("Minero PID %d termina ejecución.\n", getpid());
    exit(EXIT_SUCCESS);
}
