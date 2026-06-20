CC      ?= clang
CFLAGS  ?= -std=c11 -Wall -Wextra -g
DEFS    := -DSI_ARENA_IMPL -DSI_TUI_IMPL -DSI_PACK_IMPL
ROOT    := ..
TINYEXPR_DIR := $(ROOT)/third_party/tinyexpr
TINYEXPR_DEFS := -DSIS_USE_TINYEXPR
TINYEXPR_INC := -I$(TINYEXPR_DIR)

.PHONY: all check compile_commands clean demo_sheet

all: check

check: si_impl
	./si_impl

si_impl: si_impl.c
	$(CC) $(CFLAGS) $(DEFS) -I$(ROOT) si_impl.c -o $@

# Requires bear: brew install bear
compile_commands: si_impl.c
	bear -- $(MAKE) si_impl

clean:
	rm -f si_impl si_impl.o demo_sheet

demo_sheet: ../sheet/demo_sheet.c $(TINYEXPR_DIR)/tinyexpr.c
	$(CC) $(CFLAGS) $(TINYEXPR_DEFS) -I$(ROOT) $(TINYEXPR_INC) \
		../sheet/demo_sheet.c $(TINYEXPR_DIR)/tinyexpr.c -lm -o $@
