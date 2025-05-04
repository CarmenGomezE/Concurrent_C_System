#ifndef MEMORIA_H
#define MEMORIA_H

#include <semaphore.h>
#include "miner.h"

#define MQ_NAME "/mensages"
#define MAX_WALLETS 100
#define SHM_NAME "/mining_shm"
#define SEM_INIT_NAME "/sem_initial"
#define SEM_WINNER_NAME "/sem_winner"
#define SEM_VOTE_NAME "/sem_vote"
#define MAX_MINERS 10
#define BLOCK_TARGET 12345 

typedef struct{
    pid_t pid;
    int coins;
} Wallet;

typedef struct {
    int type;
    int id_block;
    long int target;
    long int solution;
    int validated;      
    int favor_votes;
    int total_votes;
    Wallet wallets[MAX_WALLETS];
    int num_wallets;
    pid_t winner_pid;     
} Block;

typedef struct {
    int initialized;
    int miner_count;
    int num_wallets;
    int votes[MAX_MINERS];
    pid_t miner_pids[MAX_MINERS];
    Wallet wallets[MAX_WALLETS];
    Block last_block;
    Block current_block;
} SharedData;

#endif