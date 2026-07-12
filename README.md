# 🛸 I KNOW WHAT I SAW

> **October 3rd, 1987.** The lights came to the farm.
> Your crops are flat in circles. The cows won't go near the pond.
> The dog has been barking at the sky all week and he is not sorry.

A retro alien-abduction RPG in **portable C99** — the *same game code* runs on
a PC, in your terminal, and on a $4 microcontroller. 👽

---

## 🎮 Play it

```sh
sudo apt install build-essential libsdl2-dev     # or: brew install sdl2
make run
```

**Controls** — arrows/WASD move · `Z`/space = **A** · `X` = **B** · `Enter` = **START**
`Esc` = pause menu · `F11` / `Alt+Enter` = fullscreen

🎮 **Gamepads just work.** Plug one in mid-game and it's picked up automatically —
`OPTIONS → CONTROLS` shows what's connected, and lets you swap A/B and toggle rumble.
✅ **Tested on a real Nintendo Switch Pro controller.** Xbox, DualShock and no-name
USB pads go through the same SDL layer, so they should behave identically.

---

## 🔨 Build commands

| Command | What you get |
|---|---|
| `make run` | 🖥️ build **and launch** the desktop game |
| `make pc` | 🖥️ desktop binary → `build/iknowwhatisaw` |
| `make ascii` | ⌨️ **the whole game in your terminal** → `build/iknowwhatisaw-ascii` |
| `make esp32` | 🔌 the handheld (ESP-IDF) |
| `make flash` | 🔌 esp32 + flash + serial monitor |
| `make check` | ✅ **the tripwire** — proves the core still compiles freestanding |
| `make dist` | 📦 **shippable Linux + Windows builds** (see below) |
| `make clean` | 🧹 |

`make ascii` needs **no SDL at all** — it renders every frame as coloured ASCII in
any terminal ≥ 120×40. Same game, same code, zero graphics libraries.

---

## 📦 Shipping it (Steam, itch.io, a zip file)

```sh
sudo apt install mingw-w64      # once, for the Windows cross-compile
make dist
```

That produces **two self-contained folders**. Point a Steam depot at each and ship —
the player needs nothing installed, SDL travels with the game:

```
dist/linux/     iknowwhatisaw  +  lib/libSDL2-2.0.so.0  +  run.sh
dist/windows/   iknowwhatisaw.exe  +  SDL2.dll
```

🐧 **Linux** bundles SDL next to the binary with an `$ORIGIN/lib` rpath rather than
static-linking it. A *static* SDL2 still drags in ~50 system libraries (X11, Wayland,
PulseAudio, ALSA…) and pins you to one distro's versions of them. Bundling the `.so`
is what actually survives the trip between machines — and it's what the Steam Linux
Runtime expects.

🪟 **Windows** is cross-compiled with mingw-w64 and built as a GUI subsystem app
(no console window pops up behind the game). The SDL2 Windows SDK is vendored in
`vendor/` — `make vendor-sdl` re-fetches it.

💾 **Saves and settings go to the OS's per-user folder**, never next to the binary
(a shipped game lives somewhere read-only):

| | |
|---|---|
| Windows | `%APPDATA%\Weeks\I Know What I Saw\` |
| Linux | `~/.local/share/Weeks/I Know What I Saw/` |
| macOS | `~/Library/Application Support/Weeks/…` |

Change the `SAVE_ORG` / `SAVE_APP` strings in
[main_sdl.c](platform/desktop/main_sdl.c) to whatever you register on Steam.

---

## 🌾 What's in the game

**🚜 The overworld** — Zelda-style. Walking, collision, a scrolling camera, NPCs who
talk, enterable buildings, eight maps (farm, town, three interiors, the north ridge,
the wrecked Hollis place, the grocery-store car park) wired together by roads and doors.

**⚔️ Two ways to fight, and they're the same wound**
- **Turn-based battles** — `FIGHT / SHOOT / ITEM / RUN`, HP, XP, levels, and every
  creature has special moves drawn from its legend.
- **Zelda mode** — once you have the family shotgun you can just *shoot things in the
  field*. They stagger, get knocked back, and come apart in a bloody mess.
- 🩸 **Damage carries across.** Put four shells into the goblin and let it catch you
  anyway, and it walks into the battle **bleeding**, with a visibly short bar. Hurt
  something in a fight and *run*, and it's still hurt when you shoot at it.
- A field kill is quicker and more dangerous, so it pays **60%** of the XP. Walking
  into one instead gets you the full amount.

**🐜 The bestiary** — giant ants, soldier ants, **ANT HILLS** that never move and just
keep making more, the **MUTANT QUEEN** (bloated, winged, and *faster than you can run*),
the Hopkinsville goblin, the Dover demon, Mothman, the chupacabra, the greys, reptoids,
Loch Ness, sasquatch, dogman, and **THE TALL ONE** — grey, red-eyed, arms past the knees,
and it doesn't charge. It's just closer every time you look up.

**🌒 Day and night** — dusk falls, the outdoors goes moonlit and cold. Your flashlight
cuts a white hole in it; sodium street lamps cut warm orange ones and *flicker*, because
the ones out there always do.

**🎒 Items** — herbs, medkits, shells, the family shotgun, a flashlight that matters, a
brass key, and **PA'S TNT** (it was for stumps). The pack starts *empty* and only lists
what you've actually found, so it never spoils an item you haven't met.

**🎁 The kindness of strangers** — on every map, exactly **one** person is holding
something for you, drawn at random when you walk in. You won't know who until you talk
to them. Resets on the day clock, so you can't farm it by door-scumming.

**💀 Death costs you.** Lose 50% of your XP.

**🌅 The world resets.** Kill something and it *stays* dead — walk out, walk back, still
gone. Then the sun comes up and they've dug their way back out. Bosses die once, forever.

**🎬 Scary cutscenes** — a creeping intro (a light in the sky, a face that lunges once,
words in the static you almost don't read), and a driving sequence down a night highway
that ends the way you know it ends.

**💾 Save games, a pause menu, an options screen** — separate music/SFX volume, fullscreen,
gamepad settings. All of it persists.

---

## 🎵 The music

A **software tracker**, written from scratch, no floats, no libc:

- 🎹 Multiple voices — a bassline *under* a melody
- 📈 **Envelopes** — notes swell and decay instead of switching on and off
- 🌊 Pulse (three widths), triangle, sawtooth, noise
- 〰️ **Vibrato**, so a held note wavers like somebody's playing it
- 🥁 **Pitch slide** — a pitch falling off a cliff is what your ear hears as a *hit*.
  That's the whole trick behind the drum kit.

Songs for the farm, the town, battles, boss fights, the highway, the title, and the end
of the prologue. The whole score is in **A minor** and leans on the flat second and the
tritone — the two most uncomfortable intervals in western music. They never resolve.
That's deliberate.

| | Desktop | ESP32 |
|---|---|---|
| Sample rate | 44.1 kHz | 22.05 kHz |
| Voices | 8 | 4 |

Same songs, same code. A song with more tracks than the machine has voices simply
loses its *last* tracks — which is why every song is ordered **lead → bass → drums →
sweetening**. The little chip keeps the tune and the drums and loses the pad.

---

## 🏛️ The architecture (read this before hacking)

```
src/game/     THE GAME. Portable C99. No OS, no malloc, no files, no floats.
              This code does not know what a computer is.

platform/     One small file per machine. Its whole job:
  desktop/      call game_update(buttons) 60× a second,
  esp32/        put game_framebuffer() on a screen,
  terminal/     feed game_audio_fill() to a speaker.
```

The contract between the two halves is **four functions** in
[game.h](src/game/game.h). A platform layer is ~200 lines. That's why this can run on
an ESP32, a bare-metal Pi, a RISC-V board, or Steam — *port the thin layer, never the
game.*

> ⛔ **The golden rule:** never include an OS header (or SDL, or ESP-IDF) inside
> `src/game/`.
>
> ✅ Run **`make check`** to prove it. It compiles the core `-ffreestanding
> -fno-builtin` — no OS, no libc. Run it often. It is the tripwire that keeps this
> game portable to bare metal.

The platform also owns anything the core can't: **saving** (a file here, NVS there),
**the window**, **gamepads**, **rumble**. Each is a tiny protocol in `game.h` — the core
records what the player *wants* and the platform makes it so. That's why `OPTIONS` shows
a `DISPLAY` row on the PC and not on the handheld: a menu row that does nothing when you
press it is a lie.

---

## 🗺️ Where everything lives

| You want to… | Go to |
|---|---|
| 🎛️ Resolution, walk speed, HP, buttons, **every tunable number** | [config.h](src/game/config.h) |
| 🌗 Day/night length, night darkness, lamp radius | `DAY_LEN_TICKS` etc. in [config.h](src/game/config.h) |
| 🔫 How many shells an enemy takes in the field | `ow_hits` in `species[]`, [assets.c](src/game/assets.c) |
| 🐜 Ant-hill spawn rate, TNT damage, boss chase speed | [config.h](src/game/config.h) |
| 👹 Add an enemy species / NPC look | recipe: "THE CAST" in [assets.h](src/game/assets.h) · tables in [assets.c](src/game/assets.c) |
| 🎒 Add an item | recipe: "THE ITEMS" in [assets.h](src/game/assets.h) |
| 🗣️ Add an NPC + dialog, place enemies, drop items | spawn lists in [assets/maps.h](src/game/assets/maps.h) |
| 🏠 Add a map / an enterable building | recipe at top of [assets/maps.h](src/game/assets/maps.h) |
| 🎨 Sprites, tiles, the font (all ASCII art) | [assets/sprites.h](src/game/assets/sprites.h) · [tiles.h](src/game/assets/tiles.h) · [font.h](src/game/assets/font.h) |
| 🌈 The colour palette | `pal()` at the top of [assets.c](src/game/assets.c) |
| 🎵 Write a song / a sound effect | [audio.c](src/game/audio.c) |
| 🎬 Intro, title, options, pause | [intro.c](src/game/intro.c) |
| 🚶 Walking, camera, doors, dialog, the pack, Zelda mode | [overworld.c](src/game/overworld.c) |
| ⚔️ Battles, game over | [battle.c](src/game/battle.c) |
| 🚐 The driving cutscene, END OF PROLOGUE | [cutscene.c](src/game/cutscene.c) |
| ➕ A whole new scene (shop? map screen?) | recipe at top of [game_internal.h](src/game/game_internal.h) |
| ⌨️ Keyboard & gamepad mapping | [main_sdl.c](platform/desktop/main_sdl.c) |
| 🔌 ESP32 pins / wiring | [esp32_main.c](platform/esp32/main/esp32_main.c) |

Every one of those files opens with a comment explaining its recipe.

---

## ⚡ The 60-second content recipes

**👾 A new enemy** — add `SPECIES_MANTIS` to the enum in `assets.h`, one row of stats in
`species[]` in `assets.c`, then spawn it on any map:
`{ ENT_ALIEN, 12, 7, SPECIES_MANTIS, 0 }`.
Wandering, chasing, ambushes, battles, XP, gore — all automatic.

**🎒 A new item** — an id in the enum, a sprite, one row in `item_info[]`, then drop it:
`{ ENT_ITEM, 12, 16, ITEM_LANTERN, 0 }`. Walking into it pockets it.

**🗣️ A new NPC** — one line in a map's spawn list:
`{ ENT_NPC, 9, 4, LOOK_VILLAGER, "THE PIGS SCREAM AT NOTHING NOW." }`
Write a `~` anywhere in the line and it becomes the player's name.

**🎵 A new song** — an id in the `MUSIC_*` enum, a few note tracks, one row in
`song_table[]`. `NOTE(N_A, 4)` is an A above middle C.

---

## 🖼️ How the art works

Every sprite, tile and map is an **ASCII drawing**, one character per pixel, decoded at
boot. No tools, no binary blobs, no asset pipeline — open the header and draw.

```c
static const char *SPR_ART_TALL_0[16] = {
    "......0aa0......",
    ".....0aaaa0.....",
    ".....0raar0.....",   /* r = red. the eyes. they do not blink. */
    ".....0aaaa0.....",
    "......0aa0......",   /* the neck is too long as well */
    ...
```

Maps are the same idea, one character per 16×16 tile:

```
"T..RRRRR.....................T"     T tree   R roof   # path
"T..HHDHH....,................T"     H wall   D door   . grass
"T....#########################"     <- the road out of the farm
```

🚨 If art is malformed — a row too short, a palette letter that doesn't exist, a spawn
placed inside a wall, a line of dialog too long for its buffer — **the game still boots**
and `game_init()` hands the platform an error count. Mistakes show up as loud magenta
pixels, not as silent corruption.

---

## 🔬 Testing & debugging

- 🧠 **All state lives in one struct**, `G` ([game_internal.h](src/game/game_internal.h)).
  Break anywhere, inspect the entire game.
- 📸 `./build/iknowwhatisaw --frames 600 --shot 300 out.bmp` — run headless and screenshot
  any frame. Works under `SDL_VIDEODRIVER=dummy` for CI.
- 🎲 The core is **deterministic** (fixed timestep + its own RNG): the same inputs replay
  the same game on every platform.

---

## 🔌 The handheld (ESP32)

The same core, unchanged, on an **ESP32-C3 Super Mini**: ST7789 display, buttons on a
PCF8574 I2C expander, sound through a MAX98357A I2S amp, saves in NVS. Pin tables are in
the comment at the top of [esp32_main.c](platform/esp32/main/esp32_main.c).

```sh
cd platform/esp32 && idf.py set-target esp32c3 && idf.py build flash monitor
```

Currently **~68% of the app partition free**. Audio drops to 4 voices at 22 kHz
(see the `target_compile_definitions` in its `CMakeLists.txt`).

⚠️ **Status: not yet run on hardware.** Expect to tweak pins, orientation, colour order
(`BGR`/`RGB`), and the ST7789's `invert_color`.

---

## 🧭 Porting to a new machine

Copy [main_sdl.c](platform/desktop/main_sdl.c) as your template and implement its four
duties: a 60 Hz loop, button reads, a framebuffer blit, an audio sink (optional — silence
is fine). Everything else — saving, the window, pads — is an *optional* protocol you can
ignore.

The core needs **~92 KB of RAM** (77 KB framebuffer + decoded art + game state) and
**no FPU**.

---

## 📋 Known gaps

- 🪟 The Windows `.exe` cross-compiles and links cleanly, but **has not been run on an
  actual Windows machine** yet.
- 🔌 The ESP32 build has **never been flashed to real hardware**.
- 🔊 The handheld doesn't persist its volume settings yet (its NVS store is right there).
- 📖 Part 1 doesn't exist. `CONTINUE` at END OF PROLOGUE starts you over.
