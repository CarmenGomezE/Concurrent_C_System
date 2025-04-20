#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "comprobador.h"

int comprobar(long objetivo, long solucion) {
    return objetivo < solucion;
}

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
            exit(EXIT_FAILURE);
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
