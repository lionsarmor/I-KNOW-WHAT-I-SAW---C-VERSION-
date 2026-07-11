/* ============================================================================
 * config.h -- ALL the knobs in one place.
 *
 * This file is the first place to look when you want to change how the game
 * feels: screen size, walk speed, battle numbers, colors, timing.
 * Nothing in here touches hardware -- it is shared by every platform.
 * ==========================================================================*/
#ifndef CONFIG_H
#define CONFIG_H

/* ---- Screen ---------------------------------------------------------------
 * 240x160 is the classic GBA resolution. It fits centered on a cheap
 * 320x240 ILI9341 SPI display (with a black border), and the whole
 * RGB565 framebuffer is 240*160*2 = 76,800 bytes -- comfortable on an
 * ESP32's ~520 KB of RAM.  The desktop build scales it up 4x.
 * If you change these, everything (camera, menus, intro) adapts, but
 * check your target display can fit the new size.
 */
#define SCREEN_W 240
#define SCREEN_H 160

/* One map tile is 16x16 pixels. Sprites are also 16x16. */
#define TILE 16

/* ---- Timing ---------------------------------------------------------------
 * The game is a fixed-timestep machine: the platform layer calls
 * game_update() exactly 60 times per second. All durations in the code
 * are measured in these ticks (60 ticks = 1 second).
 */
#define TICKS_PER_SEC 60

/* ---- Audio ----------------------------------------------------------------
 * Mono, signed 16-bit. 22050 Hz keeps the math cheap on a microcontroller.
 */
#define AUDIO_RATE 22050

/* ---- Player / gameplay tunables ------------------------------------------*/
#define PLAYER_SPEED      2   /* pixels moved per tick while walking        */
#define PLAYER_MAX_HP    20   /* starting max health                        */
#define PLAYER_BASE_ATK   3   /* base damage before level bonus + rng       */
#define XP_PER_LEVEL     10   /* need level * XP_PER_LEVEL to level up      */
#define ALIEN_WANDER_SPEED 1  /* pixels per tick when an alien wanders      */
/* Per-ENEMY stats (hp, attack, xp) live in the species[] table in
 * assets.c -- see "THE CAST & THE BESTIARY". */

/* ---- Items ----------------------------------------------------------------*/
#define HERB_HEAL         8   /* HP restored by one herb                    */
#define SHELLS_PER_BOX    4   /* shotgun shells in one SHELLS pickup        */
#define SHOTGUN_BASE_DMG  8   /* a blast does this + level + rng(0..4)      */

/* The world restocks: every this many ticks, every ITEM you've taken comes
 * back where it lay. Herbs regrow, the store gets a delivery, somebody
 * leaves another box of shells on the porch -- otherwise the shotgun runs
 * dry and the boss is unbeatable.
 *
 * Gifts and BOSSES never come back: only ENT_ITEM spawns are restocked.
 * (see restock_items() in overworld.c)
 *
 * One full day+night is a natural rhythm: sleep on it, and the fields have
 * grown back. Shorten it if you want to farm shells faster. */
#define ITEM_RESPAWN_TICKS (DAY_LEN_TICKS + NIGHT_LEN_TICKS)

/* ---- Day / night ----------------------------------------------------------
 * The overworld clock. Day and night each last this many ticks
 * (60 ticks = 1 second). Set SHORT for testing -- a real playthrough
 * probably wants 5+ minutes a side. Dusk/dawn is the fade between.
 * Night only darkens OUTDOOR maps (indoors the lamps are lit).
 */
#define DAY_LEN_TICKS    (60 * TICKS_PER_SEC)   /* 1 minute of daylight   */
#define NIGHT_LEN_TICKS  (60 * TICKS_PER_SEC)   /* 1 minute of night      */
#define DUSK_LEN_TICKS   (3 * TICKS_PER_SEC)    /* sunset / sunrise fade  */
#define NIGHT_BRIGHTNESS 112                    /* 256 = day. lower = darker */

/* ---- The flashlight -------------------------------------------------------
 * Switch it on from the PACK (START) and it carves a pool of real light
 * out of the night around you. Inside HALF this radius it's as bright as
 * day; from there it falls off to the ambient night level.
 * Costs nothing in daylight -- the whole pass is skipped.
 */
#define FLASHLIGHT_RADIUS 60   /* pixels */

/* ---- Your name ------------------------------------------------------------
 * Chosen on the name screen at the start of a new game. NPCs who know you
 * use it: write a '~' in their dialog and it is replaced with this.
 * Keep it short -- it has to fit in the HUD and in a textbox line.
 */
#define PLAYER_NAME_MAX 8
#define NAME_DEFAULT "PLAYER"

/* The most text one NPC can say. Speech is PAGED (three lines at a time,
 * press A for the next page), so a line can be as long as you like -- but
 * it still has to fit in this buffer once '~' has been swapped for the
 * player's name. assets_init() checks every line in the game against this
 * and reports any that are too long, rather than silently chopping the end
 * off somebody's sentence. */
#define DIALOG_BUF_MAX 512

/* ---- Buttons --------------------------------------------------------------
 * The abstract gamepad. The CORE only ever sees these bits; each platform
 * decides what physical key / GPIO pin produces them.
 *   Desktop mapping:  platform/desktop/main_sdl.c  (SDL scancodes)
 *   ESP32 mapping:    platform/esp32/main/esp32_main.c (GPIO pin numbers)
 * Add a new button by adding a bit here and wiring it in each platform.
 */
#define BTN_UP     (1u << 0)
#define BTN_DOWN   (1u << 1)
#define BTN_LEFT   (1u << 2)
#define BTN_RIGHT  (1u << 3)
#define BTN_A      (1u << 4)   /* confirm / talk / attack   */
#define BTN_B      (1u << 5)   /* cancel / back             */
#define BTN_START  (1u << 6)   /* start / pause             */

#endif /* CONFIG_H */
