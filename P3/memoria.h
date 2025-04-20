#ifndef MEMORIA_H
#define MEMORIA_H

#include <semaphore.h>
#include <mqueue.h>

#define BUFFER_SIZE 6
#define SHM_NAME "/memoria_compartida"
#define MEM_SIZE sizeof(MemoriaCompartida)
#define MQ_NAME "/cola_de_mensajes"

typedef struct {
    long objetivo;
    long solucion;
    int es_valido;
} Bloque;

typedef struct {
    sem_t mutex;
    sem_t empty;
    sem_t fill;
    Bloque buffer[BUFFER_SIZE];
    int in;
    int out;
} MemoriaCompartida;

#endif 