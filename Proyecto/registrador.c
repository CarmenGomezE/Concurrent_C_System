#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "monitor.h"
#include "pow.h"
#include "memoria.h"

void registrador(int *pipefd) {
    BloqueMessage msg;
    FILE *file; 

    close(pipefd[1]);
    while (read(pipefd[0], &msg, sizeof(msg))>0) {
       printf("Registrador: Bloque recibido, Objetivo: %ld, Solución: %ld\n", msg.block.target, msg.block.solution);

        file = fopen("bloques_registrados.txt", "a");
        if (file) {
            fprintf(file, "Objetivo: %ld, Solución: %ld\n", msg.block.target, msg.block.solution);
            fclose(file);
        } else {
            perror("fopen");
        }  
    }
    close(pipefd[0]);
    printf("Registrador ha finalizado");
    exit(EXIT_SUCCESS);
}