/**
 * @file monitor.c
 * @author Carmen Gómez, María Pozo.
 * @brief Implementation of the monitor process.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "miner.h"
#include "monitor.h"
#include "pow.h"

/*Implementation of the monitor process. */
int monitor(int fd1[2], int fd2[2]){
    long int targetPipe, solutionPipe;
    const char *monitorString;

    /* Close of the write end of monitor in the first pipe. */
    close(fd1[1]);
    /* Close of the read end of monitor in the second pipe. */
    close(fd2[0]);
    
    /*While the miner writes values, the monitor reads them using the pipe. */
    while(read(fd1[0], &targetPipe, sizeof(targetPipe))>0 && read(fd1[0], &solutionPipe, sizeof(solutionPipe))>0){
        /*Comprobation*/
        if(targetPipe==pow_hash(solutionPipe)){
            printf("Solution accepted: %08ld --> %08ld\n", targetPipe, solutionPipe);
            monitorString="SolutionAccepted";
        }
        else{
            printf("Solution rejected: %08ld !-> %08ld\n", targetPipe, solutionPipe);
            monitorString="SolutionRejected";
        }
        /* The message is written in pipe. */
        if (write(fd2[1], monitorString, strlen(monitorString) + 1) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
    } 
    /* Close of the read end of monitor in the first pipe. */
    close(fd1[0]);
    /* Close of the write end of monitor in the second pipe. */
    close(fd2[1]);
    /*Ends.*/
    exit(EXIT_SUCCESS);
}