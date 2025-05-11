/**
 * @file miner.h
 * @author María Pozo, Carmen Gómez.
 * @brief Implementation of the Minero program.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef MINER_H
#define MINER_H 

void miner(int *pipefd, int seconds, int threads);
void msg_send_termination();
void miner_cleanup();
void perdedor(int *pipefd);
void ganador(int *pipefd, int long solution, int long target);
void *miner_thread(void *arg);
void sigusr2_handler(int signo) ;
void sigusr1_handler(int signo);

#endif

