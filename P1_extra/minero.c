#include <stdio.h>
#include <stlib.h>
#include <pthread.h>
#include "pow.h"

void minar() {
    
}

int minero(int nrondas, int nhilos, unsigned int obj) {
    int i, j;
    int *idhilos = (int *) malloc (nhilos*sizeof(idhilos[0]));
    unsigned int rango;

    for (i = 0; i < nrondas; i++) {
        printf("Ronda %d\n", i + 1);

        rango = POW_LIMIT / nhilos;

        for (j = 0; j < nhilos; j++) {
            if (phread_create(&idhilos[i], NULL, minar, ) != 0) {
                perror("Error al crear el hilo %d\n", j);
                exit(EXIT_FAILURE);
            }
        }

        for (j = 0; j < nhilos; i++) {
            phread_join(idhilos[i], NULL);
        }
    }

    free(idhilos);

    exit(EXIT_SUCESS);
}

int main(int argc, char argv[]) {
    int nrondas, nhilos;
    unsigned int obj;

    if (argc != 4) {
        perror("Los argumentos deben ser: <nrondas> <nhilos> <obj>");
        return EXIT_FAILURE;
    }

    nrondas = atoi(argv[1]);
    nhilos = atoi(argv[2]);
    obj = (unsigned int)atoi(argv[3]);

    if (nrondas <= 0 || nhilos <= 0 || obj <= 0) {
        perror("Argumentos no válidos");
        return EXIT_FAILURE;
    }

    return minero(nrondas, nhilos, obj);
}