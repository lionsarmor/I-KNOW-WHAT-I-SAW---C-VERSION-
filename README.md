# 🛸 I KNOW WHAT I SAW

> **October 3rd, 1987.** The lights came to the farm.
> Your crops are flat in circles. The cows won't go near the pond.
> The dog has been barking at the sky all week and he is not sorry.

A retro alien-abduction RPG in **portable C99** — the *same game code* runs on Windows,
Linux, **Android**, **in a browser**, in your terminal, and on a $4 microcontroller. 👽

One core. Six platforms. `src/game/` doesn't know what a computer is.

---

## 🎮 Play it

```sh
sudo apt install build-essential libsdl2-dev     # or: brew install sdl2
make run
```

…or `make serve` and play it **in a browser** at `http://localhost:8000`.

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
| `make zip` | 🗜️ ...packed into one file you can drop into Discord |
| `make apk` | 📱 **an Android APK** → `dist/iknowwhatisaw.apk` |
| `make apk-install` | 📱 ...and push it straight to a plugged-in phone |
| `make android-sdk` | ⚙️ one-time: the whole Android toolchain into `~/android-tools` |
| `make web` | 🌐 **WebAssembly** → `dist/web/` — playable in any browser |
| `make serve` | 🌐 ...and serve it at `http://localhost:8000` |
| `make emsdk` | ⚙️ one-time: Emscripten into `~/emsdk` |
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

## 🌐 The web (WebAssembly)

```sh
make emsdk     # once: Emscripten into ~/emsdk
make serve     # build + http://localhost:8000
```

Drop `dist/web/` on any static host — GitHub Pages, itch.io, Netlify — and the game is a
**link**, not a download. ~940 KB total (750 KB of it the WebAssembly itself).

Same `src/game`, same `main_sdl.c`. Emscripten ships SDL2, so `-sUSE_SDL=2` is most of
the "port". Saves persist in **IndexedDB**, keyboard and gamepad both work.

Three things had to be got right, and each of them is a trap:

⏱️ **The browser calls you at the MONITOR's refresh rate, not at 60Hz.**
`requestAnimationFrame` fires once per display frame — 144 times a second on a 144Hz
screen. This game is a
fixed-timestep machine where one `game_update()` *is* one 60th of a second, so letting the
browser drive it directly made it run at **2.4× speed** on exactly the monitors gamers own.
`frame_step()` now only does work when a 60th of a second has really elapsed.

💾 **The web's filesystem is a lie.** Emscripten's FS is in RAM: `fopen`/`fwrite` succeed,
and the save evaporates on reload. The web build mounts **IDBFS** (IndexedDB) at `/save`
and syncs after every write. Verified by saving, reloading the page, and getting the save
back.

🚪 **A browser tab cannot close itself.** So the core learned a new capability
(`game_enable_quit(0)`) and the pause menu drops `QUIT` and offers `SAVE GAME` instead of
`SAVE AND QUIT`. A menu row that does nothing when you press it is a lie.

> Memory is **fixed, not growable**, deliberately: `ALLOW_MEMORY_GROWTH` makes Emscripten
> hand JS a *resizable* `ArrayBuffer`, which current Chrome's `TextDecoder` flatly refuses
> to decode — the runtime throws on its first string and the canvas stays black. This game's
> footprint is fixed and known, so it just asks for what it needs up front.

---

## 📱 Android

```sh
make android-sdk     # once: JDK + SDK + NDK into ~/android-tools (~2.6GB, no sudo)
make apk             # -> dist/iknowwhatisaw.apk
make apk-install     # ...and push it to a plugged-in phone via adb
```

Ships all four ABIs (arm64, armv7, x86, x86_64), so it runs on phones, tablets and
Android handhelds alike.

**`src/game/` did not change by one line to run on a phone.** The core still compiles
freestanding. On-screen controls live entirely in
[platform/android/touch.c](platform/android/touch.c) — the platform turns fingers into
the same `BTN_*` bits a keyboard makes, and the core never learns what a finger is.

👍 **The controls sit in the letterbox bars, not on the game.** The picture is 3:2 and a
phone is much wider, so once it's scaled to fit there's empty space down each side —
that's where your thumbs go. Nothing covers the picture. The d-pad's *touch* radius is
much bigger than the circle it draws, because a d-pad you have to hit exactly is a d-pad
that feels broken.

🎮 A Bluetooth gamepad works on Android too, through the same SDL layer as the desktop.
The Android **back gesture** is this platform's ESC — it opens the pause menu.

> ⚠️ **The project path must not contain spaces** — `ndk-build` *is* GNU make, and GNU
> make cannot handle them. `make apk` works around this by mirroring the sources into a
> space-free staging tree (`~/.cache/ikwis-apk`) and building there, so it doesn't matter
> what your folder is called.

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

**🌧️ The sky does its own thing.** Rain rolls in on its own clock — the daylight goes out
of the world, streaks fall fast and sideways, and every so often **lightning** splits the
whole screen white. And rarely, a **TORNADO** walks across the map: a dark funnel, narrow
where it's chewing the ground and widening as it climbs out of frame, dragging debris and
shaking the earth the entire time it's on the map. It's drawn in *world* coordinates — it's
a thing standing in the field, not a decal on the screen, and it slides past as you walk.

**🔦 Something is looking for her.** In the grocery-store car park, **searchlight beams**
wander the tarmac — on for a stretch, then gone for longer. They don't sweep in a tidy
back-and-forth; two different periods on each axis means the pool doubles back and lingers,
so it reads as *hunting* rather than as a machine doing a pattern. At night they cut cold
white holes in the dark. The girl went out at nine to bring the trolleys in.

**💥 The screen jolts when you're hit.** Scaled by the damage, decaying to nothing over a
few frames — a shake that stays at full strength for its whole life reads as a broken
renderer, not a punch. Dynamite shakes it harder. In the overworld it moves the **camera**,
not the renderer, so the world lurches while the HUD you're reading stays rock-steady.

**🎒 Items** — herbs, medkits, shells, the family shotgun, a flashlight that matters, a
brass key, and **PA'S TNT** (it was for stumps). The pack starts *empty* and only lists
what you've actually found, so it never spoils an item you haven't met.

**🎁 The kindness of strangers** — on every map, exactly **one** person is holding
something for you, drawn at random when you walk in. You won't know who until you talk
to them. They still say **their own written line first** — a random gift never costs you
a piece of the story. Resets on the day clock, so you can't farm it by door-scumming.

**💀 Death costs you.** Lose 50% of your XP — and **you have to sit and read why.** The
death screen accepts no input until all four lines are out (`GAMEOVER_READ_TICKS`); the
PRESS START prompt appears at exactly the tick the button starts working, never before.
And **THE TALL ONE is standing behind the words** — seven times life size, fading up so
slowly you're not sure it's there until it is, flickering out for a frame now and then.
It never gets anywhere near lit. You didn't see it in the fight. You're seeing it now.

**🌅 The world resets.** Kill something and it *stays* dead — walk out, walk back, still
gone. Then the sun comes up and they've dug their way back out. Bosses die once, forever.

**🎬 Scary cutscenes** — a creeping intro (a light in the sky, a face that lunges once,
words in the static you almost don't read), and a driving sequence down a night highway
that ends the way you know it ends.

**📖 You cannot mash through the words.** A does nothing until a page has finished typing
itself out — *then* it turns the page. Everyone mashes the confirm button, and the game
used to let them blow through every line in it without reading one. Battle text has a
minimum time on screen for the same reason (`MSG_MIN_TICKS`), and the intro skips on
**START only**, never on A. The prompt in the corner appears exactly when the button
starts working, so the game never lies about what it's listening for.

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

| | Desktop / Web / Android | ESP32 |
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

platform/     One small layer per machine. Its whole job, forever:
                1. call game_update(buttons) 60× a second
                2. put game_framebuffer() on a screen
                3. feed game_audio_fill() to a speaker

  desktop/    SDL2 — Windows, Linux, macOS
  android/    SDL2 + touch.c   (an APK)
  web/        SDL2 via Emscripten   (WebAssembly)
  esp32/      bare metal — SPI display, I2S amp, NVS saves
  terminal/   stdout. Coloured ASCII. No graphics library at all.
```

Desktop, Android and the **web all share `main_sdl.c`** — SDL papers over the rest. Android
adds `touch.c` (a phone has no buttons, so we draw some); the web adds a 60Hz gate and
IndexedDB saves. Everything else is `#ifdef`. The terminal build proves the point from the
other end: a "screen" can be stdout printing coloured ASCII.

Platforms also **declare what they can do**, and the menus follow — the ESP32 has no
resizable window, the web has no way to quit. `game_enable_display_menu()`,
`game_enable_controls_menu()`, `game_enable_quit()`. A menu row that does nothing when you
press it is a lie.

The contract between the two halves is **four functions** in
[game.h](src/game/game.h). The leanest platform layer —
[main_term.c](platform/terminal/main_term.c), which draws the game as ASCII in a
terminal — is **~290 lines**. That's the real cost of a new machine. (`main_sdl.c` is
679, but it serves *three* platforms and does gamepads, touch, settings and saves on top
of the four duties.)

*Port the thin layer, never the game.*

> ⛔ **The golden rule:** never include an OS header (or SDL, or ESP-IDF) inside
> `src/game/`.
>
> ✅ Run **`make check`** to prove it. It compiles the core `-ffreestanding
> -fno-builtin` — no OS, no libc. Run it often. It is the tripwire that keeps this
> game portable to bare metal.

The platform also owns anything the core can't: **saving** (a file here, NVS there, IndexedDB
in a browser), **the window**, **gamepads**, **rumble**. Each is a tiny protocol in
`game.h` — the core records what the player *wants*, and the platform makes it so.

---

## 🗺️ Where everything lives

| You want to… | Go to |
|---|---|
| 🎛️ Resolution, walk speed, HP, buttons, **every tunable number** | [config.h](src/game/config.h) |
| 🌗 Day/night length, night darkness, lamp radius | `DAY_LEN_TICKS` etc. in [config.h](src/game/config.h) |
| 🔫 How many shells an enemy takes in the field | `ow_hits` in `species[]`, [assets.c](src/game/assets.c) |
| 🐜 Ant-hill spawn rate, TNT damage, boss chase speed | [config.h](src/game/config.h) |
| 📖 How long battle text must stay up before A works | `MSG_MIN_TICKS` in [config.h](src/game/config.h) |
| 💀 How long the death screen holds you | `GAMEOVER_READ_TICKS` in [config.h](src/game/config.h) |
| 💥 Screen-shake strength / duration | `SHAKE_*` in [config.h](src/game/config.h) |
| 🌧️ How often it rains, how rare a tornado is | `WX_*` in [config.h](src/game/config.h) |
| 🔦 Searchlight size and how often they're on | `SEARCH_*` in [config.h](src/game/config.h) |
| 🚐 The van (two of them: `van_top` from above, `van_big` from behind) | [sprites.h](src/game/assets/sprites.h) |
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
| 📱 On-screen touch controls | [touch.c](platform/android/touch.c) |
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

Most are 16×16, but they don't have to be: the **van seen from above** (`SPR_ART_VANTOP`)
is 48×64 — three tiles wide, four long, drawn 1:1 so its pixels match the tarmac it's
parked on. There is a *second*, different van (`SPR_ART_VAN_BIG_*`, 32×32) seen from
**behind**, for the highway cutscene where you're staring at its back doors while a light
comes down. **They are not interchangeable** and the game looks ridiculous if you swap
them — I know, because I did it, and put the arse-end of the van on the tarmac as seen
from the sky.

> 💡 **For anything big, generate the art rather than typing it.** 64 rows of exactly 48
> characters is a guaranteed off-by-one. Compose it from rectangles in a throwaway script,
> render a PNG preview, *then* paste it into the header. `game_init()` validates every row
> length and palette letter at boot and hands the platform an error count, so a mistake is
> loud rather than silent — but it's faster not to make one.

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
is fine). Everything else — saving, the window, pads, rumble — is an *optional* protocol
you can ignore.

### 📏 The budget

| | |
|---|---|
| Code | **99 KB** |
| Const data (art, maps, songs) | **21 KB** |
| **RAM** | **140 KB** — 75 KB framebuffer + ~60 KB decoded art + game state |
| Needs | no FPU, no malloc, no libc, no files |

> Measure it yourself, don't trust this table: `make check && size -t build/check/*.o`.
> The `bss` column is the RAM. It creeps up every time somebody draws a big sprite — the
> 48×64 top-down van alone costs 6 KB of decoded pixels.

That budget is what decides whether a machine is a **port** or a **rewrite**. Anything with
~140 KB of RAM and a linear framebuffer is a port (a new `platform/` file, core untouched).
Anything without a framebuffer — a C64, an NES — is a rewrite of `gfx.c`, not a port. Be
honest about which one you're doing.

Fun fact: **240×160 is the Game Boy Advance's screen**, and GBA mode 3 is a 240×160×16bpp
linear framebuffer — *exactly* `gfx_fb`, and it fits in the GBA's 96 KB of VRAM with the
rest landing in its 256 KB EWRAM. That target has been sitting there since day one.

---

## 📋 Known gaps

- 🪟 The Windows `.exe` cross-compiles and links cleanly, but **has not been run on an
  actual Windows machine** yet.
- 📱 The APK builds and verifies (all four ABIs, correct launch activity, debug-signed),
  but **has not been installed on a real phone**. The on-screen controls in particular
  will want tuning by thumb — that's the sort of thing you cannot get right on a desk.
- 🌐 The web build is **tested and working** (Chromium, real GPU: intro, title, input,
  audio, save → reload → save restored). Not yet tried in Firefox or Safari.
- 🔌 The ESP32 build has **never been flashed to real hardware**.
- 🔊 The handheld doesn't persist its volume settings yet (its NVS store is right there).
- 📖 Part 1 doesn't exist. `CONTINUE` at END OF PROLOGUE starts you over.
