/**
 * @file comprobador.c
 * @author María Pozo, Carmen Gómez.
 * @brief Implementation of the comprobador process.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "comprobador.h"

/*Function to check the objective and the solution. */
int comprobar(long objetivo, long solucion) {
    return objetivo < solucion;
}

/*Implementation of the comprobador process. */
void comprobador(MemoriaCompartida *mem, int lag) {
    mqd_t mq;
    Bloque b;

    mq = mq_open(MQ_NAME, O_RDONLY);
    if (mq == (mqd_t) - 1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    printf("[%d] Checking blocks...\n", getpid());

    while (1) {
        if (mq_receive(mq, (char *)&b, sizeof(Bloque), NULL) == -1) {
            perror("mq_receive");
            return;
        }

        if (b.es_valido == -1) {
            printf("[%d] Finishing\n", getpid());

            sem_wait(&mem->empty);
            sem_wait(&mem->mutex);

            mem->buffer[mem->in] = b;
            mem->in = (mem->in + 1) % BUFFER_SIZE;

            sem_post(&mem->mutex);
            sem_post(&mem->fill);
            break;
        }

        b.es_valido = comprobar(b.objetivo, b.solucion);

        sem_wait(&mem->empty);
        sem_wait(&mem->mutex);

        mem->buffer[mem->in] = b;
        mem->in = (mem->in + 1) % BUFFER_SIZE;

        sem_post(&mem->mutex);
        sem_post(&mem->fill);

        usleep(lag*1000);
    }

    mq_close(mq);
}