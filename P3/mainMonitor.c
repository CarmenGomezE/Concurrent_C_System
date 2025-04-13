
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include "memoria.h"
#include "monitor.h"
#include "comprobador.h"


/*Monitor main program.*/
int main(int argc, char *argv[]){
    int lag, fd, creador;
    MemoriaCompartida *mem;

    if(argc!=2){
        perror("The arguments should be: ./monitor <LAG>");
        exit(EXIT_FAILURE);
    }

    lag= atoi(argv[1]);

    if(lag<0){
        perror("The arguments should be positive.");
        return EXIT_FAILURE;
    }

    fd= shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if(fd==-1){
        if(errno==EEXIST){
            fd= shm_open(SHM_NAME, O_RDWR, 0666);
            if(fd==-1){
                perror("shm_open existente");
                exit(EXIT_FAILURE);
            }
        }  else{
            perror("shm_open");
            exit(EXIT_FAILURE);
        }
    } else {
        ftruncate(fd, MEM_SIZE);
        creador=1;
    }

    mem= mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(mem==MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    if(creador){
        mem->bloque_valido=0;
        comprobador(mem);
    } else{
        monitor(mem, lag);
    }

    return 0;
}