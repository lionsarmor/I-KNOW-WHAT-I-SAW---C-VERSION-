/* ============================================================================
 * cutscene.c -- the end of the prologue.
 *
 *   ST_DRIVE     the highway at night. Forty minutes to the police station.
 *                You do not get there.
 *   ST_PROLOGUE  the BAM, the title, END OF PROLOGUE, and the menu.
 *
 * TIMELINE (drive_t, in ticks -- 60/sec. Tweak the DRIVE_* constants):
 *
 *   0 ........... engine turns over. road scrolling, stars, radio hiss.
 *   LIGHT ....... one light in the mirror. Just a light. Could be anything.
 *   FOLLOW ...... it's still there. It's bigger. It is not a plane.
 *   SWOOP ....... it comes down, and it comes down FAST.
 *   BEAM ........ the road goes white underneath you.
 *   LIFT ........ the van leaves the ground.
 *   BAM ......... white. Title. END OF PROLOGUE.
 *
 * The whole scare is that it takes its time. Don't shorten the middle.
 * ==========================================================================*/
#include "game_internal.h"

#define DRIVE_LIGHT   150
#define DRIVE_FOLLOW  330
#define DRIVE_SWOOP   540
#define DRIVE_BEAM    690
#define DRIVE_LIFT    780
#define DRIVE_BAM     930

/* the horizon: everything above is sky, below is road */
#define HORIZON 70

/* How far the van climbs before the white. Tuned against the saucer's final
 * position so the two never overlap -- see the note in drive_render(). */
#define LIFT_PX 34

/* deterministic sparkle for the stars -- not the game rng */
static uint32_t sn = 0x51ED270Bu;
static uint32_t snoise(void)
{
    sn ^= sn << 13; sn ^= sn >> 17; sn ^= sn << 5;
    return sn;
}

void drive_start(void)
{
    G.drive_t = 0;
    G.state   = ST_DRIVE;
    G.t       = 0;
    audio_music(MUSIC_DRIVE);
}

void drive_update(void)
{
    G.drive_t++;

    if (G.drive_t == DRIVE_SWOOP)
        audio_sfx(SFX_STING);

    /* The road goes green. The tyre-hum of the highway stops dead and THIS
     * takes over -- a tone that will not hold still, climbing by semitones,
     * arriving nowhere. It plays right through the lift and into the white. */
    if (G.drive_t == DRIVE_BEAM)
        audio_music(MUSIC_BEAM);

    if (G.drive_t == DRIVE_LIFT)
        audio_sfx(SFX_HURT);

    if (G.drive_t >= DRIVE_BAM) {
        audio_sfx(SFX_STING);
        audio_music(MUSIC_PROLOGUE);
        G.end_sel = 2;              /* CONTINUE is the safe default */
        G.state   = ST_PROLOGUE;
        G.t       = 0;
    }
}

/* ADD light to a pixel instead of replacing it.
 *
 * gfx_pixel() overwrites, which is right for sprites and wrong for a BEAM:
 * an opaque triangle of green would simply erase the van it is supposed to
 * be lifting. Light adds. So the van stays there, lit sick green from above,
 * and you watch it leave the road. */
static void glow_pixel(int x, int y, uint16_t col, int a)
{
    if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H || a <= 0)
        return;
    if (a > 256) a = 256;

    uint16_t p = gfx_fb[y * SCREEN_W + x];
    int r = ((p >> 11) & 0x1F) + ((((col >> 11) & 0x1F) * a) >> 8);
    int g = ((p >>  5) & 0x3F) + ((((col >>  5) & 0x3F) * a) >> 8);
    int b = ( p        & 0x1F) + ((( col        & 0x1F) * a) >> 8);
    if (r > 31) r = 31;
    if (g > 63) g = 63;
    if (b > 31) b = 31;
    gfx_fb[y * SCREEN_W + x] = (uint16_t)((r << 11) | (g << 5) | b);
}

/* how far down the saucer has come: 0 at SWOOP, 256 by LIFT */
static int swoop_progress(void)
{
    if (G.drive_t < DRIVE_SWOOP) return 0;
    if (G.drive_t >= DRIVE_LIFT) return 256;
    return 256 * (G.drive_t - DRIVE_SWOOP) / (DRIVE_LIFT - DRIVE_SWOOP);
}

void drive_render(void)
{
    int t = G.drive_t;

    /* ---- night sky ------------------------------------------------------*/
    gfx_clear(RGB565(8, 8, 22));
    for (int i = 0; i < 40; i++) {          /* stars, fixed by seed */
        uint32_t r = snoise();
        int sx = (int)(r % SCREEN_W);
        int sy = (int)((r >> 9) % HORIZON);
        gfx_pixel(sx, sy, RGB565(120, 120, 150));
    }
    sn = 0x51ED270Bu;                       /* reseed: the stars hold still */

    /* ---- the road, rushing at you --------------------------------------*/
    gfx_fill_rect(0, HORIZON, SCREEN_W, SCREEN_H - HORIZON, RGB565(26, 24, 30));
    /* verges, converging on the vanishing point */
    for (int y = HORIZON; y < SCREEN_H; y++) {
        int spread = (y - HORIZON) * 3;
        gfx_pixel(SCREEN_W / 2 - 14 - spread, y, RGB565(70, 66, 60));
        gfx_pixel(SCREEN_W / 2 + 14 + spread, y, RGB565(70, 66, 60));
    }
    /* the dashed centre line, coming at you fast */
    for (int d = 0; d < 6; d++) {
        int phase = (t * 4 + d * 40) % 240;
        int y = HORIZON + phase * (SCREEN_H - HORIZON) / 240;
        int w = 1 + (y - HORIZON) / 12;
        int h = 1 + (y - HORIZON) / 14;
        gfx_fill_rect(SCREEN_W / 2 - w / 2, y, w, h, RGB565(190, 180, 120));
    }

    /* ---- THE STREET LIGHTS ---------------------------------------------
     * Sodium lamps down both shoulders, rushing past. They're what makes
     * the road feel like a road -- and they're the only reason you can see
     * how empty it is. Each one is drawn in perspective from its phase:
     * far away it's a dot high up near the horizon; close, it's a pole
     * that towers over you and slides off the edge of the screen.
     */
    for (int i = 0; i < 5; i++) {
        int phase = (t * 3 + i * 56) % 280;
        if (phase < 6)
            continue;
        /* z: 0 at the horizon, 256 right on top of you */
        int z    = phase * 256 / 280;
        int y    = HORIZON + (z * z / 256) * (SCREEN_H - HORIZON) / 256;
        int out  = 18 + (z * z / 256) * 150 / 256;   /* how far off-centre */
        int hgt  = 4 + (z * z / 256) * 90 / 256;     /* how tall it stands */
        int lw   = 1 + z / 90;

        for (int side = 0; side < 2; side++) {
            int px = SCREEN_W / 2 + (side ? out : -out);
            if (px < -20 || px > SCREEN_W + 20)
                continue;

            /* the pole */
            gfx_fill_rect(px - lw / 2, y - hgt, lw, hgt, RGB565(48, 46, 52));
            /* the arm, reaching in over the road */
            int arm = 2 + hgt / 6;
            gfx_fill_rect(side ? px - arm : px, y - hgt, arm, 1,
                          RGB565(48, 46, 52));

            /* the head, and a TIGHT sodium bloom around it -- a street
             * lamp, not a sun. The glow falls off fast (1/ring^2). */
            int hx = side ? px - arm : px + arm;
            int hy = y - hgt;
            int gr = 2 + hgt / 34;                  /* 2..4 px core */
            for (int ring = 3; ring >= 1; ring--) {
                int rr = gr * ring;
                int a  = 130 / (ring * ring);
                if (a < 8)
                    continue;
                for (int gy = -rr; gy <= rr; gy++)
                    for (int gx = -rr; gx <= rr; gx++)
                        if (gx * gx + gy * gy <= rr * rr)
                            gfx_pixel(hx + gx, hy + gy,
                                      gfx_dim(RGB565(255, 190, 90), a));
            }
            gfx_fill_rect(hx - gr / 2, hy - gr / 2,
                          gr < 2 ? 1 : gr, gr < 2 ? 1 : gr,
                          RGB565(255, 240, 190));

            /* the pool it throws on the tarmac: a flat ellipse at its feet */
            int prx = 4 + hgt / 5;                  /* half-width  */
            int pry = 2 + hgt / 14;                 /* half-height */
            for (int gy = -pry; gy <= pry; gy++) {
                int w = prx - (prx * (gy < 0 ? -gy : gy)) / (pry ? pry : 1);
                int a = 46 - (gy < 0 ? -gy : gy) * 3;
                if (a < 6)
                    continue;
                for (int gx = -w; gx <= w; gx++)
                    gfx_pixel(hx + gx, y + gy,
                              gfx_dim(RGB565(210, 155, 70), a));
            }
        }
    }

    /* ---- the van, from behind ------------------------------------------*/
    int prog = swoop_progress();
    int vscale = 2;                          /* 32x32 art -> 64x64 on screen */
    int vy = SCREEN_H - VAN_H * vscale + 6;
    int vx = (SCREEN_W - VAN_W * vscale) / 2;

    /* engine shudder */
    vx += ((t / 4) % 2) ? 1 : 0;
    vy += ((t / 6) % 2) ? 1 : 0;

    /* ...and then the ground lets go of it.
     *
     * It only climbs LIFT_PX. It has to stop short of the saucer: if it goes
     * far enough to reach the thing lifting it, the saucer sprite simply
     * draws over the top of it and the shot becomes a picture of a saucer.
     * The van has to stay BELOW, hanging in the beam, with green either side
     * of it -- that's the image. */
    if (t >= DRIVE_LIFT) {
        int lift = (t - DRIVE_LIFT) * LIFT_PX / (DRIVE_BAM - DRIVE_LIFT);
        vy -= lift;
        vx += ((t / 2) % 2) ? 3 : -3;        /* and it swings, badly */
    }

    /* The BRAKE LIGHTS come on the moment he realises. */
    int braking = (t >= DRIVE_FOLLOW);
    gfx_blit_ex(van_big[braking ? 1 : 0], VAN_W, VAN_H, vx, vy,
                vscale, 256, 0);

    /* ---- IT ------------------------------------------------------------*/
    if (t >= DRIVE_LIGHT) {
        /* a light, high up and far back. It gets closer. It gets bigger. */
        int scale = 1;
        int gx = SCREEN_W / 2 - 8;
        int gy = 10;
        int bright = 90;

        if (t >= DRIVE_SWOOP) {
            scale  = 1 + prog * 3 / 256;     /* 1 -> 4: it looms */
            /* Its TOP rides up as it grows, so its BOTTOM edge lands around
             * y=60 -- low enough to feel on top of you, high enough to leave
             * the van room to hang under it in the beam. */
            gy     = 10 - prog * 14 / 256;
            bright = 150 + prog * 106 / 256;
        } else if (t >= DRIVE_FOLLOW) {
            scale  = 2;
            bright = 120;
            gy     = 10 + (int)((t / 20) % 2);
        } else {
            gy = 12 + (int)((t / 30) % 2);   /* it just hangs there */
        }
        gx = SCREEN_W / 2 - (16 * scale) / 2;

        /* THE GLOW. Nested ellipses, dimmest first -- you see the light
         * bleeding into the sky before you see the shape making it. */
        int cx0 = gx + 8 * scale, cy0 = gy + 7 * scale;
        for (int ring = 4; ring >= 1; ring--) {
            int rx = (7 + ring * 4) * scale / 2;
            int ry = (3 + ring * 2) * scale / 2;
            int a  = bright / (ring * 3 + 2);
            uint16_t c = gfx_dim(RGB565(130, 210, 240), a);
            for (int y = -ry; y <= ry; y++) {
                int w = rx - (rx * (y < 0 ? -y : y)) / (ry ? ry : 1);
                for (int x = -w; x <= w; x++)
                    gfx_pixel(cx0 + x, cy0 + y, c);
            }
        }

        int ufo = ((t / 6) % 2) ? SPR_UFO_1 : SPR_UFO;
        gfx_blit_ex(sprites[ufo].px, TILE, TILE, gx, gy, scale, bright, 0);

        /* ---- THE BEAM -------------------------------------------------------
         * RADIOACTIVE GREEN. Not a spotlight -- a column of something that
         * is bad for you. It's built from three colours across its width:
         * a near-white core that's too bright to look at, sick green either
         * side of it, and a deep irradiated edge that bleeds into the dark.
         *
         * And it does not sit still. Bands of brightness CRAWL UP it (the
         * `scan` term below), so the beam always looks like it is pulling.
         */
        if (t >= DRIVE_BEAM) {
            int b = 256 * (t - DRIVE_BEAM) / (DRIVE_BAM - DRIVE_BEAM);
            int top = gy + 12 * scale;
            int cx  = SCREEN_W / 2;
            int hgt = SCREEN_H - top;
            if (hgt < 1) hgt = 1;

            for (int y = top; y < SCREEN_H; y++) {
                /* a CONE: narrow at the saucer, and it swallows the road */
                int half = (7 + (y - top) * 52 / hgt) * b / 256;
                if (half < 1)
                    continue;

                /* bands crawling UP the shaft. This is the whole difference
                 * between a beam and a triangle of paint. */
                int scan = 210 + 46 * (((y * 3 - (int)t * 5) & 31) < 12);

                for (int x = cx - half; x <= cx + half; x++) {
                    int off  = (x > cx) ? (x - cx) : (cx - x);
                    int edge = 256 - (off * 200 / (half ? half : 1));
                    int a    = (90 + (y - top) * 130 / hgt);
                    a = a * b / 256 * edge / 256 * scan / 256;
                    if (a < 6)
                        continue;

                    /* colour by how far across the shaft we are */
                    int q = off * 256 / (half ? half : 1);   /* 0 core .. 256 */
                    uint16_t col = (q <  70) ? RGB565(225, 255, 190)  /* core  */
                                 : (q < 165) ? RGB565( 90, 255,  80)  /* green */
                                             : RGB565( 20, 170,  40); /* edge  */
                    glow_pixel(x, y, col, a);
                }
            }

            /* the pool it burns onto the tarmac, and the sick glow it throws
             * back up onto the underside of the saucer */
            int prad = 30 * b / 256;
            for (int y = SCREEN_H - 14; y < SCREEN_H; y++)
                for (int x = cx - prad * 2; x <= cx + prad * 2; x++) {
                    int dx = (x - cx) / 2, dy = y - (SCREEN_H - 4);
                    int d2 = dx * dx + dy * dy * 4;
                    if (d2 > prad * prad || prad < 1)
                        continue;
                    int a = 150 - d2 * 150 / (prad * prad);
                    glow_pixel(x, y, RGB565(120, 255, 110), a * b / 256);
                }
        }
    }

    /* ---- captions -------------------------------------------------------
     * On a dark plate, because the van is right underneath them. */
    {
        const char *l = 0;
        uint16_t col = RGB565(150, 150, 160);
        if (t < 110) {
            l = "FORTY MINUTES TO THE STATION.";
        } else if (t >= DRIVE_FOLLOW && t < DRIVE_FOLLOW + 110) {
            l = "IT'S KEEPING PACE.";
            col = RGB565(210, 170, 170);
        } else if (t >= DRIVE_BEAM && t < DRIVE_BEAM + 90) {
            l = "NO. NO, NO, NO --";
            col = RGB565(235, 90, 80);
        }
        if (l) {
            int tw = gfx_text_width(l, 1);
            int tx = (SCREEN_W - tw) / 2;
            gfx_fill_rect(0, SCREEN_H - 17, SCREEN_W, 17, RGB565(6, 6, 12));
            gfx_fill_rect(0, SCREEN_H - 18, SCREEN_W, 1, RGB565(40, 40, 55));
            gfx_text(tx, SCREEN_H - 13, l, col);
        }
    }

    /* the last few frames go white. That's the BAM. */
    if (t >= DRIVE_BAM - 8) {
        int w = 255 * (t - (DRIVE_BAM - 8)) / 8;
        gfx_fill_rect(0, 0, SCREEN_W, SCREEN_H,
                      gfx_dim(RGB565(255, 255, 255), w));
    }
}

/* ============================ END OF PROLOGUE ==============================
 * A city on the horizon, a moon, and a cluster of lights over it that are
 * not aircraft. The title, and the menu.
 * ==========================================================================*/

/* the skyline, drawn once into a fixed shape (no rng at draw time) */
static const unsigned char SKYLINE[24] = {
    12, 20, 16, 34, 28, 22, 40, 30, 18, 26, 44, 36,
    24, 30, 20, 38, 48, 32, 22, 28, 18, 34, 24, 14
};

void prologue_update(void)
{
    if (PRESSED(BTN_UP)) {
        G.end_sel = (G.end_sel + 2) % 3;
        audio_sfx(SFX_BLIP);
    }
    if (PRESSED(BTN_DOWN)) {
        G.end_sel = (G.end_sel + 1) % 3;
        audio_sfx(SFX_BLIP);
    }

    if (!PRESSED(BTN_A) && !PRESSED(BTN_START))
        return;

    switch (G.end_sel) {
    case 0:                      /* SAVE GAME -- the platform writes it */
        G.save_pending = 1;
        audio_sfx(SFX_CONFIRM);
        break;
    case 1:                      /* LOAD GAME */
        if (!G.has_save) {
            audio_sfx(SFX_BLIP);
            break;
        }
        G.load_pending = 1;
        audio_sfx(SFX_CONFIRM);
        break;
    default:                     /* CONTINUE
                                  * PART 1 ISN'T BUILT YET. Until it is,
                                  * this starts the prologue over -- you keep
                                  * your name, you lose everything else. */
        audio_sfx(SFX_CONFIRM);
        overworld_start_game();
        break;
    }
}

void prologue_render(void)
{
    /* the first moments are still white -- the BAM carrying over */
    if (G.t < 6) {
        gfx_clear(RGB565(255, 255, 255));
        return;
    }

    /* ---- a city, some night, somewhere else ----------------------------*/
    gfx_clear(RGB565(10, 12, 30));

    /* THE MOON */
    int mx = SCREEN_W - 44, my = 26, mr = 9;
    for (int y = -mr; y <= mr; y++)
        for (int x = -mr; x <= mr; x++)
            if (x * x + y * y <= mr * mr)
                gfx_pixel(mx + x, my + y, RGB565(226, 226, 208));
    gfx_fill_rect(mx - 4, my - 3, 3, 3, RGB565(198, 198, 182));  /* a sea */
    gfx_fill_rect(mx + 2, my + 3, 4, 2, RGB565(198, 198, 182));

    /* THE LIGHTS. A cluster. They are not aircraft: aircraft don't do that. */
    static const int LX[7] = { 40, 58, 49, 72, 33, 64, 52 };
    static const int LY[7] = { 30, 24, 40, 34, 44, 46, 18 };
    for (int i = 0; i < 7; i++) {
        int pulse = 150 + (int)(((G.frame / (6 + i)) % 2) ? 80 : 0);
        int drift = (int)((G.frame / (40 + i * 7)) % 2);
        uint16_t c = gfx_dim(RGB565(255, 214, 120), pulse);
        gfx_fill_rect(LX[i] + drift, LY[i], 2, 2, c);
        gfx_pixel(LX[i] - 1 + drift, LY[i] + 1, gfx_dim(c, 90));
        gfx_pixel(LX[i] + 2 + drift, LY[i] + 1, gfx_dim(c, 90));
    }

    /* THE SKYLINE, black against all of it */
    int bw = SCREEN_W / 24;
    for (int i = 0; i < 24; i++) {
        int h = SKYLINE[i];
        int bx = i * bw;
        int by = SCREEN_H - 34 - h;
        gfx_fill_rect(bx, by, bw - 1, h + 34, RGB565(6, 6, 14));
        /* a few windows still on */
        for (int w = 0; w < h / 8; w++) {
            int wy = by + 4 + w * 7;
            if (((i * 7 + w * 3) % 5) == 0)
                gfx_fill_rect(bx + 2, wy, 2, 2, RGB565(180, 160, 90));
            if (((i * 5 + w * 2) % 4) == 0)
                gfx_fill_rect(bx + bw - 4, wy, 2, 2, RGB565(140, 130, 80));
        }
    }

    /* ---- THE TITLE ------------------------------------------------------*/
    const char *t1 = "I KNOW";
    const char *t2 = "WHAT I SAW";
    uint16_t red = (G.t % 240 < 3) ? RGB565(90, 16, 12) : RGB565(216, 40, 32);
    gfx_text_ex((SCREEN_W - gfx_text_width(t1, 2)) / 2, 14, t1, red, 2);
    gfx_text_ex((SCREEN_W - gfx_text_width(t2, 2)) / 2, 32, t2, red, 2);

    const char *e = "END OF PROLOGUE";
    gfx_text((SCREEN_W - gfx_text_width(e, 1)) / 2, 56, e,
             RGB565(210, 210, 220));

    /* ---- the menu -------------------------------------------------------*/
    static const char *OPT[3] = { "SAVE GAME", "LOAD GAME", "CONTINUE" };
    for (int i = 0; i < 3; i++) {
        int oy = 92 + i * 16;
        int ox = (SCREEN_W - gfx_text_width(OPT[i], 1)) / 2;
        uint16_t col;
        if (i == 1 && !G.has_save)
            col = RGB565(90, 90, 100);          /* nothing to load */
        else
            col = (i == G.end_sel) ? RGB565(255, 255, 255)
                                   : RGB565(140, 140, 155);
        /* a plate behind the text so it reads over the city */
        gfx_fill_rect(ox - 8, oy - 3, gfx_text_width(OPT[i], 1) + 16, 13,
                      RGB565(10, 10, 20));
        gfx_text(ox, oy, OPT[i], col);
        if (i == G.end_sel)
            gfx_cursor(ox - 14, oy, G.frame);
    }

    /* the toast (SAVED. / LOADED. / NO SAVE FOUND) -- the overworld HUD
     * usually draws this, and we are very much not in the overworld */
    if (G.toast_ticks > 0 && G.toast) {
        G.toast_ticks--;
        int tw = gfx_text_width(G.toast, 1);
        gfx_fill_rect((SCREEN_W - tw) / 2 - 6, 74, tw + 12, 14,
                      RGB565(16, 16, 24));
        gfx_text((SCREEN_W - tw) / 2, 77, G.toast,
                 G.toast_good ? RGB565(150, 240, 150)
                              : RGB565(240, 110, 100));
    }
}
