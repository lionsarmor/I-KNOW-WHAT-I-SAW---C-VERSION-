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
    case ST_DIALOG:    dialog_update();    dialog_render();    break;
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
