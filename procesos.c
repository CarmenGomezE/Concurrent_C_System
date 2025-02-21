#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include "pow.h"

int main( ){
    pid_t pidMinero, pidMonitor;

    pidMinero= fork();
    if(pidMinero<0){
        perror("fork");
        return EXIT_FAILURE;
    }

    if(pidMinero==0){
        
        pidMonitor= fork();
        if(pidMonitor<0){
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if(pidMonitor==0){
            printf("Proceso monitor con PID: %d. \n", getpid() );
            exit(EXIT_SUCCESS);
        }

        else{
            printf("Proceso minero con PID: %d, creo proceso monitor con PID: %d.\n", getpid(), pidMonitor);
            wait(NULL);
            exit(EXIT_SUCCESS);
        }
    }
    else{
        printf("Proceso padre con PID: %d, creo minero con PID: %d\n", getpid(), pidMinero);
        wait(NULL);
        exit(EXIT_SUCCESS);
    }

    return EXIT_SUCCESS;
}
