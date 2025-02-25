/**
 * @file monitor.h
 * @author Carmen Gómez, María Pozo.
 * @brief Implementation of the monitor process.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef _MONITOR_H
#define _MONITOR_H

/**
 * @brief This function implementates the monitor process. This process 
 * recives throw a pipe the target and solution sent by the miner process.
 * Then checks if the solution is correct and sends the result of the 
 * comprobation back to the miner using another pipe.
 *
 * @param fd1 File descriptor of the first pipe.
 * @param fd2 File descriptor of the second pipe.
 * @return The exit of the monitor process.
 */
int monitor( int fd1[2], int fd2[2] );

#endif