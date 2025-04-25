#ifndef MINER_H
#define MINER_H

#define SHM_NAME "/mining_shm"
#define SEM_INIT_NAME "/sem_initial"
#define SEM_WINNER_NAME "/sem_winner"
#define SEM_VOTE_NAME "/sem_vote"
#define MAX_MINERS 10
#define BLOCK_TARGET 12345  

void miner(int *pipefd, int seconds, int threads);
void msg_send_termination();
void miner_cleanup();
void perdedor(int *pipefd);
void ganador(int *pipefd);
void *miner_thread(void *arg);
void sigusr2_handler(int signo) ;
void sigusr1_handler(int signo);

#endif

