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
 * Mono, signed 16-bit, and a software tracker (see audio.h).
 *
 * THE TWO NUMBERS THAT DIFFER PER MACHINE. Both can be overridden by the
 * build (-DAUDIO_CHANS=4), which is exactly what the ESP32 does -- same
 * songs, same code, fewer voices and a cheaper mix.
 *
 *   DESKTOP   44100Hz, 8 voices  -- 7 for music, 1 for sfx
 *   ESP32     22050Hz, 4 voices  -- 3 for music, 1 for sfx. A four-track
 *             song keeps its lead, bass and drums and loses the pad. It is
 *             the same tune; it just has less air around it.
 *
 * Every extra voice costs one multiply-add per output sample. On a 160MHz
 * RISC-V at 22kHz that is nothing; be careful anyway.
 */
#ifndef AUDIO_RATE
#define AUDIO_RATE 44100
#endif

#ifndef AUDIO_CHANS
#define AUDIO_CHANS 8
#endif

/* How many notches the volume sliders have. */
#define VOL_STEPS   10
#define VOL_DEFAULT  7

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
#define SHELLS_PER_BOX    6   /* shotgun shells in one SHELLS pickup        */
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

/* ---- ZELDA MODE: fighting out in the field --------------------------------
 * Once you have the shotgun you can just SHOOT things in the overworld
 * (press B). No menus, no turn order. Creatures notice you from
 * AGGRO_RADIUS away and come at a run; a blast staggers them, and enough
 * blasts and they come apart.
 *
 * It's quicker and more dangerous than a proper battle, so it pays LESS:
 * an overworld kill is worth OVERWORLD_XP_PCT of the creature's XP. Walk
 * into one instead and you get the full amount for the turn-based fight.
 */
#define SHOOT_COOLDOWN     16   /* ticks between blasts (pump the gun) */
#define SHOT_SPEED          6   /* pixels per tick                     */
#define SHOT_RANGE        100   /* pixels before the shot dies         */
#define AGGRO_RADIUS       72   /* they notice you from this far       */
#define CHASE_SPEED         1   /* pixels per tick when charging       */
#define ENEMY_STUN_TICKS   45   /* how long a hit staggers them        */
#define KNOCKBACK          10   /* pixels a blast shoves them          */
#define OVERWORLD_XP_PCT   60   /* an overworld kill is worth 60%      */

/* THE BOSS IS DIFFERENT.
 * The turn-based fight with the goblin is meant to be brutal, so gunning it
 * down in the field is a legitimate alternative -- but not a free one. It
 * eats a lot of shells (see ow_hits in species[], assets.c) and, unlike
 * every other thing out here:
 *
 *   - it barely staggers, so you cannot chain-stun it into a statue;
 *   - a blast does not knock it back AT ALL. Kelly, Kentucky, 1955: they
 *     shot at it from the porch all night and it kept coming back;
 *   - once you shoot it, it charges as fast as you can run.
 *
 * And you can't shoot without turning to face it. Let it reach you and you
 * get the turn-based fight anyway -- with whatever HP you have left. */
#define BOSS_STUN_TICKS     8   /* it rocks, and that's all            */
#define BOSS_CHASE_SPEED    2   /* exactly PLAYER_SPEED. it keeps up.  */

/* ---- THE ANT HILL ----------------------------------------------------------
 * A rooted species with a `brood` (see species[] in assets.c) opens up and
 * pushes another ant out every ANTHILL_BROOD_TICKS -- but only while it has
 * fewer than BROOD_MAX of its children alive on the map, so a hill you
 * ignore becomes a problem and a hill you ignore forever does not become an
 * infinite one. Kill the hill and the spawning stops; the ants it already
 * made are still your problem.
 *
 * ANTHILL_SOLDIER_PCT of what crawls out are the tougher SOLDIER ants.
 */
#define ANTHILL_BROOD_TICKS 600   /* 10s between ants -- slow enough that
                                     you can cross the farm, talk to the
                                     cows, and not be swarmed doing it   */
#define BROOD_MAX             2   /* live children on the map at once     */
#define ANTHILL_SOLDIER_PCT  30   /* the rest are ordinary giant ants      */

/* ---- PA'S TNT --------------------------------------------------------------
 * The panic button. In a battle it's a flat, enormous hit -- more than the
 * shotgun does, and it doesn't care how tough the thing is.
 *
 * Out in the field you lob it: it lands TNT_THROW pixels ahead of you and
 * blows a hole in everything inside TNT_RADIUS. It takes TNT_OW_HITS off
 * each of them (so it drops most things outright) and, crucially, leaves
 * whatever survives reeling for TWICE the normal stagger -- which is the
 * only way to buy real distance from something that doesn't stagger, like
 * the goblin. It's the answer to being cornered.
 */
#define TNT_DMG           34   /* battle damage. it does not roll.     */
#define TNT_RADIUS        44   /* blast radius in pixels               */
#define TNT_THROW         26   /* how far ahead of you it lands        */
#define TNT_OW_HITS        3   /* field hits taken off everything near */
#define TNT_STUN_MULT      2   /* survivors reel for DOUBLE the usual  */
#define TNT_BOOM_TICKS    26   /* how long the fireball is on screen   */

/* ---- WHAT THE TOWN GIVES YOU -----------------------------------------------
 * People help. Not all of them, and not on cue: on each map, exactly ONE
 * person -- picked at random when you walk in -- is holding something for
 * you, and you won't know who until you talk to them. They hand over a
 * random item and patch you up while they're at it.
 *
 * It resets on the same clock as the fields (ITEM_RESPAWN_TICKS), so you
 * can't farm it by walking in and out of a door: one kindness per place,
 * per day. */
#define BOON_HEAL         12   /* HP they patch you up for             */
#define MAX_SHOTS           4
#define MAX_GORE           28

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

/* A street lamp throws a wider, warmer pool than your torch -- and it
 * flickers, because the ones out here always do. */
#define LAMP_RADIUS       46

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
/* One NPC's whole speech, after the '~' name expansion. It has to hold the
 * LONGEST line in maps.h -- and now also a generous NPC line PLUS the gift
 * line tacked on after it (see boon_line), so it needs headroom. */
#define DIALOG_BUF_MAX 768

/* A battle message has to sit on screen at least this long before A will
 * dismiss it. Battle text appears instantly (no typewriter), so without this
 * a player leaning on the button never sees "IT DRAINS 9 AND SWELLS!" at
 * all -- the message is drawn and gone inside two frames. */
#define MSG_MIN_TICKS 26

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
