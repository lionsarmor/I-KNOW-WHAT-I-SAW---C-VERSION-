#!/usr/bin/env python3
"""Generate Steam store + library capsule art for the demo.

Everything is drawn from the game's own assets so the art matches what players
see: the UFO is SPR_ART_UFO, the title is set in the game's 8x8 font (font.h),
the palette is the game's palette. Pixel elements scale with nearest-neighbour
so they stay crisp at every size.

Outputs PNGs into dist/steam/capsules/ (gitignored; regenerate any time):
  header_460x215     store header / library header
  small_231x87       search + lists (title must read small)
  main_616x353       featured carousel
  vertical_374x448   store vertical capsule
  library_600x900    library grid (portrait)
  library_hero_1920x620
  page_background_1920x1080
  community_icon_184

Run:  python3 tools/make_capsules.py
"""
import re
import random
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter, ImageChops

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "dist" / "steam" / "capsules"

# ---- UFO sprite + palette (verbatim from the game) ------------------------
UFO = [
    "................", "................", ".....000000.....", "....0eeeeee0....",
    "...0e111111e0...", "...0e1eeee1e0...", "..0eeeeeeeeee0..", ".0kkkkkkkkkkkk0.",
    "0kKyKkkKkkKyKkk0", "0kkkkkkkkkkkkkk0", ".0KKKKKKKKKKKK0.", "..00KKKKKKKK00..",
    "....0y0..0y0....", ".....0....0.....", "................", "................",
]
PAL = {"0": (16, 16, 24), "1": (240, 240, 240), "e": (112, 224, 208),
       "k": (150, 150, 158), "K": (90, 90, 98), "y": (232, 200, 96)}

MINT = (112, 224, 208)
STRAW = (232, 200, 96)
WHITE = (238, 238, 244)
INK = (10, 10, 18)
SKY_TOP, SKY_MID, SKY_BOT = (44, 38, 78), (24, 22, 48), (10, 10, 20)


# ---- the game's 8x8 font, parsed from font.h ------------------------------
def load_font():
    txt = (ROOT / "src/game/assets/font.h").read_text()
    glyphs = {}
    for m in re.finditer(r"G\(\s*'(\\.|[^'])'\s*\)\s*=\s*\{([^}]*)\}", txt):
        raw, body = m.group(1), m.group(2)
        ch = {"\\'": "'", "\\\\": "\\"}.get(raw, raw)
        try:
            rows = [int(v.strip(), 0) for v in body.split(",") if v.strip()]
        except ValueError:
            continue  # skip the example glyph in the header comment
        if len(rows) == 8:
            glyphs[ch] = [[(r >> (7 - b)) & 1 for b in range(8)] for r in rows]
    glyphs.setdefault(" ", [[0] * 8 for _ in range(8)])
    return glyphs


FONT = load_font()


def text_mask(text, spacing=1):
    """1x L-mode mask of a single line in the game font (white glyphs)."""
    w = len(text) * (8 + spacing) - spacing if text else 1
    img = Image.new("L", (max(w, 1), 8), 0)
    px = img.load()
    x = 0
    for ch in text:
        g = FONT.get(ch, FONT[" "])
        for ry in range(8):
            for rx in range(8):
                if g[ry][rx]:
                    px[x + rx, ry] = 255
        x += 8 + spacing
    return img


def draw_title(lines, scale, fill=WHITE, outline=INK, thick=None,
               glow=MINT, align="left", leading=3):
    """RGBA image of stacked title lines: pixel font, thick outline, soft glow."""
    thick = thick if thick is not None else max(2, scale // 3)
    masks = [text_mask(l) for l in lines]
    big = [m.resize((m.width * scale, m.height * scale), Image.NEAREST) for m in masks]
    tw = max(m.width for m in big)
    lh = 8 * scale
    th = len(big) * lh + (len(big) - 1) * (leading * scale)
    pad = thick + scale * 2
    W, H = tw + pad * 2, th + pad * 2
    out = Image.new("RGBA", (W, H), (0, 0, 0, 0))

    # combined mask for the glow
    comb = Image.new("L", (W, H), 0)
    y = pad
    for m in big:
        x = pad + {"left": 0, "center": (tw - m.width) // 2,
                   "right": tw - m.width}[align]
        comb.paste(m, (x, y), m)
        y += lh + leading * scale

    if glow:
        g = comb.filter(ImageFilter.GaussianBlur(scale * 1.5))
        glow_layer = Image.new("RGBA", (W, H), glow + (0,))
        glow_layer.putalpha(g.point(lambda v: int(v * 0.55)))
        out.alpha_composite(glow_layer)

    # outline: union (per-pixel max) of the glyph mask shifted around a disc,
    # then paint the ink colour through it and the fill colour on top.
    off = [(dx, dy)
           for dx in range(-thick, thick + 1)
           for dy in range(-thick, thick + 1)
           if dx * dx + dy * dy <= thick * thick and (dx or dy)]
    ring = Image.new("L", (W, H), 0)
    for dx, dy in off:
        ring = ImageChops.lighter(ring, _shifted(comb, dx, dy))
    out.paste(Image.new("RGBA", (W, H), outline + (255,)), (0, 0), ring)
    out.paste(Image.new("RGBA", (W, H), fill + (255,)), (0, 0), comb)
    return out


def _shifted(mask, dx, dy):
    s = Image.new("L", mask.size, 0)
    s.paste(mask, (dx, dy))
    return s


# ---- scene pieces ---------------------------------------------------------
def lerp(a, b, t):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3))


def sky(w, h):
    img = Image.new("RGB", (w, h))
    px = img.load()
    for y in range(h):
        t = y / max(h - 1, 1)
        c = lerp(SKY_TOP, SKY_MID, t * 2) if t < 0.5 else lerp(SKY_MID, SKY_BOT, (t - 0.5) * 2)
        for x in range(w):
            px[x, y] = c
    return img


def stars(img, seed, n, ymax):
    rnd = random.Random(seed)
    d = ImageDraw.Draw(img, "RGBA")
    for _ in range(n):
        x, y = rnd.randint(0, img.width), rnd.randint(0, int(ymax))
        b = rnd.randint(60, 190)
        r = rnd.choice([0, 0, 0, 1])
        d.ellipse([x - r, y - r, x + r, y + r], fill=(230, 232, 245, b))


def ufo_img(scale):
    spr = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    p = spr.load()
    for y, line in enumerate(UFO):
        for x, ch in enumerate(line):
            if ch != ".":
                p[x, y] = PAL[ch] + (255,)
    return spr.resize((16 * scale, 16 * scale), Image.NEAREST)


def beam(w, h, cx, top_y, bot_y, half_top, half_bot):
    layer = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    d.polygon([(cx - half_top, top_y), (cx + half_top, top_y),
               (cx + half_bot, bot_y), (cx - half_bot, bot_y)],
              fill=MINT + (70,))
    layer = layer.filter(ImageFilter.GaussianBlur(max(4, w // 120)))
    core = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    dc = ImageDraw.Draw(core)
    dc.polygon([(cx - half_top // 2, top_y), (cx + half_top // 2, top_y),
                (cx + half_bot // 2, bot_y), (cx - half_bot // 2, bot_y)],
               fill=MINT + (40,))
    core = core.filter(ImageFilter.GaussianBlur(max(3, w // 200)))
    layer.alpha_composite(core)
    return layer


def barn(w, h, ground_y):
    """Near-black farm silhouette across the bottom: ground, barn, a fence."""
    layer = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    sil = (7, 8, 16, 255)
    d.rectangle([0, ground_y, w, h], fill=sil)
    # barn: body + gambrel roof, sitting left-of-centre
    bw = int(w * 0.20)
    bx = int(w * 0.60)
    by = ground_y
    bh = int(bw * 0.62)
    d.rectangle([bx, by - bh, bx + bw, by], fill=sil)
    rt = by - bh
    d.polygon([(bx - bw * 0.10, rt), (bx + bw * 0.5, rt - bh * 0.55),
               (bx + bw * 1.10, rt)], fill=sil)
    # silo
    sx = bx + bw + int(bw * 0.12)
    sw = int(bw * 0.28)
    d.rectangle([sx, by - bh * 0.9, sx + sw, by], fill=sil)
    d.pieslice([sx, by - bh * 0.9 - sw * 0.5, sx + sw, by - bh * 0.9 + sw * 0.5],
               180, 360, fill=sil)
    # a lit barn window (straw)
    d.rectangle([bx + bw * 0.42, by - bh * 0.55, bx + bw * 0.58, by - bh * 0.35],
                fill=STRAW + (255,))
    # fence posts marching to the left
    fy = ground_y
    for i in range(6):
        fx = int(w * 0.05) + i * int(w * 0.075)
        d.rectangle([fx, fy - h * 0.03, fx + max(2, w // 260), fy], fill=sil)
    d.line([int(w * 0.05), fy - h * 0.022, int(w * 0.05) + 5 * int(w * 0.075),
            fy - h * 0.022], fill=sil, width=max(2, h // 300))
    return layer


def frame(img):
    """Subtle inner vignette so art reads on any store background."""
    v = Image.new("L", img.size, 0)
    d = ImageDraw.Draw(v)
    m = max(img.size) // 12
    d.rectangle([0, 0, img.width, img.height], fill=90)
    d.rectangle([m, m, img.width - m, img.height - m], fill=0)
    v = v.filter(ImageFilter.GaussianBlur(m))
    dark = Image.new("RGB", img.size, (6, 6, 12))
    return Image.composite(dark, img, v)


# ---- compositions ---------------------------------------------------------
def fit_scale(lines, max_w, max_h, spacing=1, leading=3):
    """Largest integer pixel scale whose text block fits the given box."""
    longest = max((len(l) for l in lines), default=1)
    nat_w = longest * (8 + spacing) - spacing
    nat_h = len(lines) * 8 + (len(lines) - 1) * leading
    return max(1, int(min(max_w / nat_w, max_h / nat_h)))


def compose(w, h):
    img = sky(w, h)
    aspect = w / h
    ground_y = int(h * (0.80 if aspect >= 1.25 else 0.86))
    stars(img, 42, int(w * h / 2600), ground_y - 4)
    img = img.convert("RGBA")

    if aspect >= 1.25:   # wide: title left, saucer beaming down on the right
        lines = ["I KNOW", "WHAT I SAW"]
        ts = fit_scale(lines, w * 0.55, h * 0.50)
        title = draw_title(lines, ts, align="left")
        tx = int(w * 0.05)
        tcy = int(h * 0.40)
        img_title_y = tcy - title.height // 2

        us = max(2, h // 52)
        u = ufo_img(us)
        ucx = int(w * 0.82)
        uy = int(h * 0.20)
        img.alpha_composite(barn(w, h, ground_y))
        img.alpha_composite(beam(w, h, ucx, uy + int(u.height * 0.72),
                                 ground_y, u.width // 3, int(u.width * 0.9)))
        img.alpha_composite(u, (ucx - u.width // 2, uy))
        img.alpha_composite(title, (tx, img_title_y))

        sub = ["AN ALIEN ABDUCTION RPG"]
        ss = fit_scale(sub, title.width, h * 0.12)
        subimg = draw_title(sub, ss, fill=STRAW, glow=None, align="left", leading=2)
        img.alpha_composite(subimg, (tx + ts, img_title_y + title.height - ts))
    else:                # tall: saucer up top, title stacked in the beam
        lines = ["I KNOW", "WHAT I", "SAW"]
        us = max(3, w // 32)
        u = ufo_img(us)
        uy = int(h * 0.08)
        img.alpha_composite(barn(w, h, ground_y))
        img.alpha_composite(beam(w, h, w // 2, uy + int(u.height * 0.72),
                                 ground_y, u.width // 3, int(u.width * 1.0)))
        img.alpha_composite(u, (w // 2 - u.width // 2, uy))
        ts = fit_scale(lines, w * 0.80, h * 0.34)
        title = draw_title(lines, ts, align="center")
        img.alpha_composite(title, (w // 2 - title.width // 2, int(h * 0.52)))
    return frame(img.convert("RGB"))


def compose_small(w, h):
    """231x87-ish: title-forward, tiny saucer, no busy silhouette."""
    img = sky(w, h)
    stars(img, 7, 40, h)
    img = img.convert("RGBA")
    ts = max(2, h // 26)
    title = draw_title(["I KNOW", "WHAT I SAW"], ts, align="left", leading=2)
    scale = min(1.0, (w - h * 0.9) / title.width, (h - 8) / title.height)
    if scale < 1.0:
        title = title.resize((int(title.width * scale), int(title.height * scale)))
    img.alpha_composite(title, (int(h * 0.14), h // 2 - title.height // 2))
    u = ufo_img(max(2, h // 20))
    img.alpha_composite(u, (w - u.width - int(h * 0.12), h // 2 - u.height // 2))
    return frame(img.convert("RGB"))


def compose_icon(s):
    img = sky(s, s).convert("RGBA")
    stars(img, 3, 20, s)
    u = ufo_img(max(2, s // 20))
    img.alpha_composite(u, (s // 2 - u.width // 2, s // 2 - u.height // 2))
    return img.convert("RGB")


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    jobs = {
        "header_460x215": compose(460, 215),
        "main_616x353": compose(616, 353),
        "vertical_374x448": compose(374, 448),
        "library_600x900": compose(600, 900),
        "library_hero_1920x620": compose(1920, 620),
        "page_background_1920x1080": compose(1920, 1080),
        "small_231x87": compose_small(231, 87),
        "community_icon_184": compose_icon(184),
    }
    for name, im in jobs.items():
        p = OUT / f"{name}.png"
        im.save(p)
        print(f"wrote {p.relative_to(ROOT)}  ({im.width}x{im.height})")


if __name__ == "__main__":
    main()
