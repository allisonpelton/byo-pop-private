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

`tools/build.sh native` (target `bonnies`) and `tools/build.sh web`. `tools/check.sh bonnies` plays the logos, the
menu, a new player, the difficulty, the story and a tutorial word from a script (`tools/menu_test.txt`, with `-fast
-seed=1`) and reruns it, which must give the same screens. Run the game in tests only under `build/native/rundesk.exe`
(see the repository's CLAUDE.md). `tools/origcmp.py` compares a capture of the original (tools/origrun) with a port
screenshot.

## Status

The whole game is translated: the menus, players, options, help, high scores, the classic and action games over the
50 books, the story and the book ends. Natively and on the web it plays from the logos into the books; compared with
the original the screens checked so far match (`docs/ENGINE.md`, "Comparing with the original"). Mods: none yet
(`docs/MOD_IDEAS.md` lists ideas).
