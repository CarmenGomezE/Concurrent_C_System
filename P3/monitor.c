#include <stdio.h>
#include <unistd.h>
#include "monitor.h"

void monitor(MemoriaCompartida *mem, int lag_ms) {
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
