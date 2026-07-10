# =============================================================================
# Makefile -- desktop (SDL2) build.
#
#   make          build  ->  build/iknowwhatisaw
#   make run      build + run
#   make check    cross-compile the CORE only as freestanding code.
#                 No SDL, no libc: this is the "will it still port to a
#                 microcontroller?" tripwire. Run it often.
#   make clean
#
# Needs: a C compiler and SDL2 dev headers
#   Debian/Ubuntu:  sudo apt install build-essential libsdl2-dev
#   Arch:           sudo pacman -S sdl2
#   macOS:          brew install sdl2
# =============================================================================

CC      ?= cc
CFLAGS  += -O2 -g -std=gnu99 -Wall -Wextra -Isrc/game
CFLAGS  += $(shell sdl2-config --cflags)
LDLIBS  += $(shell sdl2-config --libs)

CORE_SRC := $(wildcard src/game/*.c)
DESK_SRC := platform/desktop/main_sdl.c
BIN      := build/iknowwhatisaw

$(BIN): $(CORE_SRC) $(DESK_SRC) $(wildcard src/game/*.h src/game/assets/*.h)
	@mkdir -p build
	$(CC) $(CFLAGS) $(CORE_SRC) $(DESK_SRC) -o $@ $(LDLIBS)

run: $(BIN)
	./$(BIN)

# Compile the core with no OS and no standard library. If this passes,
# the game logic has stayed portable to bare-metal targets.
check:
	@mkdir -p build/check
	cd build/check && $(CC) -ffreestanding -fno-builtin -c \
	    $(addprefix ../../,$(CORE_SRC)) \
	    -std=gnu99 -Wall -Wextra -I../../src/game
	@echo "core is freestanding-clean"

clean:
	rm -rf build *.o

.PHONY: run check clean
