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
    /* With no save on disk there's nothing to choose: START just plays.
     * With one, you get CONTINUE / NEW GAME. */
    if (G.has_save && (PRESSED(BTN_UP) || PRESSED(BTN_DOWN))) {
        G.title_sel ^= 1;
        audio_sfx(SFX_BLIP);
    }

    if (!PRESSED(BTN_START) && !PRESSED(BTN_A))
        return;

    audio_sfx(SFX_CONFIRM);
    if (G.has_save && G.title_sel == 0) {
        overworld_resume();          /* the save is already loaded into G */
        return;
    }
    /* reseed rng from how long the player dawdled -> unique run */
    G.rng ^= (G.frame * 2654435761u) | 1u;
    name_start();          /* who are you? then the game begins */
}

/* ============================ THE NAME SCREEN ==============================
 * A grid of letters with a little UFO hovering over the one you're on.
 *
 *   dpad   move the saucer
 *   A      beam that letter down into your name
 *   B      backspace
 *   START  done  (an empty name defaults to PLAYER)
 *
 * The grid is 7 across; the last two cells are BACK and DONE so the whole
 * thing is playable on a d-pad and two buttons -- which is all the handheld
 * has.
 */
#define NAME_COLS 7
#define NAME_CELL_W 24
#define NAME_CELL_H 18

/* 26 letters + a space, then BACK and DONE = 29 cells (5 rows of 7, the
 * last row short). Cell content is looked up here. */
#define NAME_CELLS 29
#define CELL_BACK 27
#define CELL_DONE 28

static char name_cell_char(int i)
{
    if (i < 26) return (char)('A' + i);
    if (i == 26) return ' ';       /* a space, for two-word names */
    return 0;                      /* BACK / DONE aren't letters   */
}

void name_start(void)
{
    G.player.name[0] = '\0';
    G.name_len = 0;
    G.name_sel = 0;
    G.state = ST_NAME;
    G.t = 0;
}

static void name_accept(void)
{
    if (G.name_len == 0) {
        /* nobody typed anything: you're PLAYER, and that's fine */
        const char *d = NAME_DEFAULT;
        int i = 0;
        for (; d[i] && i < PLAYER_NAME_MAX; i++)
            G.player.name[i] = d[i];
        G.player.name[i] = '\0';
    }
    audio_sfx(SFX_CONFIRM);
    overworld_start_game();
}

void name_update(void)
{
    int col = G.name_sel % NAME_COLS;
    int row = G.name_sel / NAME_COLS;

    if (PRESSED(BTN_LEFT))  { col--; audio_sfx(SFX_BLIP); }
    if (PRESSED(BTN_RIGHT)) { col++; audio_sfx(SFX_BLIP); }
    if (PRESSED(BTN_UP))    { row--; audio_sfx(SFX_BLIP); }
    if (PRESSED(BTN_DOWN))  { row++; audio_sfx(SFX_BLIP); }

    int rows = (NAME_CELLS + NAME_COLS - 1) / NAME_COLS;
    if (col < 0) col = NAME_COLS - 1;
    if (col >= NAME_COLS) col = 0;
    if (row < 0) row = rows - 1;
    if (row >= rows) row = 0;

    int sel = row * NAME_COLS + col;
    if (sel >= NAME_CELLS)          /* the short last row: clamp into it */
        sel = NAME_CELLS - 1;
    G.name_sel = sel;

    if (PRESSED(BTN_START)) { name_accept(); return; }

    if (PRESSED(BTN_B)) {           /* backspace, wherever you are */
        if (G.name_len > 0)
            G.player.name[--G.name_len] = '\0';
        audio_sfx(SFX_BLIP);
        return;
    }

    if (!PRESSED(BTN_A))
        return;

    if (G.name_sel == CELL_DONE) { name_accept(); return; }

    if (G.name_sel == CELL_BACK) {
        if (G.name_len > 0)
            G.player.name[--G.name_len] = '\0';
        audio_sfx(SFX_BLIP);
        return;
    }

    if (G.name_len < PLAYER_NAME_MAX) {
        G.player.name[G.name_len++] = name_cell_char(G.name_sel);
        G.player.name[G.name_len] = '\0';
        audio_sfx(SFX_PICKUP);      /* the little abduction chime */
    } else {
        audio_sfx(SFX_BLIP);        /* full up */
    }
}

void name_render(void)
{
    gfx_clear(RGB565(10, 10, 24));

    const char *title = "WHAT DO THEY CALL YOU?";
    gfx_text((SCREEN_W - gfx_text_width(title, 1)) / 2, 8, title,
             RGB565(200, 200, 210));

    /* the name so far, in a lit box */
    char shown[PLAYER_NAME_MAX + 2];
    int n = 0;
    for (; n < G.name_len; n++)
        shown[n] = G.player.name[n];
    /* a blinking underscore where the next letter lands */
    if (G.name_len < PLAYER_NAME_MAX && (G.frame % 60) < 30)
        shown[n++] = '-';
    shown[n] = '\0';

    int bw = PLAYER_NAME_MAX * 16 + 12;
    gfx_fill_rect((SCREEN_W - bw) / 2, 20, bw, 22, RGB565(20, 20, 34));
    gfx_rect     ((SCREEN_W - bw) / 2, 20, bw, 22, RGB565(120, 120, 140));
    if (G.name_len == 0 && (G.frame % 60) >= 30) {
        /* hint at the default while they dither */
        const char *d = NAME_DEFAULT;
        gfx_text_ex((SCREEN_W - gfx_text_width(d, 2)) / 2, 24, d,
                    RGB565(70, 70, 86), 2);
    } else {
        gfx_text_ex((SCREEN_W - gfx_text_width(shown, 2)) / 2, 24, shown,
                    RGB565(255, 255, 255), 2);
    }

    /* the letter grid */
    int gx = (SCREEN_W - NAME_COLS * NAME_CELL_W) / 2 + 4;
    int gy = 52;
    for (int i = 0; i < NAME_CELLS; i++) {
        int cx = gx + (i % NAME_COLS) * NAME_CELL_W;
        int cy = gy + (i / NAME_COLS) * NAME_CELL_H;
        uint16_t col = (i == G.name_sel) ? RGB565(255, 255, 255)
                                         : RGB565(150, 150, 165);
        if (i == CELL_BACK) {
            gfx_text(cx - 4, cy, "DEL", col);
        } else if (i == CELL_DONE) {
            gfx_text(cx - 2, cy, "OK", (i == G.name_sel)
                                        ? RGB565(255, 236, 150)
                                        : RGB565(190, 170, 110));
        } else {
            char c[2] = { name_cell_char(i), '\0' };
            if (c[0] == ' ') {          /* draw the space cell as a bar */
                gfx_fill_rect(cx + 1, cy + 6, 8, 2, col);
            } else {
                gfx_text(cx + 1, cy, c, col);
            }
        }
    }

    /* THE SAUCER, hovering over the chosen cell (it bobs) */
    int sx = gx + (G.name_sel % NAME_COLS) * NAME_CELL_W - 4;
    int sy = gy + (G.name_sel / NAME_COLS) * NAME_CELL_H - 13
           + (int)((G.frame / 10) % 2);
    int ufo = ((G.frame / 8) % 2) ? SPR_UFO_1 : SPR_UFO;
    gfx_blit(sprites[ufo].px, TILE, TILE, sx, sy);

    const char *help = "A PICK   B DEL   START DONE";
    gfx_text((SCREEN_W - gfx_text_width(help, 1)) / 2, SCREEN_H - 12, help,
             RGB565(110, 110, 125));
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

    if (G.has_save) {
        /* someone has been here before */
        static const char *opt[2] = { "CONTINUE", "NEW GAME" };
        for (int i = 0; i < 2; i++) {
            int ox = (SCREEN_W - gfx_text_width(opt[i], 1)) / 2;
            gfx_text(ox, 106 + i * 14, opt[i],
                     (i == G.title_sel) ? RGB565(255, 255, 255)
                                        : RGB565(120, 120, 128));
            if (i == G.title_sel)
                gfx_cursor(ox - 12, 106 + i * 14, G.frame);
        }
    } else if (G.t % 60 < 40) {
        /* blinking prompt (on 40 ticks, off 20) */
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
