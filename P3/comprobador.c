#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "comprobador.h"

int comprobar(long objetivo, long solucion) {
    return objetivo == solucion;
}

void comprobador(MemoriaCompartida *mem) {
    int contador = 0;
    while (1) {
        Bloque b;
        b.objetivo = contador;
        b.solucion = contador;
        b.es_valido = comprobar(b.objetivo, b.solucion);

        if (contador == 10) {
            b.es_valido = -1;
        }

        sem_wait(&mem->empty);
        sem_wait(&mem->mutex);

        mem->buffer[mem->in] = b;
        mem->in = (mem->in + 1) % BUFFER_SIZE;

        sem_post(&mem->mutex);
        sem_post(&mem->fill);

        if (b.es_valido == -1) break;

        printf("[Comprobador] Enviado: %ld -> %ld (%s)\n", b.objetivo, b.solucion, b.es_valido ? "correcto" : "incorrecto");

        usleep(1000 * 500);
        contador++;
    }
}