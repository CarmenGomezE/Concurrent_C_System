TARGETS = monitor miner

SRCS_MONITOR = mainMonitor.c comprobador.c monitor.c
SRCS_MINER = minero.c

CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lrt -lpthread

all: $(TARGETS)

monitor: $(SRCS_MONITOR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

miner: $(SRCS_MINER)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean 