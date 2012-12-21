CC=gcc
CFLAGS=-Wall -std=gnu99 -ggdb3

all: emulator

trace: trace.c utilities.c packet.c
	@echo "Building trace..."; \
	$(CC) $(CFLAGS) -o $@ $^;   \
	echo "  [complete]"

emulator: emulator.c utilities.c packet.c forwardtable.c log.c parsetopology.c
	@echo "Building emulator..."; \
	$(CC) $(CFLAGS) -o $@ $^;   \
	echo "  [complete]"
	
clean:
	rm -rf *.o trace emulator

