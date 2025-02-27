CC = gcc
CFLAGS = -Wall -Wextra -pthread
OBJ = principal.o miner.o monitor.o pow.o
DEPS = pow.h miner.h monitor.h

all: practica1

# Regla para compilar el ejecutable
practica1: $(OBJ)
	$(CC) $(CFLAGS) -o mrush $(OBJ) -lm

# Regla para compilar archivos .c a .o
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

# Limpia archivos objeto y el ejecutable
clean:
	rm -f $(OBJ) mrush
