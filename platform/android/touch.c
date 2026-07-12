/* ============================================================================
 * touch.c -- ON-SCREEN CONTROLS. Android only.
 *
 * A phone has no buttons, so we draw some. This file is the ENTIRE extent of
 * the game's knowledge of touchscreens, and it lives in the platform layer:
 * it turns fingers into the same BTN_* bits a keyboard or a gamepad produces,
 * and the core never learns what a finger is. src/game/ does not change by a
 * single line to run on Android.
 *
 * WHERE THE CONTROLS GO
 *   The game is 240x160 (3:2). A phone in landscape is far wider than that,
 *   so once the picture is scaled to fit there are big empty bars down each
 *   side. That is where the controls go: THUMBS ON THE BARS, NOT ON THE GAME.
 *   Nothing covers the picture unless the screen is nearly 3:2 already, and
 *   even then everything is drawn translucent.
 *
 *   left  bar : the d-pad
 *   right bar : B (shoot) and A (confirm), positioned like a real pad
 *   top-right : START (the pack) -- small, out of the way of your thumbs
 *
 * WHY THE D-PAD IS BIGGER THAN IT LOOKS
 *   Its touch radius (`hit`) is much larger than the circle we draw. Thumbs
 *   are imprecise and invisible under themselves; a d-pad you must hit
 *   exactly is a d-pad that feels broken. It reads generously and draws
 *   honestly.
 * ==========================================================================*/
#include <SDL.h>
#include "game.h"
#include "touch.h"

#define MAX_FINGERS 8

static struct {
    SDL_FingerID id;
    int   down;
    float x, y;          /* window pixels */
} fingers[MAX_FINGERS];

/* the layout, recomputed whenever the window changes size */
typedef struct { int cx, cy, r, hit; } pad_t;

static pad_t dpad, btn_a, btn_b, btn_start;
static int   win_w = 1, win_h = 1;
static int   game_x0, game_x1;    /* where the picture actually sits */

/* Ask SDL where the 240x160 picture landed inside the window, so we can put
 * the controls in the bars BESIDE it rather than on top of it. */
void touch_layout(SDL_Renderer *ren, int w, int h)
{
    win_w = w > 0 ? w : 1;
    win_h = h > 0 ? h : 1;

    SDL_Rect vp;
    SDL_RenderGetViewport(ren, &vp);
    float sx = 1.0f, sy = 1.0f;
    SDL_RenderGetScale(ren, &sx, &sy);

    /* viewport is in logical units; scale back up to window pixels */
    game_x0 = (int)(vp.x * sx);
    game_x1 = (int)((vp.x + vp.w) * sx);
    if (game_x1 <= game_x0 || game_x1 > win_w) {   /* no logical size set */
        game_x0 = 0;
        game_x1 = win_w;
    }

    int left_bar  = game_x0;
    int right_bar = win_w - game_x1;

    /* thumb-sized, in a way that survives a 5" phone and a 12" tablet */
    int unit = win_h / 5;
    if (unit < 40) unit = 40;

    /* If there's a bar, sit in the middle of it. If there isn't, tuck into
     * the bottom corner of the picture and rely on being translucent. */
    dpad.cx = (left_bar > unit * 2) ? left_bar / 2 : unit + unit / 3;
    dpad.cy = win_h - unit - unit / 3;
    dpad.r  = unit;
    dpad.hit = unit * 3 / 2;          /* forgiving: see the note up top */

    int rx = (right_bar > unit * 2) ? game_x1 + right_bar / 2
                                    : win_w - unit - unit / 3;

    /* A is the near one (confirm, pressed constantly), B sits up-left of it
     * -- the same relationship they have on a real pad. */
    btn_a.cx = rx + unit / 3;
    btn_a.cy = win_h - unit - unit / 4;
    btn_a.r  = unit * 3 / 5;
    btn_a.hit = btn_a.r * 5 / 4;

    btn_b.cx = rx - unit * 2 / 3;
    btn_b.cy = win_h - unit * 2;
    btn_b.r  = unit * 3 / 5;
    btn_b.hit = btn_b.r * 5 / 4;

    btn_start.cx = win_w - unit / 2 - 8;
    btn_start.cy = unit / 2 + 8;
    btn_start.r  = unit / 3;
    btn_start.hit = btn_start.r * 3 / 2;
}

void touch_event(const SDL_Event *e)
{
    if (e->type != SDL_FINGERDOWN && e->type != SDL_FINGERUP &&
        e->type != SDL_FINGERMOTION)
        return;

    /* SDL gives finger positions as 0..1 of the window */
    float x = e->tfinger.x * (float)win_w;
    float y = e->tfinger.y * (float)win_h;

    if (e->type == SDL_FINGERUP) {
        for (int i = 0; i < MAX_FINGERS; i++)
            if (fingers[i].down && fingers[i].id == e->tfinger.fingerId)
                fingers[i].down = 0;
        return;
    }

    /* down or motion: find this finger, or take a free slot */
    for (int i = 0; i < MAX_FINGERS; i++)
        if (fingers[i].down && fingers[i].id == e->tfinger.fingerId) {
            fingers[i].x = x;
            fingers[i].y = y;
            return;
        }
    for (int i = 0; i < MAX_FINGERS; i++)
        if (!fingers[i].down) {
            fingers[i].down = 1;
            fingers[i].id   = e->tfinger.fingerId;
            fingers[i].x    = x;
            fingers[i].y    = y;
            return;
        }
}

static int inside(const pad_t *p, float x, float y)
{
    float dx = x - (float)p->cx, dy = y - (float)p->cy;
    return dx * dx + dy * dy <= (float)(p->hit * p->hit);
}

uint16_t touch_buttons(void)
{
    uint16_t b = 0;

    for (int i = 0; i < MAX_FINGERS; i++) {
        if (!fingers[i].down)
            continue;
        float x = fingers[i].x, y = fingers[i].y;

        if (inside(&btn_a, x, y))     b |= BTN_A;
        if (inside(&btn_b, x, y))     b |= BTN_B;
        if (inside(&btn_start, x, y)) b |= BTN_START;

        if (inside(&dpad, x, y)) {
            float dx = x - (float)dpad.cx;
            float dy = y - (float)dpad.cy;
            /* a dead zone in the middle, so resting a thumb doesn't walk */
            float dead = (float)dpad.r * 0.28f;
            if (dx * dx + dy * dy < dead * dead)
                continue;

            /* Diagonals ARE allowed: if one axis is at least 40% of the
             * other, both fire. Locking to 4 directions makes a touch d-pad
             * feel like it's fighting you. */
            float ax = dx < 0 ? -dx : dx;
            float ay = dy < 0 ? -dy : dy;
            if (ax > ay * 0.4f) b |= (dx > 0) ? BTN_RIGHT : BTN_LEFT;
            if (ay > ax * 0.4f) b |= (dy > 0) ? BTN_DOWN  : BTN_UP;
        }
    }
    return b;
}

/* ---- drawing ------------------------------------------------------------ */

static void fill_circle(SDL_Renderer *r, int cx, int cy, int rad)
{
    for (int dy = -rad; dy <= rad; dy++) {
        int w = (int)SDL_sqrt((double)(rad * rad - dy * dy));
        SDL_Rect s = { cx - w, cy + dy, w * 2, 1 };
        SDL_RenderFillRect(r, &s);
    }
}

static void ring(SDL_Renderer *r, int cx, int cy, int rad, int thick)
{
    for (int i = 0; i < thick; i++) {
        int rr = rad - i;
        for (int a = 0; a < 360; a += 2) {
            double t = a * 3.14159265 / 180.0;
            SDL_RenderDrawPoint(r, cx + (int)(rr * SDL_cos(t)),
                                   cy + (int)(rr * SDL_sin(t)));
        }
    }
}

/* Held controls light up. You cannot see your own thumb, so the only way to
 * know the game registered the press is to show it. */
static int held_a, held_b, held_start, held_dir;

void touch_draw(SDL_Renderer *r)
{
    uint16_t b = touch_buttons();
    held_a     = (b & BTN_A) != 0;
    held_b     = (b & BTN_B) != 0;
    held_start = (b & BTN_START) != 0;
    held_dir   = b & (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    /* ---- the d-pad: a ring, and a cross of arrows inside it ---- */
    SDL_SetRenderDrawColor(r, 200, 200, 210, 40);
    fill_circle(r, dpad.cx, dpad.cy, dpad.r);
    SDL_SetRenderDrawColor(r, 230, 230, 240, 110);
    ring(r, dpad.cx, dpad.cy, dpad.r, 2);

    int arm = dpad.r * 3 / 5;
    int th  = dpad.r / 4;
    struct { int dx, dy; uint16_t bit; } arms[4] = {
        {  0, -1, BTN_UP    }, {  0, 1, BTN_DOWN  },
        { -1,  0, BTN_LEFT  }, {  1, 0, BTN_RIGHT },
    };
    for (int i = 0; i < 4; i++) {
        int lit = (held_dir & arms[i].bit) != 0;
        SDL_SetRenderDrawColor(r, lit ? 255 : 220, lit ? 240 : 220,
                                  lit ? 150 : 230, lit ? 220 : 90);
        SDL_Rect s;
        if (arms[i].dx) {
            s.w = arm; s.h = th;
            s.x = dpad.cx + (arms[i].dx > 0 ? th / 2 : -th / 2 - arm);
            s.y = dpad.cy - th / 2;
        } else {
            s.w = th; s.h = arm;
            s.x = dpad.cx - th / 2;
            s.y = dpad.cy + (arms[i].dy > 0 ? th / 2 : -th / 2 - arm);
        }
        SDL_RenderFillRect(r, &s);
    }

    /* ---- the face buttons ---- */
    struct { pad_t *p; int lit; int cr, cg, cb; } bt[3] = {
        { &btn_a,     held_a,     120, 230, 130 },   /* A: green, confirm  */
        { &btn_b,     held_b,     235, 110,  90 },   /* B: red, the gun    */
        { &btn_start, held_start, 200, 200, 215 },
    };
    for (int i = 0; i < 3; i++) {
        pad_t *p = bt[i].p;
        int lit = bt[i].lit;
        SDL_SetRenderDrawColor(r, bt[i].cr, bt[i].cg, bt[i].cb,
                               lit ? 210 : 60);
        fill_circle(r, p->cx, p->cy, p->r);
        SDL_SetRenderDrawColor(r, 240, 240, 245, lit ? 235 : 120);
        ring(r, p->cx, p->cy, p->r, 2);
    }
}
