/* ============================================================================
 * gfx.c -- software renderer implementation. Pure C, no OS, no float.
 * ==========================================================================*/
#include "gfx.h"
#include "assets.h" /* for font8 */

uint16_t gfx_fb[SCREEN_W * SCREEN_H];

void gfx_clear(uint16_t color)
{
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++)
        gfx_fb[i] = color;
}

void gfx_pixel(int x, int y, uint16_t color)
{
    if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H)
        return;
    gfx_fb[y * SCREEN_W + x] = color;
}

void gfx_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    /* clip to screen */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0)
        return;
    for (int j = 0; j < h; j++) {
        uint16_t *row = &gfx_fb[(y + j) * SCREEN_W + x];
        for (int i = 0; i < w; i++)
            row[i] = color;
    }
}

void gfx_rect(int x, int y, int w, int h, uint16_t color)
{
    gfx_fill_rect(x,         y,         w, 1, color);
    gfx_fill_rect(x,         y + h - 1, w, 1, color);
    gfx_fill_rect(x,         y,         1, h, color);
    gfx_fill_rect(x + w - 1, y,         1, h, color);
}

uint16_t gfx_dim(uint16_t color, int bright)
{
    if (bright >= 256) return color;
    if (bright <= 0)   return 0;
    /* scale each RGB565 channel separately */
    int r = (color >> 11) & 0x1F;
    int g = (color >> 5)  & 0x3F;
    int b =  color        & 0x1F;
    r = (r * bright) >> 8;
    g = (g * bright) >> 8;
    b = (b * bright) >> 8;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

void gfx_blit(const uint16_t *px, int w, int h, int x, int y)
{
    gfx_blit_ex(px, w, h, x, y, 1, 256, 0);
}

void gfx_blit_ex(const uint16_t *px, int w, int h, int x, int y,
                 int scale, int bright, int flip_h)
{
    if (scale < 1) scale = 1;
    for (int sy = 0; sy < h; sy++) {
        for (int sx = 0; sx < w; sx++) {
            int src_x = flip_h ? (w - 1 - sx) : sx;
            uint16_t c = px[sy * w + src_x];
            if (c == COLOR_KEY)
                continue;
            if (bright < 256)
                c = gfx_dim(c, bright);
            if (scale == 1) {
                gfx_pixel(x + sx, y + sy, c);
            } else {
                gfx_fill_rect(x + sx * scale, y + sy * scale,
                              scale, scale, c);
            }
        }
    }
}

/* ---- text -----------------------------------------------------------------
 * font8 lives in assets/font.h: 8 bytes per glyph, one bit per pixel,
 * MSB = leftmost pixel, glyphs indexed by (char - 32).
 */
static const unsigned char *glyph_for(char ch)
{
    if (ch >= 'a' && ch <= 'z')       /* fold lowercase to uppercase */
        ch = (char)(ch - 'a' + 'A');
    if (ch < 32 || ch > 126)
        ch = ' ';
    return font8[ch - 32];
}

void gfx_text_ex(int x, int y, const char *s, uint16_t color, int scale)
{
    int cx = x;
    for (; *s; s++) {
        if (*s == '\n') {              /* newlines supported for free */
            cx = x;
            y += 8 * scale + scale;    /* 1px (scaled) line gap */
            continue;
        }
        const unsigned char *g = glyph_for(*s);
        for (int row = 0; row < 8; row++) {
            unsigned char bits = g[row];
            for (int col = 0; col < 8; col++) {
                if (bits & (0x80u >> col)) {
                    if (scale == 1)
                        gfx_pixel(cx + col, y + row, color);
                    else
                        gfx_fill_rect(cx + col * scale, y + row * scale,
                                      scale, scale, color);
                }
            }
        }
        cx += 8 * scale;
    }
}

void gfx_text(int x, int y, const char *s, uint16_t color)
{
    gfx_text_ex(x, y, s, color, 1);
}

int gfx_text_width(const char *s, int scale)
{
    int n = 0, best = 0;
    for (; *s; s++) {
        if (*s == '\n') { n = 0; continue; }
        n++;
        if (n > best) best = n;
    }
    return best * 8 * scale;
}
