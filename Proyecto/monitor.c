#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "monitor.h"
#include "memoriaMonitor.h"
#include "pow.h"

void monitor() {
    Block b;

    printf("[%d] Printing blocks...\n", getpid());

    while (1) {
        sem_wait(&mem->fill);
        sem_wait(&mem->mutex);

        b = mem->buffer[mem->out];
        mem->out = (mem->out + 1) % BUFFER_SIZE;

        sem_post(&mem->mutex);
        sem_post(&mem->empty);

        if (b.validated == -1) {
            printf("[%d] Finishing\n", getpid());
            break;
        }

        if (b.validated)
            printf("Solution accepted: %08ld --> %08ld\n", b.objetivo, b.solucion);
        else
            printf("Solution rejected: %08ld !-> %08ld\n", b.objetivo, b.solucion);
    }
}