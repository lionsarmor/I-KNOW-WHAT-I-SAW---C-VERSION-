/* ============================================================================
 * intro.c -- the opening cinematic and the title screen.
 *
 * Timeline (in ticks, 60/sec -- tweak the PHASE_* constants freely):
 *
 *   0 ......... black. a slow heartbeat. TV static at the edges of
 *               vision. a date types itself out, like a case file.
 *   TEXT2 ..... second caption line.
 *   LIGHT ..... a light appears high in the sky. it descends. it is
 *               not a plane. it is not swamp gas.
 *   FACE ...... something is in the dark. it gets brighter SO slowly,
 *               and the closer it gets the harder the picture shakes.
 *               words you almost don't catch flash in the static.
 *   LUNGE ..... it is suddenly MUCH closer.
 *   BAM ....... white flash. the STING hits. TITLE SCREEN.
 *
 * Press START at any point to skip to the title.
 *
 * The scare toolkit here (steal these for your own scenes): brightness
 * ramps that never reach full, motion that accelerates, things that
 * vanish for a frame, single-frame subliminal text, and one (1) lunge.
 * More than one lunge and it stops being scary.
 * ==========================================================================*/
#include "game_internal.h"

/* --- timeline (edit me) ---------------------------------------------------*/
#define PHASE_TEXT2  150   /* second caption appears        */
#define PHASE_LIGHT  270   /* the light in the sky          */
#define PHASE_FACE   420   /* the face starts to fade in    */
#define PHASE_LUNGE  760   /* it lunges                     */
#define PHASE_BAM    790   /* flash + title                 */

static const char *CAPTION_1 = "OCTOBER 3, 1987.";
static const char *CAPTION_2 = "THE NIGHT THE LIGHTS\nCAME TO THE FARM.";

/* the words in the static: 4 ticks each, gone before you're sure
 * you read them */
static const struct { uint32_t at; const char *text; } WHISPERS[] = {
    { PHASE_FACE +  90, "IT SEES YOU" },
    { PHASE_FACE + 170, "DON'T LOOK UP" },
    { PHASE_FACE + 250, "THEY COME BACK" },
    { PHASE_FACE + 310, "RUN" },
};
#define NUM_WHISPERS (sizeof WHISPERS / sizeof WHISPERS[0])

/* cheap deterministic "static" noise -- NOT the game rng, so the intro
 * doesn't change how battles roll */
static uint32_t noise_seed = 0xBADC0DEu;
static uint32_t noise(void)
{
    noise_seed ^= noise_seed << 13;
    noise_seed ^= noise_seed >> 17;
    noise_seed ^= noise_seed << 5;
    return noise_seed;
}

static void draw_static(int density /* pixels per frame */, int bright)
{
    for (int i = 0; i < density; i++) {
        uint32_t r = noise();
        int x = (int)(r % SCREEN_W);
        int y = (int)((r >> 9) % SCREEN_H);
        uint8_t v = (uint8_t)(r >> 20);
        gfx_pixel(x, y, gfx_dim(RGB565(v, v, v), bright));
    }
}

/* typewriter: reveal one character every 3 ticks, starting at t0 */
static void draw_typed(const char *s, int x, int y, uint32_t t0, uint16_t col)
{
    if (G.t < t0)
        return;
    int reveal = (int)((G.t - t0) / 3);
    char buf[64];
    int i = 0;
    for (; s[i] && i < reveal && i < 63; i++)
        buf[i] = s[i];
    buf[i] = '\0';
    gfx_text(x, y, buf, col);
}

/* a soft glowing blob: bright core, two dimmer halos */
static void draw_glow(int x, int y, int r, uint16_t col)
{
    gfx_fill_rect(x - r * 2, y - r,     r * 4, r * 2, gfx_dim(col, 60));
    gfx_fill_rect(x - r,     y - r / 2, r * 2, r,     gfx_dim(col, 140));
    gfx_fill_rect(x - r / 2, y - r / 4, r,     r / 2, col);
}

static void goto_title(void)
{
    audio_sfx(SFX_STING);
    audio_music(MUSIC_TITLE);
    G.state = ST_TITLE;
    G.t = 0;
}

void intro_update(void)
{
    if (PRESSED(BTN_START) || PRESSED(BTN_A))   /* skip */
        goto_title();
    else if (G.t == PHASE_BAM)
        goto_title();
}

void intro_render(void)
{
    gfx_clear(0);

    /* --- the light in the sky, before the face ------------------------- */
    if (G.t >= PHASE_LIGHT && G.t < PHASE_FACE + 40) {
        uint32_t lt = G.t - PHASE_LIGHT;
        /* it descends and drifts left, pulsing. the pulse quickens. */
        int x = SCREEN_W - 50 - (int)(lt / 6);
        int y = 16 + (int)(lt / 10);
        int pulse = (int)((lt / (10 - (lt > 100 ? 6 : 0))) % 2);
        draw_glow(x, y, 3 + pulse, RGB565(200, 220, 255));
        /* and once, only once, a second light answers it */
        if (lt > 90 && lt < 118)
            draw_glow(x - 70, y + 24, 2, RGB565(255, 200, 150));
    }

    /* --- the face, barely there ----------------------------------------- */
    if (G.t >= PHASE_FACE) {
        uint32_t ft = G.t - PHASE_FACE;
        int lunging = G.t >= PHASE_LUNGE;

        /* brightness creeps from 0 up to ~45% and no further --
         * you never get a good look at it */
        int bright = (int)(ft * 115 / (PHASE_BAM - PHASE_FACE));

        /* it flickers: some frames it simply isn't there */
        int gone = ((noise() & 31) == 0) && !lunging;

        /* the shake grows as it brightens... */
        int amp = 1 + (int)(ft / 120);
        int jx = (int)(noise() % (uint32_t)(2 * amp + 1)) - amp;
        int jy = (int)(noise() % (uint32_t)(2 * amp + 1)) - amp;

        /* ...and in the last half-second it LUNGES */
        int scale = 4;
        if (lunging) {
            scale  = 5;
            bright = 150;
            jx *= 3;
            jy *= 3;
        }

        int x = (SCREEN_W - FACE_W * scale) / 2 + jx;
        int y = (SCREEN_H - FACE_H * scale) / 2 + jy;

        if (!gone) {
            gfx_blit_ex(face_big, FACE_W, FACE_H, x, y, scale, bright, 0);

            /* after a while the eyes begin to burn on their own,
             * pulsing brighter than the face around them */
            if (ft > 120) {
                int pulse = 120 + (int)((ft / 4) % 60);
                uint16_t glow = gfx_dim(RGB565(255, 40, 30), pulse);
                /* eye glint positions in face-art pixels, scaled */
                gfx_fill_rect(x + 11 * scale, y + 12 * scale,
                              scale, scale, glow);
                gfx_fill_rect(x + 20 * scale, y + 12 * scale,
                              scale, scale, glow);
            }
        }
    }

    /* --- static & captions over the top --------------------------------- */
    draw_static(G.t < PHASE_FACE ? 60 : 30,
                G.t >= PHASE_LUNGE ? 220 : 140);

    if (G.t < PHASE_FACE + 60) {
        draw_typed(CAPTION_1, 24, 28, 20,          RGB565(150, 150, 150));
        draw_typed(CAPTION_2, 24, 44, PHASE_TEXT2, RGB565(150, 150, 150));
    }

    /* --- the words you almost read --------------------------------------*/
    for (unsigned i = 0; i < NUM_WHISPERS; i++)
        if (G.t >= WHISPERS[i].at && G.t < WHISPERS[i].at + 4) {
            const char *w = WHISPERS[i].text;
            /* jittered off-center placement, dim dried-blood red */
            int wx = (SCREEN_W - gfx_text_width(w, 1)) / 2
                     + (int)(noise() % 31) - 15;
            int wy = 30 + (int)(noise() % 100);
            gfx_text(wx, wy, w, RGB565(120, 20, 16));
        }
}

/* ============================ TITLE SCREEN =================================*/

void title_update(void)
{
    if (PRESSED(BTN_START)) {
        audio_sfx(SFX_CONFIRM);
        /* reseed rng from how long the player dawdled -> unique run */
        G.rng ^= (G.frame * 2654435761u) | 1u;
        overworld_start_game();
    }
}

void title_render(void)
{
    /* white flash for the first few ticks -- the BAM */
    if (G.t < 5) {
        gfx_clear(RGB565(255, 255, 255));
        return;
    }

    gfx_clear(0);

    /* the face looms behind the logo. mostly steady. mostly. */
    int fx = (SCREEN_W - FACE_W * 4) / 2;
    int fy = 8;
    int fb = 70;
    if (G.t % 197 < 2) { fx += 2; fb = 95; }     /* it moves. did it move? */
    gfx_blit_ex(face_big, FACE_W, FACE_H, fx, fy, 4, fb, 0);

    /* THE TITLE (it flickers like a dying neon sign every few seconds) */
    const char *l1 = "I KNOW";
    const char *l2 = "WHAT I SAW";
    uint16_t red = (G.t % 240 < 3) ? RGB565(90, 16, 12)
                                   : RGB565(216, 40, 32);
    gfx_text_ex((SCREEN_W - gfx_text_width(l1, 2)) / 2, 44, l1, red, 2);
    gfx_text_ex((SCREEN_W - gfx_text_width(l2, 2)) / 2, 64, l2, red, 2);

    /* blinking prompt (on 40 ticks, off 20) */
    if (G.t % 60 < 40) {
        const char *p = "PRESS START!";
        gfx_text((SCREEN_W - gfx_text_width(p, 1)) / 2, 108, p,
                 RGB565(255, 255, 255));
    }

    /* controls hint -- update this if you change the platform mappings */
    const char *hint = "Z:A  X:B  ENTER:START";
    gfx_text((SCREEN_W - gfx_text_width(hint, 1)) / 2, 146, hint,
             RGB565(90, 90, 90));

    draw_static(12, 120);   /* the static never fully goes away */
}
