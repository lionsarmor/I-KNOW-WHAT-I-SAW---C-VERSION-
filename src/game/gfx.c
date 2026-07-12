/* ============================================================================
 * gfx.c -- software renderer implementation. Pure C, no OS, no float.
 * ==========================================================================*/
#include "gfx.h"
#include "assets.h" /* for font8 */
#include "assets/font_small.h"

uint16_t gfx_fb[SCREEN_W * SCREEN_H];

/* the screen-shake displacement. See gfx_origin(). */
static int org_x, org_y;

void gfx_origin(int x, int y)
{
    org_x = x;
    org_y = y;
}

void gfx_add_pixel(int x, int y, uint16_t color, int a)
{
    x += org_x;
    y += org_y;
    if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H || a <= 0)
        return;
    if (a > 256) a = 256;

    uint16_t p = gfx_fb[y * SCREEN_W + x];
    int r = ((p >> 11) & 0x1F) + ((((color >> 11) & 0x1F) * a) >> 8);
    int g = ((p >>  5) & 0x3F) + ((((color >>  5) & 0x3F) * a) >> 8);
    int b = ( p        & 0x1F) + ((( color        & 0x1F) * a) >> 8);
    if (r > 31) r = 31;
    if (g > 63) g = 63;
    if (b > 31) b = 31;
    gfx_fb[y * SCREEN_W + x] = (uint16_t)((r << 11) | (g << 5) | b);
}

void gfx_blend_pixel(int x, int y, uint16_t color, int a)
{
    x += org_x;
    y += org_y;
    if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H || a <= 0)
        return;
    if (a > 256) a = 256;
    int inv = 256 - a;

    uint16_t p = gfx_fb[y * SCREEN_W + x];
    int r = (((p >> 11) & 0x1F) * inv + ((color >> 11) & 0x1F) * a) >> 8;
    int g = (((p >>  5) & 0x3F) * inv + ((color >>  5) & 0x3F) * a) >> 8;
    int b = (( p        & 0x1F) * inv + ( color        & 0x1F) * a) >> 8;
    gfx_fb[y * SCREEN_W + x] = (uint16_t)((r << 11) | (g << 5) | b);
}

void gfx_clear(uint16_t color)
{
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++)
        gfx_fb[i] = color;
}

void gfx_pixel(int x, int y, uint16_t color)
{
    x += org_x;
    y += org_y;
    if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H)
        return;
    gfx_fb[y * SCREEN_W + x] = color;
}

void gfx_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    x += org_x;
    y += org_y;
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

void gfx_night(int bright, const light_t *lights, int nlights)
{
    if (bright >= 256)
        return;                     /* broad daylight: nothing to dim */

    if (nlights > MAX_LIGHTS)
        nlights = MAX_LIGHTS;

    int span = 256 - bright;

    for (int y = 0; y < SCREEN_H; y++) {
        /* Which lights even reach this scanline? Usually none or one, so
         * the per-pixel loop below stays tiny. */
        int act[MAX_LIGHTS], nact = 0;
        for (int i = 0; i < nlights; i++) {
            int dy = y - lights[i].y;
            if (dy < 0) dy = -dy;
            if (dy < lights[i].r)
                act[nact++] = i;
        }

        for (int x = 0; x < SCREEN_W; x++) {
            int b = bright;
            int warm = 0;

            for (int k = 0; k < nact; k++) {
                const light_t *L = &lights[act[k]];
                int dx = x - L->x, dy = y - L->y;
                int d2 = dx * dx + dy * dy;
                int r_out2 = L->r * L->r;
                if (d2 >= r_out2)
                    continue;

                int r_in  = L->r / 2;
                int r_in2 = r_in * r_in;
                int lb;
                if (d2 <= r_in2) {
                    lb = 256;                       /* the bright core */
                } else {
                    /* linear in SQUARED distance: cheap, and the edge
                     * comes out soft rather than a hard disc */
                    lb = 256 - span * (d2 - r_in2) / (r_out2 - r_in2);
                }
                if (lb > b) {           /* brightest light wins -- they do
                                           NOT add up to daylight */
                    b = lb;
                    warm = L->warm;
                }
            }

            if (b >= 256 && warm == 0)
                continue;               /* untouched daylight */

            uint16_t c = gfx_fb[y * SCREEN_W + x];
            int r = (c >> 11) & 0x1F;
            int g = (c >> 5)  & 0x3F;
            int l =  c        & 0x1F;

            /* moonlight keeps its blue; a sodium street lamp does the
             * opposite -- it eats the blue and leaves everything amber */
            int br = b + (warm * (256 - b) / 256) / 2;
            int bg = b;
            int bl = b + (256 - b) / 3 - (warm / 3);
            if (bl < 0) bl = 0;

            r = (r * br) >> 8;
            g = (g * bg) >> 8;
            l = (l * bl) >> 8;
            if (r > 31) r = 31;
            if (g > 63) g = 63;
            if (l > 31) l = 31;

            gfx_fb[y * SCREEN_W + x] = (uint16_t)((r << 11) | (g << 5) | l);
        }
    }
}

void gfx_cursor(int x, int y, uint32_t frame)
{
    /* triangle wave over one second: 0..30..0 -- a slow, steady pulse
     * (a hard blink would fight the text for attention) */
    int t    = (int)(frame % 60);
    int glow = (t < 30) ? t : 60 - t;
    uint16_t col = RGB565(255, 170 + 85 * glow / 30, 90);   /* amber->gold */

    /* solid right-pointing triangle: rows 1,2,3,4,3,2,1 px wide, so the
     * flat edge is on the left and the point lands at (x+3, y+3) */
    for (int i = 0; i < 7; i++) {
        int w = (i < 4) ? i + 1 : 7 - i;
        gfx_fill_rect(x, y + i, w, 1, col);
    }
    gfx_fill_rect(x + 3, y + 3, 1, 1, RGB565(255, 255, 220));   /* glint */
}

void gfx_blit(const uint16_t *px, int w, int h, int x, int y)
{
    gfx_blit_ex(px, w, h, x, y, 1, 256, 0);
}

void gfx_blit_flash(const uint16_t *px, int w, int h, int x, int y,
                    uint16_t color, int flip_h)
{
    for (int sy = 0; sy < h; sy++)
        for (int sx = 0; sx < w; sx++) {
            int src = flip_h ? (w - 1 - sx) : sx;
            if (px[sy * w + src] != COLOR_KEY)
                gfx_pixel(x + sx, y + sy, color);
        }
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

/* ITALIC text: the same glyphs, SHEARED -- each row of the 8x8 glyph slides
 * right by (7 - row) / 3, so the top leans 2px ahead of the feet. A bitmap
 * font can't slant its strokes, but a slanted stack of rows reads as italic
 * at this size, and it's what makes the internal monologue look like a
 * different voice instead of a different color. Scale 1 only -- it's a
 * textbox face, not a headline. */
void gfx_text_italic(int x, int y, const char *s, uint16_t color)
{
    int cx = x;
    for (; *s; s++) {
        if (*s == '\n') {
            cx = x;
            y += 9;
            continue;
        }
        const unsigned char *g = glyph_for(*s);
        for (int row = 0; row < 8; row++) {
            unsigned char bits = g[row];
            int shear = (7 - row) / 3;             /* 2,2,1,1,1,0,0,0 */
            for (int col = 0; col < 8; col++)
                if (bits & (0x80u >> col))
                    gfx_pixel(cx + col + shear, y + row, color);
        }
        cx += 8;
    }
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

/* ---- the small font --------------------------------------------------------
 * 3x5 glyphs, 4px apart. See assets/font_small.h.
 */
static const unsigned char *glyph_small(char ch)
{
    if (ch >= 'a' && ch <= 'z')                 /* fold to uppercase */
        ch = (char)(ch - 'a' + 'A');
    if (ch < FONT_SMALL_FIRST || ch > FONT_SMALL_LAST)
        ch = ' ';
    return font_small[ch - FONT_SMALL_FIRST];
}

void gfx_text_small(int x, int y, const char *s, uint16_t color)
{
    for (int cx = x; *s; s++, cx += FONT_SMALL_ADV) {
        const unsigned char *g = glyph_small(*s);
        for (int row = 0; row < FONT_SMALL_ROWS; row++)
            for (int col = 0; col < FONT_SMALL_W; col++)
                if (g[row] & (1u << (FONT_SMALL_W - 1 - col)))
                    gfx_pixel(cx + col, y + row, color);
    }
}

void gfx_text_small_outlined(int x, int y, const char *s, uint16_t color)
{
    /* the halo first (the glyph nudged one pixel in every direction), then
     * the glyph on top -- so it reads against ANY background and needs no
     * panel behind it */
    static const int off[8][2] = {
        {-1,-1},{0,-1},{1,-1},{-1,0},{1,0},{-1,1},{0,1},{1,1}
    };
    uint16_t halo = RGB565(8, 8, 12);
    for (int i = 0; i < 8; i++)
        gfx_text_small(x + off[i][0], y + off[i][1], s, halo);
    gfx_text_small(x, y, s, color);
}

int gfx_text_small_width(const char *s)
{
    int n = 0;
    for (; *s; s++)
        n++;
    return n * FONT_SMALL_ADV;
}
