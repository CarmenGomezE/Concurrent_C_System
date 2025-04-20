/**
 * @file comprobador.h
 * @author María Pozo, Carmen Gómez.
 * @brief Implementation of comprobador.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef COMPROBADOR_H
#define COMPROBADOR_H

#include "memoria.h"

/**
 * @brief Comprobador process
 *
 * @param mem Shared memory segment.
 * @param lag Miliseconds of lag.
 */
void comprobador(MemoriaCompartida *mem, int lag);

#endif