#ifndef MINER_H
#define MINER_H

#define SHM_NAME "/mining_shm"
#define SEM_INIT_NAME "/sem_init"
#define SEM_WINNER_NAME "/sem_winner"
#define MAX_MINERS 10
#define BLOCK_TARGET 12345  
#define POW_LIMIT 1000000  

typedef struct {
    int initialized;
    int miner_count;
    pid_t miner_pids[MAX_MINERS];
    int block_target;
    int winner_pid; 
} SharedData;

void miner(int seconds, int threads);

#endif

