# wasm-bejeweled

A decompiled, moddable reimplementation of **Bejeweled Deluxe 1.87** (PopCap Games, 2001; Steam build)
that runs natively on Windows and in the browser as WebAssembly. You need your own copy of the game: no
PopCap assets are included in this repository or in its builds.

- **Game code** (`src/game`) is a function-by-function C++ translation of the original game classes
  (`Board`, `WinDMApp`, `TitleScreen`, high-score and options dialogs). Every function is annotated with
  its address in the original executable, and gameplay (scoring, RNG order, timing, animations, save and
  high-score file formats) follows the binary.
- **Framework** (`src/SexyAppFramework`) is PopCap's public SexyAppFramework 1.3 with a new SDL2 backend:
  GPU rendering (so mods can ship high-resolution art), a software mixer with libxmp-lite for the
  tracker music, and a 100 Hz fixed update loop identical to the original.
- **Mods** (`src/platform`) overlay any game file, add HD images, patch single sprites, remap music and
  sounds, change strings and gameplay tunables, and keep separate high scores when gameplay changes.

## Getting the game files

The port reads the `images`, `data`, `sounds`, `music` and `properties` folders of an installed copy.

- **Native**: the executable finds a Steam install automatically, or run it with
  `-gamedir="<Bejeweled Deluxe folder>"`. For development, `python tools/stage_assets.py` copies those
  folders into `assets/` (git-ignored).
- **Web**: the page asks for the game folder (or a .zip of it) on first start and copies the needed files
  into the browser's storage. Nothing is uploaded.

## Building

Toolchain (all free): [llvm-mingw](https://github.com/mstorsjo/llvm-mingw) (i686 target), SDL2 MinGW
development package, [Emscripten](https://emscripten.org) 6.x, CMake 3.20+ and Ninja.

```bash
tools/build.sh native   # build/native/bejeweled.exe
tools/build.sh web      # build/web/bejeweled.html (+ .js, .wasm)
```

Edit the paths at the top of `tools/build.sh` (or set `SDL2_DIR`, `EMSDK_ROOT`, `LLVM_MINGW_ROOT`) to
match your installation. To try the web build locally, serve `build/web` over HTTP, e.g.
`python -m http.server 18765 --directory build/web`, and open `bejeweled.html`.

On the web, the game files, mods and saves stay in the browser's storage for that site (IndexedDB).
Saving works as in the original: quitting saves a game in progress, and the next start offers
"Continue old game?". Quit Game closes the game and offers Play again. A browser tab can also be
closed, reloaded or crash without the game hearing about it. So the page also keeps an up-to-date
copy, in localStorage, of what quitting would save at that moment, and the next start restores it.

Command-line options (native; the web shell passes the equivalent paths):

| Option | Meaning |
|---|---|
| `-gamedir=<dir>` | game data folder |
| `-userdir=<dir>` | saves, settings (`registry.ini`) and `mods.json` |
| `-modsdir=<dir>` | mods folder (default: `mods` next to the executable) |
| `-nomods` | start without mods |
| `-seed=<n>` | fixed random seed (default: time based, like the original) |
| `-script=<file>`, `-fast`, `-trace` | scripted input and screenshots for automated tests |

## Mods

A mod is a folder in the mods directory. It contains files at the same paths the game uses
(`images/`, `sounds/`, `music/`, `data/`, `properties/`), plus an optional `mod.json`. When two mods
change the same file, the one higher in the load order wins. On the web, use the **Mods** panel to
add mod folders or .zip files, enable/disable them and change their order. Natively, edit
`<userdir>/mods.json` (`{"order": [...], "disabled": [...]}`); new mods are enabled automatically.

```jsonc
{
  "id": "my-mod", "name": "My Mod", "version": "1.0", "author": "you",
  "description": "What it changes",

  // HD art: put images/gemsheet6@2x.png next to (or instead of) the 1x file, or declare a scale
  "imageScale": 2,
  "images": { "images/gemsheet6": { "scale": 2 } },

  // replace part of an image (here: one gem's two animation rows) without shipping the whole sheet
  "imagePatches": [ { "target": "images/gemsheet6", "image": "patches/gem@2x.png", "x": 0, "y": 520 } ],

  // play other files for game paths; music can start/loop anywhere and use Ogg instead of S3M
  "files": {
    "music/1984-network.s3m": { "file": "music/track.ogg", "start": 0, "loopStart": 12.5, "volume": 0.8 },
    "sounds/go1.ogg": "voice/go.ogg"
  },

  // replace on-screen text (ids from properties/default.xml), or list extra properties files
  "strings": { "EXCELLENT": "AWESOME!" },

  // gameplay tunables (see src/game/GameConfig.h); mods that use them get their own high-score table
  "tunables": { "scoring": { "hintPenalty": 25, "setValues": [10, 20, 30, 40, 60, 80, 110, 160, 210] },
                "board": { "gemColors": 6 } }
}
```

Images follow the SexyAppFramework rules: `name.png`/`.jpg`/`.gif` with an optional `_name.*` alpha mask.
A mod's mask is only applied to the mod's own image, and a PNG with transparency needs no mask.

### Included mods and generators

| Mod | How to get it |
|---|---|
| `example-crystal-gems`: new HD gem art (original artwork) | `python tools/make_example_mods.py` |
| `example-voice-lines`: string overrides | same script |
| `example-relaxed`: cheaper hints, separate high scores | same script |
| `mines-trilliant`: orange gem as a trilliant cut shaped like the Colorado School of Mines triangle | `python tools/make_trilliant_mod.py` |
| `true-gem`: every gem becomes a badly recoloured "True..." meme card, the logo reads BE TRUE LED, EXCELLENT! becomes TRUE... and NO MOVES! becomes NO TRUE... | `python tools/make_true_mod.py --image <picture>` |
| `hd4x-textures`: every image at 4x, upscaled from your own game files with Real-ESRGAN (needs [realesrgan-ncnn-vulkan](https://github.com/xinntao/Real-ESRGAN/releases) and a Vulkan GPU). Shows its detail in a large window or fullscreen | `python tools/make_hd4x_mod.py` |
| `bejeweled2-audio`: Bejeweled 2's music and sound effects (needs your Bejeweled 2 Deluxe install and an ffmpeg build with libopenmpt) | `python tools/make_bej2_audio_mod.py` |

Mods built from other games' files or third-party pictures are generated locally and git-ignored.

When any mod is enabled, two small signs show it: hints get a fourth sparkle, and the G in banner text
(GO!, ...) gets a crossbar.

HD art (`name@2x.png` .. `name@4x.png`) also reaches images the game builds at run time, such as the
board background, the banner text and the shrinking gems. Software drawing repeats each draw on the HD
pixels whenever HD art is involved (`MemoryImage::BeginHDMirror`).

Planned gameplay mods (board shapes, other move types, special gems, undo, replays, ...) are listed in
[docs/MOD_ROADMAP.md](docs/MOD_ROADMAP.md).

## How it was reverse engineered

See [docs/RE_NOTES.md](docs/RE_NOTES.md). In short:

1. `WinBej.exe` is PopCap's DRM wrapper; the real game is an unencrypted PE appended to it.
   `python tools/extract_game_exe.py` carves it out to `re/bin/BejeweledGame.exe`.
2. Ghidra 12 analyses it headlessly, recovers the C++ classes from RTTI and exports every decompiled
   function (`tools/ghidra_scripts/ExportAll.java`, run by `tools/ghidra_headless.sh`).
3. The framework functions the game calls were identified against the public SexyAppFramework source
   ([docs/FRAMEWORK_NOTES.md](docs/FRAMEWORK_NOTES.md)); the game classes were analysed into a behaviour
   spec ([docs/GAME_NOTES.md](docs/GAME_NOTES.md)). Recovered names are applied back to Ghidra with
   `tools/ghidra_scripts/ApplySymbols.java` (`re/symbols/*.jsonl`).
4. The game code was translated against that spec and checked function by function against the
   decompilation and disassembly.

`re/` (decompiler output) is git-ignored; regenerate it with the tools above. The research behind the
approach (comparable projects such as Zuma's CircleShootApp and PvZ-Portable) is summarized in
[docs/RESEARCH_SYNTHESIS.md](docs/RESEARCH_SYNTHESIS.md).

## Licenses and credits

- This product includes portions of the PopCap Games Framework, © 2005-2009 PopCap Games, Inc. All rights
  reserved. (http://popcapframework.sourceforge.net/). See `src/SexyAppFramework/LICENSE.txt`.
- Bejeweled is a trademark of Electronic Arts / PopCap Games. This project is not affiliated with or
  endorsed by them. The game's code, art, music and sounds remain their property; use your own copy.
- [SDL2](https://libsdl.org) (zlib license), [libxmp-lite](https://github.com/libxmp/libxmp) (MIT),
  [stb_image / stb_vorbis / stb_image_write](https://github.com/nothings/stb) (public domain / MIT).
