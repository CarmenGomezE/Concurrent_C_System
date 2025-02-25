/**
 * @file pow.c
 * @author SOPER teaching team.
 * @brief Computation of the POW.
 * @version 2.0
 * @date 2024-02-01
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "pow.h"

#define PRIME POW_LIMIT     /*!< Maximum number for the prime number used in the pow function. */
#define BIG_X 435679812     /*!< Constant number that is multiplied in the pow function. */
#define BIG_Y 100001819     /*!< Constant number that is added in the pow function. */

long int pow_hash(long int x) {
  long int result = (x * BIG_X + BIG_Y) % PRIME;
  return result;
}