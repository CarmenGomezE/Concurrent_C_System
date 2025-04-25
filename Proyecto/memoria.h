#ifndef MEMORIA_H
#define MEMORIA_H

#include <semaphore.h>
#include "miner.h"

#define QUEUE_NAME "/mensages"

typedef struct {
    long int target;
    long int solution;
    int valid;      
    int votes[MAX_MINERS];     
} Bloque;

typedef struct {
    int initialized;
    int miner_count;
    pid_t miner_pids[MAX_MINERS];
    long block_target;
    pid_t winner_pid; 
    int vote_approved;
    int vote_count;
    int miner_coins[MAX_MINERS];
    Bloque current_block;
} SharedData;

typedef struct {
    long mtype;
    Bloque block;
} BloqueMessage;

typedef struct {
    long msg_type;
    char msg_text[100];
} Message;

#endif