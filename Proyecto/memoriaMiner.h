#ifndef MEMORIA_H
#define MEMORIA_H

#include <semaphore.h>
#include "miner.h"

#define MQ_NAME "/mensages"

typedef struct {
    int type;
    int id;
    long int target;
    long int solution;
    int valid;      
    int votes[MAX_MINERS]; 
    long int miner_coins[MAX_MINERS];
    pid_t winner_pid;     
} Bloque;

typedef struct {
    int initialized;
    int miner_count;
    pid_t miner_pids[MAX_MINERS];
    long int miner_coins[MAX_MINERS];
    long block_target;
    int vote_approved;
    int vote_count;
    Bloque current_block;
} SharedData;

#endif