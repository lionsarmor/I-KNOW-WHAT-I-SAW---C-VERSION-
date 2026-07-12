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
    /* START skips the cinematic -- deliberately, on a button you have to
     * reach for. NOT A: A is the button players mash, and mashing it used to
     * blow through the one scene the whole game is selling itself on. */
    if (PRESSED(BTN_START))
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

    /* Tell them how to skip -- quietly, and only after they've had a moment
     * to be unsettled. A no longer does it (see intro_update), so without
     * this a player who wanted out would just mash and feel trapped. */
    if (G.t > 180) {
        const char *k = "START: SKIP";
        gfx_text_small(SCREEN_W - gfx_text_small_width(k) - 6, SCREEN_H - 9, k,
                       RGB565(70, 70, 78));
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

/* The title menu. CONTINUE only exists if there's a save to continue, so the
 * rows are built at run time rather than hard-coded -- ROW_* are what the
 * chosen index MEANS, which is the only thing the rest of the code cares
 * about. */
enum { ROW_CONTINUE, ROW_NEWGAME, ROW_OPTIONS };

static int title_rows(int *out)
{
    int n = 0;
    if (G.has_save)
        out[n++] = ROW_CONTINUE;
    out[n++] = ROW_NEWGAME;
    out[n++] = ROW_OPTIONS;
    return n;
}

static const char *title_row_label(int row)
{
    switch (row) {
    case ROW_CONTINUE: return "CONTINUE";
    case ROW_NEWGAME:  return "NEW GAME";
    default:           return "OPTIONS";
    }
}

void title_update(void)
{
    int rows[3];
    int n = title_rows(rows);

    if (G.title_sel >= n)
        G.title_sel = 0;              /* a save appeared/vanished under us */

    if (PRESSED(BTN_UP)) {
        G.title_sel = (G.title_sel + n - 1) % n;
        audio_sfx(SFX_BLIP);
    }
    if (PRESSED(BTN_DOWN)) {
        G.title_sel = (G.title_sel + 1) % n;
        audio_sfx(SFX_BLIP);
    }

    if (!PRESSED(BTN_START) && !PRESSED(BTN_A))
        return;

    audio_sfx(SFX_CONFIRM);
    switch (rows[G.title_sel]) {
    case ROW_CONTINUE:
        overworld_resume();          /* the save is already loaded into G */
        return;

    case ROW_OPTIONS:
        options_start(ST_TITLE);
        return;

    default:
        /* reseed rng from how long the player dawdled -> unique run */
        G.rng ^= (G.frame * 2654435761u) | 1u;
        name_start();      /* who are you? then the game begins */
        return;
    }
}

/* ============================ OPTIONS ======================================
 * One screen, and it stays playable on a d-pad and two buttons -- which is
 * all the handheld has.
 *
 *   UP/DOWN     pick a row
 *   LEFT/RIGHT  change it       (A also flips it, for one-button pads)
 *   B / START   back to the title
 *
 * DISPLAY is the only setting so far. The core does NOT touch the window: it
 * writes G.fullscreen, and the platform reads it through
 * game_want_fullscreen() and makes the world match (see game.h).
 */
enum { OPT_DISPLAY, OPT_MUSIC, OPT_SFX, OPT_CONTROLS, OPT_BACK, NUM_OPTS };

/* DISPLAY and CONTROLS only appear on a machine that HAS a resizable window
 * and pads -- the platform declares each (game_enable_display_menu /
 * game_enable_controls_menu). The handheld shows only the two volumes: its
 * screen is 240x160 and always will be, and its buttons are soldered on. A
 * menu row that does nothing when you press it is a lie. */
static int opt_rows(int *out)
{
    int n = 0;
    if (G.display_menu)
        out[n++] = OPT_DISPLAY;
    out[n++] = OPT_MUSIC;
    out[n++] = OPT_SFX;
    if (G.controls_menu)
        out[n++] = OPT_CONTROLS;
    out[n++] = OPT_BACK;
    return n;
}

/* OPTIONS is reachable from the title AND from the pause screen (ESC), and
 * it must go back to whichever one you came from -- if the music is too loud
 * you should not have to quit the game to turn it down. */
void options_start(int from)
{
    G.opt_from = from;
    G.opt_sel  = 0;
    G.state    = ST_OPTIONS;
    G.t        = 0;
}

void options_update(void)
{
    int rows[NUM_OPTS];
    int n = opt_rows(rows);
    if (G.opt_sel >= n)
        G.opt_sel = 0;

    if (PRESSED(BTN_UP)) {
        G.opt_sel = (G.opt_sel + n - 1) % n;
        audio_sfx(SFX_BLIP);
    }
    if (PRESSED(BTN_DOWN)) {
        G.opt_sel = (G.opt_sel + 1) % n;
        audio_sfx(SFX_BLIP);
    }

    int leave = PRESSED(BTN_B) || PRESSED(BTN_START);
    int row   = rows[G.opt_sel];
    int dec   = PRESSED(BTN_LEFT);
    int inc   = PRESSED(BTN_RIGHT);

    switch (row) {
    case OPT_DISPLAY:
        if (dec || inc || PRESSED(BTN_A)) {
            G.fullscreen = !G.fullscreen;   /* the platform picks this up */
            audio_sfx(SFX_CONFIRM);
        }
        break;

    case OPT_MUSIC:
        if (dec || inc) {
            game_set_music_volume(G.music_vol + (inc ? 1 : -1));
            audio_sfx(SFX_BLIP);
        }
        break;

    case OPT_SFX:
        if (dec || inc) {
            game_set_sfx_volume(G.sfx_vol + (inc ? 1 : -1));
            /* play something AT the new level, so you're setting it by ear
             * and not by looking at a bar */
            audio_sfx(SFX_CONFIRM);
        }
        break;

    case OPT_CONTROLS:
        if (PRESSED(BTN_A)) {
            audio_sfx(SFX_CONFIRM);
            G.ctl_sel = 0;
            G.state = ST_CONTROLS;
            G.t = 0;
            return;
        }
        break;

    default:
        if (PRESSED(BTN_A))
            leave = 1;
        break;
    }

    if (leave) {
        audio_sfx(SFX_BLIP);
        G.state = (state_t)(G.opt_from ? G.opt_from : ST_TITLE);
        G.t = 0;
    }
}

/* a 10-notch level meter. Filled notches are bright, empty ones are a dark
 * outline, so the level reads at a glance without a number. */
static void draw_meter(int x, int y, int val, int sel)
{
    for (int i = 0; i < VOL_STEPS; i++) {
        int bx = x + i * 8;
        int h  = 3 + i;                          /* it ramps up, like volume */
        int by = y + 11 - h;
        if (i < val)
            gfx_fill_rect(bx, by, 6, h,
                          sel ? RGB565(255, 200, 90) : RGB565(150, 120, 60));
        else
            gfx_rect(bx, by, 6, h, RGB565(70, 66, 74));
    }
}

/* ============================ CONTROLS =====================================
 * What's plugged in, and the two things about it worth arguing over.
 *
 * SDL's GameController layer already turns an Xbox pad, a Switch Pro pad and
 * a PlayStation pad into the same buttons -- so there is nothing to remap and
 * no per-brand code anywhere in this game. What it CAN'T decide for you is
 * which button you think means "yes": Nintendo and Xbox put A and B in
 * opposite corners, and people are religious about it. Hence SWAP A/B.
 */
enum { CTL_PAD, CTL_SWAP, CTL_RUMBLE, CTL_BACK, NUM_CTL };

void controls_update(void)
{
    if (PRESSED(BTN_UP)) {
        G.ctl_sel = (G.ctl_sel + NUM_CTL - 1) % NUM_CTL;
        audio_sfx(SFX_BLIP);
    }
    if (PRESSED(BTN_DOWN)) {
        G.ctl_sel = (G.ctl_sel + 1) % NUM_CTL;
        audio_sfx(SFX_BLIP);
    }

    int flip = PRESSED(BTN_LEFT) || PRESSED(BTN_RIGHT) || PRESSED(BTN_A);
    int leave = PRESSED(BTN_B) || PRESSED(BTN_START);

    switch (G.ctl_sel) {
    case CTL_SWAP:
        if (flip) {
            G.swap_ab = !G.swap_ab;      /* the platform reads this when it
                                            maps the pad's buttons */
            audio_sfx(SFX_CONFIRM);
        }
        break;
    case CTL_RUMBLE:
        if (flip) {
            G.rumble_on = !G.rumble_on;
            audio_sfx(SFX_CONFIRM);
            if (G.rumble_on)
                rumble(200);             /* so you can feel what you turned on */
        }
        break;
    case CTL_BACK:
        if (PRESSED(BTN_A))
            leave = 1;
        break;
    default:
        break;                           /* CTL_PAD is just a readout */
    }

    if (leave) {
        audio_sfx(SFX_BLIP);
        G.opt_sel = 0;
        G.state = ST_OPTIONS;
        G.t = 0;
    }
}

void controls_render(void)
{
    gfx_clear(0);
    gfx_blit_ex(face_big, FACE_W, FACE_H, (SCREEN_W - FACE_W * 4) / 2, 8,
                4, 40, 0);

    const char *t = "CONTROLS";
    gfx_text_ex((SCREEN_W - gfx_text_width(t, 2)) / 2, 16, t,
                RGB565(216, 40, 32), 2);

    uint16_t on   = RGB565(255, 255, 255);
    uint16_t off  = RGB565(120, 120, 128);
    uint16_t val  = RGB565(255, 200, 90);

    static const char *label[NUM_CTL] = { "GAMEPAD", "SWAP A/B", "RUMBLE",
                                          "BACK" };
    for (int i = 0; i < NUM_CTL; i++) {
        int y = 54 + i * 18;
        int sel = (i == G.ctl_sel);
        gfx_text(24, y, label[i], sel ? on : off);
        if (sel)
            gfx_cursor(12, y, G.frame);

        const char *v = 0;
        if (i == CTL_PAD)
            v = G.pad_name[0] ? G.pad_name : "NONE - KEYBOARD";
        else if (i == CTL_SWAP)
            v = G.swap_ab ? "ON" : "OFF";
        else if (i == CTL_RUMBLE)
            v = G.rumble_on ? "ON" : "OFF";
        if (!v)
            continue;

        /* the pad's name can be long, so it gets the small font */
        if (i == CTL_PAD) {
            int vx = SCREEN_W - 16 - gfx_text_small_width(v);
            gfx_text_small_outlined(vx, y + 2, v,
                                    G.pad_name[0] ? val : off);
        } else {
            int vx = SCREEN_W - 16 - gfx_text_width(v, 1);
            gfx_text(vx, y, v, sel ? val : off);
        }
    }

    const char *hint = G.pad_name[0]
        ? "LEFT/RIGHT CHANGE   B: BACK"
        : "PLUG IN A PAD - IT IS FOUND AUTOMATICALLY";
    gfx_text_small_outlined(
        (SCREEN_W - gfx_text_small_width(hint)) / 2, 142, hint,
        RGB565(150, 150, 160));

    draw_static(10, 110);
}

void options_render(void)
{
    gfx_clear(0);

    /* the face is still back there, watching you fiddle with the settings */
    gfx_blit_ex(face_big, FACE_W, FACE_H, (SCREEN_W - FACE_W * 4) / 2, 8,
                4, 40, 0);

    const char *title = "OPTIONS";
    gfx_text_ex((SCREEN_W - gfx_text_width(title, 2)) / 2, 20, title,
                RGB565(216, 40, 32), 2);

    uint16_t on  = RGB565(255, 255, 255);
    uint16_t off = RGB565(120, 120, 128);

    int rows[NUM_OPTS];
    int n = opt_rows(rows);

    for (int i = 0; i < n; i++) {
        int y   = 46 + i * 18;
        int sel = (i == G.opt_sel);

        static const char *label[NUM_OPTS] = {
            "DISPLAY", "MUSIC", "SOUND", "CONTROLS", "BACK"
        };
        gfx_text(20, y, label[rows[i]], sel ? on : off);
        if (sel)
            gfx_cursor(8, y, G.frame);

        if (rows[i] == OPT_DISPLAY) {
            const char *val = G.fullscreen ? "FULLSCREEN" : "WINDOW";
            int vx = SCREEN_W - 18 - gfx_text_width(val, 1);
            gfx_text(vx, y, val, sel ? RGB565(255, 200, 90) : off);
            if (sel) {
                gfx_text(vx - 10, y, "-", on);
                gfx_text(vx + gfx_text_width(val, 1) + 2, y, "-", on);
            }
        } else if (rows[i] == OPT_MUSIC) {
            draw_meter(SCREEN_W - 18 - VOL_STEPS * 8, y - 2, G.music_vol, sel);
        } else if (rows[i] == OPT_SFX) {
            draw_meter(SCREEN_W - 18 - VOL_STEPS * 8, y - 2, G.sfx_vol, sel);
        }
    }

    const char *hint = "LEFT/RIGHT CHANGE   B: BACK";
    gfx_text_small_outlined(
        (SCREEN_W - gfx_text_small_width(hint)) / 2, 146, hint,
        RGB565(150, 150, 160));

    draw_static(10, 110);
}

/* ============================ THE PAUSE SCREEN =============================
 * ESC. The world stops but it does not go away -- it's still there behind
 * the panel, dimmed and crawling with static, and it is still watching you.
 *
 *   SAVE AND QUIT   writes the save, and leaves only once it has LANDED
 *   QUIT            leaves now, and loses everything since the last save
 *   CANCEL          back to exactly where you were
 *
 * SAVE AND QUIT is greyed out anywhere the save wouldn't mean anything --
 * the title, the intro, a cutscene, mid-battle.
 */
enum { PZ_SAVEQUIT, PZ_OPTIONS, PZ_QUIT, PZ_CANCEL, NUM_PZ };

/* QUIT only exists on a machine you can actually leave. In a browser tab you
 * cannot close the page from inside it, so that row is dropped and SAVE AND
 * QUIT becomes plain SAVE GAME (it saves and hands you back to the game). */
static int pause_rows(int *out)
{
    int n = 0;
    out[n++] = PZ_SAVEQUIT;
    out[n++] = PZ_OPTIONS;
    if (G.can_quit)
        out[n++] = PZ_QUIT;
    out[n++] = PZ_CANCEL;
    return n;
}

/* Is there a game worth saving right now? The save blob records the world
 * and the player, not a menu or a battle turn -- so these are the moments
 * where writing it and reloading it would put you back where you were. */
static int pause_can_save(void)
{
    return G.pause_from == ST_OVERWORLD ||
           G.pause_from == ST_DIALOG    ||
           G.pause_from == ST_PACK;
}

static void pause_leave(void)
{
    G.state = (state_t)G.pause_from;
    G.t     = 0;
}

void pause_update(void)
{
    /* while a SAVE AND QUIT is in flight, the menu is not interactive --
     * the platform is writing bytes and will call game_save_done() */
    if (G.quit_after_save || G.quit_pending)
        return;

    int rows[NUM_PZ];
    int n = pause_rows(rows);
    if (G.pause_sel >= n)
        G.pause_sel = 0;

    if (PRESSED(BTN_UP)) {
        G.pause_sel = (G.pause_sel + n - 1) % n;
        audio_sfx(SFX_BLIP);
    }
    if (PRESSED(BTN_DOWN)) {
        G.pause_sel = (G.pause_sel + 1) % n;
        audio_sfx(SFX_BLIP);
    }

    /* B backs out, same as CANCEL -- ESC again does too, via the platform
     * calling game_request_quit() while we're already here (it no-ops), so
     * B and START are the ways back. */
    if (PRESSED(BTN_B) || PRESSED(BTN_START)) {
        audio_sfx(SFX_BLIP);
        pause_leave();
        return;
    }

    if (!PRESSED(BTN_A))
        return;

    switch (rows[G.pause_sel]) {
    case PZ_OPTIONS:
        /* ...and it comes BACK here when you're done, not to the title. */
        audio_sfx(SFX_CONFIRM);
        options_start(ST_PAUSE);
        return;

    case PZ_SAVEQUIT:
        if (!pause_can_save()) {
            audio_sfx(SFX_BLIP);        /* nothing here worth writing */
            return;
        }
        /* Raise the ordinary save flag. The platform writes the bytes and
         * calls game_save_done(), which is where we actually leave. */
        G.save_pending    = 1;
        G.quit_after_save = 1;
        G.pause_msg       = "SAVING...";
        audio_sfx(SFX_CONFIRM);
        return;

    case PZ_QUIT:
        audio_sfx(SFX_CONFIRM);
        G.quit_pending = 1;
        return;

    default:
        audio_sfx(SFX_BLIP);
        pause_leave();
        return;
    }
}

/* The world, still there, dimmed under scanlines and a vignette. This is
 * what makes it feel like a pause and not a screen change: you can still
 * see where you were standing. */
static void pause_veil(void)
{
    for (int y = 0; y < SCREEN_H; y++) {
        /* every other line is darker -- a CRT that isn't coping */
        int base = (y & 1) ? 58 : 96;
        for (int x = 0; x < SCREEN_W; x++) {
            /* vignette: the corners fall away into nothing */
            int dx = x - SCREEN_W / 2, dy = y - SCREEN_H / 2;
            int vig = 256 - (dx * dx + dy * dy) / 90;
            if (vig < 70) vig = 70;

            uint16_t *p = &gfx_fb[y * SCREEN_W + x];
            *p = gfx_dim(*p, base * vig / 256);
        }
    }
}

void pause_render(void)
{
    /* the frozen world, drawn but NOT advanced (see render_scene) */
    render_scene(G.pause_from);
    pause_veil();
    draw_static(70, 150);            /* and the signal is getting worse */

    /* ---- the panel ------------------------------------------------------ */
    const int pw = 172, ph = 106;
    const int px = (SCREEN_W - pw) / 2, py = 22;

    gfx_fill_rect(px, py, pw, ph, RGB565(10, 8, 12));

    /* a border that flickers like the title's dying neon */
    uint16_t edge = (G.frame % 190 < 3) ? RGB565(90, 16, 12)
                                        : RGB565(200, 36, 30);
    gfx_rect(px, py, pw, ph, edge);
    gfx_rect(px + 2, py + 2, pw - 4, ph - 4, RGB565(40, 10, 10));

    /* corner ticks -- the frame of something being LOOKED AT */
    for (int i = 0; i < 2; i++) {
        int cx = i ? px + pw - 7 : px + 1;
        for (int j = 0; j < 2; j++) {
            int cy = j ? py + ph - 7 : py + 1;
            gfx_fill_rect(cx, cy, 6, 1, RGB565(255, 220, 200));
            gfx_fill_rect(i ? cx + 5 : cx, cy, 1, 6, RGB565(255, 220, 200));
        }
    }

    /* ---- the title, and the seam under it -------------------------------- */
    const char *ttl = "PAUSED";
    gfx_text((SCREEN_W - gfx_text_width(ttl, 1)) / 2, py + 10, ttl,
             RGB565(230, 220, 220));
    gfx_fill_rect(px + 14, py + 22, pw - 28, 1, RGB565(70, 18, 16));

    /* ---- the rows -------------------------------------------------------- */
    int rows[NUM_PZ];
    int n = pause_rows(rows);
    int savable = pause_can_save();
    int busy    = G.quit_after_save || G.quit_pending;

    for (int i = 0; i < n; i++) {
        int r = rows[i];
        const char *label =
            (r == PZ_SAVEQUIT) ? (G.can_quit ? "SAVE AND QUIT" : "SAVE GAME")
          : (r == PZ_OPTIONS)  ? "OPTIONS"
          : (r == PZ_QUIT)     ? "QUIT"
                               : "CANCEL";

        int y = py + 32 + i * 14;
        int dead = (r == PZ_SAVEQUIT && !savable) || busy;
        uint16_t col = dead               ? RGB565(80, 76, 84)
                     : (i == G.pause_sel) ? RGB565(255, 255, 255)
                                          : RGB565(140, 138, 146);
        /* QUIT is the one that costs you something. It reads hot. */
        if (!dead && r == PZ_QUIT && i == G.pause_sel)
            col = RGB565(255, 120, 90);

        gfx_text(px + 30, y, label, col);
        if (i == G.pause_sel && !busy)
            gfx_cursor(px + 18, y, G.frame);
    }

    /* ---- the footer: a warning, or whatever just went wrong -------------- */
    int sel_row = rows[G.pause_sel < n ? G.pause_sel : 0];
    const char *foot = G.pause_msg;
    if (!foot)
        foot = (sel_row == PZ_QUIT && !busy)
             ? "UNSAVED PROGRESS WILL BE LOST"
             : (!savable && sel_row == PZ_SAVEQUIT)
             ? "NOTHING TO SAVE YET"
             : "";
    if (foot[0])
        gfx_text_small_outlined((SCREEN_W - gfx_text_small_width(foot)) / 2,
                                py + ph - 12, foot, RGB565(210, 90, 70));
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

    /* the menu: CONTINUE (only if there's a save), NEW GAME, OPTIONS.
     * Bottom-aligned so it doesn't jump when CONTINUE appears. */
    int rows[3];
    int n = title_rows(rows);
    int top = 132 - n * 14;
    for (int i = 0; i < n; i++) {
        const char *label = title_row_label(rows[i]);
        int ox = (SCREEN_W - gfx_text_width(label, 1)) / 2;
        int oy = top + i * 14;
        gfx_text(ox, oy, label,
                 (i == G.title_sel) ? RGB565(255, 255, 255)
                                    : RGB565(120, 120, 128));
        if (i == G.title_sel)
            gfx_cursor(ox - 12, oy, G.frame);
    }

    /* controls hint -- update this if you change the platform mappings */
    const char *hint = "Z:A  X:B  ENTER:START";
    gfx_text((SCREEN_W - gfx_text_width(hint, 1)) / 2, 146, hint,
             RGB565(90, 90, 90));

    draw_static(12, 120);   /* the static never fully goes away */
}
