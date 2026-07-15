/* ============================================================================
 * game.c -- the heart: owns the game state and dispatches to whichever
 * scene is active. This is deliberately tiny; the interesting code lives
 * in intro.c / overworld.c / battle.c.
 * ==========================================================================*/
#include "game_internal.h"

game_t G;

/* ---- RNG ------------------------------------------------------------------
 * xorshift32: three shifts, good enough for damage rolls, identical on
 * every platform (no libc rand()). Reseeded from the frame counter when
 * the player presses START, so runs differ.
 */
uint32_t rng_next(void)
{
    uint32_t x = G.rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    G.rng = x;
    return x;
}

int rng_range(int lo, int hi)
{
    return lo + (int)(rng_next() % (uint32_t)(hi - lo + 1));
}

/* ---- lifecycle ------------------------------------------------------------*/
int game_init(void)
{
    int asset_errors = assets_init();
    audio_init();

    G.state = ST_INTRO;
    G.frame = 0;
    G.t     = 0;
    G.rng   = 0x1D872665u;   /* any nonzero seed works */

    /* Sensible until the platform loads the player's saved settings over
     * the top -- and correct forever on a platform that has none. */
    G.music_vol = VOL_DEFAULT;
    G.sfx_vol   = VOL_DEFAULT;
    G.can_quit  = 1;          /* a platform that can't exit says so */
    audio_apply_volumes();

    audio_music(MUSIC_INTRO);   /* the heartbeat starts immediately */
    return asset_errors;
}

/* Draw a scene WITHOUT advancing it. Every xxx_render() is a pure function
 * of G, so it is safe to call one for a scene that isn't the current one --
 * which is exactly what the PAUSE screen does to paint the frozen world
 * behind its menu, at no cost in memory. */
void render_scene(int s)
{
    switch (s) {
    case ST_INTRO:     intro_render();     break;
    case ST_TITLE:     title_render();     break;
    case ST_OVERWORLD: overworld_render(); break;
    case ST_NAME:      name_render();      break;
    case ST_OPTIONS:   options_render();   break;
    case ST_DIALOG:    dialog_render();    break;
    case ST_PACK:      pack_render();      break;
    case ST_SLEEP:     sleep_render();     break;
    case ST_DRIVE:     drive_render();     break;
    case ST_PROLOGUE:  prologue_render();  break;
    case ST_BATTLE:    battle_render();    break;
    case ST_GAMEOVER:  gameover_render();  break;
    case ST_JOURNAL:   journal_render();   break;
    case ST_NIGHT:     night_render();     break;
    case ST_PART2END:  part2end_render();  break;
    case ST_PAUSE:     pause_render();     break;
    case ST_CONTROLS:  controls_render();  break;
    case ST_CHURCH:    church_render();    break;
    case ST_PART1END:  part1end_render();  break;
    }
}

void game_update(uint16_t buttons_held)
{
    /* edge detection: pressed = down now, up last tick */
    G.pressed = (uint16_t)(buttons_held & ~G.held);
    G.held    = buttons_held;

    /* The ambient bed belongs to the WORLD -- scenes where the overworld
     * is still around you (walking, talking, the pack, a fight on the same
     * grass, dozing off) keep it; cutscenes, menus and the title do not.
     * Crickets over the highway cutscene was a bug, not a mood. The
     * overworld's ambient_refresh() brings the bed back the moment you're
     * standing in a field again. */
    switch (G.state) {
    case ST_OVERWORLD: case ST_DIALOG: case ST_PACK: case ST_BATTLE:
    case ST_PAUSE:     case ST_JOURNAL: case ST_SLEEP:
        break;
    default:
        audio_ambient(AMB_NONE);
        break;
    }

    switch (G.state) {
    case ST_INTRO:     intro_update();     break;
    case ST_TITLE:     title_update();     break;
    case ST_OVERWORLD: overworld_update(); break;
    case ST_NAME:      name_update();      break;
    case ST_OPTIONS:   options_update();   break;
    case ST_DIALOG:    dialog_update();    break;
    case ST_PACK:      pack_update();      break;
    case ST_SLEEP:     sleep_update();     break;
    case ST_DRIVE:     drive_update();     break;
    case ST_PROLOGUE:  prologue_update();  break;
    case ST_BATTLE:    battle_update();    break;
    case ST_GAMEOVER:  gameover_update();  break;
    case ST_JOURNAL:   journal_update();   break;
    case ST_NIGHT:     night_update();     break;
    case ST_PART2END:  part2end_update();  break;
    case ST_PAUSE:     pause_update();     break;
    case ST_CONTROLS:  controls_update();  break;
    case ST_CHURCH:    church_update();    break;
    case ST_PART1END:  part1end_update();  break;
    }

    if (G.shake_t > 0)
        G.shake_t--;

    /* Draw whatever scene we are in NOW -- an update may have switched it,
     * and a brand-new scene should never show one frame of the old one. */
    render_scene(G.state);
    gfx_origin(0, 0);        /* never leak a shake into the next frame */

    G.frame++;
    G.t++;      /* scenes reset this to 0 when they change G.state */
}

const uint16_t *game_framebuffer(void)
{
    return gfx_fb;
}

/* ============================ SAVE GAMES ====================================
 * The blob is written by hand, one byte at a time, little-endian. No
 * memcpy of structs, no compiler padding, no endianness surprises: a save
 * written by the desktop build loads on the ESP32 and vice versa.
 *
 * TO SAVE A NEW FIELD: append it at the end of both walks and bump
 * SAVE_VERSION. Old saves then fail the version check and are ignored
 * rather than being misread.
 * ==========================================================================*/
#define SAVE_MAGIC   0x494B5753u   /* "IKWS" */
#define SAVE_VERSION 11            /* v11: PART 2 -- the ship. The blob grew
                                      with it (five new items, one new map,
                                      one new species in the journal). v10
                                      was the journal; v9 the South Side;
                                      v8 holy water and the rosary; v7 the
                                      pistol. Old saves fail the check and
                                      are ignored, as designed. */

/* Exactly how many bytes game_save_write() lays down. Kept next to the
 * writer so the two can't drift apart, and checked at COMPILE TIME below:
 * add a couple of items or maps without noticing and the blob would
 * quietly run off the end of the platform's buffer. */
#define SAVE_BYTES (4 /*magic*/ + 4 /*version*/                              \
                  + 9 * 4 /*x y dir hp max level xp lamp rosary*/            \
                  + NUM_ITEMS * 4                                            \
                  + (PLAYER_NAME_MAX + 1)                                           \
                  + 4 /*map*/ + 4 /*daytime*/ + 4 /*restock*/                \
                  + 4 /*seen*/ + 4 /*flags*/ + 4 /*rng*/                       \
                  + 4 /*boons_done*/                                         \
                  + NUM_MAPS * 4                                             \
                  + 4 /*species_seen*/ + NUM_SPECIES /*kills, a byte each*/  \
                  + 4 /*checksum*/)

/* If this line won't compile, the save blob outgrew GAME_SAVE_SIZE.
 * Raise GAME_SAVE_SIZE in game.h (and bump SAVE_VERSION). */
typedef char save_blob_fits[(SAVE_BYTES <= GAME_SAVE_SIZE) ? 1 : -1];

/* a tiny cursor so the two walks can't drift out of step */
typedef struct { uint8_t *p; int n; } wcur_t;
typedef struct { const uint8_t *p; int n; } rcur_t;

static void w32(wcur_t *c, uint32_t v)
{
    c->p[c->n++] = (uint8_t)(v      );
    c->p[c->n++] = (uint8_t)(v >>  8);
    c->p[c->n++] = (uint8_t)(v >> 16);
    c->p[c->n++] = (uint8_t)(v >> 24);
}

static uint32_t r32(rcur_t *c)
{
    uint32_t v = (uint32_t)c->p[c->n]
               | ((uint32_t)c->p[c->n + 1] <<  8)
               | ((uint32_t)c->p[c->n + 2] << 16)
               | ((uint32_t)c->p[c->n + 3] << 24);
    c->n += 4;
    return v;
}

/* sum of every byte before the checksum field -- catches a truncated or
 * corrupted blob, which matters on a handheld that can lose power mid-write */
static uint32_t save_sum(const uint8_t *b, int upto)
{
    uint32_t s = 0;
    for (int i = 0; i < upto; i++)
        s += (uint32_t)b[i] * (uint32_t)(i + 1);
    return s;
}

void game_save_write(uint8_t *buf)
{
    wcur_t c = { buf, 0 };
    for (int i = 0; i < GAME_SAVE_SIZE; i++)
        buf[i] = 0;

    w32(&c, SAVE_MAGIC);
    w32(&c, SAVE_VERSION);

    w32(&c, (uint32_t)G.player.x);
    w32(&c, (uint32_t)G.player.y);
    w32(&c, (uint32_t)G.player.dir);
    w32(&c, (uint32_t)G.player.hp);
    w32(&c, (uint32_t)G.player.max_hp);
    w32(&c, (uint32_t)G.player.level);
    w32(&c, (uint32_t)G.player.xp);
    w32(&c, (uint32_t)G.player.lamp);
    w32(&c, (uint32_t)G.player.rosary);
    for (int i = 0; i < NUM_ITEMS; i++)
        w32(&c, (uint32_t)G.player.items[i]);
    for (int i = 0; i < PLAYER_NAME_MAX + 1; i++)      /* your name, one byte each */
        buf[c.n++] = (uint8_t)G.player.name[i];

    w32(&c, (uint32_t)G.map_id);
    w32(&c, G.daytime);
    w32(&c, G.last_restock);
    w32(&c, (uint32_t)G.items_seen);
    w32(&c, G.flags);
    w32(&c, G.rng);
    w32(&c, (uint32_t)G.boons_done);
    for (int m = 0; m < NUM_MAPS; m++)
        w32(&c, (uint32_t)G.spawns_gone[m]);

    /* the journal: what you saw, and how many you put down */
    w32(&c, (uint32_t)G.species_seen);
    for (int i = 0; i < NUM_SPECIES; i++)
        buf[c.n++] = G.species_kills[i];

    /* checksum goes last, over everything written so far */
    w32(&c, save_sum(buf, c.n));
}

int game_save_load(const uint8_t *buf, int len)
{
    if (len < GAME_SAVE_SIZE)
        return 0;

    rcur_t c = { buf, 0 };
    if (r32(&c) != SAVE_MAGIC)   return 0;
    if (r32(&c) != SAVE_VERSION) return 0;

    /* Read into locals FIRST. If anything is wrong we must leave the live
     * game untouched, not half-overwritten. */
    player_t p;
    p.walking = 0;
    p.anim    = 0;
    p.x       = (int)r32(&c);
    p.y       = (int)r32(&c);
    p.dir     = (int)r32(&c);
    p.hp      = (int)r32(&c);
    p.max_hp  = (int)r32(&c);
    p.level   = (int)r32(&c);
    p.xp      = (int)r32(&c);
    p.lamp    = (int)r32(&c);
    p.rosary  = (int)r32(&c);
    for (int i = 0; i < NUM_ITEMS; i++)
        p.items[i] = (int)r32(&c);
    for (int i = 0; i < PLAYER_NAME_MAX + 1; i++)
        p.name[i] = (char)buf[c.n++];
    p.name[PLAYER_NAME_MAX] = '\0';                    /* never trust the blob */

    int      map     = (int)r32(&c);
    uint32_t daytime = r32(&c);
    uint32_t restock = r32(&c);
    uint32_t seen    = r32(&c);              /* items_seen: 32 bits now */
    uint32_t flags   = r32(&c);
    uint32_t rng     = r32(&c);
    uint16_t boons   = (uint16_t)r32(&c);
    uint32_t gone[NUM_MAPS];
    for (int m = 0; m < NUM_MAPS; m++)
        gone[m] = r32(&c);

    uint32_t seen_sp = r32(&c);             /* the journal (widened for TAN) */
    uint8_t  kills[NUM_SPECIES];
    for (int i = 0; i < NUM_SPECIES; i++)
        kills[i] = buf[c.n++];

    if (r32(&c) != save_sum(buf, c.n - 4))
        return 0;                       /* corrupt / truncated */

    /* Don't trust the numbers just because the checksum matched -- a save
     * from an older content build could point at a map that no longer
     * exists and walk us straight off the end of maps[]. */
    if (map < 0 || map >= NUM_MAPS)   return 0;
    if (p.max_hp <= 0 || p.level <= 0) return 0;
    if (p.hp < 0 || p.hp > p.max_hp)   return 0;

    G.player = p;
    G.map_id = map;
    G.daytime = daytime;
    G.last_restock = restock;
    G.items_seen = seen;    /* was written as 32 bits all along (w32) */
    G.flags = flags;
    G.rng = rng ? rng : 0x1D872665u;    /* an all-zero rng never advances */
    G.boons_done = boons;
    for (int m = 0; m < NUM_MAPS; m++)
        G.spawns_gone[m] = gone[m];
    G.species_seen = seen_sp;
    for (int i = 0; i < NUM_SPECIES; i++)
        G.species_kills[i] = kills[i];

    /* Blood is NOT in the blob (see blood_t): a loaded game starts with
     * clean ground. This doubles as the array's init -- a zeroed G would
     * otherwise claim a pool at (0,0) of map 0. */
    blood_clear();

    G.has_save = 1;

    /* WHERE we were called from decides what happens next:
     *   - at boot the game is still on the intro, so we only record that a
     *     save exists; the title screen then offers CONTINUE.
     *   - from the PACK the player just chose LOAD, and they mean NOW: drop
     *     them straight into the loaded game. */
    if (G.state != ST_INTRO && G.state != ST_TITLE)
        overworld_resume();

    return 1;
}

static void toast(const char *text, int good)
{
    G.toast       = text;
    G.toast_good  = good;
    G.toast_ticks = 120;     /* two seconds */
}

int game_save_pending(void)
{
    int p = G.save_pending;
    G.save_pending = 0;      /* one shot: report it once, then forget */
    return p;
}

void game_save_done(int ok)
{
    if (ok)
        G.has_save = 1;      /* LOAD is now selectable, and so is CONTINUE */
    toast(ok ? "SAVED." : "SAVE FAILED", ok);

    /* SAVE AND QUIT: the write has landed, so now -- and ONLY now -- we go.
     * If it FAILED we stay open and say so on the pause panel. Quitting
     * anyway would throw away the very thing they asked us to protect. */
    if (G.quit_after_save) {
        G.quit_after_save = 0;
        if (!ok) {
            G.pause_msg = G.can_quit ? "COULD NOT SAVE. NOT LEAVING."
                                     : "COULD NOT SAVE.";
        } else if (G.can_quit) {
            G.quit_pending = 1;
        } else {
            /* nowhere to go -- this is a browser tab. Saved; carry on. */
            G.state = (state_t)G.pause_from;
            G.t = 0;
        }
    }
}

int game_load_pending(void)
{
    int p = G.load_pending;
    G.load_pending = 0;
    return p;
}

void game_load_done(int ok)
{
    /* On success game_save_load() has already dropped us into the loaded
     * game, so this is only about telling the player what happened. */
    toast(ok ? "LOADED." : "NO SAVE FOUND", ok);
}

/* ---- DISPLAY ---------------------------------------------------------------
 * The whole of the core's involvement with the window. See game.h.
 * G.fullscreen is what the player asked for in OPTIONS; the platform reads
 * it and makes the window match, and pushes the truth back if it changes
 * the mode by some other route (an F11 key, say). */
int game_want_fullscreen(void)
{
    return G.fullscreen;
}

void game_set_fullscreen(int on)
{
    G.fullscreen = on ? 1 : 0;
}

/* ---- QUITTING -------------------------------------------------------------
 * ESC does not close the game. It raises the pause screen, and the player
 * decides. See game.h, and pause_update() in intro.c. */
void game_request_quit(void)
{
    if (G.state == ST_PAUSE)
        return;                 /* already asking; ESC again is handled there */

    G.pause_from = (int)G.state;
    G.pause_sel  = 0;
    G.pause_msg  = 0;
    G.state      = ST_PAUSE;
    G.t          = 0;
    audio_sfx(SFX_BLIP);
}

int game_quit_pending(void)
{
    return G.quit_pending;
}

/* ---- GAMEPADS -------------------------------------------------------------
 * See game.h. The core stores what the player chose and what's plugged in;
 * it never touches the device. */
void game_set_pad(const char *name)
{
    int i = 0;
    if (name)
        for (; name[i] && i < (int)sizeof G.pad_name - 1; i++)
            G.pad_name[i] = name[i];
    G.pad_name[i] = '\0';
}

/* ---- VOLUME ---------------------------------------------------------------
 * The core keeps them as menu notches; audio.c wants 0..255. One place does
 * the conversion, and it is called whenever either changes. */
void audio_apply_volumes(void)
{
    audio_volume(G.music_vol * 255 / VOL_STEPS,
                 G.sfx_vol   * 255 / VOL_STEPS);
}

static int clamp_vol(int v)
{
    if (v < 0) return 0;
    if (v > VOL_STEPS) return VOL_STEPS;
    return v;
}

int  game_music_volume(void)      { return G.music_vol; }
int  game_sfx_volume(void)        { return G.sfx_vol;   }
void game_set_music_volume(int v) { G.music_vol = clamp_vol(v);
                                    audio_apply_volumes(); }
void game_set_sfx_volume(int v)   { G.sfx_vol = clamp_vol(v);
                                    audio_apply_volumes(); }

void game_enable_controls_menu(int on) { G.controls_menu = on ? 1 : 0; }
void game_enable_display_menu(int on)  { G.display_menu  = on ? 1 : 0; }
void game_enable_quit(int on)          { G.can_quit      = on ? 1 : 0; }
int  game_swap_ab(void)                { return G.swap_ab; }
void game_set_swap_ab(int on)          { G.swap_ab = on ? 1 : 0; }
int  game_rumble_enabled(void)         { return G.rumble_on; }
void game_set_rumble(int on)           { G.rumble_on = on ? 1 : 0; }

/* The game asks for a shake; the platform collects it once per frame and
 * the request is cleared. A stronger request in the same frame wins. */
/* THE SCREEN JOLTS. Magnitude decays to zero over the duration, so a hit
 * lands hard and settles. A stronger shake arriving mid-shake wins. */
void shake(int mag, int ticks)
{
    if (mag <= G.shake_mag && G.shake_t > 0)
        return;
    G.shake_mag = mag;
    G.shake_t   = ticks;
    G.shake_len = ticks;
}

/* Where the screen is, this frame. Alternating each tick is what makes it
 * read as a JOLT rather than a drift -- and it uses the frame counter, not
 * the game rng, so a battle still replays identically. */
void shake_offset(int *x, int *y)
{
    if (G.shake_t <= 0 || G.shake_len <= 0) {
        *x = *y = 0;
        return;
    }
    int m = G.shake_mag * G.shake_t / G.shake_len;   /* it decays */
    if (m < 1) m = 1;

    uint32_t n = G.frame * 2654435761u;
    *x = ((G.frame & 1) ? m : -m) * (1 + (int)((n >> 13) & 1)) / 2;
    *y = ((G.frame & 2) ? m : -m) * (1 + (int)((n >> 19) & 1)) / 3;
}

/* The game asks for a shake; the platform collects it once per frame and
 * the request is cleared. A stronger request in the same frame wins. */
void rumble(int strength)
{
    if (!G.rumble_on)
        return;
    if (strength > G.rumble_req)
        G.rumble_req = strength;
}

int game_rumble_take(void)
{
    int r = G.rumble_req;
    G.rumble_req = 0;
    return r;
}
