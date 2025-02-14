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
            printf("Solución encontrada por hilo: %d\n", i);
            break;
        }
    }

    free(arg);
    return NULL;
}

int main(int argc, char *argv[]){
    int rounds, nthread, obj;
    pid_t pidMinero;
    pthread_t *threads=NULL;
    int range, round_counter, i, error;
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
        return EXIT_FAILURE;
    }

    if(pidMinero==0){
        printf("Proceso minero con PID: %d. Minando con %d hilos, %d rondas...\n", getpid(), rounds, nthread);
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
                printf("Solución encontrada en ronda %d: %d\n", round_counter + 1, solution);
                printf("Comprobación: %ld\n", pow_hash(solution));
                obj= solution;
            }
        }
    }
    else{
        printf("Proceso padre con PID: %d, creo minero con PID: %d\n", getpid(), pidMinero);
        wait(NULL);
    }

    return EXIT_SUCCESS;
}
