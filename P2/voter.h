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

#define INFO_SISTEMA "ficheroProcesos.txt"
#define VOTACION "ficheroVotacion.txt"
#define NOMBRE_SEM1 "sem"
#define NOMBRE_SEM2 "semCandidate"
#define NOMBRE_SEM3 "semFile"
void handle_sigusr1(int argSignal);

void handle_sigterm(int argSignal);

void handle_sigusr2(int argSignal);

void read_systemInfo();

int voter( );

#endif
