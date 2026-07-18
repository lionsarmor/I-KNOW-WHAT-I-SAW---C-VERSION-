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
# The app icon (taskbar / Explorer / Steam library) is compiled in with windres,
# but only if the .rc is present -- the build still works without it. Regenerate
# the .ico from the in-game sprite with:  python3 tools/make_icon.py
WIN_ICON_RC  := platform/desktop/win_icon.rc
WIN_ICON_OBJ := $(shell test -f platform/desktop/win_icon.rc && echo build/win_icon.o)

dist-windows: $(CORE_SRC) $(DESK_SRC) $(HDRS)
	@command -v $(MINGW_CC) >/dev/null 2>&1 || { \
	    echo "ERROR: $(MINGW_CC) not found."; \
	    echo "       run once:  sudo apt install mingw-w64"; exit 1; }
	@test -d $(SDL_MINGW) || { \
	    echo "ERROR: $(SDL_MINGW) missing. run:  make vendor-sdl"; exit 1; }
	@rm -rf $(DIST)/windows && mkdir -p $(DIST)/windows
	@test -z "$(WIN_ICON_OBJ)" || { mkdir -p build && \
	    x86_64-w64-mingw32-windres -I platform/desktop \
	        $(WIN_ICON_RC) -O coff -o $(WIN_ICON_OBJ); }
	$(MINGW_CC) $(CFLAGS) -I$(SDL_MINGW)/include/SDL2 \
	    $(CORE_SRC) $(DESK_SRC) $(WIN_ICON_OBJ) \
	    -o $(DIST)/windows/iknowwhatisaw.exe \
	    -L$(SDL_MINGW)/lib -lmingw32 -lSDL2main -lSDL2 -mwindows
	@cp $(SDL_MINGW)/bin/SDL2.dll $(DIST)/windows/
	@x86_64-w64-mingw32-strip $(DIST)/windows/iknowwhatisaw.exe
	@echo "windows -> $(DIST)/windows/  (SDL2.dll alongside, icon embedded)"

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

# ---- THE WEB (WASM) --------------------------------------------------------
# `make web`  ->  dist/web/{index.html,iknowwhatisaw.js,iknowwhatisaw.wasm}
# `make serve` -> ...and serves it at http://localhost:8000
#
# Same src/game, same main_sdl.c. Emscripten ships SDL2, so -sUSE_SDL=2 is the
# whole of the "port".
#
# ONE-TIME:  make emsdk        (installs Emscripten into ~/emsdk)
#
# -sASYNCIFY is NOT used and must not be: the loop was restructured into
# frame_step() so the browser can drive it (see main_sdl.c). Asyncify would
# have been the lazy way and it doubles the binary and halves the speed.
#
# MEMORY IS FIXED, NOT GROWABLE, and that is deliberate. ALLOW_MEMORY_GROWTH
# makes Emscripten hand JS a *resizable* ArrayBuffer, and current Chrome's
# TextDecoder flatly refuses to decode one -- the runtime throws on its first
# string and the canvas stays black. We don't need growth anyway: this game's
# footprint is fixed and known (~133KB of RAM), so we just say how much we
# want up front and never ask for more.
EMSDK ?= $(HOME)/emsdk
WEB   := $(DIST)/web

# If emcc is already on PATH (CI does this) use it as-is; otherwise source
# the local emsdk. Same command either way.
web: $(CORE_SRC) $(DESK_SRC) $(HDRS) platform/web/shell.html
	@command -v emcc >/dev/null 2>&1 || test -f "$(EMSDK)/emsdk_env.sh" || { \
	    echo "ERROR: no Emscripten (not on PATH, and none at $(EMSDK))"; \
	    echo "       run once:  make emsdk"; exit 1; }
	@mkdir -p $(WEB)
	{ command -v emcc >/dev/null 2>&1 || . "$(EMSDK)/emsdk_env.sh" >/dev/null 2>&1; } && \
	emcc $(CORE_SRC) $(DESK_SRC) \
	    -Isrc/game -O2 -std=gnu99 -Wall -Wextra \
	    -Wno-missing-field-initializers \
	    -sUSE_SDL=2 \
	    -sINITIAL_MEMORY=64MB \
	    -sFORCE_FILESYSTEM=1 \
	    -lidbfs.js \
	    -sEXPORTED_RUNTIME_METHODS=ccall,FS \
	    -sEXPORTED_FUNCTIONS=_main,_web_storage_ready \
	    --shell-file platform/web/shell.html \
	    -o $(WEB)/index.html
	@touch $(WEB)/.nojekyll      # GitHub Pages: don't run Jekyll over this
	@ls -lh $(WEB)/index.* | awk '{print "  " $$9 "  " $$5}'
	@echo "  try it:  make serve"

serve: web
	@echo "  http://localhost:8000   (ctrl-c to stop)"
	@cd $(WEB) && python3 -m http.server 8000

emsdk:
	@test -d $(EMSDK) || git clone --depth 1 \
	    https://github.com/emscripten-core/emsdk.git $(EMSDK)
	@cd $(EMSDK) && ./emsdk install latest && ./emsdk activate latest
	@echo "done. now:  make web"

# ---- ANDROID ---------------------------------------------------------------
# `make apk`  ->  dist/iknowwhatisaw.apk   (debug-signed: installable, not
#                                           publishable. See `make apk-release`)
#
# Builds the SAME src/game sources every other platform uses -- look at
# platform/android/app/jni/src/Android.mk: it reaches straight up into
# src/game/. There is no copy of the game in here.
#
# On-screen controls live in platform/android/touch.c. The core does not know
# what a finger is.
#
# ONE-TIME SETUP (all of it lands in ~/android-tools, no sudo, ~2.6GB):
#     make android-sdk
ANDROID_HOME ?= $(HOME)/android-tools/sdk
JAVA_HOME    ?= $(HOME)/android-tools/jdk

# THE SPACE PROBLEM. ndk-build IS GNU make, and GNU make cannot handle spaces
# in a path -- and this project lives in a folder with spaces in its name.
# Gradle also canonicalises symlinks, so pointing a space-free symlink at the
# project does NOT get you out of it.
#
# So: mirror the sources into a space-free staging tree and build there. It's
# an rsync of a few hundred KB of game (plus SDL once), it's incremental, and
# it means `make apk` works no matter what the project folder is called.
APK_STAGE := $(HOME)/.cache/ikwis-apk
# NOTE the FLAVOUR in these paths. Adding product flavours (phone / ouya)
# moves gradle's output from apk/debug/app-debug.apk to
# apk/<flavour>/debug/app-<flavour>-debug.apk -- so the moment a flavour
# exists these paths change, gradle still reports BUILD SUCCESSFUL, and the
# copy afterwards quietly fails. Which is exactly what happened.
APK_BUILD := $(APK_STAGE)/platform/android/app/build/outputs/apk
APK_OUT   := $(APK_BUILD)/phone/debug/app-phone-debug.apk
OUYA_OUT  := $(APK_BUILD)/ouya/debug/app-ouya-debug.apk

# vendor-sdl-src is a prerequisite, not an error message: vendor/ is gitignored,
# so a fresh clone has no SDL source. Fetch it rather than telling the user to.
apk: vendor-sdl-src $(CORE_SRC) $(HDRS) platform/android/touch.c
	@test -d "$(ANDROID_HOME)" || { \
	    echo "ERROR: no Android SDK at $(ANDROID_HOME)"; \
	    echo "       run once:  make android-sdk    (~2.6GB, no sudo)"; exit 1; }
	@mkdir -p $(DIST) $(APK_STAGE)/platform/android $(APK_STAGE)/src/game \
	         $(APK_STAGE)/platform/desktop $(APK_STAGE)/vendor/SDL2-src
	@rsync -a --delete --exclude 'app/build' --exclude '.gradle' \
	    --exclude 'local.properties' --exclude 'app/jni/SDL' \
	    platform/android/ $(APK_STAGE)/platform/android/
	@rsync -a --delete src/game/      $(APK_STAGE)/src/game/
	@rsync -a --delete platform/desktop/ $(APK_STAGE)/platform/desktop/
	@rsync -a vendor/SDL2-src/        $(APK_STAGE)/vendor/SDL2-src/
	@ln -sfn ../../../../vendor/SDL2-src $(APK_STAGE)/platform/android/app/jni/SDL
	@echo "sdk.dir=$(ANDROID_HOME)" > $(APK_STAGE)/platform/android/local.properties
	cd $(APK_STAGE)/platform/android && \
	    JAVA_HOME="$(JAVA_HOME)" ANDROID_HOME="$(ANDROID_HOME)" \
	    ANDROID_SDK_ROOT="$(ANDROID_HOME)" \
	    PATH="$(JAVA_HOME)/bin:$$PATH" ./gradlew assemblePhoneDebug --no-daemon
	@cp $(APK_OUT) $(DIST)/iknowwhatisaw.apk
	@ls -lh $(DIST)/iknowwhatisaw.apk | awk '{print "  apk -> " $$9 "  " $$5}'
	@echo "  install:  adb install -r $(DIST)/iknowwhatisaw.apk"

apk-install: apk
	adb install -r $(DIST)/iknowwhatisaw.apk

# ---- THE OUYA --------------------------------------------------------------
# The 2013 microconsole. Android 4.1 (API 16) forever, a Tegra 3 (armeabi-v7a
# and nothing else), no touchscreen, and a controller with NO START and NO BACK
# button -- so the pack is on the shoulders and pause is a click of the right
# stick (see service_pad_pause in main_sdl.c).
#
# It needs NDK r21: r26's floor is API 21, which is above the OUYA's ceiling.
apk-ouya: vendor-sdl-src $(CORE_SRC) $(HDRS) platform/android/touch.c
	@test -d "$(ANDROID_HOME)/ndk/21.4.7075529" || { \
	    echo "ERROR: NDK r21 missing -- it is the last one that can target"; \
	    echo "       API 16, which is where the OUYA's firmware stopped."; \
	    echo "       run:  make android-sdk"; exit 1; }
	@mkdir -p $(DIST) $(APK_STAGE)/platform/android $(APK_STAGE)/src/game \
	         $(APK_STAGE)/platform/desktop $(APK_STAGE)/vendor/SDL2-src
	@rsync -a --delete --exclude 'app/build' --exclude '.gradle' \
	    --exclude 'local.properties' --exclude 'app/jni/SDL' \
	    platform/android/ $(APK_STAGE)/platform/android/
	@rsync -a --delete src/game/         $(APK_STAGE)/src/game/
	@rsync -a --delete platform/desktop/ $(APK_STAGE)/platform/desktop/
	@rsync -a vendor/SDL2-src/           $(APK_STAGE)/vendor/SDL2-src/
	@ln -sfn ../../../../vendor/SDL2-src $(APK_STAGE)/platform/android/app/jni/SDL
	@echo "sdk.dir=$(ANDROID_HOME)" > $(APK_STAGE)/platform/android/local.properties
	cd $(APK_STAGE)/platform/android && \
	    JAVA_HOME="$(JAVA_HOME)" ANDROID_HOME="$(ANDROID_HOME)" \
	    ANDROID_SDK_ROOT="$(ANDROID_HOME)" \
	    PATH="$(JAVA_HOME)/bin:$$PATH" ./gradlew assembleOuyaDebug --no-daemon
	@cp $(OUYA_OUT) $(DIST)/iknowwhatisaw-ouya.apk
	@ls -lh $(DIST)/iknowwhatisaw-ouya.apk | awk '{print "  ouya apk -> " $$9 "  " $$5}'
	@echo "  install:  adb connect <ouya-ip>  &&  adb install -r $(DIST)/iknowwhatisaw-ouya.apk"

apk-ouya-install: apk-ouya
	adb install -r $(DIST)/iknowwhatisaw-ouya.apk

# SDL2 SOURCE (Android compiles SDL itself; the Windows build uses a prebuilt)
vendor-sdl-src:
	@test -d vendor/SDL2-src && echo "already have vendor/SDL2-src" || { \
	    mkdir -p vendor; \
	    curl -sSL https://github.com/libsdl-org/SDL/releases/download/release-2.30.11/SDL2-2.30.11.tar.gz \
	        -o /tmp/sdl2-src.tar.gz && \
	    tar xzf /tmp/sdl2-src.tar.gz -C vendor && \
	    mv vendor/SDL2-2.30.11 vendor/SDL2-src && \
	    ln -sfn ../../../../vendor/SDL2-src platform/android/app/jni/SDL && \
	    echo "-> vendor/SDL2-src"; }

# The whole Android toolchain, into $(HOME)/android-tools. No sudo, ~2.6GB.
android-sdk:
	@bash tools/install-android-sdk.sh

dist-clean:
	rm -rf $(DIST)

clean:
	rm -rf build *.o

.PHONY: pc ascii esp32 flash run run-ascii check clean zip zip-windows zip-linux \
        apk apk-install apk-ouya apk-ouya-install android-sdk vendor-sdl-src \
        web serve emsdk \
        dist dist-linux dist-windows vendor-sdl dist-clean
