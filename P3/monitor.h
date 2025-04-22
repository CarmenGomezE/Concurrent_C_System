/**
 * @file monitor.h
 * @author María Pozo, Carmen Gómez.
 * @brief Implementation of monitor.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef MONITOR_H
#define MONITOR_H

#include "memoria.h"

/**
 * @brief Monitor process
 *
 * @param mem Shared memory segment.
 * @param lag Miliseconds of lag.
 */
void monitor(MemoriaCompartida *mem, int lag, int soy_creador);

#endif