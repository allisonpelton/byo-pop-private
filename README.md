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

### Progress

| Game | Model | Latest phase done | On main since | Mods | Port work | Mod work | Other | Total |
|---|---|---|---|---:|---:|---:|---:|---:|
| [Bejeweled Deluxe](games/bejeweled/README.md) | Opus | Every phase but modding | 2026-10-18 | 33 | $396 | $1,275 | $17 | $1,689 |
| [Big Money Deluxe](games/bigmoney/docs/README.md) | Opus | Every phase but modding | 2026-10-18 | 12 | $387 | $132 | $8 | $527 |
| [BookWorm Deluxe](games/bookworm/docs/README.md) | Opus | Every phase but modding | 2026-10-19 | 19 | $500 | $186 | $12 | $699 |
| [Noah's Ark Deluxe](games/noahsark/docs/README.md) | Sonnet | Every phase but modding | 2026-10-25 | 5 | $287 | $17 | | $304 |
| [Seven Seas Deluxe](games/sevenseas/docs/README.md) | Sonnet | Every phase but modding | 2026-10-25 | | $214 | | | $214 |
| [Atomica Deluxe](games/atomica/docs/README.md) | Sonnet | Every phase but modding | 2026-10-25 | 1 | $345 | $13 | | $357 |
| [Mummy Maze Deluxe](games/mummymaze/docs/README.md) | Sonnet | Every phase but modding | 2026-10-25 | 2 | $330 | $25 | | $355 |
| [TipTop Deluxe](games/tiptop/docs/README.md) | Opus | Every phase but modding | 2026-10-25 | 1 | $281 | $2 | | $283 |
| [Bookworm Adventures Deluxe](games/bookwormadv/docs/README.md) | Opus | Every phase but modding | 2026-10-25 | 1 | $1,087 | $2 | | $1,089 |
| [Ning Po Mahjong Deluxe](games/ningpo/docs/README.md) | Opus | Every phase but modding | 2026-10-26 | | $167 | | | $167 |
| [Pixelus Deluxe](games/pixelus/docs/README.md) | Sonnet | Every phase but modding | 2026-10-26 | | $403 | | | $403 |
| [Alchemy Deluxe](games/alchemy/docs/README.md) | Sonnet | Every phase but modding | 2026-10-26 | 1 | $247 | $16 | | $263 |
| [Word Harmony](games/wordharmony/docs/README.md) | Opus | Every phase but modding | 2026-10-26 | 1 | $302 | $8 | | $309 |
| [Cosmic Bugs](games/cosmicbugs/docs/README.md) | Opus | Every phase but modding | 2026-10-26 | 2 | $341 | $11 | | $352 |
| [Water Bugs](games/waterbugs/docs/README.md) | Sonnet | Every phase but modding | 2026-10-26 | 1 | $285 | $1 | | $286 |
| [Rocket Mania Deluxe](games/rocketmania/docs/README.md) | Sonnet | Every phase but modding | 2026-10-26 | 1 | $343 | $8 | | $351 |
| [Dynomite Deluxe](games/dynomite/docs/README.md) | Opus | Every phase but modding | 2026-10-26 | | $427 | | | $428 |
| [Bookworm Adventures Volume 2](games/bookwormadv2/docs/README.md) | Sonnet | Registration out | 2026-10-26 | | $460 | | | $460 |
| [Mahjong Escape: Ancient Japan](games/mahjongjapan/docs/README.md) | Opus | Every phase but modding | 2026-11-01 | 2 | $515 | $2 | | $517 |
| [Mahjong Escape: Ancient China](games/mahjongchina/docs/README.md) | Opus | Every phase but modding | 2026-11-01 | 2 | with Ancient Japan | | | |
| [Bonnie's Bookstore Deluxe](games/bonnies/README.md) | Opus | Every phase but modding | 2026-11-01 | 1 | $318 | $2 | | $320 |
| [Bejeweled Twist](games/bejeweledtwist/docs/NOTES.md) | Opus | Every phase but modding | 2026-11-02 | 2 | $1,508 | $10 | $6 | $1,525 |
| [Bejeweled 2 Deluxe](games/bejeweled2/docs/IDENTICAL_PLAY.md) | Sonnet | Every phase but modding | 2026-11-08 | 1 | $671 | $19 | | $690 |
| [AstroPop Deluxe](games/astropop/docs/IDENTICAL_PLAY.md) | Sonnet | Every phase but modding | 2026-11-08 | 1 | $623 | $11 | | $635 |
| [Typer Shark! Deluxe](games/typershark/docs/IDENTICAL_PLAY.md) | Sonnet | Every phase but modding | 2026-11-09 | 2 | $588 | $26 | | $614 |
| [Venice Deluxe](games/venice/docs/IDENTICAL_PLAY.md) | Sonnet | Every phase but modding | 2026-11-12 | 2 | $335 | $11 | | $346 |
| [Heavy Weapon Deluxe](games/heavyweapon/docs/IDENTICAL_PLAY.md) | Opus | Every phase but modding | 2026-11-12 | 2 | $370 | $19 | | $389 |
| [Hammer Heads Deluxe](games/hammerheads/docs/IDENTICAL_PLAY.md) | Sonnet | Every phase but modding | 2026-11-13 | 2 | $383 | $6 | | $389 |
| [Pizza Frenzy](games/pizzafrenzy/docs/README.md) | Opus | Fixes mod | 2026-11-15 | 1 | $509 | | | $509 |
| [Feeding Frenzy Deluxe](games/feedingfrenzy/docs/README.md) | Sonnet | Porting | 2026-11-16 | | $210 | | | $210 |
| [Peggle Deluxe](games/peggle/docs/GAME_NOTES.md) | Sonnet | Porting | 2026-11-16 | | $183 | | | $183 |
| [Feeding Frenzy 2 Deluxe](games/feedingfrenzy2/docs/WORKING_NOTES.md) | Opus | Porting | 2026-11-16 | | $476 | | | $476 |
| [Bejeweled 3](games/bejeweled3/docs/NOTES.md) | Opus | Porting | 2026-11-16 | | $473 | | | $473 |
| [Banana Bugs](games/bananabugs/docs/GAME_NOTES.md) | Opus | Porting | 2026-11-21 | | $302 | | | $302 |
| [Talismania Deluxe](games/talismania/docs/WORKING_NOTES.md) | Sonnet | Porting | 2026-11-21 | | $48 | | | $48 |
| [Zuma's Revenge!](games/zumasrevenge/docs/NOTES.md) | Opus | Porting | 2026-11-21 | | $252 | | | $252 |
| Chuzzle Deluxe | Opus | Porting | | | $281 | | | $281 |
| Gyromancer | Opus | Porting | | | $241 | | | $241 |
| Iggle Pop Deluxe | Sonnet | Porting | | | $214 | | | $214 |
| Peggle Nights | Sonnet | Porting | | | under $1 | | | under $1 |
| Shared framework, tools and coordination | | | | | $2,304 | $270 | $1,719 | $4,293 |
| **All** | | | | **98** | **$17,608** | **$2,072** | **$1,764** | **$21,444** |

A game is done, and goes on the live site, only once it has been through every phase but modding (the user, 2026-11-08):
the faults the user reported, identical play in both phases, registration and the buttons that lead out of a game taken
out, its fixes mod, the readability pass to [docs/CODE_STANDARDS.md](docs/CODE_STANDARDS.md), the coverage check, the
performance pass, taking out the other code it does not need, and, for a game ported from its Steam copy, the check
against its PopCap Game Collection version. The column "Latest phase done" names the furthest phase a
game has finished with every phase before it finished too, and "Every phase but modding" a game that is done; 
`tools/readme_status.py` writes it from `docs/game_status.json` at each merge. "Looks identical"
means both phases of its identical-play pass are finished, as each game's `docs/IDENTICAL_PLAY.md` reports: with the
same input, a series of games plays out as in the original exe (the same boards, random draws, scores and endings), and
its screens, dialogs, sound and music, focus handling and web build match the original's, with every difference fixed or
explained. "Plays identical" means the first of those phases is done. A game with no fix candidates (no bug of the
original that a fix would change, nor a wait that only costs the player time) has no fixes mod. The user approved the readability standards on 2026-11-10, so the other games take that pass now.

The costs are those of 2026-11-23: what the work's tokens would cost at the API prices in [docs/TOKEN_ECONOMY.md](docs/TOKEN_ECONOMY.md),
each model at its own; the subscription the work ran on does not bill them. A game's cost counts its own sessions with
their subagents and workflow agents, and the coordinator's reviews, merges and fixes of that game. Until 2026-10-20 one
session ported Bejeweled and set up the repository, and its work counts for Bejeweled. Port and mod work are split
commit by commit: each commit gets the cost of the work since the one before, classified by what it did; the finishing
phases count as port work. "Other" is publishing, the website, licences, usage measurement and rules for the sessions.
The Mods column counts each game's mods, its fixes and LiveSplit mods among them. The two Mahjong Escape games share
one engine, and their costs are one figure, on Ancient Japan's row. `tools/game_costs.py` computes the costs; the table is refreshed when
asked.

### Performance

| Game | Load time, port | Load time, original | Processor time, port | Processor time, original | Memory, port | Memory, original |
|---|---:|---:|---:|---:|---:|---:|
| Bejeweled Deluxe | 0.88 s | 1.06 s | 6.1 % of one core at 1x on the software screen (126 M cycles a second; 2x on the GPU: 7.4 %, 154 M) | 5.9 % of one core (123 M cycles a second, 15 runs) | 22.5 MB at 1x on the software screen (22.4 to 25.5); 55 MB at the default 2x window | 34.5 MB (36 on 2026-11-09) |
| Big Money Deluxe | 0.73 s | 1.38 s | 3.0 to 5.8 s a minute | 5.5 to 11.2 s a minute | 37 to 42 MB | 42 to 44 MB |
| BookWorm Deluxe | 0.42 to 0.48 s | 1.67 to 1.83 s | 6.8 to 7.5 % of one core (medians of five alternating pairs, 2D in its 640x480 window; 1.40 to 1.57 M cycles an update) | 11.7 to 14.2 % of one core (medians; 2.41 to 2.92 M cycles an update; its window 640x480) | 34.4 MB at the title, 47.0 MB at the peak (2D at 1x; 62.3 and 78.6 MB in 3D in its default 2x window) | 41.9 MB at the title, 43.1 MB at the peak |
| Noah's Ark Deluxe | 0.60 s | 2.12 s | 8.0 s a minute | 18.1 and 19.0 s a minute | 63 and 66 MB | 67 MB |
| Seven Seas Deluxe | 1.07 s | 1 to 2 s | 9.1 G cycles a minute at 640x480 (software screen; 5.9 in the default 1280x960, on the GPU) | 10.5 to 14.8 G cycles a minute (medians of 4 and of 2 runs; its window 640x480) | 34 to 37 MB at 640x480, 66 MB in the default window | 41.5 to 51.2 MB |
| Atomica Deluxe | 1.20 s | 1.07 s | 18 % of one core (1x, the software screen; the same 20 s stretch of a scripted game) | 21 % of one core | 58 to 63 MB (the same stretch; 113 to 118 MB in its default 2x window on the GPU) | 66 to 67 MB |
| Mummy Maze Deluxe | 0.38 and 0.44 s | 4.5 s | 7.7 G cycles a minute | 8.7 G cycles a minute | 58 MB | 65 and 66 MB |
| TipTop Deluxe | 0.6 to 0.9 s | 4.7 and 4.8 s | 4.07 G cycles in 30 s of strategy, 1.87 G in 20 s at the menu (640x480, 62.5 passes a second) | 4.96 G and 2.07 G (paced to 62 passes a second) | 82 and 83 MB | 46 MB |
| Bookworm Adventures Deluxe | 1.70 s | 6.4 s | 7.9 G cycles a minute at the title (medians of five pinned pairs, 3D in its 800x600 window; 16.0 G full screen at 3840x2160) | 24.4 G cycles a minute (medians; 2D, its window 800x600) | 79.3 MB at the title, 101.1 MB at the peak (3D at 1x; 77.3 and 100.1 full screen) | 91.9 MB at the title, 92.2 MB at the peak (2D) |
| Ning Po Mahjong Deluxe | 0.93 s | 1.02 s | 7.5 % of one core (software screen, 1x) | 11.8 % of one core | 19 MB | 30 MB (20 MB in the 2026-11-15 runs) |
| Pixelus Deluxe | 0.9 to 1.4 s | 0.8 to 1.0 s | 2.0 to 3.3 M cycles an update in 2D at 1x with the software screen (median 2.20, 0.64 of the original in the same rounds); 2.1 to 3.2 on the GPU (median 2.9 with its mixer) | 3.0 to 4.6 M cycles an update (median 3.43 in the same rounds; 2.6 to 3.6 earlier) | 66.8 to 69.6 MB in 2D at 1x (about 100 MB in 3D, its default, and in its default 2x window) | 91 to 93 MB |
| Alchemy Deluxe | 1.74 s | 1.76 s | 1.6 to 2.0 M cycles an update in 2D at 1x with the software screen (0.93 to 1.11 of the original in three sets of pairs, 0.86 to 1.04 without its mixer thread); 2.2 M on the GPU in the default 2x window (1.5 of the original) | 1.5 to 2.1 M cycles an update (paced as a 60 Hz screen, the stretch where it kept up with the bot's game) | 35 to 39 MB in 2D at 1x (68 to 72 MB in the default 2x window) | 51 to 56 MB |
| Word Harmony | 0.71 s | 2.6 to 3.4 s | 3.26 G cycles a minute in its own 800x600 window, 8.82 G full screen (its default) | 11.81 G cycles a minute windowed, 10.19 G full screen | 88 to 94 MB | 53 to 57 MB |
| Cosmic Bugs | 0.33 to 0.38 s | 0.60 s | 86 M cycles a second on a board in its own 800x600 window, 102 M in the default 2x window (sound off, as the original's) | 110 M cycles a second (its loop paced to 60 Hz) | 130 MB | 88 and 89 MB |
| Water Bugs | 2.76 s | 3.16 s | 118 M cycles a second on a board in its own 800x600 window (108 with sound off, as the original's), 128 M in the default 2x window | 136 M cycles a second (its loop paced to 60 Hz) | 104 MB | 97 MB |
| Rocket Mania Deluxe | 1.8 to 2.0 s | 0.9 and 1.1 s | 1.86 to 3.95 M cycles an update in 2D in its own 640x480 window (9 to 19 % of one core; 0.43 to 0.73 of the original in pinned pairs) | 4.18 to 5.44 M cycles an update (paired runs on a loaded machine; 4.02 to 4.57 M quiet, 19 to 22 % of one core) | 71 MB settled, 92 to 112 MB at the loading peak (2D at 1x; 147 MB on the GPU path) | 57 MB |
| Dynomite Deluxe | 0.96 s | 2.2 s | 11.4 G cycles a minute at the game's own 640x480, 9.1 G in its default 1280x960 window | 12.9 G cycles a minute at 60 Hz | 48 to 51 MB at 640x480, 84 MB in the default window | 63 MB |
| Bookworm Adventures Volume 2 | | | | | | |
| Mahjong Escape: Ancient Japan | 1.31 s | | 1.15 G cycles in 30 s in its own 800x600 window, 1.86 G at 2x, 2.20 G full screen (its default) | 2.80 G (windowed) | 18 MB at the main menu, 39 MB at the peak of a whole play in its own window (37 MB at 2x, 38 MB full screen) | 41 MB |
| Mahjong Escape: Ancient China | 1.29 s | | 1.00 G cycles in 30 s in its own 800x600 window, 1.65 G at 2x, 1.34 G full screen (its default) | 3.17 G (windowed) | 18 MB at the main menu, 32 MB at the peak of a whole play in its own window (30 MB at 2x, 31 MB full screen) | 32 MB |
| Bonnie's Bookstore Deluxe | 6.8 and 12.9 s | 6.2 and 6.4 s | 7.9 G cycles at 800x600, 6.1 G in its default 1600x1200 window | 9.5 G and 7.3 G cycles in the same rounds | 52 MB at 800x600, 58 MB at 1600x1200 | 54 MB |
| Bejeweled Twist | 2.71 s | 10.81 s | 38.8 G cycles a minute in 2D, 43.2 in 3D | 41.5 G cycles a minute in 2D, 47.2 in 3D | 354 to 390 MB in 2D, 250 to 258 in 3D | 183 to 189 MB in 2D, 332 to 340 in 3D |
| Bejeweled 2 Deluxe | 0.80 s in 2D on the software screen, 1.03 s on a GPU, 1.25 s in 3D | 4.10 s in 2D, 4.38 s in 3D (dgVoodoo2) | 5.1 % of one core in 2D on the software screen, 5.8 % on a GPU, 5.4 % in 3D (idle board, sound threads left out; 5.7, 6.4 and 5.9 % with the mixer) | 9.6 % of one core in 2D, 58.1 % in 3D (dgVoodoo2 included) | 67 MB on the software screen, 83 MB on a GPU, 93 MB in 3D | 44 MB in 2D, 151 MB in 3D (dgVoodoo2) |
| AstroPop Deluxe | 1.0 to 1.3 s | 1.5 s (2D), 5 to 13 s (3D) | 4.25 G cycles in a 2,030-update stretch (2D), 3.02 G (3D) | 6.13 G (2D), 3.43 G (3D) | 119 MB (2D), 84 MB (3D) | 81 to 83 MB (2D), 118 MB (3D) |
| Typer Shark! Deluxe | 0.72 to 0.82 s | 0.76 to 1.28 s | 2.50 G cycles in 20 s without its mixer, 2.70 G with it (3D, visible desktop, medians) | 2.54 G cycles in 20 s (3D; its sound a silent stand-in) | 86 to 89 MB | 49 to 51 MB |
| Venice Deluxe | | | 2.61 M cycles an update (31.1 % of one core) in its own 800x600 window, 2.37 M at 2x; 0.75 M in 3D | 4.92 M cycles an update (58.8 % of one core) in 2D | 36 to 38 MB at a first board | |
| Heavy Weapon Deluxe | | | 2.67 M cycles an update (12.8 % of one core) in 2D in its own 640x480 window, 2.74 M at 2x, its mixer left out (2.94 and 3.01 M with it) | 3.125 M cycles an update (15.0 % of one core) in 2D | 109 MB at the first board of the campaign | |
| Hammer Heads Deluxe | | | 2.26 M cycles an update in 3D with vsync, its default (0.71 of the original in the same rounds, 0.47 to 1.03) | 3.17 M cycles an update (paced) | 142 MB at the peak | |

Every number comes from the game's own notes, its latest measurement there: the native port under rundesk and the
original under origrun, back to back on one machine, windowed and silent, over the same scripted play, with the load
time from the process's start to the first screen that takes input and the memory the peak of the process's private
bytes (Atomica's and AstroPop's ports over a 20-second stretch of play and Venice's and Heavy Weapon's at a first
board, as `docs/MEMORY.md` measured them after the memory work, and the Mahjong games' early in a game, where their notes
have nothing later; Venice and Heavy Weapon have no figure for their originals). Processor time is as each game measured it, a share of one core over a stretch of play or seconds over a scripted
minute (Atomica's over the last 20 seconds of a scripted game, as the cycle counts of the process's threads, medians
of ten alternating pairs: port 7.5, original 8.6 billion cycles, at the processor's nominal 2.11 GHz; Bookworm
Adventures was measured at rest on the title; Bejeweled Twist's and Mummy Maze's are the cycles their threads ran in a minute of the
same scripted game (Mummy Maze's the medians of five alternating rounds, the port in the original's own 640x480 window), the memory that game's peak; TipTop's, Cosmic Bugs' and Atomica's originals are paced at 60 Hz,
as on a player's screen). Most runs
shared the machine with other sessions' games, and each game has its own play and its own measure, so a row compares a
port with its own original, never one game with another; an empty cell means the notes give no number.
Dynomite Deluxe's processor time is the cycles its threads ran in the same scripted minute of Time Trial, the
medians of five pinned rounds run in turn, with its original windowed at 640x480 and drawing at most 60 times a second
as in full screen, measured again on 2026-11-15 on a busier machine than its first figures. At that size the port draws its
screen in software, as the original does; in a larger window it draws on the GPU. It is under its original at both sizes. Rocket Mania's, Bejeweled 2's and AstroPop's ports ran in the original's own window
size too (Bejeweled 2's on an idle board); AstroPop's 3D is 0.88 of its original with vsync and level with it when paced like it (3.02 and 3.41 G against 3.43).

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
