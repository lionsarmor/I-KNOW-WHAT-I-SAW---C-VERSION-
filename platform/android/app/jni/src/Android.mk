# ============================================================================
# The Android build of the SAME sources every other platform uses.
#
# Note what is NOT here: a copy of the game. LOCAL_SRC_FILES reaches up out of
# platform/android and straight into src/game/. Fix a bug once, every platform
# gets it -- the whole point of the project.
# ============================================================================
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH  := ../SDL
# from platform/android/app/jni/src, five levels up is the repo root
REPO      := $(LOCAL_PATH)/../../../../..

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/$(SDL_PATH)/include \
    $(REPO)/src/game \
    $(REPO)/platform/android

LOCAL_SRC_FILES := \
    $(REPO)/src/game/assets.c \
    $(REPO)/src/game/audio.c \
    $(REPO)/src/game/battle.c \
    $(REPO)/src/game/cutscene.c \
    $(REPO)/src/game/game.c \
    $(REPO)/src/game/gfx.c \
    $(REPO)/src/game/intro.c \
    $(REPO)/src/game/overworld.c \
    $(REPO)/platform/desktop/main_sdl.c \
    $(REPO)/platform/android/touch.c

# same as the desktop Makefile: the content tables omit trailing fields on
# purpose and C zero-fills them.
LOCAL_CFLAGS := -std=gnu99 -O2 -Wall -Wextra -Wno-missing-field-initializers

LOCAL_SHARED_LIBRARIES := SDL2
LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid

include $(BUILD_SHARED_LIBRARY)
