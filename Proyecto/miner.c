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

/* ==== Estructura de hilos ==== */
typedef struct minerParams {
    int start;
    int end;
    long int target;
    long int *solution;
    int *solution_flag;
} MinerParams;

int shm_fd;

/* ==== Manejadores de señales ==== */
void sigusr1_handler(int signo) { 
    (void)signo; 
    printf("[DEBUG] 1 recibido\n"); 
    shared_data->start_mining = 1; 
}

void sigusr2_handler(int signo) { 
    (void)signo; 
    printf("[DEBUG] 2 recibido\n"); 
    shared_data->start_voting = 1; 
}

void alarm_handler(int signo) { 
    (void)signo; 
    printf("[DEBUG] A recibido\n"); 
    shared_data->tiempo_terminado = 1; 
}

void sigint_handler(int signo) { 
    (void)signo; 
    printf("[DEBUG] C recibido\n"); 
    shared_data->finish = 1;
}

void *miner_thread(void *arg) {
    MinerParams *params = (MinerParams*)arg;
    if (*(params->solution_flag)) return NULL;

    for (int i = params->start; i < params->end; i++) {
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
    for (i = 0; i < MAX_MINERS; i++) shared_data->votes[i] = -1;

    shared_data->current_block.id_block++;
    shared_data->current_block.winner_pid = getpid();
    shared_data->current_block.solution = solution;
    shared_data->current_block.target = target;
    shared_data->current_block.validated = 0;
    shared_data->current_block.favor_votes = 0;
    shared_data->current_block.total_votes = 0;
    shared_data->current_block.type = 0;

    sem_post(sem_vote);

    sem_wait(sem_initial);
    for (i = 0; i < shared_data->miner_count; i++) {
        if (shared_data->miner_pids[i] != getpid()) {
            kill(shared_data->miner_pids[i], SIGUSR2);
        }
    }
    sem_post(sem_initial);

    int attempts = 0, max_attempts = 100;
    while (shared_data->current_block.total_votes < shared_data->miner_count - 1 && attempts++ < max_attempts) {
        usleep(100000);
    }

    b = shared_data->current_block;

    sem_wait(sem_initial);
    b.num_wallets = shared_data->num_wallets;
    for (int i = 0; i < b.num_wallets; i++) {
        b.wallets[i] = shared_data->wallets[i];
    }
    sem_post(sem_initial);

    if (shared_data->current_block.favor_votes >= (shared_data->miner_count - 1) / 2) {
        printf("Bloque aprobado.\n");
        b.validated = 1;
        sem_wait(sem_initial);
        for (i = 0; i < shared_data->num_wallets; i++) {
            if (shared_data->wallets[i].pid == getpid()) {
                shared_data->wallets[i].coins++;
                break;
            }
        }
        sem_post(sem_initial);
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

    sem_wait(sem_initial);
    shared_data->last_block = b;
    shared_data->current_block.target = solution;
    shared_data->start_mining = 0;
    sem_post(sem_initial);

    for (i = 0; i < shared_data->miner_count; i++) {
        kill(shared_data->miner_pids[i], SIGUSR1);
    }
}

void perdedor(int *pipefd) {
    Block b;
    int vote = 0;

    printf("[PERDEDOR] PID %d esperando SIGUSR2...\n", getpid());
    while (!shared_data->start_voting) pause();
    shared_data->start_voting = 0;
    
    if (pow_hash(shared_data->current_block.solution) == shared_data->current_block.target) {
        vote = 1;
    }

    sem_wait(sem_vote);
    shared_data->current_block.total_votes++;
    if (vote) shared_data->current_block.favor_votes++;
    for (int i = 0; i < MAX_MINERS; i++) {
        if (shared_data->miner_pids[i] == getpid()) {
            shared_data->votes[i] = vote;
            break;
        }
    }
    printf("Ha votado\n");
    sem_post(sem_vote);

    printf("[PERDEDOR] Voto: %s\n", vote ? "AFIRMATIVO" : "NEGATIVO\n");

    sem_wait(sem_initial);
    b = shared_data->current_block;
    b.num_wallets = shared_data->num_wallets;
    for (int i = 0; i < b.num_wallets; i++) {
        b.wallets[i] = shared_data->wallets[i];
    }
    sem_post(sem_initial);

    close(pipefd[0]);
    write(pipefd[1], &b, sizeof(Block));
    close(pipefd[1]);

    while (!shared_data->start_mining && !shared_data->tiempo_terminado) pause();
}

void miner_cleanup() {
    int last= 0;
    if (shared_data != NULL) {

        sem_wait(sem_initial);
        for (int i = 0; i < shared_data->miner_count; i++) {
            if (shared_data->miner_pids[i] == getpid()) {
                shared_data->miner_pids[i] = 0;
                break;
            }
        }
        shared_data->miner_count--;

        if (shared_data->miner_count == 0) {
            last = 1;
        }
        sem_post(sem_initial);

        if (last) {
            printf( "Liberando recursos\n");
            shared_data->initialized=0;
            memset(shared_data, 0, sizeof(SharedData));
            munmap(shared_data, sizeof(SharedData));
            shared_data = NULL; 
            close(shm_fd);
            shm_unlink(SHM_NAME);
            sem_close(sem_initial);
            sem_close(sem_winner);
            sem_close(sem_vote);
            sem_unlink(SEM_INIT_NAME);
            sem_unlink(SEM_WINNER_NAME);
            sem_unlink(SEM_VOTE_NAME);

            Block end_msg = {.type = -1};
            mqd_t mq = mq_open(MQ_NAME, O_WRONLY);
            if (mq != (mqd_t) -1) {
                mq_send(mq, (char *)&end_msg, sizeof(Block), 0);
                mq_close(mq);
            }
        }
    }
}

void miner(int *pipefd, int seconds, int n_threads) {
    int i, created;
    pthread_t *threads=NULL;
    MinerParams params[n_threads];

    /*Values to which the pointers of the minerParam structure are going to point.*/
    long int solution = 0;
    int solution_flag = 0;

    sigset_t set;
    
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigprocmask(SIG_UNBLOCK, &set, NULL);

    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);
    signal(SIGALRM, alarm_handler);
    signal(SIGINT, sigint_handler);

    created = 0;
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_fd == -1) {
    if (errno == EEXIST) {
        shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("shm_open");
            exit(EXIT_FAILURE);
        }
    } else {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    } else {
        created = 1; 
    }

    if (created) {
        if (ftruncate(shm_fd, MEM_SIZE) == -1) {
            perror("ftruncate");
            exit(EXIT_FAILURE);
        }
    }

    shared_data = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    printf("Número de mineros activos: %d\n", shared_data->miner_count);

    sem_unlink(SEM_INIT_NAME);
    sem_initial = sem_open(SEM_INIT_NAME, O_CREAT, 0666, 1);
    sem_unlink(SEM_WINNER_NAME);
    sem_winner  = sem_open(SEM_WINNER_NAME, O_CREAT, 0666, 1);
    sem_unlink(SEM_VOTE_NAME);
    sem_vote    = sem_open(SEM_VOTE_NAME, O_CREAT, 0666, 1);

    if (sem_initial == SEM_FAILED || sem_winner == SEM_FAILED || sem_vote == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    if (!shared_data->initialized) {
        sem_wait(sem_initial);
        if (!shared_data->initialized) {
            shared_data->initialized = 1;
            shared_data->miner_count = 0;
            shared_data->num_wallets = 0;
            shared_data->current_block.id_block = 0;
            shared_data->current_block.target = BLOCK_TARGET;
            shared_data->start_mining = 0; 
            shared_data->start_voting = 0;   
            shared_data->tiempo_terminado = 0; 
            shared_data->finish = 0;

            shared_data->miner_pids[0] = getpid();
            shared_data->wallets[0].pid = getpid();
            shared_data->wallets[0].coins = 0;
            shared_data->miner_count = 1;
            shared_data->num_wallets = 1;
        }
        sem_post(sem_initial); 
    } else {
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
        sem_post(sem_initial);
    }

    if (shared_data->current_block.id_block == 0) {
        for (i = 0; i < shared_data->miner_count; ++i) {
            kill(shared_data->miner_pids[i], SIGUSR1);
        }
    }

    alarm(seconds);

    printf("Número de mineros activos: %d\n", shared_data->miner_count);

    while (!shared_data->tiempo_terminado && !shared_data->finish) {
        while (!shared_data->start_mining && !shared_data->tiempo_terminado && !shared_data->finish) pause();
        if (shared_data->tiempo_terminado==1 || shared_data->finish==1) break;

        printf("Iniciando la ronda");

        int range = POW_LIMIT / n_threads;
        solution_flag = 0;
        solution=0;

        threads= (pthread_t*)malloc(n_threads*sizeof(threads[0]));
        if(threads==NULL){
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < n_threads; i++) {
            params[i].start = i * range;
            params[i].end = (i + 1) * range;
            params[i].target = shared_data->current_block.target;
            params[i].solution = &solution;
            params[i].solution_flag = &solution_flag;
            pthread_create(&threads[i], NULL, miner_thread, &params[i]);
        }

        for (i = 0; i < n_threads; i++) {
            if (pthread_join(threads[i], NULL)) {
                    perror("pthread_join");
                    exit(EXIT_FAILURE);
                }
        }
        free(threads);

        sem_wait(sem_winner);

        if (solution_flag && !shared_data->is_winner) {
            shared_data->is_winner = 1;
            ganador(pipefd, solution, shared_data->current_block.target);
        } else {
            perdedor(pipefd);
        }

        sem_post(sem_winner); 
    }

    miner_cleanup();
    printf("Minero PID %d terminado.\n", getpid());
    exit(EXIT_SUCCESS);
}
