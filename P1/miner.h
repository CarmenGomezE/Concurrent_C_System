/**
 * @file miner.h
 * @author Carmen Gómez, María Pozo.
 * @brief Implementation of the miner process.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef _MINER_H
#define _MINER_H

/** MinerParams structure that stores the parameters used by the miner threads.*/
typedef struct _minerParams _minerParams;

/**
 * @brief Computes the following hash function:
 * f(x) = (X x + Y) % P.
 *
 * @param target Number that is going to be searched.
 * @param rounds Number of rounds of search.
 * @param n_threads Number of threads that are going to search for the target.
 * @param fd1 File descriptor of the first pipe.
 * @param fd2 File descriptor of the second pipe.
 * @return The exit of the miner process.
 */
int miner( long int target, int rounds, int n_threads, int fd1[2], int fd2[2]);

/**
 * @brief Computes the following hash function:
 * f(x) = (X x + Y) % P.
 *
 * @param arg Pointer to the minerParams structure.
 * @return Void pointer (NULL).
 */
void *miner_thread(void *arg);

#endif