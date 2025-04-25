#ifndef MEMORIA_H
#define MEMORIA_H

#include <semaphore.h>

typedef struct {
    long int target;
    long int solution;
    int valid;      
    int votes[MAX_MINERS];     
} Bloque;

typedef struct {
    int num_miners; 
    int ronda_actual;  
    int max_miners;    
    int ganador_pid;    
    Bloque bloques[10];
    sem_t sem_mutex;   
    sem_t sem_votacion; 
    sem_t sem_finalizado;
} Sistema;

#endif