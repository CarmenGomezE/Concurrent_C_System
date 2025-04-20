#ifndef MEMORIA_H
#define MEMORIA_H

#include <semaphore.h>

typedef struct {
    int solucion;       
    int votos_pos;    
    int votos_neg;     
    int monedas;        
    int bloque_listo;  
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