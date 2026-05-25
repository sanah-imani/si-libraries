CC      ?= clang
CFLAGS  ?= -std=c11 -Wall -Wextra -g
DEFS    := -DSI_ARENA_IMPL -DSI_TUI_IMPL -DSI_PACK_IMPL
ROOT    := ..

.PHONY: all check compile_commands clean

all: check

check: si_impl
	./si_impl

si_impl: si_impl.c
	$(CC) $(CFLAGS) $(DEFS) -I$(ROOT) si_impl.c -o $@

# Requires bear: brew install bear
compile_commands: si_impl.c
	bear -- $(MAKE) si_impl

clean:
	rm -f si_impl si_impl.o
