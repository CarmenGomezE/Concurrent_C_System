# Nombres de los ejecutables
TARGETS = monitor miner

# Archivos fuente
SRCS_MONITOR = mainMonitor.c comprobador.c monitor.c
SRCS_MINER = minero.c

# Compilador y flags
CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lrt -lpthread

# Regla por defecto
all: $(TARGETS)

# Reglas de construcción
monitor: $(SRCS_MONITOR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

miner: $(SRCS_MINER)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Limpieza
clean:
	rm -f $(TARGETS)

purge: clean
	-@shm_unlink /memoria_monitor 2>/dev/null || true
	-@mq_unlink /blockchain_queue 2>/dev/null || true

.PHONY: all clean purge