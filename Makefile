CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -Iinclude
LDFLAGS ?=
PTHREAD_FLAGS ?= -pthread

SRC = src/main.c src/scheduler.c src/memory.c src/sync.c src/filesystem.c

.PHONY: all clean test demo kernel kernel-clean kernel-smoke

all: oslab proc_client

oslab: $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(PTHREAD_FLAGS) $(LDFLAGS)

proc_client: src/proc_client.c
	$(CC) $(CFLAGS) -o $@ src/proc_client.c

test: oslab
	./oslab --test

demo: oslab
	./oslab --demo

kernel:
	$(MAKE) -C kernel

kernel-clean:
	$(MAKE) -C kernel clean

kernel-smoke: kernel proc_client
	bash tests/run_kernel_smoke.sh

clean:
	rm -f oslab proc_client
	$(MAKE) -C kernel clean || true
