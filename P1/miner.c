/**
 * @file miner.c
 * @author Carmen Gómez, María Pozo.
 * @brief Implementation of the miner process.
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
#include "pow.h"
#include "miner.h"
#include "monitor.h"

/**
 * @struct MinerParams structure.
 * @brief This structure stores the parameters used by the miner threads.
 */
typedef struct minerParams{
    int start;                  /*!< Number where the thead is going to start searching. */
    int end;                    /*!< Number where the thead is going to end searching. */
    long int target;            /*!< Number that is going to be searched. */
    long int *solution;         /*!< Pointer to the solution found by a thread. */
    int *solution_flag;         /*!< Pointer to the flag that indicates if the solution has been found. */
} MinerParams;

/*Function that implementates the thread minery.*/
void *miner_thread(void *arg){
    MinerParams *params = (MinerParams*)arg;
    int i;
    /*If the solution has already been found return.*/
    if(*(params->solution_flag)==1){
        return NULL;
    }

    /*Each thread tries their values until a solution is found.*/
    for(i=params->start; i<params->end; i++){
        if((int)pow_hash((long int)i)==params->target){
            *(params->solution) = (long int)i;
            *(params->solution_flag) = 1;
            break;
        }
    }
    return NULL;
}

/*Implementation of the miner process.*/
int miner( long int target, int rounds, int n_threads, int fd1[2], int fd2[2]){
    pid_t pidMonitor;
    pthread_t *threads=NULL;
    int range, round_counter, i, status;
    MinerParams params[n_threads];
    char readbuffer[80];
    long int targetPipe, solutionPipe;

    /*Values to which the pointers of the minerParam structure are going to point.*/
    long int solution = 0;
    int solution_flag = 0;

    /*Miner process creates Monitor process.*/
    pidMonitor= fork();
    if(pidMonitor<0){
        perror("fork");
        exit(EXIT_FAILURE);
    }

    /*Code of the Monitor process.*/
    if(pidMonitor==0){
        monitor( fd1, fd2);
    }

    /* Close of the read end of miner in the first pipe.*/
    close(fd1[0]);
    /* Close of the write end of miner in the second pipe.*/
    close(fd2[1]);

    /*The rounds start.*/
    for(round_counter=0; round_counter<rounds; round_counter++){
        solution_flag=0;
        solution=0;
        range= POW_LIMIT/n_threads;

        /*Memory allocation of the n_threads.*/
        threads= (pthread_t*)malloc(n_threads*sizeof(threads[0]));
        if(threads==NULL){
            return EXIT_FAILURE;
        }

        /*Asignation of the minerParams for all the threads.*/
        for (i = 0; i < n_threads; i++) {
            params[i].start = i * range;
            params[i].end = (i + 1) * range; 
            params[i].target = target;
            params[i].solution = &solution;  
            params[i].solution_flag = &solution_flag;

            /*Creation of the n_threads*/
            if(pthread_create(&threads[i], NULL, miner_thread, &params[i])){
                perror("pthread_create");
                return EXIT_FAILURE;
            }
        }

        /*Wait for the threads to end correctly.*/
        for(i=0; i<n_threads; i++){
            if (pthread_join(threads[i], NULL)) {
                perror("pthread_join");
                exit(EXIT_FAILURE);
            }
        }

        /*Writes in the first pipe the value of the target.*/
        targetPipe= target;
        if (write(fd1[1], &targetPipe, sizeof(targetPipe)) == -1) {
            perror("write1");
            exit(EXIT_FAILURE);
        }
        /*Writes in the first pipe the value of the solution.*/
        solutionPipe= solution;
        if (write(fd1[1], &solutionPipe, sizeof(solutionPipe)) == -1) {
            perror("write2");
            exit(EXIT_FAILURE);
        }

        /*Reads using the second pipe the result of the comprobation.*/
        if (read(fd2[0], readbuffer, sizeof(readbuffer)) == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        /*Checks the comprobation.*/
        if(!strcmp(readbuffer, "SolutionRejected")){
            printf("The solution has been invalidated\n");

            /* Close of the write end of miner in the first pipe.*/
            close(fd1[1]);
            /* Close of the read end of miner in the second pipe.*/
            close(fd2[0]);

            /*In case of wrong solution, waits for the Monitor process to end and prints its end code.*/
            if (waitpid(pidMonitor, &status, 0) > 0) {
                if (WIFEXITED(status)) {
                    printf("Monitor exited with status %d\n", WEXITSTATUS(status));
                } else {
                    printf("Monitor exited unexpectedly\n");
                }
            } else {
                perror("waitpid (Monitor)");
            }
            exit(EXIT_SUCCESS);
        }

        /*If the solution has been found, the new target is the solution.*/
        if(solution_flag==1){
            target=solution;
        }
    }

    /* Close of the write end of miner in the first pipe.*/
    close(fd1[1]);
    /* Close of the read end of miner in the second pipe.*/
    close(fd2[0]);

    /*Waits for the Monitor process to end and prints its end code.*/
    if (waitpid(pidMonitor, &status, 0) > 0) {
        if (WIFEXITED(status)) {
            printf("Monitor exited with status %d\n", WEXITSTATUS(status));
        } else {
            printf("Monitor exited unexpectedly\n");
        }
    } else {
        perror("waitpid (Monitor)");
    }

    /*Ends.*/
    exit(EXIT_SUCCESS);
}