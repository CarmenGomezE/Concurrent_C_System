#ifndef MEMORIA_H
#define MEMORIA_H

#include <semaphore.h>
#include "miner.h"

#define BUFFER_SIZE 5
#define SHM_NAME_MONITOR "/monitor_shared_memory"
#define MEM_SIZE sizeof(MonitorSharedMemory)
#define MQ_NAME "/messages_queue"
#define MQ_MAX_MSSG_SIZE sizeof(SharedData)
#define MQ_MAX_MSSGS 10

typedef struct {
    int solucion;       
    int votos_pos;    
    int votos_neg;     
    int monedas;        
    int bloque_listo;  
} Bloque;

typedef struct {  
    int in;
    int out;
    Bloque bloques[BUFFER_SIZE];
    sem_t sem_mutex;   
    sem_t sem_empty; 
    sem_t sem_fill;
} MonitorSharedMemory;

#endif