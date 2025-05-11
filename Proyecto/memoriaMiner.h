/**
 * @file memoriaMiner.h
 * @author María Pozo, Carmen Gómez.
 * @brief Implementation of the Monitor and Comprobador programs.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef MEMORIAMINER_H
#define MEMORIAMINER_H

#include <semaphore.h>
#include "miner.h"

#define MQ_NAME "/mensages"                  /*!< Message queue name. */
#define MQ_MAX_MSSGS 10                      /*!< Maximum messages in the message queue. */
#define MQ_MAX_MSSG_SIZE sizeof(Block)       /*!< Maximum size of the messages int he message queue. */
#define MAX_WALLETS 100                      /*!< MAximum wallets. */
#define SHM_NAME "/mining_shm"               /*!< Miner shared memory name. */
#define SEM_INIT_NAME "/sem_initial"         /*!< Initial semaphore name. */
#define SEM_WINNER_NAME "/sem_winner"        /*!< Winner semaphore name. */
#define SEM_VOTE_NAME "/sem_vote"            /*!< Voter semaphore name. */
#define MAX_MINERS 100                       /*!< Maximum miners. */
#define BLOCK_TARGET 12345                   /*!< Block targer. */
#define MEM_MINER_SIZE sizeof(SharedData)    /*!< Miner shared memory size. */

/** Wallet structure that stores the information of the wallet of each Minero. */
typedef struct{
    pid_t pid;
    int coins;
} Wallet;


/** Block structure that stores the information of each round. */
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

/** SharedData structure that stores the information of the miner net. */
typedef struct {
    int initialized;
    int miner_count;
    int num_wallets;
    int votes[MAX_MINERS];
    pid_t miner_pids[MAX_MINERS];
    Wallet wallets[MAX_WALLETS];
    Block last_block;
    Block current_block;            
    int start_mining;              
    int start_voting;                      
    int is_winner;                            
} SharedData;

#endif