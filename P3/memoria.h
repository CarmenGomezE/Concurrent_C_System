/**
 * @file memoria.h
 * @author María Pozo, Carmen Gómez.
 * @brief Implementation of the miner and monitor programs.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef MEMORIA_H
#define MEMORIA_H

#include <semaphore.h>
#include <mqueue.h>

#define BUFFER_SIZE 6 /*!< Buffer size. */
#define SHM_NAME "/memoria_compartida" /*!< Shared memory name. */
#define MEM_SIZE sizeof(MemoriaCompartida) /*!< Shared memory size. */
#define MQ_NAME "/cola_de_mensajes" /*!< Messages queue name. */

/** Bloque structure that stores the information of each block.*/
typedef struct {
    long objetivo;
    long solucion;
    int es_valido;
} Bloque;

/** MemoriaCompartida structure that stores the information of the shared memory segment.*/
typedef struct {
    sem_t mutex;
    sem_t empty;
    sem_t fill;
    Bloque buffer[BUFFER_SIZE];
    int in;
    int out;
} MemoriaCompartida;

#endif 