#ifndef MEMORIA_H
#define MEMORIA_H

#define SHM_NAME "/memoria_monitor"
#define MEM_SIZE sizeof(MemoriaCompartida)

typedef struct {
    int bloque_valido;
    char mensaje[256];
} MemoriaCompartida;

#endif