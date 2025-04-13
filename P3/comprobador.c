#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "comprobador.h"

void comprobador(MemoriaCompartida *mem) {
    printf(">> Soy el Comprobador\n");
    int contador = 0;
    while (1) {
        snprintf(mem->mensaje, sizeof(mem->mensaje), "Mensaje %d generado", contador);
        mem->bloque_valido = 1;
        printf("[Comprobador] Nuevo dato generado: %s\n", mem->mensaje);
        contador++;
        sleep(2);
    }
}