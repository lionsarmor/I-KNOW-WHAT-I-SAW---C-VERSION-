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

    audio_music(MUSIC_INTRO);   /* the heartbeat starts immediately */
    return asset_errors;
}

void game_update(uint16_t buttons_held)
{
    /* edge detection: pressed = down now, up last tick */
    G.pressed = (uint16_t)(buttons_held & ~G.held);
    G.held    = buttons_held;

    switch (G.state) {
    case ST_INTRO:     intro_update();     intro_render();     break;
    case ST_TITLE:     title_update();     title_render();     break;
    case ST_OVERWORLD: overworld_update(); overworld_render(); break;
    case ST_NAME:      name_update();      name_render();      break;
    case ST_DIALOG:    dialog_update();    dialog_render();    break;
    case ST_PACK:      pack_update();      pack_render();      break;
    case ST_SLEEP:     sleep_update();     sleep_render();     break;
    case ST_BATTLE:    battle_update();    battle_render();    break;
    case ST_GAMEOVER:  gameover_update();  gameover_render();  break;
    }

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
#define SAVE_VERSION 3             /* v2 last_restock, v3 the player's name */

/* Exactly how many bytes game_save_write() lays down. Kept next to the
 * writer so the two can't drift apart, and checked at COMPILE TIME below:
 * add a couple of items or maps without noticing and the blob would
 * quietly run off the end of the platform's buffer. */
#define SAVE_BYTES (4 /*magic*/ + 4 /*version*/                              \
                  + 8 * 4 /*x y dir hp max level xp lamp*/                   \
                  + NUM_ITEMS * 4                                            \
                  + (PLAYER_NAME_MAX + 1)                                           \
                  + 4 /*map*/ + 4 /*daytime*/ + 4 /*restock*/                \
                  + 4 /*seen*/ + 4 /*rng*/                                   \
                  + NUM_MAPS * 4                                             \
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
    for (int i = 0; i < NUM_ITEMS; i++)
        w32(&c, (uint32_t)G.player.items[i]);
    for (int i = 0; i < PLAYER_NAME_MAX + 1; i++)      /* your name, one byte each */
        buf[c.n++] = (uint8_t)G.player.name[i];

    w32(&c, (uint32_t)G.map_id);
    w32(&c, G.daytime);
    w32(&c, G.last_restock);
    w32(&c, (uint32_t)G.items_seen);
    w32(&c, G.rng);
    for (int m = 0; m < NUM_MAPS; m++)
        w32(&c, (uint32_t)G.spawns_gone[m]);

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
    for (int i = 0; i < NUM_ITEMS; i++)
        p.items[i] = (int)r32(&c);
    for (int i = 0; i < PLAYER_NAME_MAX + 1; i++)
        p.name[i] = (char)buf[c.n++];
    p.name[PLAYER_NAME_MAX] = '\0';                    /* never trust the blob */

    int      map     = (int)r32(&c);
    uint32_t daytime = r32(&c);
    uint32_t restock = r32(&c);
    uint16_t seen    = (uint16_t)r32(&c);
    uint32_t rng     = r32(&c);
    uint16_t gone[NUM_MAPS];
    for (int m = 0; m < NUM_MAPS; m++)
        gone[m] = (uint16_t)r32(&c);

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
    G.items_seen = seen;
    G.rng = rng ? rng : 0x1D872665u;    /* an all-zero rng never advances */
    for (int m = 0; m < NUM_MAPS; m++)
        G.spawns_gone[m] = gone[m];

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
