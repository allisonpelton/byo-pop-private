# byo-pop

**Bring Your Own Pop[Cap]**: decompilations, SDL2/WebAssembly ports and mods of PopCap's classic
casual games. Each game is reimplemented from its original executable, function by function, on a
shared portable copy of PopCap's SexyAppFramework, and runs natively on Windows and in the browser.

Repository: https://github.com/allisonpelton/byo-pop

## Bring your own game files

No PopCap assets are included in this repository or in anything it builds. Every game needs your own
installed copy (for example from Steam):

- **Native**: each game finds its Steam install by itself, or takes `-gamedir=<folder>`. For
  development, a staging script per game copies the needed folders into `assets/<game>/`.
- **Web**: the page asks for the game folder (or a .zip of it) on first start and copies the files it
  needs into the browser's storage. Nothing is uploaded.
- **Your saves**: a game started with `-importsaves` (or `-importsaves=<install folder>`) brings the saves, high scores,
  players and settings of the original installed on this computer into its user folder (`%APPDATA%/byo-pop/<Game
  Name>`), reads the original's registry and folders only and exits; the web listing page's "Your saves" section does
  the same from `reg export` files and a folder you choose (`docs/WEB_PLAYER.md`, "Importing saves from an installed
  game").
- Mods that are built from game files (HD upscales, other games' audio) are generated locally by
  scripts and never committed. The decompiler output in `re/<game>/` is regenerated locally the same
  way.

`assets/`, `re/`, `build*/` and the generated mods are git-ignored; keep it that way.

## Games

### Progress (2026-10-26)

| Game | Model | Status | On main since | Mods | Port work | Mod work | Other | Total |
|---|---|---|---|---:|---:|---:|---:|---:|
| [Bejeweled Deluxe](games/bejeweled/README.md) | Opus | Done, with graphics, sound, text and tunables mods and 18 stages of code mods (board shapes, gravity, replays, up to 12 gem colours, new gems) with curated modes | 2026-10-18 | 32 | $251 | $1,268 | $11 | $1,530 |
| [Big Money Deluxe](games/bigmoney/docs/README.md) | Opus | Done, with mods: replays, LiveSplit, a puzzle solver, board and money variants | 2026-10-18 | 11 | $263 | $129 | $8 | $400 |
| [BookWorm Deluxe](games/bookworm/docs/README.md) | Opus | Done, with mods: undo, a word solver, replays, a daily board, a word journal, widescreen, LiveSplit, more word lists | 2026-10-19 | 18 | $369 | $149 | $12 | $531 |
| [Noah's Ark Deluxe](games/noahsark/docs/README.md) | Sonnet | Done, with mods: undo, a move helper, tide speed, a daily board | 2026-10-25 | 4 | $62 | $5 | | $67 |
| [Seven Seas Deluxe](games/sevenseas/docs/README.md) | Sonnet | Done | 2026-10-25 | | $54 | | | $54 |
| [Atomica Deluxe](games/atomica/docs/README.md) | Sonnet | Done | 2026-10-25 | | $105 | | | $105 |
| [Mummy Maze Deluxe](games/mummymaze/docs/README.md) | Sonnet | Done | 2026-10-25 | | $84 | | | $84 |
| [TipTop Deluxe](games/tiptop/docs/README.md) | Opus | Done | 2026-10-25 | | $106 | | | $106 |
| [Bookworm Adventures Deluxe](games/bookwormadv/docs/README.md) | Opus | Done, with PopLua, PopAnim, Flash cutscenes and JPEG 2000 | 2026-10-25 | | $538 | $2 | | $539 |
| [Ning Po Mahjong Deluxe](games/ningpo/docs/README.md) | Opus | Done | 2026-10-26 | | $66 | | | $66 |
| [Pixelus Deluxe](games/pixelus/docs/README.md) | Sonnet | Done | 2026-10-26 | | $91 | | | $91 |
| [Alchemy Deluxe](games/alchemy/docs/README.md) | Sonnet | Done | 2026-10-26 | | $70 | | | $70 |
| [Word Harmony](games/wordharmony/docs/README.md) | Opus | Done, on Sprout's first-generation engine | 2026-10-26 | | $115 | | | $115 |
| [Cosmic Bugs](games/cosmicbugs/docs/README.md) | Opus | Done, on Retro64's engine | 2026-10-26 | | $125 | | | $125 |
| [Water Bugs](games/waterbugs/docs/README.md) | Sonnet | Done, on Cosmic Bugs' engine | 2026-10-26 | | $45 | | | $45 |
| [Rocket Mania Deluxe](games/rocketmania/docs/README.md) | Sonnet | Done | 2026-10-26 | | $134 | | | $134 |
| [Dynomite Deluxe](games/dynomite/docs/README.md) | Opus | Done | 2026-10-26 | | $164 | | | $164 |
| [Bookworm Adventures Volume 2](games/bookwormadv2/docs/README.md) | Sonnet | Done, on Bookworm Adventures' engine | 2026-10-26 | | $195 | | | $195 |
| Bejeweled Twist, Mahjong Escape: Ancient Japan and Ancient China, Bonnie's Bookstore Deluxe, Venice Deluxe, Heavy Weapon Deluxe, Hammer Heads Deluxe, Bejeweled 2 Deluxe, AstroPop Deluxe | Opus or Sonnet | Waiting for a slot, in this order | | | under $1 each | | | |
| Shared framework, tools and coordination | | | | | $373 | $156 | $191 | $720 |
| **All** | | | | **65** | **$3,211** | **$1,709** | **$223** | **$5,143** |

"Done" means the whole game plays natively and in the browser. The costs are what the work's tokens would cost at the
API prices in [docs/TOKEN_ECONOMY.md](docs/TOKEN_ECONOMY.md), each model at its own; the subscription the work ran on
does not bill them. A game's cost counts its own sessions with their subagents and workflow agents, and the
coordinator's reviews, merges and fixes of that game. Until 2026-10-20 one session ported Bejeweled and set up the
repository, and its work counts for Bejeweled. Port and mod work are split commit by commit: each commit gets the cost
of the work since the one before, classified by what it did. "Other" is publishing, the website, licences, usage
measurement and rules for the sessions. `tools/game_costs.py` computes the costs; the table is refreshed when asked.

Later candidates, and what it takes to port each, are surveyed in
[docs/POPCAP_GAMES_SURVEY.md](docs/POPCAP_GAMES_SURVEY.md); planned mods are in
[docs/MOD_ROADMAP.md](docs/MOD_ROADMAP.md).

## Mods

Every game loads mods the same way (`platform/ModManager.h`). A mod is a folder of files at the game's own paths
(`images/`, `sounds/`, `music/`, `data/`, `properties/`), plus an optional `mod.json`. Each game's README lists
what its mods can change.

- **The mod menu.** Press **F2** in any game, natively or in the browser. It lists the installed mods in load
  order. There you can turn each one on or off, move it, and change the settings a mod offers. It also shows
  each mod's errors and what is still waiting. Every change is saved at once. **Restart game** applies
  changes that need a restart; with a game in progress it asks first, and the game is saved as on Quit.
- **Native.** Mods are folders in the mods folder (`-modsdir=<dir>`, default `mods` next to the game).
  `-nomods` starts once without them. The menu can then turn off a mod that stops the game from starting, and
  Restart game brings the mods back.
- **Web.** The page's **Mods** button opens the menu while the game runs. Before the game starts, it opens a
  panel that adds mods (a folder or a .zip) and removes them. Inside the game, the menu's **Add or remove
  mods** opens the same panel. The panel also offers **Play without mods** when a mod stopped the game from
  starting.
- **What is stored.** The menu writes two files in the user dir, next to the saves:
  - `mods.json`: the order and the mods turned off;
  - `mod-settings.json`: the values that differ from their defaults.

  Both can be edited by hand while the game is closed. Keys the game does not know are kept.
- **Scores.** Mods and values that change play give the player separate high scores and a separate saved game.
  The menu marks them SCORES.

How a mod declares settings, and how their values reach its data, is in
[docs/MOD_MENU.md](docs/MOD_MENU.md); the design is in [docs/MOD_MENU_DESIGN.md](docs/MOD_MENU_DESIGN.md).

## Layout

```
framework/         portable SexyAppFramework 1.3 subset with an SDL2 backend (shared)
platform/          VFS, audio mixer, mod manager and mod settings, image loader, JSON, browser save
                   backup, the web page's C functions, Steam install lookup (shared)
third_party/       stb, libxmp-lite
cmake/             the shared CMake toolchain file (llvm-mingw, i686) and the web link options
web/               the HTML shell the web builds use, and modpanel.js, every page's Mods panel (shared)
tools/             shared pipeline: build.sh, Ghidra scripts, symbol merging, .pak reader, rundesk
                   (invisible test runs), fwtest, smoke, publish.py (docs/PUBLISHING.md)
docs/              notes shared by every game: framework identification, games survey, mod roadmap,
                   mod settings and the mod menu, research
games/<game>/      one game: src/ (the translated game code), docs/, mods/, tools/, CMakeLists.txt,
                   README.md
assets/<game>/     your staged game files (git-ignored)
re/<game>/         Ghidra exports of your game's executable (git-ignored)
build/             native/, web/, web-test/, native-novariants/ (git-ignored)
```

## Building

Toolchain (all free): [llvm-mingw](https://github.com/mstorsjo/llvm-mingw) (i686 target), the SDL2
MinGW development package, [Emscripten](https://emscripten.org) 6.x, CMake 3.20+ and Ninja. One CMake
build makes every game's targets.

```bash
tools/build.sh native       # build/native: bejeweled.exe, the tests and tools
tools/build.sh web          # build/web: bejeweled.html (+ .js, .wasm)
tools/build.sh novariants   # build/native-novariants: Bejeweled without its code-mod variant paths
tools/build.sh web-test     # build/web-test: the web build automated browser tests use
```

Edit the paths at the top of `tools/build.sh` (or set `SDL2_DIR`, `EMSDK_ROOT`, `LLVM_MINGW_ROOT`) to
match your installation. A web build packs Bejeweled's staged files into the fwtest (and smoke) test
pages, so it leaves those out until `assets/bejeweled/` exists; configure again after staging.

[docs/BUILDS.md](docs/BUILDS.md) lists every folder under `build/` and what it is for; [docs/WEB_PLAYER.md](docs/WEB_PLAYER.md)
explains how the browser version works.

To try a web build locally, serve `build/web` over HTTP, e.g.
`python -m http.server 18765 --directory build/web`, and open the game's page (`bejeweled.html`).
Automated tests open it with `?silent=1`, which plays no sound.

Each game's README covers its game files, options, mods and tests.

## Licenses and credits

- This product includes portions of the PopCap Games Framework, © 2005-2009 PopCap Games, Inc. All rights
  reserved. (http://popcapframework.sourceforge.net/). See `framework/LICENSE.txt`.
- Bejeweled, Big Money and the other games named here are trademarks of Electronic Arts / PopCap Games.
  This project is not affiliated with or endorsed by them. The games' code, art, music and sounds remain
  their property; use your own copies.
- [SDL2](https://libsdl.org) (zlib license), [libxmp-lite](https://github.com/libxmp/libxmp) (MIT),
  [stb_image / stb_vorbis / stb_image_write](https://github.com/nothings/stb) (public domain / MIT).
