/**
 * @file minero.c
 * @author María Pozo, Carmen Gómez.
 * @brief Implementation of the miner process.
 * @version 2.0
 * @date 2025-02-24
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <mqueue.h>
#include <sys/mman.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "memoria.h"

mqd_t mq; /*Shared memory segment. */
FILE *f; /*File with random solutions. */

/*Function for int signal. */
void manejar_senial(int sig) {
    (void)sig;
    if (f) fclose(f);
    if (mq) {
        mq_close(mq);
        mq_unlink(MQ_NAME);
    }
    exit(0);
}

/*Main program that implements miner process. */
int main(int argc, char *argv[]) {
    int rounds, lag, i;
    struct mq_attr atributos;
    long objetivo;
    Bloque b;

    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ROUNDS> <LAG>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    rounds = atoi(argv[1]);
    lag = atoi(argv[2]);
    if (lag < 10) lag = 10;

    signal(SIGINT, manejar_senial);

    atributos.mq_flags = 0;
    atributos.mq_maxmsg = 7;
    atributos.mq_msgsize = sizeof(Bloque);
    atributos.mq_curmsgs = 0;

    mq = mq_open(MQ_NAME, O_WRONLY | O_CREAT, 0666, &atributos);
    if (mq == (mqd_t) - 1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    objetivo = 0;

    f = fopen("bloques.txt", "r");
    if (!f) {
        perror("fopen");
        mq_close(mq);
        exit(EXIT_FAILURE);
    }

    printf("[%d] Generating blocks...\n", getpid());

    for (i = 0; i < rounds; i++) {
        memset(&b, 0, sizeof(Bloque));

        printf("[%d] Enviando bloque: objetivo = %ld, solucion = %ld, es_valido = %d\n", getpid(), b.objetivo, b.solucion, b.es_valido);


        if (fscanf(f, "%ld", &b.solucion) != 1) {
            perror("fscanf");
            break;
        }
        b.objetivo = objetivo;
        b.es_valido = 1;


        if (mq_send(mq, (char *)&b, sizeof(Bloque), 0) == -1) {
            perror("mq_send");
            break;
        }

        objetivo = b.solucion;

        usleep(lag*1000);
    }

    memset(&b, 0, sizeof(Bloque));
    b.es_valido = -1;
    mq_send(mq, (char *)&b, sizeof(Bloque), 0);

    fclose(f);
    mq_close(mq);
    mq_unlink(MQ_NAME);

    printf("[%d] Finishing\n", getpid());

    return 0;
}