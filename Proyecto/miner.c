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
#include <errno.h>

#include "miner.h"
#include "monitor.h"
#include "pow.h"

/* ==== Definiciones globales ==== */

static SharedData *shared_data;
static sem_t *sem_init;
static sem_t *sem_winner;
static volatile sig_atomic_t start_mining = 0;
static volatile sig_atomic_t stop_requested = 0;
static volatile sig_atomic_t is_winner = 0;

/* ==== Manejadores de señales ==== */

void sigusr1_handler(int signo) {
    start_mining = 1;
}

void sigusr2_handler(int signo) {
    stop_requested = 1;
}

/* ==== Estructura para los hilos ==== */
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

/* ==== Espera a que el sistema esté inicializado ==== */
void wait_for_initialization() {
    while (!shared_data->initialized) {
        usleep(100000);
    }
}

/* ==== Lógica principal del minero ==== */
void miner(int seconds, int n_threads) {
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);

    /* Abrir o crear memoria compartida */
    int shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    int first_miner = (ftruncate(shm_fd, sizeof(SharedData)) == 0);

    shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    /* Crear/abrir semáforo de inicialización */
    sem_init = sem_open(SEM_INIT_NAME, O_CREAT, 0666, 1);
    if (sem_init == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    sem_winner = sem_open(SEM_WINNER_NAME, O_CREAT, 0666, 1);
    if (sem_winner == SEM_FAILED) {
        perror("sem_open winner");
        exit(EXIT_FAILURE);
    }

    sem_wait(sem_init);
    if (first_miner && !shared_data->initialized) {
        shared_data->initialized = 1;
        shared_data->miner_count = 0;
        shared_data->block_target = BLOCK_TARGET;
        printf("🔧 Primer minero inicializa el sistema.\n");
    } else {
        wait_for_initialization();
    }

    if (shared_data->miner_count >= MAX_MINERS) {
        fprintf(stderr, "Sistema lleno. No se puede registrar el minero.\n");
        sem_post(sem_init);
        exit(EXIT_FAILURE);
    }

    shared_data->miner_pids[shared_data->miner_count++] = getpid();
    printf("Minero registrado: PID %d\n", getpid());

    if (first_miner) {
        for (int i = 0; i < shared_data->miner_count; ++i) {
            kill(shared_data->miner_pids[i], SIGUSR1);
        }
    }

    sem_post(sem_init);

    /* Esperar a que empiece la ronda */
    while (!start_mining) pause();

    printf("🏁 Minero PID %d comenzando a minar con %d hilos durante %d segundos...\n",
           getpid(), n_threads, seconds);

    /* Crear pipes y monitor */
    int fd1[2], fd2[2];
    if (pipe(fd1) == -1 || pipe(fd2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pidMonitor = fork();
    if (pidMonitor < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pidMonitor == 0) {
        close(fd1[1]); // monitor lee de fd1[0]
        close(fd2[0]); // monitor escribe en fd2[1]
        monitor(fd1, fd2);
        exit(EXIT_SUCCESS);
    }

    close(fd1[0]); // minero escribe en fd1[1]
    close(fd2[1]); // minero lee de fd2[0]

    long int target = shared_data->block_target;
    long int solution = 0;
    int solution_flag = 0;
    int range = POW_LIMIT / n_threads;

    pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
    MinerParams *params = malloc(n_threads * sizeof(MinerParams));

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

    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Enviar resultado al Monitor */
    write(fd1[1], &target, sizeof(target));
    write(fd1[1], &solution, sizeof(solution));

    char response[80];
    read(fd2[0], response, sizeof(response));

    if (strcmp(response, "SolutionRejected") == 0) {
        printf("Solución rechazada. Terminando.\n");
    } else {
        printf("Solución aceptada: %ld\n", solution);
        shared_data->block_target = solution;
    }

    close(fd1[1]);
    close(fd2[0]);

    if (is_winner) {
        sem_wait(sem_winner);
        if (shared_data->winner_pid == -1) {
            shared_data->winner_pid = getpid();
            printf("Minero PID %d se ha proclamado ganador de la ronda.\n", getpid());
        } else {
            printf("Otro minero ya ha ganado. Minero PID %d se convierte en perdedor.\n", getpid());
        }
        sem_post(sem_winner);
    }

    if (!is_winner) {
        printf("Minero PID %d ha perdido la ronda.\n", getpid());
    }

    int status;
    waitpid(pidMonitor, &status, 0);

    printf("Minero PID %d termina ejecución.\n", getpid());

    free(threads);
    free(params);
}
