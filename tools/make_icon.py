#!/usr/bin/env python3
"""Render the in-game UFO sprite into app icons.

Produces, next to platform/desktop/main_sdl.c:
  iknowwhatisaw.ico   multi-resolution Windows icon (embedded via windres)
  icon.png            512x512 master (Linux .desktop, Steam capsule base, etc.)

The sprite + palette are copied verbatim from the game so the icon is the exact
saucer players see:  SPR_ART_UFO in src/game/assets/sprites.h, palette in
src/game/assets.c.  Re-run after changing either:  python3 tools/make_icon.py
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter

# --- SPR_ART_UFO, verbatim from src/game/assets/sprites.h -------------------
UFO = [
    "................",
    "................",
    ".....000000.....",
    "....0eeeeee0....",
    "...0e111111e0...",
    "...0e1eeee1e0...",
    "..0eeeeeeeeee0..",
    ".0kkkkkkkkkkkk0.",
    "0kKyKkkKkkKyKkk0",
    "0kkkkkkkkkkkkkk0",
    ".0KKKKKKKKKKKK0.",
    "..00KKKKKKKK00..",
    "....0y0..0y0....",
    ".....0....0.....",
    "................",
    "................",
]

# --- palette (RGB), the entries used by the UFO, from src/game/assets.c -----
PAL = {
    "0": (16, 16, 24),     # near-black outline
    "1": (240, 240, 240),  # white
    "e": (112, 224, 208),  # visitor mint (canopy)
    "k": (150, 150, 158),  # gray (hull)
    "K": (90, 90, 98),     # dark gray (hull shadow)
    "y": (232, 200, 96),   # straw yellow (lights / beam)
}

ICON = 512          # master size
CELL = 25           # px per sprite pixel  (16*25 = 400 sprite area)
SPRITE = 16 * CELL  # 400
OFF = (ICON - SPRITE) // 2   # centering padding (56)

DESK = Path(__file__).resolve().parent.parent / "platform" / "desktop"


def lerp(a, b, t):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3))


def main():
    # night-sky gradient background
    top, bot = (26, 26, 46), (9, 9, 17)
    bg = Image.new("RGB", (ICON, ICON))
    px = bg.load()
    for y in range(ICON):
        row = lerp(top, bot, y / (ICON - 1))
        for x in range(ICON):
            px[x, y] = row

    # soft mint tractor-beam glow under the saucer
    glow = Image.new("L", (ICON, ICON), 0)
    gd = ImageDraw.Draw(glow)
    cx = OFF + SPRITE // 2
    cy = OFF + int(12.5 * CELL)
    gd.ellipse([cx - 130, cy - 60, cx + 130, cy + 150], fill=70)
    glow = glow.filter(ImageFilter.GaussianBlur(40))
    mint = Image.new("RGB", (ICON, ICON), (112, 224, 208))
    bg = Image.composite(mint, bg, glow)

    # the saucer, nearest-neighbour so pixels stay crisp
    spr = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    sp = spr.load()
    for y, line in enumerate(UFO):
        for x, ch in enumerate(line):
            if ch != ".":
                sp[x, y] = (*PAL[ch], 255)
    spr = spr.resize((SPRITE, SPRITE), Image.NEAREST)

    out = bg.convert("RGBA")
    out.alpha_composite(spr, (OFF, OFF))
    out = out.convert("RGB")

    png = DESK / "icon.png"
    out.save(png)
    ico = DESK / "iknowwhatisaw.ico"
    out.save(ico, sizes=[(256, 256), (128, 128), (64, 64), (48, 48), (32, 32), (16, 16)])
    print(f"wrote {png}")
    print(f"wrote {ico}")


if __name__ == "__main__":
    main()
