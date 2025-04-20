/**
 * @file monitor.c
 * @author María Pozo, Carmen Gómez.
 * @brief Implementation of the monitor process.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <stdio.h>
#include <unistd.h>
#include "monitor.h"

/*Implementation of the monitor process. */
void monitor(MemoriaCompartida *mem, int lag) {
    Bloque b;

    printf("[%d] Printing blocks...\n", getpid());

    while (1) {
        sem_wait(&mem->fill);
        sem_wait(&mem->mutex);

        b = mem->buffer[mem->out];
        mem->out = (mem->out + 1) % BUFFER_SIZE;

        sem_post(&mem->mutex);
        sem_post(&mem->empty);

        if (b.es_valido == -1) {
            printf("[%d] Finishing\n", getpid());
            break;
        }

        if (b.es_valido)
            printf("Solution accepted: %08ld --> %08ld\n", b.objetivo, b.solucion);
        else
            printf("Solution rejected: %08ld !-> %08ld\n", b.objetivo, b.solucion);

        usleep(lag_ms * 1000);
    }
}
