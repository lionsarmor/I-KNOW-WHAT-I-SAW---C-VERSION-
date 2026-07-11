/* ============================================================================
 * gfx.h -- tiny software renderer.
 *
 * Everything draws into one static RGB565 framebuffer (gfx_fb).
 * All functions clip against the screen edges, so you can draw partly
 * (or entirely) off-screen without crashing.
 * ==========================================================================*/
#ifndef GFX_H
#define GFX_H

#include <stdint.h>
#include "config.h"

/* Pack 8-bit R,G,B into a 16-bit RGB565 pixel. */
#define RGB565(r, g, b) \
    ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

/* This exact color means "transparent" in sprite data (classic magenta).
 * Blit functions skip these pixels. */
#define COLOR_KEY 0xF81F

extern uint16_t gfx_fb[SCREEN_W * SCREEN_H];

void gfx_clear(uint16_t color);
void gfx_pixel(int x, int y, uint16_t color);
void gfx_fill_rect(int x, int y, int w, int h, uint16_t color);
void gfx_rect(int x, int y, int w, int h, uint16_t color); /* 1px outline */

/* Multiply a color's brightness. bright: 0 = black .. 256 = unchanged.
 * Used by the intro to fade the alien in from the dark. */
uint16_t gfx_dim(uint16_t color, int bright);

/* THE NIGHT PASS. Dims the whole framebuffer to `bright` (256 = broad
 * daylight, and the call becomes a no-op). Blue fades less than red and
 * green, so nights look moonlit instead of merely dark.
 *
 * If `radius` > 0 it also cuts a pool of light around (lx, ly): full
 * daylight within radius/2, falling off to the ambient level at radius.
 * That's the flashlight. Pass radius = 0 for no light.
 *
 * Call it after drawing the world, before drawing the HUD (so the HUD
 * stays readable in the dark). */
void gfx_night(int bright, int lx, int ly, int radius);

/* The menu cursor: a pulsing right-pointing triangle, drawn 7px tall
 * with its point at (x+3, y+3).
 * It is NOT text -- the 8x8 font has no '>' glyph (see assets/font.h),
 * so drawing one with gfx_text() silently produces nothing. */
void gfx_cursor(int x, int y, uint32_t frame);

/* Blit a rectangle of pixels (skipping COLOR_KEY).
 * The _ex version adds: integer scale (1,2,3..), brightness (0..256),
 * and horizontal flip (used to mirror left-facing sprites to the right). */
void gfx_blit(const uint16_t *px, int w, int h, int x, int y);
void gfx_blit_ex(const uint16_t *px, int w, int h, int x, int y,
                 int scale, int bright, int flip_h);

/* 8x8 bitmap font text. Only these characters exist (see assets/font.h):
 *   A-Z 0-9 space ! ? . , : ' - /
 * Lowercase input is drawn as uppercase. Unknown chars draw as blank. */
void gfx_text(int x, int y, const char *s, uint16_t color);
void gfx_text_ex(int x, int y, const char *s, uint16_t color, int scale);
int  gfx_text_width(const char *s, int scale); /* for centering */

#endif /* GFX_H */
