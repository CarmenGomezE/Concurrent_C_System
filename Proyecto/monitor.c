/**
 * @file monitor.c
 * @author  María Pozo, Carmen Gómez.
 * @brief Implementation of the Monitor process.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "monitor.h"
#include "pow.h"
#include "memoriaMonitor.h"

/**
 * @brief Prints a block
 * @param b The block that is being printed
 */
void print_block(Block b) {
    int i;

    printf("Id: %04d\n", b.id_block);
    printf("Winner: %d\n", b.winner_pid);
    printf("Target: %ld\n", b.target);

    if (b.validated == 0) {
        printf("Solution: %08ld (rejected)\n", b.solution);
    } else {
        printf("Solution %08ld (validated)\n", b.solution);
    }

    printf("Votes: %d/%d\n", b.favor_votes, b.total_votes);

    printf("Wallets: ");
    for (i = 0; i < b.num_wallets; i++) {
        printf("%d: %02d ", b.wallets[i].pid, b.wallets[i].coins);
    }

    printf("\n");
}

/**
 * @brief Process Monitor
 */
void monitor(SharedMemory *mem) {
    Block b;

    while (1) {
        sem_wait(&mem->fill);
        sem_wait(&mem->mutex);

        b = mem->buffer[mem->out];
        mem->out = (mem->out + 1) % BUFFER_SIZE;

        sem_post(&mem->mutex);
        sem_post(&mem->empty);

        if (b.validated == -1) {
            break;
        }

        print_block(b);
    }

    exit(EXIT_SUCCESS);
}