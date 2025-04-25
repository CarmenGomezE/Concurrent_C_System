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
 
int main (int argc, char *argv[]) {
    int fd;
    pid_t monitor_pid;
    struct mq_atrr atributos;
    mqd_t mq;
    Bloque b;

    if (argc != 1) {
        perror("There should be no arguments");
        exit(EXIT_FAILURE);
    }
 
    fd = shm_open(SHM_NAME_MONITOR, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, MEM_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    mem = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    sem_init(&mem->mutex, 1, 1);
    sem_init(&mem->empty, 1, BUFFER_SIZE);
    sem_init(&mem->fill, 1, 0);

    monitor_pid = fork();
    if (monitor_pid == 0) {
        monitor(mem);
        exit(EXIT_SUCCESS);
    }

    atributos.mq_flags = 0;
    atributos.mq_maxmsg = MQ_MAX_MSSGS;
    atributos.mq_msgsize = MQ_MAX_MSSG_SIZE;
    atributos.mq_curmsgs = 0;

    mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, 0666, &atributos);
    if (mq == (mqd_t) - 1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    while (1) {
        if (mq_receive(mq, (char *)&b, sizeof(Bloque), NULL) == -1) {
            perror("mq_receive");
            exit(EXIT_FAILURE);
        }

        //Falta tambien si el bloque es el de terminación

        //Si el bloque b es válido
        sem_wait(&mem->empty);
        sem_wait(&mem->mutex);
        mem->buffer[mem->in] = b;
        mem->in = (mem->in + 1) % BUFFER_SIZE;
        sem_post(&mem->mutex);
        sem_post(&mem->fill);
    }

    waitpid(monitor_pid, NULL, 0);

    sem_destroy(&mem->mutex);
    sem_destroy(&mem->empty);
    sem_destroy(&mem->fill);
    shm_unlink(SHM_NAME_MONITOR);

    mq_close(mq);
    mq_unlink(MQ_NAME);
 
    exit(EXIT_SUCCESS);
}
 