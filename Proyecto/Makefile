TARGETS = miner monitor

SRCS_MONITOR = mainComprobador.c monitor.c pow.c
SRCS_MINER = mainMiner.c miner.c pow.c registrador.c

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
