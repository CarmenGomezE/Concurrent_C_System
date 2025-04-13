#include <stdio.h>
#include <unistd.h>
#include "monitor.h"

void monitor(MemoriaCompartida *mem, int lag_ms) {
    printf(">> Soy el Monitor\n");
    while (1) {
        if (mem->bloque_valido) {
            printf("[Monitor] Mensaje recibido: %s\n", mem->mensaje);
            mem->bloque_valido = 0;
        } else {
            printf("[Monitor] Esperando nuevo mensaje...\n");
        }
        usleep(lag_ms * 1000);
    }
}
