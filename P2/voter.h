/**
 * @file voter.h
 * @author Carmen Gómez, María Pozo.
 * @brief Implementation of the voters process.
 * @version 2.0
 * @date 2025-03-07
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef _VOTER_H
#define _VOTER_H

void handle_sigusr1(int argSignal);

void handle_sigterm(int argSignal);

int voter( );

#endif
