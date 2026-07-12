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

# =============================================================================
# SHIPPING IT.  `make dist` builds both, into dist/ .
#
#   dist/linux/     iknowwhatisaw + lib/libSDL2 + run.sh   (Steam depot)
#   dist/windows/   iknowwhatisaw.exe + SDL2.dll           (Steam depot)
#
# Each folder is SELF-CONTAINED: point a Steam depot at it and ship. The
# player needs nothing installed -- SDL travels with the game.
#
# WINDOWS needs the cross-compiler (one time):
#     sudo apt install mingw-w64
# The SDL2 Windows SDK is vendored in vendor/SDL2-mingw (make vendor-sdl).
#
# NOTE: saves do NOT go next to the binary -- they go to the OS's per-user
# app-data dir (SDL_GetPrefPath, see main_sdl.c). A shipped game lives
# somewhere read-only, so a save file beside the .exe would fail for
# everyone who didn't build it themselves.
# =============================================================================
VERSION   ?= 0.1.0
DIST      := dist
SDL_MINGW := vendor/SDL2-mingw/x86_64-w64-mingw32
MINGW_CC  := x86_64-w64-mingw32-gcc
SDL_MINGW_URL := https://github.com/libsdl-org/SDL/releases/download/release-2.30.11/SDL2-devel-2.30.11-mingw.tar.gz

dist: dist-linux dist-windows
	@echo
	@echo "  ready to ship:"
	@du -sh $(DIST)/linux $(DIST)/windows

# ---- LINUX -----------------------------------------------------------------
# SDL2 is BUNDLED next to the binary rather than statically linked: a static
# SDL2 still drags in ~50 system libraries (X11, wayland, pulse, alsa...) and
# pins you to this machine's versions of them. Bundling the .so and pointing
# the loader at $ORIGIN/lib is what actually travels between distros -- and
# it's what the Steam Linux Runtime expects.
dist-linux: $(CORE_SRC) $(DESK_SRC) $(HDRS)
	@rm -rf $(DIST)/linux && mkdir -p $(DIST)/linux/lib
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(CORE_SRC) $(DESK_SRC) \
	    -o $(DIST)/linux/iknowwhatisaw \
	    $(SDL_LIBS) -Wl,-rpath,'$$ORIGIN/lib'
	@cp -L $$(ldd $(DIST)/linux/iknowwhatisaw | awk '/libSDL2/{print $$3}') \
	    $(DIST)/linux/lib/
	@printf '#!/bin/sh\ncd "$$(dirname "$$0")" && exec ./iknowwhatisaw "$$@"\n' \
	    > $(DIST)/linux/run.sh && chmod +x $(DIST)/linux/run.sh
	@strip $(DIST)/linux/iknowwhatisaw
	@echo "linux  -> $(DIST)/linux/  (SDL bundled, rpath \$$ORIGIN/lib)"

# ---- WINDOWS ---------------------------------------------------------------
# -mwindows = no console window pops up behind the game.
dist-windows: $(CORE_SRC) $(DESK_SRC) $(HDRS)
	@command -v $(MINGW_CC) >/dev/null 2>&1 || { \
	    echo "ERROR: $(MINGW_CC) not found."; \
	    echo "       run once:  sudo apt install mingw-w64"; exit 1; }
	@test -d $(SDL_MINGW) || { \
	    echo "ERROR: $(SDL_MINGW) missing. run:  make vendor-sdl"; exit 1; }
	@rm -rf $(DIST)/windows && mkdir -p $(DIST)/windows
	$(MINGW_CC) $(CFLAGS) -I$(SDL_MINGW)/include/SDL2 \
	    $(CORE_SRC) $(DESK_SRC) \
	    -o $(DIST)/windows/iknowwhatisaw.exe \
	    -L$(SDL_MINGW)/lib -lmingw32 -lSDL2main -lSDL2 -mwindows
	@cp $(SDL_MINGW)/bin/SDL2.dll $(DIST)/windows/
	@x86_64-w64-mingw32-strip $(DIST)/windows/iknowwhatisaw.exe
	@echo "windows -> $(DIST)/windows/  (SDL2.dll alongside)"

# Fetch the SDL2 Windows SDK (no sudo, lands in vendor/, gitignored).
vendor-sdl:
	@mkdir -p vendor
	@test -d $(SDL_MINGW) && echo "already have $(SDL_MINGW)" || { \
	    echo "fetching SDL2 mingw SDK..."; \
	    curl -sSL $(SDL_MINGW_URL) -o /tmp/sdl2-mingw.tar.gz && \
	    tar xzf /tmp/sdl2-mingw.tar.gz -C vendor && \
	    mv vendor/SDL2-2.30.11 vendor/SDL2-mingw && \
	    echo "-> vendor/SDL2-mingw"; }

# ---- ONE FILE TO HAND SOMEBODY ---------------------------------------------
# `make zip` -> dist/*.zip, ready to drag into Discord / itch.io / an email.
#
# It has to be a ZIP, not a bare .exe: the game needs SDL2.dll sitting NEXT to
# it, and an .exe on its own will just fail to start on the other person's
# machine with an unhelpful box about a missing DLL. Ship them together.
#
# The zip contains ONE folder, so it doesn't vomit loose files into whatever
# directory they unzip it in.
ZIPNAME := iknowwhatisaw-$(VERSION)

zip: zip-windows zip-linux
	@echo
	@ls -lh $(DIST)/*.zip $(DIST)/*.tar.gz 2>/dev/null | awk '{print "  " $$9 "  " $$5}'
	@echo "  ^ drag that into Discord"

zip-windows: dist-windows
	@rm -rf $(DIST)/pack && mkdir -p $(DIST)/pack/$(ZIPNAME)
	@cp -r $(DIST)/windows/. $(DIST)/pack/$(ZIPNAME)/
	@printf 'I KNOW WHAT I SAW\r\n\r\nDouble-click iknowwhatisaw.exe.\r\nKeep SDL2.dll in the same folder - the game needs it.\r\n\r\nArrows/WASD move. Z = A. X = B. Enter = START. Esc = menu.\r\nF11 = fullscreen. Plug in a gamepad and it just works.\r\n' \
	    > $(DIST)/pack/$(ZIPNAME)/README.txt
	@cd $(DIST)/pack && zip -qr ../$(ZIPNAME)-windows.zip $(ZIPNAME)
	@rm -rf $(DIST)/pack
	@echo "windows zip -> $(DIST)/$(ZIPNAME)-windows.zip"

zip-linux: dist-linux
	@rm -rf $(DIST)/pack && mkdir -p $(DIST)/pack/$(ZIPNAME)
	@cp -r $(DIST)/linux/. $(DIST)/pack/$(ZIPNAME)/
	@printf 'I KNOW WHAT I SAW\n\nRun ./run.sh\n(or ./iknowwhatisaw - SDL is bundled in lib/)\n' \
	    > $(DIST)/pack/$(ZIPNAME)/README.txt
	@cd $(DIST)/pack && tar czf ../$(ZIPNAME)-linux.tar.gz $(ZIPNAME)
	@rm -rf $(DIST)/pack
	@echo "linux tarball -> $(DIST)/$(ZIPNAME)-linux.tar.gz"

dist-clean:
	rm -rf $(DIST)

clean:
	rm -rf build *.o

.PHONY: pc ascii esp32 flash run run-ascii check clean zip zip-windows zip-linux \
        dist dist-linux dist-windows vendor-sdl dist-clean
