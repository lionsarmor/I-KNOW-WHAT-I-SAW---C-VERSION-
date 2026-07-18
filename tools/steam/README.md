# Shipping the demo on Steam

Your desktop build is already Steam-shaped: Steam launches a plain executable
out of a folder, and `make dist` produces exactly that —
`iknowwhatisaw.exe` + `SDL2.dll` (Windows) and `iknowwhatisaw` + `lib/` + `run.sh`
(Linux). Saves already go to the per-user dir via `SDL_GetPrefPath()`, so nothing
in the game code needs to change to run from a read-only Steam install.

This folder automates the **upload**. The rest is Steamworks-backend clicking
that only you can do. Checklist below.

---

## What only you can do (Steamworks backend)

- [ ] **The demo is its own app with its own App ID.** A demo is *not* a branch of
      the main game — it's a separate app, with its own depots, that you associate
      with the parent game so a **Download Demo** button appears on the game's
      store page. Demos are always **free**.
- [ ] **One content depot** (`4985931`, the auto-created `AppID+1`). Both Windows
      and Linux files ship in it under `windows/` and `linux/` subfolders — at
      ~4 MB total it's not worth splitting. `upload.sh` already targets this depot.
- [ ] **Set OS launch options** on the demo app (*Installation → General
      Installation → Launch Options*), two entries against the one depot:
      - Windows → executable `windows/iknowwhatisaw.exe`, OS filter **Windows**
      - Linux → executable `linux/iknowwhatisaw`, OS filter **Linux**
      (Point the Linux launch at the binary directly — the `$ORIGIN/lib` rpath
      finds the bundled SDL. `linux/run.sh` works too but isn't needed.)
- [ ] **Store page** for the demo app (*Store Presence*): short + full
      description, at least 5 screenshots, a capsule image set (see sizes below),
      tags, and the **content-rating questionnaire**. Header capsule 460×215,
      small capsule 231×87, main capsule 616×353, plus library art if you want it.
- [ ] **Content survey / maturity questionnaire** — required before review.
- [ ] **Submit for review** — Valve reviews both the store page *and* a build that
      is set live on the default branch. Turnaround is typically a few business days.
- [ ] After approval, **set a release date / hit release**, and on the *parent*
      game app enable the demo association so the button shows up.

---

## What this folder does (the upload)

1. Fill in `steam.conf` — `APPID` + `DEPOT_CONTENT` are already set to your demo's
   IDs; you just add `STEAM_USER` (your Steam account **username**, not email).
2. Install `steamcmd` on the upload machine (this repo's box is fine; it builds
   Windows via mingw + the vendored SDL):
   ```sh
   sudo add-apt-repository multiverse && sudo apt install steamcmd
   # or download from https://developer.valvesoftware.com/wiki/SteamCMD
   ```
3. **Dry run first** — builds, stages both platforms, generates + validates the
   VDFs, uploads nothing:
   ```sh
   tools/steam/upload.sh --preview
   ```
4. **Real upload** — the build lands in Steamworks but stays **off every branch**:
   ```sh
   tools/steam/upload.sh
   ```
   (First run prompts for Steam Guard once, then caches the session.)
5. **Promote it**: *SteamPipe → Builds* → pick your new build → set it live on the
   `default` branch (or a beta branch first, e.g. `--branch demo`, to test).

### Flags
| flag | effect |
|---|---|
| `--preview` / `-p` | build + stage + validate, upload nothing |
| `--branch NAME` | auto-promote the upload to branch `NAME` (skip the manual step) |
| `--no-build` | reuse whatever's already in `dist/`, don't recompile |

Everything the script generates lands in `build/steam/` (gitignored):
`content/{windows,linux}/` staged payloads, `scripts/*.vdf` the concrete build
scripts, `output/` steamcmd's manifests and logs.

---

## Store art (capsules)

`python3 tools/make_capsules.py` renders the store/library art from the game's
own assets (the UFO sprite, the 8x8 font, the game palette) into
`dist/steam/capsules/` (gitignored — regenerate any time). These are uploaded by
hand in the Steamworks web UI (*Store Presence → Graphical Assets*), not by
SteamPipe. What goes where:

| file | size | Steamworks slot |
|---|---|---|
| `header_460x215` | 460×215 | Header capsule (store + library header) |
| `small_231x87` | 231×87 | Small capsule (search, lists) |
| `main_616x353` | 616×353 | Main capsule (featured/carousel) |
| `vertical_374x448` | 374×448 | Vertical capsule |
| `library_600x900` | 600×900 | Library capsule (portrait grid) |
| `library_hero_1920x620` | 1920×620 | Library hero |
| `page_background_1920x1080` | 1920×1080 | Page background |
| `community_icon_184` | 184×184 | Community icon |

They're clean, on-brand, and good enough to launch a demo with — but they're
programmer-art placeholders. Swap in a real trailer + screenshots + hand-drawn
capsules before you'd want the game *featured*. The app icon in the .exe comes
from the same UFO via `tools/make_icon.py`.

---

## Gotchas worth knowing

- **Promote deliberately.** `steam.conf`'s `BRANCH` is empty on purpose so a script
  run can never silently push a broken build to live players. Promote in the web UI,
  or use `--branch` only when you mean it.
- **The default branch needs a live build to pass review** — upload, then set live,
  *then* submit.
- **Executable bit**: because you upload from Linux, SteamPipe keeps `+x` on the
  Linux binary. If you ever upload from Windows, Steam can't and the Linux launch
  fails — so always run `upload.sh` from Linux/macOS.
- **Version bumps**: `make dist` reads `VERSION` from the `Makefile` (currently
  `0.1.0`); that string goes into the Steam build description automatically.
