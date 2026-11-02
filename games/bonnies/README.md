# Bonnie's Bookstore Deluxe

A decompiled reimplementation of **Bonnie's Bookstore Deluxe** (New Crayon Games, published by PopCap Games, 2008) that
runs natively on Windows and in the browser as WebAssembly, part of [byo-pop](../../README.md). You need your own copy of
the game: no game files are included in this repository or in its builds, and the game's tables, texts and dictionary
are read from your copy at run time.

The original is not a SexyAppFramework game: it runs on New Crayon's own engine over SDL 1.2, SDL_image and FMOD 3, with
no scripting. `engine/` is that engine and `game/` the game, translated function by function (each function carries its
address in the original exe); `src/` hosts it on the shared framework (`docs/ENGINE.md`, "The port").

## Getting the game files

The port reads `BonniesBookstore.exe` (read, never run) and the `Data`, `Graphics` and `Sound` folders of an installed
copy.

- **Native**: `build/native/bonnies.exe` finds `assets/bonnies/` next to itself or up to two levels up, or the retail
  install; or run it with `-gamedir="<Bonnie's Bookstore Deluxe folder>"`. For development,
  `python games/bonnies/tools/stage_bonnies_assets.py ["<install folder>"]` copies those files into `assets/bonnies/`
  (git-ignored). The players and preferences go to the user folder (`-userdir=<dir>`, by default SDL's preferences
  folder for "byo-pop/Bonnie's Bookstore Deluxe").
- **Web**: the page (`bonnies.html`) asks for the game folder or a .zip of it on first start and keeps the needed files
  in the browser's storage. Nothing is uploaded.

## Building and testing

`tools/build.sh native` (target `bonnies`) and `tools/build.sh web`. `tools/check.sh bonnies` plays a tour from a
script (`tools/tour_test.txt`, with `-fast -seed=1`): a new player, the classic game's tutorial, the options, high
scores, credits and help, an action game until its time is up, and Quit. A rerun must give the same screens and saves,
and three of the screens must match the frozen baseline's hashes (`BB_SHOTS` in `tools/check.sh`); a copy of the port
in `build/baseline-bonnies/bonnies.exe` must give all of them too. Run the game in tests only under `build/native/rundesk.exe`
(see the repository's CLAUDE.md).

The original is compared with the port in Windows Sandbox (`docs/ORIGRUN.md`), from a copy staged in the run folder
under `build/origrun/<run>/game`. `tools/origrun/tour.txt` plays the port's tour by the clock up to the main menu's
options, `tools/origrun/action.txt` the action game with its focus loss (from the saves the tour leaves) and
`tools/origrun/resume.txt` loads a saved game. `tools/origrun/patches.txt`, with `--patches` and `--seed 1`, seeds the
original's `rand` as `-seed=1` does the port's. `tools/shotdiff` then pairs the shots with the port's tour
(`games/bonnies/shotdiff.json` masks the animations that run on the game's clock, each with its reason). The board's
random letters cannot match in a run by the clock: the sparkles draw from `rand` every frame, so the board depends on
how many frames ran before it. The board is therefore compared on a game the original saved and both sides load
(`resume.txt`), where it matches exactly. `tools/origcmp.py` compares a single capture.

## Status

The whole game is translated: the menus, players, options, help, high scores, the classic and action games over the
50 books, the story and the book ends. Natively and on the web it plays from the logos into the books; compared with
the original in Windows Sandbox (2026-11-01) every screen of the tour up to the action game's pause and resumption
matches, apart from the masked clock animations; the action game's end is still to be captured, as it outlasts
origrun's 240 s. `docs/DEVIATIONS.md`
lists every place the port does something other than the original, the leak it fixed and the memory the original
reads before setting it. Mods: none yet (`docs/MOD_IDEAS.md` lists ideas).
