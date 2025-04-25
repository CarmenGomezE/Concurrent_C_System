/**
 * @file memoriaMonitor.h
 * @author María Pozo, Carmen Gómez.
 * @brief Implementation of the monitor and comprobador programs.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef MEMORIAMONITOR_H
#define MEMORIAMONITOR_H

#include <semaphore.h>
#include "memoriaMiner.h"

#define BUFFER_SIZE 5 /*!< Buffer size. */
#define MONITOR_SHM_NAME "/monitor_shared_memory" /*!< Shared memory name. */
#define MEM_SIZE sizeof(SharedMemory) /*!< Shared memory size. */

/** MemoriaCompartida structure that stores the information of the shared memory segment.*/
typedef struct {
    sem_t mutex;
    sem_t empty;
    sem_t fill;
    Bloque buffer[BUFFER_SIZE];
    int in;
    int out;
} SharedMemory;

#endif 