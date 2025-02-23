#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "pow.h"

int flagsol=0; 
long int solution=0;

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
            break;
        }
    }

    free(arg);
    return NULL;
}

int main(int argc, char *argv[]){
    int rounds, nthread;
    long int tarjet;
    pid_t pidMinero, pidMonitor;
    pthread_t *threads=NULL;
    int range, round_counter, i, error;
    int status;
    int *param=NULL;
    /*De las tuberías*/
    int fd1[2];
    int fd2[2];
    long int tarjetPipe, solutionPipe;
    const char *minerString, *monitorString;
    int pipe_status1, pipe_status2;
    ssize_t nbytes1, nbytes2;
    char readbuffer[80];

    /*Comprobations of the arguments*/
    if(argc!=4){
        perror("The arguments should be: <tarjet> <rounds> <n_theads>");
        return EXIT_FAILURE;
    }

    tarjet= strtol(argv[1], NULL,10);
    rounds= atoi(argv[2]);
    nthread= atoi(argv[3]);

    if(rounds<0 || nthread<0 || tarjet<0){
        perror("The arguments should be positive.");
        return EXIT_FAILURE;
    }

    /*Creation of the first pipe*/
    /*FIRST PIPE: MINER ------> MONITOR*/
    pipe_status1 = pipe(fd1);
    if (pipe_status1 == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
     /*Creation of the second pipe*/
     /*SECOND PIPE: MONITOR ------> MINER*/
    pipe_status2 = pipe(fd2);
    if (pipe_status2 == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    /*Principal process creates Miner process*/
    pidMinero= fork();
    if(pidMinero<0){
        perror("fork");
        exit(EXIT_FAILURE);
    }

    /*Code of the Miner process*/
    if(pidMinero==0){

        /*Miner process creates Monitor process*/
        pidMonitor= fork();
        if(pidMonitor<0){
            perror("fork");
            exit(EXIT_FAILURE);
        }

        /*Code of the Monitor process*/
        if(pidMonitor==0){
            /* Close of the write end of monitor in the first pipe*/
            /* Close of the read end of monitor in the second pipe*/
            close(fd1[1]);
            close(fd2[0]);
            
            for(round_counter=0; round_counter<rounds; round_counter++){
                nbytes1=read(fd1[0], &tarjetPipe, sizeof(tarjetPipe));
                if (nbytes1 == -1) {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                nbytes1= read(fd1[0], &solutionPipe, sizeof(solutionPipe));
                if (nbytes1 == -1) {
                    perror("read");
                    exit(EXIT_FAILURE);
                }

                /*Comprobación*/
                if(tarjetPipe==pow_hash(solutionPipe)){
                    printf("Solution accepted: %08ld --> %08ld\n", tarjetPipe, solutionPipe);
                    monitorString="SolutionAccepted";
                }
                else{
                    printf("Solution rejected: %08ld !-> %08ld\n", tarjetPipe, solutionPipe);
                    monitorString="SolutionRejected";
                }
                /* The message is written in the write end. */
                nbytes2 = write(fd2[1], monitorString, strlen(monitorString) + 1);
                if (nbytes2 == -1) {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }
            
            close(fd1[0]);
            close(fd2[1]);
            exit(EXIT_SUCCESS);
        }

        else{
            /*Continues the code of the Miner process*/
            /* Close of the read end of miner in the first pipe*/
            /* Close of the write end of miner in the second pipe*/
            close(fd1[0]);
            close(fd2[1]);
            /*The rounds start*/
            for(round_counter=0; round_counter<rounds; round_counter++){
                flagsol=0;
                solution=0;
                range= POW_LIMIT/nthread;

                /*Creation of the n_threads*/
                threads= (pthread_t*)malloc(nthread*sizeof(threads[0]));
                if(threads==NULL){
                    return EXIT_FAILURE;
                }

                /*EMPIEZA HABRIA QUE CAMBIARLO*/
                for(i=0; i<nthread; i++){
                    param=(int*)malloc(3*sizeof(int));
                    if(param==NULL){
                        return EXIT_FAILURE;
                    }
                    param[0]= i*range;
                    param[1]= (i+1)*range; 
                    param[2]= tarjet;

                    error = pthread_create(&threads[i], NULL, miner_thread, param);
                    if (error != 0) {
                        perror("pthread_create");
                        return EXIT_FAILURE;
                    }
                }
                /*ACABA HABRIA QUE CAMBIARLO*/

                /*For the threads to end correctly*/
                for(i=0; i<nthread; i++){
                    error = pthread_join(threads[i], NULL);
                    if (error != 0) {
                        perror("pthread_join");
                        exit(EXIT_FAILURE);
                    }
                }

                 /* The message is written in the write end. */
                nbytes2 = write(fd1[1], &tarjet, sizeof(tarjet));
                if (nbytes2 == -1) {
                    perror("write1");
                    exit(EXIT_FAILURE);
                }
                nbytes2 = write(fd1[1], &solution, sizeof(solution));
                if (nbytes2 == -1) {
                    perror("write2");
                    exit(EXIT_FAILURE);
                }

                /* Read the message of the Monitor sent by the second pipe*/
                nbytes1 = read(fd2[0], readbuffer, sizeof(readbuffer));
                if (nbytes1 == -1) {
                    perror("read");
                    exit(EXIT_FAILURE);
                }

                /*The solution has been found, and a new tarjet is setted*/
                if(flagsol==1){
                    tarjet= solution;
                }

                /*Checks the comprobation*/
                if(!strcmp(readbuffer, "SolutionRejected")){
                    printf("The solution has been invalidated\n");

                    /*Waits for the Monitor process to end and prints its end code*/
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
            close(fd1[1]);
            close(fd2[0]);

            /*Waits for the Monitor process to end and prints its end code*/
            if (waitpid(pidMonitor, &status, 0) > 0) {
                if (WIFEXITED(status)) {
                    printf("Monitor exited with status %d\n", WEXITSTATUS(status));
                } else {
                    printf("Monitor exited unexpectedly\n");
                }
            } else {
                perror("waitpid (Monitor)");
            }
            /*Ends*/
            exit(EXIT_SUCCESS);
        }
    }

    /*Code of the Principal process*/
    else{
        /*Waits for the Miner process to end and prints its end code*/
        if (waitpid(pidMinero, &status, 0) > 0) {
            if (WIFEXITED(status)) {
                printf("Miner exited with status %d\n", WEXITSTATUS(status));
            } else {
                printf("Miner exited unexpectedly\n");
            }
        } else {
            perror("waitpid (Miner)");
        }
        /*Ends*/
        exit(EXIT_SUCCESS);
    }

    return EXIT_SUCCESS;
}
