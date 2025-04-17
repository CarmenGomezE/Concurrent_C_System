#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/stat.h>
#include "memoria.h"
#include "monitor.h"
#include "comprobador.h"

int main(int argc, char *argv[]) {
    int fd, creador = 0;
    MemoriaCompartida *mem;
    int lag = (argc == 2) ? atoi(argv[1]) : 500;

    fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (fd == -1) {
        fd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (fd == -1) {
            perror("shm_open");
            exit(EXIT_FAILURE);
        }
    } else {
        ftruncate(fd, MEM_SIZE);
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
        comprobador(mem);
    } else {
        monitor(mem, lag);
    }

    return 0;
}