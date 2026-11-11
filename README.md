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

### Progress (2026-11-09)

| Game | Model | Status | On main since | Mods | Port work | Mod work | Other | Total |
|---|---|---|---|---:|---:|---:|---:|---:|
| [Bejeweled Deluxe](games/bejeweled/README.md) | Opus | Done: identical, registration out, fixes mod, readability pass (the standards' pilot), coverage check, performance pass, unneeded code out, and the check against its PopCap Game Collection version (the same build as Steam's). Mods: graphics, sound, text, tunables and 18 stages of code mods (board shapes, gravity, replays, up to 12 gem colours, new gems) with curated modes | 2026-10-18 | 33 | $348 | $1,270 | $17 | $1,635 |
| [Big Money Deluxe](games/bigmoney/docs/README.md) | Opus | Identical; registration out, fixes mod and coverage check done; the performance pass next. Mods: replays, LiveSplit, a puzzle solver, board and money variants | 2026-10-18 | 12 | $317 | $131 | $8 | $456 |
| [BookWorm Deluxe](games/bookworm/docs/README.md) | Opus | Identical; registration out and fixes mod done; the coverage check next. Mods: undo, a word solver, replays, a daily board, a word journal, widescreen, LiveSplit, more word lists | 2026-10-19 | 19 | $408 | $178 | $12 | $598 |
| [Noah's Ark Deluxe](games/noahsark/docs/README.md) | Sonnet | Done: identical, registration out, fixes mod, coverage check, performance pass, unneeded code out and the readability pass (ported from PopCap's own installer, so it has no Collection check). Mods: undo, a move helper, tide speed, a daily board | 2026-10-25 | 5 | $225 | $5 | | $230 |
| [Seven Seas Deluxe](games/sevenseas/docs/README.md) | Sonnet | Identical; registration and More Games out, no fix candidates; the coverage check next | 2026-10-25 | | $182 | | | $182 |
| [Atomica Deluxe](games/atomica/docs/README.md) | Sonnet | Identical; registration out, fixes mod, coverage check, performance pass and the removal of code it does not need done; the readability pass under way | 2026-10-25 | 1 | $250 | | | $250 |
| [Mummy Maze Deluxe](games/mummymaze/docs/README.md) | Sonnet | Identical; registration out, fixes mod, coverage check, performance pass, unneeded code out and the readability pass done (ported from PopCap's own installer, so no Collection check); one late check of its fields under way | 2026-10-25 | 1 | $227 | $3 | | $230 |
| [TipTop Deluxe](games/tiptop/docs/README.md) | Opus | Identical; registration out and fixes mod done; the coverage check next | 2026-10-25 | 1 | $202 | | | $202 |
| [Bookworm Adventures Deluxe](games/bookwormadv/docs/README.md) | Opus | Identical (screens, sound and the 3D mode compared before the two phases were named), with PopLua, PopAnim, Flash cutscenes and JPEG 2000; registration out, no fix candidates; the coverage check next | 2026-10-25 | | $808 | $2 | | $810 |
| [Ning Po Mahjong Deluxe](games/ningpo/docs/README.md) | Opus | Identical; no fix candidates; coverage check done; the performance pass next | 2026-10-26 | | $152 | | | $152 |
| [Pixelus Deluxe](games/pixelus/docs/README.md) | Sonnet | Plays identical; registration and More Games out, no fix candidates; phase two's screen differences still to fix | 2026-10-26 | | $220 | | | $220 |
| [Alchemy Deluxe](games/alchemy/docs/README.md) | Sonnet | Done: identical, registration out, fixes mod, coverage check, performance pass, unneeded code out and the readability pass (ported from PopCap's own installer, not a Steam copy, so it has no Collection check) | 2026-10-26 | 1 | $173 | $7 | | $180 |
| [Word Harmony](games/wordharmony/docs/README.md) | Opus | Identical, on Sprout's first-generation engine; registration out and fixes mod done; the coverage check next | 2026-10-26 | 1 | $220 | $6 | | $227 |
| [Cosmic Bugs](games/cosmicbugs/docs/README.md) | Opus | Identical, on Retro64's engine; registration out and fixes mod done; the coverage check next | 2026-10-26 | 1 | $224 | | | $224 |
| [Water Bugs](games/waterbugs/docs/README.md) | Sonnet | Identical, on Cosmic Bugs' engine; registration out, fixes mod and coverage check done; the performance pass next | 2026-10-26 | 1 | $173 | | | $173 |
| [Rocket Mania Deluxe](games/rocketmania/docs/README.md) | Sonnet | Plays identical; phase two largely done (a few comparisons open); registration out | 2026-10-26 | | $204 | | | $204 |
| [Dynomite Deluxe](games/dynomite/docs/README.md) | Opus | Plays identical; registration and More Games out, no fix candidates; phase two's last differences open | 2026-10-26 | | $251 | | | $251 |
| [Bookworm Adventures Volume 2](games/bookwormadv2/docs/README.md) | Sonnet | Plays identical through the first chapter and the mini-games, on Bookworm Adventures' engine; the later chapters and phase two next | 2026-10-26 | | $271 | | | $271 |
| [Mahjong Escape: Ancient Japan](games/mahjongjapan/docs/README.md) | Opus | Done: identical, logos and links out, fixes mod, coverage check, performance pass, unneeded code out and the readability pass (its original is the Collection's own copy) | 2026-11-01 | 1 | $304 | | | $304 |
| [Mahjong Escape: Ancient China](games/mahjongchina/docs/README.md) | Opus | As Ancient Japan, from its port (whose row holds most of its cost) | 2026-11-01 | 1 | $4 | | | $4 |
| [Bonnie's Bookstore Deluxe](games/bonnies/README.md) | Opus | Identical, on New Crayon's engine; fixes mod and coverage check done; the performance pass next | 2026-11-01 | 1 | $230 | | | $230 |
| [Bejeweled Twist](games/bejeweledtwist/docs/NOTES.md) | Opus | Identical, in 2D and in 3D, with PopAnim; registration out, fixes mod, readability pass (the standards' second test) and coverage check done; the performance pass next | 2026-11-02 | 1 | $1,294 | | $6 | $1,300 |
| [Bejeweled 2 Deluxe](games/bejeweled2/docs/IDENTICAL_PLAY.md) | Sonnet | Plays identical: every mode as the original at both sizes, the hidden mode, a puzzle pack and resumed games (one oddity: a game resumed in 3D differs after its first level change); registration and More Games out; the user's faults of 2026-11-10 fixed; phase two next | 2026-11-08 | | $254 | | | $254 |
| [AstroPop Deluxe](games/astropop/docs/IDENTICAL_PLAY.md) | Sonnet | Phase one: classic and survival games play as the original over many seeds, one seed still differs | 2026-11-08 | | $322 | | | $322 |
| Typer Shark! Deluxe | Sonnet | Ported and playable natively and on the web; phase one next | 2026-11-09 | | $367 | | | $367 |
| Venice Deluxe, Heavy Weapon Deluxe, Hammer Heads Deluxe, Pizza Frenzy, Iggle Pop Deluxe, Feeding Frenzy Deluxe | Opus or Sonnet | Waiting for the readability passes; then side by side, in this order | | | under $1 each | | | |
| Chuzzle Deluxe, Feeding Frenzy 2, Peggle Deluxe, Peggle Nights, Bejeweled 3, Banana Bugs, Gyromancer, Zuma's Revenge, Talismania Deluxe | | After those, two at a time | | | $19 | | | $19 |
| Shared framework, tools and coordination | | | | | $1,012 | $167 | $927 | $2,105 |
| **All** | | | | **80** | **$8,663** | **$1,767** | **$970** | **$11,401** |

A game is done, and goes on the live site, only once it has been through every phase but modding (the user, 2026-11-08):
the faults the user reported, identical play in both phases, registration and the buttons that lead out of a game taken
out, its fixes mod, the readability pass to [docs/CODE_STANDARDS.md](docs/CODE_STANDARDS.md), the coverage check, the
performance pass, taking out the other code it does not need, and, for a game ported from its Steam copy, the check
against its PopCap Game Collection version. No game has been through all of them yet; whatever games are done by the
end of December 2026 go on the site then. The Status column says which of them each game has finished. "Identical"
means both phases of its identical-play pass are finished, as each game's `docs/IDENTICAL_PLAY.md` reports: with the
same input, a series of games plays out as in the original exe (the same boards, random draws, scores and endings), and
its screens, dialogs, sound and music, focus handling and web build match the original's, with every difference fixed or
explained. "Plays identical" means the first of those phases is done. A game with no fix candidates (no bug of the
original that a fix would change, nor a wait that only costs the player time) has no fixes mod. The user approved the readability standards on 2026-11-10, so the other games take that pass now.

The costs are what the work's tokens would cost at the API prices in [docs/TOKEN_ECONOMY.md](docs/TOKEN_ECONOMY.md),
each model at its own; the subscription the work ran on does not bill them. A game's cost counts its own sessions with
their subagents and workflow agents, and the coordinator's reviews, merges and fixes of that game. Until 2026-10-20 one
session ported Bejeweled and set up the repository, and its work counts for Bejeweled. Port and mod work are split
commit by commit: each commit gets the cost of the work since the one before, classified by what it did; the finishing
phases count as port work. "Other" is publishing, the website, licences, usage measurement and rules for the sessions.
The Mods column counts each game's fixes mod. `tools/game_costs.py` computes the costs; the table is refreshed when
asked.

### Performance

| Game | Load time, port | Load time, original | Processor time, port | Processor time, original | Memory, port | Memory, original |
|---|---:|---:|---:|---:|---:|---:|
| Bejeweled Deluxe | 0.88 s | 1.06 s | 4.1 % of one core | 4.5 % of one core | 55 MB | 36 MB |
| Big Money Deluxe | 0.73 s | 1.38 s | 4.0 and 4.7 s a minute | 7.1 and 7.2 s a minute | 124 MB | 42 MB |
| BookWorm Deluxe | | | | | | |
| Noah's Ark Deluxe | 0.60 s | 2.12 s | 8.0 s a minute | 18.1 and 19.0 s a minute | 63 and 66 MB | 67 MB |
| Seven Seas Deluxe | 1.07 s | 1 to 2 s | 4.8 to 5.5 % of one core | 2.2 to 8.7 % of one core | 66 MB | 41.5 to 51.2 MB |
| Atomica Deluxe | 1.20 s | 1.07 s | 1.7 s to update 4,400 | 2.5 s to update 4,400 | 184 MB | 62 MB |
| Mummy Maze Deluxe | 0.38 and 0.44 s | 4.5 s | 2.2 s a minute | 5.7 and 5.8 s a minute | 58 MB | 65 and 66 MB |
| TipTop Deluxe | 0.6 to 0.9 s | 4.7 and 4.8 s | 29 and 33 % of one core | 5 % of one core | 82 and 84 MB | 46 MB |
| Bookworm Adventures Deluxe | 1.70 s | 6.4 s | 2.9 to 3.2 % of one core | 7.8 to 9.4 % of one core | 165 MB | 94 MB |
| Ning Po Mahjong Deluxe | 0.36 s | 0.82 s | 6.3 % of one core | 14.9 % of one core | 54 MB | 21 MB |
| Pixelus Deluxe | 0.9 to 1.4 s | 0.8 to 1.0 s | 5.7 and 7.0 s a minute | 10.4 and 11.6 s a minute | 102 and 104 MB | 93 MB |
| Alchemy Deluxe | 1.74 s | 1.76 s | 4.6 to 5.3 s a minute | 4.2 and 6.4 s a minute | 67 to 69 MB | 51 to 56 MB |
| Word Harmony | 0.71 s | 2.6 to 3.4 s | 10.3 and 13.6 s a minute | 2.2 and 3.4 s a minute | 154 and 155 MB | 53 MB |
| Cosmic Bugs | 0.6 to 0.9 s | 0.4 to 1.0 s | 18 to 19 % of one core | 4.7 and 5.6 % of one core | 130 MB | 88 and 89 MB |
| Water Bugs | 2.76 s | 3.16 s | 10 s a minute | 58 s a minute | 160 MB | 97 and 101 MB |
| Rocket Mania Deluxe | 1.8 to 2.0 s | 0.9 and 1.1 s | 4.6 to 5.7 s a minute | 8.6 to 9.3 s a minute | 132 MB | 62 MB |
| Dynomite Deluxe | 0.96 s | 2.2 s | 4.6 and 4.9 s a minute | 11.6 and 13.2 s a minute | 92 and 94 MB | 65 MB |
| Bookworm Adventures Volume 2 | | | | | | |
| Mahjong Escape: Ancient Japan | 1.31 s | | 6.0 % of one core | 3.1 % of one core | 146 MB | 40 MB |
| Mahjong Escape: Ancient China | 1.29 s | | 7.3 % of one core | 1.7 % of one core | 104 MB | 31 MB |
| Bonnie's Bookstore Deluxe | 6.8 and 12.9 s | 6.2 and 6.4 s | 7.1 and 8.5 s a minute | 3.3 and 3.4 s a minute | 87 and 89 MB | 56 and 58 MB |
| Bejeweled Twist | 2.71 s | 10.81 s | 3.3 s a minute | 21.1 s a minute | 206 MB | 338 MB |
| Bejeweled 2 Deluxe | | | | | | |
| AstroPop Deluxe | | | | | | |
| Typer Shark! Deluxe | | | | | | |

Every number comes from the game's own notes, its latest measurement there: the native port under rundesk and the
original under origrun, back to back on one machine, windowed and silent, over the same scripted play, with the load
time from the process's start to the first screen that takes input and the memory the peak of the process's private
bytes. Processor time is as each game measured it, a share of one core over a stretch of play or seconds over a scripted
minute (Atomica's runs from launch to a fixed update; Bejeweled Twist and Bookworm Adventures were measured at rest on
the menu and the title; TipTop's and Cosmic Bugs' originals are paced at 60 Hz, as on a player's screen). Most runs
shared the machine with other sessions' games, and each game has its own play and its own measure, so a row compares a
port with its own original, never one game with another; an empty cell means the notes give no number.

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
                   mod settings and the mod menu
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
