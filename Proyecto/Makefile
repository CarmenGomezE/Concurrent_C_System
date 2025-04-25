TARGETS = miner #monitor

#SRCS_MONITOR = comprobador.c monitor.c
SRCS_MINER = mainMiner.c miner.c comprobador.c monitor.c registrador.c pow.c

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
