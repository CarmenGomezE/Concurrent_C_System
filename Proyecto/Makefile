# Variables
CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
LDFLAGS = -lrt
OBJ = principal.o miner.o monitor.o registrador.o pow.o
DEPS = miner.h monitor.h registrador.h pow.h

# Nombre del ejecutable
TARGET = principal

# Regla por defecto
all: $(TARGET)

# Enlace del ejecutable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Regla para compilar archivos .c en .o
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $<

# Limpieza de archivos
clean:
	rm -f *.o $(TARGET)

# Limpieza total
distclean: clean
	rm -f *~ core

.PHONY: all clean distclean
