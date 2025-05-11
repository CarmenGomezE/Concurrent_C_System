/**
 * @file mainComprobador.c
 * @author  María Pozo, Carmen Gómez.
 * @brief Implementation of the Comprobador process.
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
#include <sys/mman.h>
#include <errno.h>
#include "pow.h"
#include "memoriaMonitor.h"
#include "monitor.h"

/** Shared memory for the monitoring. */
SharedMemory *mem = NULL;
/** Message queue for communicating with Ganador. */
mqd_t mq;

/**
 * @brief Sends the ending block
 * @param b The ending block
 */
void send_ending_block(Block b) {
    sem_wait(&mem->empty);
    sem_wait(&mem->mutex);

    mem->buffer[mem->in] = b;
    mem->in = (mem->in + 1) % BUFFER_SIZE;

    sem_post(&mem->mutex);
    sem_post(&mem->fill);
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
        memset(mem, 0, sizeof(SharedMemory));
        munmap(mem, MEM_SIZE);
        shm_unlink(SHM_NAME_MONITOR);
    }
}

/**
 * @brief Handles signal SIGINT
 * @param sig The signal SIGINT
 */
void handle_sigint(int sig) {
    (void)sig;
    Block b;
    b.type = -1;
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
 
int main (void) {
    int fd;
    pid_t monitor_pid;
    struct mq_attr attributes;
    Block b;
    int status;

    /* Handle signal SIGINT */
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }
 
    /* Create and initialize the shared memory segment of monitoring */
    fd = shm_open(SHM_NAME_MONITOR, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    /* Size the segment */
    if (ftruncate(fd, MEM_SIZE) == -1) {
        perror("ftruncate");
        close(fd);
        exit(EXIT_FAILURE);
    }

    /* Link the segment */
    mem = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
    
    /* Initialize segment semaphores */
    sem_init(&mem->mutex, 1, 1);
    sem_init(&mem->empty, 1, BUFFER_SIZE);
    sem_init(&mem->fill, 1, 0);

    /* Create the process Monitor */
    monitor_pid = fork();
    if (monitor_pid == 0) {
        monitor(mem);
        perror("Error en el monitor");
        exit(EXIT_FAILURE);
    } else if (monitor_pid == -1) {
        perror("fork");
        clean_resources();
        exit(EXIT_FAILURE);
    }

    /* Create the message queue to communicate with the process Ganador and initialize it */
    attributes.mq_flags = 0;
    attributes.mq_maxmsg = MQ_MAX_MSSGS;
    attributes.mq_msgsize = MQ_MAX_MSSG_SIZE;
    attributes.mq_curmsgs = 0;

    mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, 0666, &attributes);
    if (mq == (mqd_t) - 1) {
        perror("mq_open");
        kill(monitor_pid, SIGTERM);
        clean_resources();
        exit(EXIT_FAILURE);
    }

    while (1) {
        /* Receive a block through the message queue */
        if (mq_receive(mq, (char *)&b, sizeof(Block), NULL) == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("mq_receive");
            break;
        }

        /* Check if it is an ending block */
        if (b.type == -1) {
            send_ending_block(b);
            break;
        }

        /* Check if the block is correct and set the flag */
        b.validated = check(b);

        /* Insert the block into shared memory */
        sem_wait(&mem->empty);
        sem_wait(&mem->mutex);
        mem->buffer[mem->in] = b;
        mem->in = (mem->in + 1) % BUFFER_SIZE;
        sem_post(&mem->mutex);
        sem_post(&mem->fill);
    }

    /* Wait for the process Monitor to end */
    if (waitpid(monitor_pid, &status, 0) == -1) {
        perror("waitpid");
    } else if (!WIFEXITED(status)) {
        printf("Process Monitor didn't end correctly\n");
    }

    /* Clean resources */
    clean_resources();
    close(fd);
 
    return 0;
}
 