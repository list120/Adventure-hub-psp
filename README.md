# Lost Rune Adventure

A complete, original 2D adventure-platformer for the **PSP**, made to run in
**PPSSPP**.  Collect every rune across three lands, then enter the glowing
gate to finish the level.  Playable start-to-finish with the PSP buttons.

```
D-Pad / Analog Stick ... move
X (cross) .............. jump   (hold = higher, tap = short hop)
O (circle) ............. dash   (works in mid-air too)
START .................. start / pause
```

Three levels:

1. **The Whispering Meadow** - bright hills, water gaps and slimes. 6 runes.
2. **The Frozen Caverns** - floating ice shelves over lava; the ice is
   slippery. 8 runes.
3. **The Cloud Ascent** - a leaf-platform climb into the sky. 5 runes,
   guarded by a spikeball and bats.

When every rune in a level is collected the exit gate opens and unlocks the
next land.  The final gate shows the ending screen.

---

## Why you must build this on a PC (not on the phone)

A `.iso` that PPSSPP can boot must contain a **MIPS executable** (`EBOOT.BIN`)
compiled with the PSP SDK.  This project was authored on an Android device
that has **no compiler and no PSP toolchain**, so the source is finished but
the binary/ISO must be produced on a machine that does have the SDK
(a PC, a Linux box, WSL, or Docker).  The steps below take a couple of
minutes.

---

## Build it

You need the **PSP SDK (pspdev)** on your PATH.  Verify with `psp-config --version`.

### Option A - GitHub cloud build (PC-free, phone-friendly)  **recommended**

You only need a (free) GitHub account and a browser - no PC, no Termux:

1. Create a new GitHub repo (can be private) and upload the contents of this
   `Lost Rune Adventure` folder to it (GitHub's web uploader works from the
   phone; keep the `.github/workflows/build-iso.yml` file intact - see
   `PHONE-BUILD-GUIDE.txt` for a full walk-through if the `.github` folder
   won't upload).
2. Open the repo's **Actions** tab -> **Build Adventure.iso** -> **Run workflow**.
3. When it turns green (~2-4 min), grab `Adventure.iso` from the repo's
   **Releases** section on the right (easiest on a phone), or from the run's
   **Artifacts** list (**Adventure-iso** -> unzip).
4. Copy that ISO to `/storage/emulated/0/Ppsspp rom/` and launch it in PPSSPP.

The same workflow also runs automatically on every push, so future edits of
the source rebuild the ISO for you.

### Option B - native / WSL

```sh
cd psp-adventure
make                 # builds EBOOT.PBP
./build_iso.sh       # builds Adventure.iso
```

### Option C - Docker on a PC (nothing to install but Docker)

```sh
# from the psp-adventure folder
docker build -t lostrune .
docker run --rm -v "$PWD:/src" lostrune
```

Either way you get **`Adventure.iso`** in this folder.

## Put it on the device

Copy `Adventure.iso` into:

```
/storage/emulated/0/Ppsspp rom/
```

Then open PPSSPP, browse to that folder and launch **ADVENTURE**.

(Alternatively, without an ISO: copy the built `EBOOT.PBP` into
`/storage/emulated/0/Ppsspp rom/PSP/GAME/LostRune Adventure/EBOOT.PBP` and
launch it from the PSP *Game* menu.)

---

## Project layout

```
psp-adventure/
  Makefile          standard PSPSDK makefile
  build_iso.sh      compiles + packs the .iso
  Dockerfile        one-command build with the pspdev image
  src/
    main.c          PSP init, input, frame loop
    gfx.c/.h        GU 2D primitives (rects, gradients, circles, lines)
    text.c/.h       5x7 bitmap font drawn as geometry (no texture files)
    game.c/.h       level data, physics, enemies, state machine
    render.c/.h     the whole look: sky, parallax hills, characters, HUD, menus
```

The game uses **only the GU with flat-shaded geometry** - no external image
or sound files - so it builds with the plain PSP SDK and has nothing extra to
ship inside the ISO.

## Tweaking it

- Difficulty / feel: the `#define`s at the top of `src/game.c`
  (`GRAV`, `MOVE_SPD`, `JUMP_V`, `DASH_V`, ...).
- Levels: `load_level_1/2/3` in `src/game.c` (platforms, hazards, runes,
  enemies, scenery).
- Colours: the `RGBA(r,g,b,a)` calls.  Skies/hills are set per level.
