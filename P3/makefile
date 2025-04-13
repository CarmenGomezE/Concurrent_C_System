# Nombre del ejecutable
TARGET = monitor

# Archivos fuente
SRCS = mainMonitor.c comprobador.c monitor.c

# Archivos objeto
OBJS = $(SRCS:.c=.o)

# Compilador y flags
CC = gcc
CFLAGS = -Wall -lrt

# Regla por defecto
all: $(TARGET)

# Cómo construir el ejecutable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Regla para archivos objeto
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos temporales
clean:
	rm -f $(OBJS) $(TARGET)

# Limpiar también memoria compartida (opcional)
purge: clean
	-@shm_unlink /memoria_monitor || true

.PHONY: all clean purge