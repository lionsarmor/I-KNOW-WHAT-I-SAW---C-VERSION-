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

/* ---- SAVE GAMES -----------------------------------------------------------
 * The core has no idea what a file is, so it does not save anything: it
 * hands you a small blob of bytes and YOU decide where to put it (a file
 * on desktop, NVS on ESP32, an SD card, nothing at all).
 *
 * The whole protocol is four calls, and the platform drives all of them:
 *
 *   AT BOOT, if you have a saved blob, offer it:
 *       if (blob_exists)
 *           game_save_load(blob, len);   -- 1 = accepted, title says CONTINUE
 *
 *   EVERY FRAME, after game_update(), ask if the player hit SAVE:
 *       if (game_save_pending()) {
 *           uint8_t blob[GAME_SAVE_SIZE];
 *           game_save_write(blob);
 *           int ok = your_write_to_storage(blob, GAME_SAVE_SIZE);
 *           game_save_done(ok);          -- so the game can say SAVED / FAILED
 *       }
 *
 * The blob is fixed-size, endian-stable (little-endian on the wire) and
 * versioned, so a save written on the desktop loads on the handheld.
 * ==========================================================================*/
#define GAME_SAVE_SIZE 256

/* Serialize the current game into exactly GAME_SAVE_SIZE bytes. */
void game_save_write(uint8_t *buf);

/* Validate and apply a blob from storage. Returns 1 if it was a good save
 * (right magic + version + checksum), 0 if it was garbage or from an
 * incompatible build -- in which case the game is left untouched. */
int  game_save_load(const uint8_t *buf, int len);

/* Did the player just choose SAVE in the pack? Returns 1 exactly once,
 * then goes back to 0 -- so calling it every frame is correct. */
int  game_save_pending(void);

/* Tell the game whether the write actually worked. */
void game_save_done(int ok);

/* Same again for LOAD, which the player can also pick from the pack:
 *
 *       if (game_load_pending()) {
 *           uint8_t blob[GAME_SAVE_SIZE];
 *           int ok = your_read_from_storage(blob, GAME_SAVE_SIZE) &&
 *                    game_save_load(blob, GAME_SAVE_SIZE);
 *           game_load_done(ok);   -- says LOADED. or NO SAVE FOUND
 *       }
 *
 * A successful game_save_load() called mid-game drops the player straight
 * into the loaded game; called at boot it just makes the title offer
 * CONTINUE. The core works out which from its own state -- you don't have
 * to tell it. */
int  game_load_pending(void);
void game_load_done(int ok);

/* ---- DISPLAY ---------------------------------------------------------------
 * The core owns the OPTIONS menu; the PLATFORM owns the window. The core
 * never touches a window, a driver or a monitor -- it just records what the
 * player picked, and the platform makes the world match.
 *
 * Desktop wiring (see platform/desktop/main_sdl.c):
 *
 *   at boot   game_set_fullscreen(read_from_your_settings_file());
 *             -- so the OPTIONS menu opens showing the truth
 *
 *   each frame
 *             if (game_want_fullscreen() != window_is_fullscreen) {
 *                 make_the_window_match();
 *                 save_the_setting();
 *             }
 *
 * If your platform toggles fullscreen by some other route (an F11 key, a
 * window manager), call game_set_fullscreen() to tell the core -- otherwise
 * the menu will lie about the current state.
 *
 * Platforms with no window at all (the terminal build, the ESP32) simply
 * ignore both of these. */
int  game_want_fullscreen(void);   /* 1 = fullscreen, 0 = windowed */
void game_set_fullscreen(int on);  /* push the real state back into the core */

/* ---- QUITTING --------------------------------------------------------------
 * The player pressed ESC (or Back, or whatever your platform calls it). Do
 * NOT close the window: hand it to the core and it raises the pause screen,
 * which offers SAVE AND QUIT / QUIT / CANCEL. Losing an hour of play to a
 * misplaced thumb is not acceptable.
 *
 *   on ESC     game_request_quit();
 *   each frame if (game_quit_pending()) break;   -- now you may close
 *
 * SAVE AND QUIT goes through the ordinary save protocol above: the core
 * raises game_save_pending(), you write the blob and call game_save_done(),
 * and only THEN does game_quit_pending() come true. If your write fails the
 * game stays open and says so, rather than quietly eating the save. */
void game_request_quit(void);
int  game_quit_pending(void);

/* ---- GAMEPADS --------------------------------------------------------------
 * Same split as the display: the PLATFORM owns the hardware, the CORE owns
 * the menu. The desktop uses SDL's GameController layer, which already
 * normalises an Xbox pad, a Switch Pro pad, a PlayStation pad and most
 * no-name USB things into one layout -- so the core never learns what a
 * "Nintendo" is.
 *
 *   game_set_pad(name)          the platform says what's plugged in (or NULL)
 *   game_enable_controls_menu() the platform says "I can do pads" -- the
 *                               CONTROLS row only appears in OPTIONS if so,
 *                               so the handheld build never shows it
 *   game_swap_ab()              the platform reads this when it maps buttons
 *   game_rumble_take()          0..255, and CLEARS. Call it once per frame;
 *                               nonzero = shake the pad. (The ESP32's
 *                               vibrator motor can use exactly the same call.)
 */
/* ---- VOLUME ----------------------------------------------------------------
 * 0..VOL_STEPS (config.h). The platform reads these back after the player
 * touches OPTIONS and writes them to its settings file. */
int  game_music_volume(void);
void game_set_music_volume(int v);
int  game_sfx_volume(void);
void game_set_sfx_volume(int v);

void game_set_pad(const char *name);
void game_enable_controls_menu(int on);

/* Does this machine even HAVE a resizable window? The handheld does not, and
 * a DISPLAY row that does nothing when you press it is a lie. Call this with
 * 1 from any platform with a real window. */
void game_enable_display_menu(int on);
int  game_swap_ab(void);
void game_set_swap_ab(int on);
int  game_rumble_enabled(void);
void game_set_rumble(int on);
int  game_rumble_take(void);

#endif /* GAME_H */
