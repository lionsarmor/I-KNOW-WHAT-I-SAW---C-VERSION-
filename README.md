# I KNOW WHAT I SAW

A retro top-down RPG boilerplate that runs the *same game code* on modern
computers and on small, cheap, hackable chips.

October 3rd, 1987. The lights came to the farm. A farmer, his crops
flattened in circles, sets out for town — and the visitors are still
out there, wandering the fields.

- Creepy intro cinematic → **BAM** → title screen
- Zelda/Pokemon-style overworld: walking, collision (walls *and* people),
  a scrolling camera, NPCs with dialog, enterable buildings, five maps
  (farm, town, and three interiors) connected by roads and doors
- Turn-based battles when a visitor catches you — or when you walk into
  one — with HP, XP, levels, and a data-driven enemy species table
- 2-channel chiptune synth (square wave + noise), per-map ambience
- **Everything** — sprites, tiles, maps, font, sounds, the cast — is
  human-readable text you can edit with no tools

## Quick start (desktop)

```
sudo apt install build-essential libsdl2-dev    # or brew install sdl2
make run
```

Controls: arrows/WASD move · Z (or space) = A · X = B · Enter = START · Esc quits.

## The architecture (read this before hacking)

```
src/game/        THE GAME. Portable C99. No OS, no malloc, no files,
                 no floats. This code does not know what a computer is.
platform/        One small file per machine. Its whole job:
  desktop/         call game_update(buttons) 60x per second,
  esp32/           put game_framebuffer() on a screen,
                   feed game_audio_fill() to a speaker.
```

The contract between the two halves is **four functions** in
[game.h](src/game/game.h). A platform layer is ~200 lines. That's why this
can run on an ESP32, a bare-metal Raspberry Pi, a RISC-V board, or a Steam
build — port the thin layer, never the game.

**The golden rule:** never include an OS header (or SDL, or ESP-IDF) inside
`src/game/`. Run `make check` to prove the core still compiles freestanding
(no OS, no libc) — do this often; it's the tripwire that keeps the game
portable to bare metal.

### Where everything lives

| You want to... | Go to |
|---|---|
| Change resolution, speeds, HP, buttons | [src/game/config.h](src/game/config.h) |
| **Add an enemy species or NPC look** | recipe: "THE CAST" in [assets.h](src/game/assets.h) · tables: end of [assets.c](src/game/assets.c) |
| **Add an NPC + dialog, place enemies** | spawn lists in [assets/maps.h](src/game/assets/maps.h) |
| **Add a map / an enterable building** | recipe at top of [assets/maps.h](src/game/assets/maps.h) |
| Edit / add character sprites (ASCII art) | [src/game/assets/sprites.h](src/game/assets/sprites.h) |
| Edit / add map tiles (ASCII art) | [src/game/assets/tiles.h](src/game/assets/tiles.h) |
| Change colors (the palette) | top of [src/game/assets.c](src/game/assets.c) |
| Add sound effects / music | [src/game/audio.c](src/game/audio.c) |
| Edit the font | [src/game/assets/font.h](src/game/assets/font.h) |
| The intro & title screen | [src/game/intro.c](src/game/intro.c) |
| Walking, camera, doors, dialog box | [src/game/overworld.c](src/game/overworld.c) |
| The battle system, game over | [src/game/battle.c](src/game/battle.c) |
| Add a whole new scene (menu, shop...) | recipe at top of [src/game/game_internal.h](src/game/game_internal.h) |
| Keyboard mapping | [platform/desktop/main_sdl.c](platform/desktop/main_sdl.c) |
| ESP32 pins / wiring | [platform/esp32/main/esp32_main.c](platform/esp32/main/esp32_main.c) |

Every one of those files opens with a comment explaining its recipe
("TO ADD A TILE: ...", "TO ADD A SOUND: ...").

### The 60-second content recipes

**A new enemy** — add `SPECIES_MANTIS` to the enum in `assets.h`, one row
of stats in the `species[]` table in `assets.c` (reuse existing sprites
with a different `bright` for an instant palette-swap villain), then
spawn it on any map: `{ ENT_ALIEN, 12, 7, SPECIES_MANTIS, 0 }`. Wandering,
ambushes, battles, XP — all automatic.

**A new NPC with dialog** — one line in a map's spawn list:
`{ ENT_NPC, 9, 4, LOOK_VILLAGER, "THE PIGS SCREAM AT NOTHING NOW." }`
They stand, breathe, block your path, and talk when you face them and
press A. Want a new look? Draw two idle frames in `sprites.h` and add a
`LOOK_*` row to `npc_looks[]`.

**An enterable building** — draw the interior as its own little map
(minimum 15x10), put a doormat `M` in its wall gap, add one warp on the
outside door and one on the mat. Full walkthrough at the top of
`assets/maps.h`. A door with no warp is automatically "locked".

**A different player character** — the hero is the nine `SPR_ART_FARMER_*`
drawings in `sprites.h` (3 frames × 3 directions; left is mirrored for
right). Redraw them and the game stars whoever you want. The 16-bit
style rules (outlines, one shade color) are written at the top of that
file so new art matches.

### How the art works

Sprites and tiles are ASCII drawings, one character per pixel, decoded at
boot. The palette letters are defined once in `assets.c`:

```c
static const char *SPR_ART_ALIEN_0[16] = {
    "................",
    ".....aaaaaa.....",      /* a = alien green  */
    "....aaaaaaaa....",
    "...a00aaaa00a...",      /* 0 = near-black   */
    ...
```

Maps are the same idea, one character per 16x16 tile:

```
"T..RRRRR.....................T",     T tree   R roof   # path
"T..HHDHH....,................T",     H wall   D door   . grass
"T....#########################",     <- the road out of the farm
```

If art is malformed (a row too short, an unknown letter) the game still
boots — you get loud magenta pixels at the mistake and a warning count
from `game_init()`.

## Testing / debugging

- All state lives in one struct, `G` ([game_internal.h](src/game/game_internal.h)) —
  put a breakpoint anywhere and inspect the entire game.
- `./build/iknowwhatisaw --frames 600 --shot 300 out.bmp` runs headless-ish
  for CI and saves a screenshot of any frame.
- The core is deterministic (fixed timestep + own RNG): the same inputs
  always replay the same game on every platform.

## ESP32

A skeleton platform layer for ESP-IDF v5 + a 320x240 ILI9341 SPI display
lives in [platform/esp32/](platform/esp32/) with a pin table at the top of
`esp32_main.c`. It builds the *same* core sources. RISC-V ESP32 variants
(C3/C6) work the same way — `idf.py set-target esp32c3`.

```
cd platform/esp32 && idf.py set-target esp32 && idf.py build flash monitor
```

Status: written but not yet run on hardware — expect to tweak pins,
orientation (`swap_xy` / `mirror`) and color order (`BGR`/`RGB`).

## Porting to a new machine (bare-metal Pi, RISC-V board, ...)

Copy `platform/desktop/main_sdl.c` as your template and replace its four
duties: a 60 Hz loop, button reads, a framebuffer blit, an audio sink
(optional — silence is fine). The core needs **~92 KB of RAM**
(77 KB framebuffer + ~15 KB decoded art + game state) and no FPU.

## Ideas wired and waiting

- Items/inventory: new scene (see the scene recipe), new battle menu entry
- Battle & town music: compose in `audio.c` (the farm drone shows how),
  pick per-map tracks in `overworld_enter_map()`
- Species with unique art: the TALL ONE currently reuses the grey's
  sprites drawn darker — draw it its own frames and point its
  `species[]` row at them
- Boss fights: a species with big stats + a scripted spawn is 90% there
- Save games: serialize `G.player` — the platform layer owns storage
  (a file on desktop, NVS on ESP32), keep the core clean of it
- A second joystick button... you get it. The hooks are all labeled.
