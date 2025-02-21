#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include "pow.h"

int flagsol=0; 
int solution=0;

void *miner_thread(void *arg){
    int start, end, tarjet, i;
    int *params = (int*)arg;
    start = params[0];
    end = params[1];
    tarjet = params[2];

    if(flagsol==1){
        return NULL;
    }

    for(i=start; i<end; i++){
        if((int)pow_hash((long int)i)==tarjet){
            flagsol=1;
            solution=i;
            printf("Solution found by thread: %d\n", i);
            break;
        }
    }

    free(arg);
    return NULL;
}

int main(int argc, char *argv[]){
    int rounds, nthread, obj;
    pid_t pidMinero, pidMonitor;
    pthread_t *threads=NULL;
    int range, round_counter, i, error;
    int status;
    int *param=NULL;

    if(argc!=4){
        perror("Los argumentos deberían ser: <rounds> <hilos> <obj>");
        return EXIT_FAILURE;
    }

    rounds= atoi(argv[1]);
    nthread= atoi(argv[2]);
    obj= atoi(argv[3]);

    if(rounds<0 || nthread<0 || obj<0){
        perror("Los argumentos deben ser números positivos");
        return EXIT_FAILURE;
    }

    pidMinero= fork();
    if(pidMinero<0){
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if(pidMinero==0){
        
        pidMonitor= fork();
        if(pidMonitor<0){
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if(pidMonitor==0){
            printf("Monitor process with PID: %d. \n", getpid() );
            exit(EXIT_SUCCESS);
        }

        else{
            printf("Miner process with PID: %d, created process Monitor with PID: %d. Mining with %d threads, %d rounds...\n", getpid(), pidMonitor, rounds, nthread);
            for(round_counter=0; round_counter<rounds; round_counter++){
                flagsol=0;
                solution=0;
                range= POW_LIMIT/nthread;

                threads= (pthread_t*)malloc(nthread*sizeof(threads[0]));
                if(threads==NULL){
                    return EXIT_FAILURE;
                }

                for(i=0; i<nthread; i++){
                    param=(int*)malloc(3*sizeof(int));
                    if(param==NULL){
                        return EXIT_FAILURE;
                    }
                    param[0]= i*range; /*Start*/
                    param[1]= (i+1)*range; /*End*/
                    param[2]= obj; /*Objetivo*/

                    error = pthread_create(&threads[i], NULL, miner_thread, param);
                    if (error != 0) {
                        perror("pthread_create");
                        return EXIT_FAILURE;
                    }
                }

                for(i=0; i<nthread; i++){
                    error = pthread_join(threads[i], NULL);
                    if (error != 0) {
                        perror("pthread_join");
                        exit(EXIT_FAILURE);
                    }
                }

                if(flagsol==1){
                    printf("Solution found in round %d: %d\n", round_counter + 1, solution);
                    printf("Comprobación: %ld\n", pow_hash(solution));
                    obj= solution;
                }
            }
            
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
    }
    else{
        printf("Principal process with PID: %d, created Miner with PID: %d\n", getpid(), pidMinero);

        if (waitpid(pidMinero, &status, 0) > 0) {
            if (WIFEXITED(status)) {
                printf("Miner exited with status %d\n", WEXITSTATUS(status));
            } else {
                printf("Miner exited unexpectedly\n");
            }
        } else {
            perror("waitpid (Miner)");
        }

        exit(EXIT_SUCCESS);
    }

    return EXIT_SUCCESS;
}
