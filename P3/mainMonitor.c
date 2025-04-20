#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>

#include "memoria.h"
#include "monitor.h"
#include "comprobador.h"

MemoriaCompartida *mem = NULL;
int creador = 0;

void limpiar_recursos() {
    if (mem && creador) {
        sem_destroy(&mem->mutex);
        sem_destroy(&mem->empty);
        sem_destroy(&mem->fill);
        shm_unlink(SHM_NAME);
    }
}

void manejar_senial(int sig) {
    (void)sig;
    limpiar_recursos();
    exit(0);
}

int main(int argc, char *argv[]) {
    int fd, lag; 

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <LAG>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    lag = atoi(argv[1]);
    if (lag < 10) lag = 10;

    signal(SIGINT, manejar_senial);

    fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (fd == -1) {
        fd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (fd == -1) {
            perror("shm_open");
            exit(EXIT_FAILURE);
        }
    } else {
        if (ftruncate(fd, MEM_SIZE) == -1) {
            perror("ftruncate");
            exit(EXIT_FAILURE);
        }
        creador = 1;
    }

    mem = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    if (creador) {
        sem_init(&mem->mutex, 1, 1);
        sem_init(&mem->empty, 1, BUFFER_SIZE);
        sem_init(&mem->fill, 1, 0);
        mem->in = mem->out = 0;
        comprobador(mem, lag);
    } else {
        monitor(mem, lag);
    }

    limpiar_recursos();
    return 0;
}
