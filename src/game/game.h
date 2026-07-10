/* ============================================================================
 * game.h -- THE ENTIRE CONTRACT between the game and the outside world.
 *
 * A platform (SDL desktop, ESP32, bare-metal RISC-V, ...) only has to do
 * four things, forever, in a loop at 60 Hz:
 *
 *    1. game_init()                        -- once at boot
 *    2. game_update(buttons)               -- once per tick; runs one frame
 *                                             of logic AND redraws the
 *                                             framebuffer
 *    3. game_framebuffer()                 -- blit these pixels to a screen
 *    4. game_audio_fill(buf, n)            -- whenever the audio device is
 *                                             hungry (any thread/ISR is ok,
 *                                             it only touches audio state)
 *
 * The core calls NOTHING back. No malloc, no files, no OS. That is the
 * whole trick that makes it portable.
 * ==========================================================================*/
#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include "config.h"

/* Boot the game. Decodes all the ASCII-art assets into pixel data.
 * Returns the number of asset problems found (0 = all good). A platform
 * may print/log this; the game still runs either way. */
int game_init(void);

/* Advance the game exactly one tick (1/60 s) and redraw the framebuffer.
 * `buttons_held` is a bitmask of BTN_* for every button currently DOWN.
 * The core detects presses (edges) internally -- just report the state. */
void game_update(uint16_t buttons_held);

/* The screen: SCREEN_W * SCREEN_H pixels, row-major, RGB565.
 * NOTE for SPI LCDs (ILI9341 etc.): they usually want the two bytes of
 * each pixel swapped (big-endian). Do that swap in the PLATFORM layer,
 * not here -- see platform/esp32/main/esp32_main.c. */
const uint16_t *game_framebuffer(void);

/* Fill `out` with `nsamples` mono signed-16-bit samples at AUDIO_RATE.
 * Always succeeds (fills silence when nothing is playing). */
void game_audio_fill(int16_t *out, int nsamples);

#endif /* GAME_H */
