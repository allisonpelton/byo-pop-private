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

### Progress (2026-11-04)

| Game | Model | Status | On main since | Mods | Port work | Mod work | Other | Total |
|---|---|---|---|---:|---:|---:|---:|---:|
| [Bejeweled Deluxe](games/bejeweled/README.md) | Opus | Done and identical, with graphics, sound, text and tunables mods and 18 stages of code mods (board shapes, gravity, replays, up to 12 gem colours, new gems) with curated modes | 2026-10-18 | 32 | $279 | $1,269 | $17 | $1,565 |
| [Big Money Deluxe](games/bigmoney/docs/README.md) | Opus | Done and identical, with mods: replays, LiveSplit, a puzzle solver, board and money variants | 2026-10-18 | 11 | $291 | $131 | $8 | $430 |
| [BookWorm Deluxe](games/bookworm/docs/README.md) | Opus | Done, plays identical, with mods: undo, a word solver, replays, a daily board, a word journal, widescreen, LiveSplit, more word lists | 2026-10-19 | 18 | $375 | $172 | $12 | $560 |
| [Noah's Ark Deluxe](games/noahsark/docs/README.md) | Sonnet | Done and identical, with mods: undo, a move helper, tide speed, a daily board | 2026-10-25 | 4 | $158 | $5 | | $163 |
| [Seven Seas Deluxe](games/sevenseas/docs/README.md) | Sonnet | Done and identical | 2026-10-25 | | $137 | | | $137 |
| [Atomica Deluxe](games/atomica/docs/README.md) | Sonnet | Done and identical | 2026-10-25 | | $179 | | | $179 |
| [Mummy Maze Deluxe](games/mummymaze/docs/README.md) | Sonnet | Done and identical | 2026-10-25 | | $151 | | | $151 |
| [TipTop Deluxe](games/tiptop/docs/README.md) | Opus | Done and identical | 2026-10-25 | | $157 | | | $157 |
| [Bookworm Adventures Deluxe](games/bookwormadv/docs/README.md) | Opus | Done and identical, with PopLua, PopAnim, Flash cutscenes and JPEG 2000 | 2026-10-25 | | $708 | $2 | | $710 |
| [Ning Po Mahjong Deluxe](games/ningpo/docs/README.md) | Opus | Done, plays identical | 2026-10-26 | | $90 | | | $90 |
| [Pixelus Deluxe](games/pixelus/docs/README.md) | Sonnet | Done, plays identical | 2026-10-26 | | $164 | | | $164 |
| [Alchemy Deluxe](games/alchemy/docs/README.md) | Sonnet | Done and identical | 2026-10-26 | | $102 | | | $102 |
| [Word Harmony](games/wordharmony/docs/README.md) | Opus | Done, plays identical, on Sprout's first-generation engine | 2026-10-26 | | $144 | | | $144 |
| [Cosmic Bugs](games/cosmicbugs/docs/README.md) | Opus | Done and identical, on Retro64's engine | 2026-10-26 | | $180 | | | $180 |
| [Water Bugs](games/waterbugs/docs/README.md) | Sonnet | Done and identical, on Cosmic Bugs' engine | 2026-10-26 | | $101 | | | $101 |
| [Rocket Mania Deluxe](games/rocketmania/docs/README.md) | Sonnet | Done, plays identical | 2026-10-26 | | $175 | | | $175 |
| [Dynomite Deluxe](games/dynomite/docs/README.md) | Opus | Done, plays identical | 2026-10-26 | | $187 | | | $187 |
| [Bookworm Adventures Volume 2](games/bookwormadv2/docs/README.md) | Sonnet | Done, plays identical, on Bookworm Adventures' engine | 2026-10-26 | | $238 | | | $238 |
| [Mahjong Escape: Ancient Japan](games/mahjongjapan/docs/README.md) | Opus | Done and identical | 2026-11-01 | | $183 | | | $183 |
| [Mahjong Escape: Ancient China](games/mahjongchina/docs/README.md) | Opus | Done and identical, from Ancient Japan's port (whose row holds most of its cost) | 2026-11-01 | | $4 | | | $4 |
| [Bonnie's Bookstore Deluxe](games/bonnies/README.md) | Opus | Done and identical, on New Crayon's engine | 2026-11-01 | | $137 | | | $137 |
| [Bejeweled Twist](games/bejeweledtwist/docs/NOTES.md) | Opus | Done and identical, in 2D and in 3D, with PopAnim | 2026-11-02 | | $1,082 | | $6 | $1,088 |
| Venice Deluxe, Heavy Weapon Deluxe, Hammer Heads Deluxe, Bejeweled 2 Deluxe, AstroPop Deluxe, Typer Shark! Deluxe, Pizza Frenzy, Iggle Pop Deluxe, Feeding Frenzy Deluxe | Opus or Sonnet | Waiting until every game above is identical; then side by side, in this order | | | under $1 each | | | |
| Chuzzle Deluxe, Feeding Frenzy 2, Peggle Deluxe, Peggle Nights, Bejeweled 3, Banana Bugs, Gyromancer, Zuma's Revenge, Talismania Deluxe | | After those, one at a time | | | | | | |
| Shared framework, tools and coordination | | | | | $733 | $162 | $541 | $1,436 |
| **All** | | | | **65** | **$5,957** | **$1,741** | **$584** | **$8,282** |

"Done" means the whole game plays natively and in the browser. "Identical" means its identical-play pass is finished,
both phases, as each game's `docs/IDENTICAL_PLAY.md` reports: with the same input, a series of games plays out as in
the original exe (the same boards, random draws, scores and endings), and its screens, dialogs, sound and music, focus
handling and web build match the original's, with every difference fixed or explained. "Plays identical" means the
first of those phases is done; the second follows from 2026-11-08. The costs are what the work's tokens would cost at the
API prices in [docs/TOKEN_ECONOMY.md](docs/TOKEN_ECONOMY.md), each model at its own; the subscription the work ran on
does not bill them. A game's cost counts its own sessions with their subagents and workflow agents, and the
coordinator's reviews, merges and fixes of that game. Until 2026-10-20 one session ported Bejeweled and set up the
repository, and its work counts for Bejeweled. Port and mod work are split commit by commit: each commit gets the cost
of the work since the one before, classified by what it did. "Other" is publishing, the website, licences, usage
measurement and rules for the sessions. `tools/game_costs.py` computes the costs; the table is refreshed when asked.

Two things differ in every native port on purpose, the user's choice for all of them, with no mod needed. Full screen is
a borderless window over the whole monitor at the desktop's own resolution, never the originals' change of the display's
mode ("I'd like all the native ports to have their fullscreen modes made borderless without mods", 2026-11-07): the window
is not topmost, the cursor can leave it, and Alt-Tab only moves the focus, without minimizing the game. And the window,
which can be resized with the game's picture scaled to it at its own shape, cannot be made smaller than the game's own
screen ("It would be nice for the native version to have a minimum window size match the game's native resolution",
2026-11-07), 640x480 for most games, in the desktop's scaled units as the original's window is sized on a scaled
desktop. Both live in `framework/SexyAppBase.cpp` (`MakeWindow`, `SwitchScreenMode`), and `fwtest -hotkeytest` (the
`unit` suite) checks them. A game's own list of deviations leaves them out.

The games still to port, their order and what it takes to port each are in
[docs/POPCAP_GAMES_SURVEY.md](docs/POPCAP_GAMES_SURVEY.md) ("The plan for the remaining games" and "The remaining
games"), with the facts about every game surveyed; planned mods are in
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
- PopCap, Bejeweled, Bejeweled Twist, BookWorm, Big Money, Alchemy and Dynomite! are registered trademarks of
  Electronic Arts Inc. in the United States. This project is not affiliated with or endorsed by Electronic Arts or
  any game's publisher. The games' code, art, music and sounds remain their owners' property; use your own copies.
- [SDL2](https://libsdl.org) (zlib license), [libxmp-lite](https://github.com/libxmp/libxmp) (MIT),
  [stb_image / stb_vorbis / stb_image_write](https://github.com/nothings/stb) (public domain / MIT).
- This software is based in part on the work of the [Independent JPEG Group](https://www.ijg.org) (libjpeg 6b decodes the
  games' JPEG images, as it does in the originals; `third_party/libjpeg/README`).
