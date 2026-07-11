# =============================================================================
# Makefile -- one command per platform.
#
#   make pc         desktop (SDL2) build           ->  build/iknowwhatisaw
#   make ascii      terminal build, pure ASCII,    ->  build/iknowwhatisaw-ascii
#                   no SDL needed
#   make esp32      the handheld build (ESP-IDF; see platform/esp32/)
#   make flash      esp32 build + flash + serial monitor
#
#   make run        build + run the desktop version   (plain `make` = pc)
#   make run-ascii  build + run the terminal version  (terminal >= 120x40)
#
#   make check      cross-compile the CORE only as freestanding code.
#                   No SDL, no libc: this is the "will it still port to a
#                   microcontroller?" tripwire. Run it often.
#   make clean
#
# Needs for `pc`: a C compiler and SDL2 dev headers
#   Debian/Ubuntu:  sudo apt install build-essential libsdl2-dev
#   Arch:           sudo pacman -S sdl2
#   macOS:          brew install sdl2
# Needs for `ascii`: just a C compiler.
# Needs for `esp32`: ESP-IDF v5 installed (default: ~/esp/esp-idf --
#   override with `make esp32 IDF=/path/to/esp-idf`).
# =============================================================================

# ESP-IDF's export.sh needs bash (Ubuntu's /bin/sh is dash)
SHELL   := /bin/bash

CC      ?= cc
CFLAGS  += -O2 -g -std=gnu99 -Wall -Wextra -Isrc/game
# The content tables (spawns, species, items) are deliberately terse: one
# row per NPC, trailing fields omitted and zero-filled by C. That is the
# whole point of them, so don't warn about it.
CFLAGS  += -Wno-missing-field-initializers

# SDL flags only expand when the desktop binary is actually built, so the
# ascii/esp32 targets work on machines without SDL installed.
SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LIBS   = $(shell sdl2-config --libs)

CORE_SRC := $(wildcard src/game/*.c)
HDRS     := $(wildcard src/game/*.h src/game/assets/*.h)
DESK_SRC := platform/desktop/main_sdl.c
TERM_SRC := platform/terminal/main_term.c
BIN      := build/iknowwhatisaw
BIN_TERM := build/iknowwhatisaw-ascii

IDF ?= $(HOME)/esp/esp-idf

pc: $(BIN)

$(BIN): $(CORE_SRC) $(DESK_SRC) $(HDRS)
	@mkdir -p build
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(CORE_SRC) $(DESK_SRC) -o $@ $(SDL_LIBS)

ascii: $(BIN_TERM)

$(BIN_TERM): $(CORE_SRC) $(TERM_SRC) $(HDRS)
	@mkdir -p build
	$(CC) $(CFLAGS) $(CORE_SRC) $(TERM_SRC) -o $@

# esp32 builds happen in platform/esp32 with ESP-IDF's own build system.
# If idf.py is already on PATH (export.sh sourced) we use it as-is;
# otherwise we source it from $(IDF) first.
esp32:
	@cd platform/esp32 && { command -v idf.py >/dev/null 2>&1 || \
	    . "$(IDF)/export.sh" >/dev/null 2>&1; } && idf.py build

flash:
	@cd platform/esp32 && { command -v idf.py >/dev/null 2>&1 || \
	    . "$(IDF)/export.sh" >/dev/null 2>&1; } && idf.py flash monitor

run: $(BIN)
	./$(BIN)

run-ascii: $(BIN_TERM)
	./$(BIN_TERM)

# Compile the core with no OS and no standard library. If this passes,
# the game logic has stayed portable to bare-metal targets.
check:
	@mkdir -p build/check
	cd build/check && $(CC) -ffreestanding -fno-builtin -c \
	    $(addprefix ../../,$(CORE_SRC)) \
	    -std=gnu99 -Wall -Wextra -Wno-missing-field-initializers \
	    -I../../src/game
	@echo "core is freestanding-clean"

clean:
	rm -rf build *.o

.PHONY: pc ascii esp32 flash run run-ascii check clean
