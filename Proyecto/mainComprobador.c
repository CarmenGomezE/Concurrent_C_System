/**
 * @file mainComprobador.c
 * @author  María Pozo, Carmen Gómez.
 * @brief Implementation of the main of the Comprobador process.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <mqueue.h>
#include "pow.h"
#include "memoriaMonitor.h"
#include "monitor.h"

SharedMemory *mem = NULL;
mqd_t mq;

/**
 * @brief Sends the ending block
 * @param b The ending block
 */
void send_ending_block(Bloque b) {
    sem_wait(&mem->empty);
    sem_wait(&mem->mutex);

    mem->buffer[mem->in] = b;
    mem->in = (mem->in + 1) % BUFFER_SIZE;

    sem_post(&mem->mutex);
    sem_post(&mem->fill);
}

/**
 * @brief Handles signal SIGINT
 * @param sig The signal SIGINT
 */
void handle_sigint(int sig) {
    (void)sig;
    Block b;
    b.validated = -1;
    send_ending_block(b);
    clean_resources();
    exit(EXIT_SUCCESS);
}

/**
 * @brief Checks a block
 * @param b The block that is being checked
 * @return int 1 if correct, 0 if incorrect
 */
int check(Block b) {
    // 1. Verify the POW solution is correct
    int hash_result = pow_hash(b.solution);
    int is_pow_valid = (hash_result <= b.target);
    
    // 2. Verify the block structure is valid
    int is_structure_valid = (b.winner_pid != 0) && (b.id_block > 0) && (b.num_wallets > 0);
    
    // 3. Verify the miner is registered
    int is_miner_registered = 0;
    for (int i = 0; i < b.num_wallets; i++) {
        if (b.wallets[i].pid == b.winner_pid) {
            is_miner_registered = 1;
            break;
        }
    }
    
    return is_pow_valid && is_structure_valid && is_miner_registered;
}

/**
 * @brief Cleans resources
 */
void clean_resources() {
    if (mq != (mqd_t)-1) {
        mq_close(mq);
        mq_unlink(MQ_NAME);
    }

    if (mem != NULL) {
        sem_destroy(&mem->mutex);
        sem_destroy(&mem->empty);
        sem_destroy(&mem->fill);
        munmap(mem, MEM_SIZE);
        shm_unlink(SHM_NAME_MONITOR);
    }
}
 
int main (int argc, char *argv[]) {
    int fd;
    pid_t monitor_pid;
    struct mq_atrr attributes;
    Block b;
    int status;

    /* Cehck the arguments */
    if (argc != 1) {
        perror("There should be no arguments");
        exit(EXIT_FAILURE);
    }

    /* Handle signal SIGINT */
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }
 
    /* Crear e inicializar un segmento de memoria compartida de monitoreo */
    fd = shm_open(SHM_NAME_MONITOR, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    /* Dar tamaño al segmento */
    if (ftruncate(fd, MEM_SIZE) == -1) {
        perror("ftruncate");
        close(fd)
        exit(EXIT_FAILURE);
    }

    /* Enlazar el segmento */
    mem = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
    
    /* Inicializar los semáforos del segmento */
    sem_init(&mem->mutex, 1, 1);
    sem_init(&mem->empty, 1, BUFFER_SIZE);
    sem_init(&mem->fill, 1, 0);

    /* Crear el proceso Monitor */
    monitor_pid = fork();
    if (monitor_pid == 0) {
        monitor(mem);
        exit(EXIT_SUCCESS);
    } else if (mointor_pid == -1) {
        perror("fork");
        clean_resources();
        exit(EXIT_FAILURE);
    }

    /* Crear la cola de mensajes para comunicarse con el proceso Ganador e inicialziarla */
    attributes.mq_flags = 0;
    attributes.mq_maxmsg = MQ_MAX_MSSGS;
    attributes.mq_msgsize = MQ_MAX_MSSG_SIZE;
    attributes.mq_curmsgs = 0;

    mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, 0666, &attributes);
    if (mq == (mqd_t) - 1) {
        perror("mq_open");
        kill(monitor_pid, SIGTERM);
        clean_resourceS();
        exit(EXIT_FAILURE);
    }

    while (1) {
        /* Recibir un bloque a través de la cola de mensajes */
        if (mq_receive(mq, (char *)&b, sizeof(Block), NULL) == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("mq_receive");
            break;
        }

        /* Comprobar que el bloque sea de finalización */
        if (b.validated == -1) {
            send_ending_block(b);
            break;
        }

        /* Comprobar que el bloque sea correcto y ajustar la bandera */
        b.validated = check(b);

        /* Introducir el bloque en la memoria compartida */
        sem_wait(&mem->empty);
        sem_wait(&mem->mutex);
        mem->buffer[mem->in] = b;
        mem->in = (mem->in + 1) % BUFFER_SIZE;
        sem_post(&mem->mutex);
        sem_post(&mem->fill);
    }

    if (waitpid(monitor_pid, &status, 0) == -1) {
        perror("waitpid");
    } else if (!WIFEXITED(status)) {
        fprintf("Process Monitor didn't end correctly\n");
    }

    clean_resources();
 
    return 0;
}
 