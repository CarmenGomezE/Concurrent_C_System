/**
 * @file minero.c
 * @author María Pozo, Carmen Gómez.
 * @brief Implementation of the miner process.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "miner.h"
#include "monitor.h"
#include "pow.h"
#include "memoriaMiner.h"
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
#include <mqueue.h>

/* ==== Definiciones globales ==== */
static SharedData *shared_data;
static sem_t *sem_initial;
static sem_t *sem_winner;
static sem_t *sem_vote;

int shm_fd;

static volatile sig_atomic_t start_mining = 0;
static volatile sig_atomic_t stop_requested = 0;
static volatile sig_atomic_t tiempo_terminado = 0;
static volatile sig_atomic_t is_winner = 0;
static volatile sig_atomic_t finish = 0;

/* ==== Manejadores de señales ==== */
void sigusr1_handler(int signo) { (void)signo; printf("[DEBUG] 1 recibido"); start_mining = 1; }
void sigusr2_handler(int signo) { (void)signo; printf("[DEBUG] 2 recibido"); stop_requested = 1; }
void alarm_handler(int signo) { 
    (void)signo; 
    printf("[DEBUG] A recibido"); 
    tiempo_terminado = 1; 
    miner_cleanup();
}

void sigint_handler(int signo) { 
    (void)signo; 
    printf("[DEBUG] C recibido"); 
    finish = 1; 
    miner_cleanup();
}

/* ==== Estructura de hilos ==== */
typedef struct minerParams {
    int start;
    int end;
    long int target;
    long int *solution;
    int *solution_flag;
} MinerParams;

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

void ganador(int *pipefd, long int solution, long int target) {
    Block b;
    mqd_t mq;
    int i;

    printf("[GANADOR] PID %d se proclama ganador.\n", getpid());

    sem_wait(sem_vote);
    for (i = 0; i < MAX_MINERS; i++) shared_data->votes[i] = 0;

    shared_data->current_block.id_block++;
    shared_data->current_block.winner_pid = getpid();
    shared_data->current_block.solution = solution;
    shared_data->current_block.target = target;
    shared_data->current_block.validated = 0;
    shared_data->current_block.favor_votes = 0;
    shared_data->current_block.total_votes = 0;
    shared_data->current_block.type = 0;

    sem_post(sem_vote);

    for (i = 0; i < shared_data->miner_count; i++) {
        if (shared_data->miner_pids[i] != getpid()) {
            kill(shared_data->miner_pids[i], SIGUSR2);
        }
    }

    int attempts = 0, max_attempts = 100;
    while (shared_data->current_block.total_votes < shared_data->miner_count - 1 && attempts++ < max_attempts) {
        usleep(100000);
    }

    b = shared_data->current_block;
    b.num_wallets = shared_data->num_wallets;
    for (int i = 0; i < b.num_wallets; i++) {
        b.wallets[i] = shared_data->wallets[i];
    }
    if (shared_data->current_block.favor_votes >= (shared_data->miner_count - 1) / 2) {
        printf("Bloque aprobado.\n");
        b.validated = 1;
        for (i = 0; i < shared_data->num_wallets; i++) {
            if (shared_data->wallets[i].pid == getpid()) {
                shared_data->wallets[i].coins++;
                break;
            }
        }
    } else {
        printf("Bloque rechazado.\n");
    }

    mq = mq_open(MQ_NAME, O_WRONLY);
    if (mq != (mqd_t)-1) {
        mq_send(mq, (char *)&b, sizeof(Block), 0);
        mq_close(mq);
    }

    close(pipefd[0]);
    write(pipefd[1], &b, sizeof(Block));
    close(pipefd[1]);

    shared_data->last_block = b;
    shared_data->current_block.target = solution;
    for (i = 0; i < shared_data->miner_count; i++) {
        kill(shared_data->miner_pids[i], SIGUSR1);
    }

    start_mining = 0;
}

void perdedor(int *pipefd) {
    Block b;
    int vote = 0;

    printf("[PERDEDOR] PID %d esperando SIGUSR2...\n", getpid());
    while (!stop_requested) pause();

    stop_requested = 0;
    if (pow_hash(shared_data->current_block.solution) == shared_data->current_block.target) {
        vote = 1;
    }

    sem_wait(sem_vote);
    shared_data->current_block.total_votes++;
    if (vote) shared_data->current_block.favor_votes++;
    sem_post(sem_vote);

    printf("[PERDEDOR] Voto: %s\n", vote ? "AFIRMATIVO" : "NEGATIVO");

    b = shared_data->current_block;
    b.num_wallets = shared_data->num_wallets;
    for (int i = 0; i < b.num_wallets; i++) {
        b.wallets[i] = shared_data->wallets[i];
    }
    close(pipefd[0]);
    write(pipefd[1], &b, sizeof(Block));
    close(pipefd[1]);

    start_mining = 0;
    while (!start_mining && !tiempo_terminado) pause();
}

void miner_cleanup() {
    int err=0,count;
    if (shared_data != NULL) {
        sem_wait(sem_initial);
        for (int i = 0; i < shared_data->miner_count; i++) {
            if (shared_data->miner_pids[i] == getpid()) {
                shared_data->miner_pids[i] = 0;
                break;
            }
        }
        shared_data->miner_count--;
        count= shared_data->miner_count;
        sem_post(sem_initial);

        if (count == 0) {
            printf( "Liberando recursos\n");
            shared_data->initialized=0;
            if(munmap(shared_data, sizeof(SharedData))) err=1;
            close(shm_fd);
            if(shm_unlink(SHM_NAME)==-1) err=1;
            if(sem_close(sem_initial)) err=1;
            if(sem_close(sem_winner)) err=1;
            if(sem_close(sem_vote)) err=1;
            if(sem_unlink(SEM_INIT_NAME)) err=1;
            if(sem_unlink(SEM_WINNER_NAME)) err=1;
            if(sem_unlink(SEM_VOTE_NAME)) err=1;

            if(err==1){
                perror("liberando recursos");
            }

            Block end_msg = {.type = -1};
            mqd_t mq = mq_open(MQ_NAME, O_WRONLY);
            if (mq != (mqd_t) -1) {
                mq_send(mq, (char *)&end_msg, sizeof(Block), 0);
                mq_close(mq);
            }
            /**Borrar con comprobador y eso*/
            mq_unlink(MQ_NAME);
        }
    }
}

void miner(int *pipefd, int seconds, int n_threads) {
    int i;
    int solution_flag = 0;
    long int solution = 0;
    long int target = 0;
    pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
    MinerParams *params = malloc(n_threads * sizeof(MinerParams));

    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigprocmask(SIG_UNBLOCK, &set, NULL);

    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);
    signal(SIGALRM, alarm_handler);
    signal(SIGINT, sigint_handler);

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_fd == -1) {
        shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("shm_open");
            exit(EXIT_FAILURE);
        }else {
            if (ftruncate(shm_fd, sizeof(SharedData)) == -1) {
                perror("ftruncate");
                exit(EXIT_FAILURE);
            }
        }
    }

    shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sem_unlink(SEM_INIT_NAME);
    sem_initial = sem_open(SEM_INIT_NAME, O_CREAT, 0666, 1);
    sem_unlink(SEM_WINNER_NAME);
    sem_winner  = sem_open(SEM_WINNER_NAME, O_CREAT, 0666, 1);
    sem_unlink(SEM_VOTE_NAME);
    sem_vote    = sem_open(SEM_VOTE_NAME, O_CREAT, 0666, 1);

    if (!shared_data->initialized) {
        sem_wait(sem_initial);
        if (!shared_data->initialized) {
            shared_data->initialized = 1;
            shared_data->miner_count = 0;
            shared_data->num_wallets = 0;
            shared_data->current_block.id_block = 0;
            shared_data->current_block.target = BLOCK_TARGET;
        }
    }else{
        sem_wait(sem_initial);
        if (shared_data->miner_count >= MAX_MINERS) {
            sem_post(sem_initial);
            fprintf(stderr, "Maximo de mineros alcanzado\n");
            exit(EXIT_FAILURE);
        }

        shared_data->miner_pids[shared_data->miner_count] = getpid();
        shared_data->wallets[shared_data->num_wallets].pid = getpid();
        shared_data->wallets[shared_data->num_wallets].coins = 0;
        shared_data->miner_count++;
        shared_data->num_wallets++;
    }
    sem_post(sem_initial);

    if (shared_data->current_block.id_block == 0) {
        for (i = 0; i < shared_data->miner_count; ++i) {
            kill(shared_data->miner_pids[i], SIGUSR1);
        }
    }

    alarm(seconds);

    while (!tiempo_terminado && !finish) {
        while (!start_mining && !tiempo_terminado && !finish) pause();
        if (tiempo_terminado) break;
        start_mining = 0;

        printf("Iniciando la ronda");

        int range = POW_LIMIT / n_threads;
        solution_flag = 0;

        for (i = 0; i < n_threads; i++) {
            params[i].start = i * range;
            params[i].end = (i + 1) * range;
            params[i].target = shared_data->current_block.target;
            params[i].solution = &solution;
            params[i].solution_flag = &solution_flag;

            target=shared_data->current_block.target;

            pthread_create(&threads[i], NULL, miner_thread, &params[i]);
        }

        for (i = 0; i < n_threads; i++) {
            pthread_join(threads[i], NULL);
        }

        if (solution_flag && sem_trywait(sem_winner) == 0) {
            is_winner = 1;
        }

        if (is_winner) {
            ganador(pipefd, solution, target);
        } else {
            perdedor(pipefd);
        }
    }

    for (i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(params);
    printf("Minero PID %d terminado.\n", getpid());
    exit(EXIT_SUCCESS);
}
