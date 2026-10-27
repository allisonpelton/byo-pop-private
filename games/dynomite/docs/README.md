# Dynomite Deluxe port

A port of PopCap's Dynomite Deluxe (2003) that plays natively (Windows, SDL2) and in the browser (WebAssembly). It is a
translation of the original's game classes into code on the shared framework; it ships none of the game's pictures,
sounds, music, puzzle packs or texts, which come from your own copy of the game.

## Status (2026-10-26)

The game is translated: the loading and title screens, the main menu with its pages (start, modes, skill, the fossil
challenge's kind, saved games, puzzle packs), the side bar with Inchy and his call-outs, the board with the slingshot, the
aim and its path, the grid of eggs with its sticks of dynamite, baby eggs, bombs, power-ups and the ceiling, and the four
modes: Stomped Puzzle (the rounds of a puzzle pack), Endless (Panic, with the whirleys), Fossil (the fragments and the
museum) and Time Trial. Then the options dialog with the high scores and credits, the tutorial with its steps and arrows,
the high-score name dialog, and the saved games: the save dialog's eight slots with their pictures, the name dialog, the
overwrite and "no slot" dialogs, and Continue from the main menu. It runs unmodified, with no mod hooks.

Left out on purpose: the unregistered copy's pieces (the register dialog, vtable 0x4fc890; the purchase pitch, 0x4ff5e0;
the dialogs 14 and 21 at 0x432740 and 0x4411d0), because the Steam copy is registered. The cursor clip of the board
(`ClipCursor`, `Dynomite.cpp`) is a TODO for a platform call, but nothing in the game turns it on.

Two things differ from the original's files on disk. The saved games (`SLOT0.SAV` to `SLOT7.SAV`), and the high scores (the
`.HS` files) are in the user folder rather than beside the exe. Stomped Puzzle copies its pack to
`STOMPED.TMP` in the game's folder; the port keeps that copy in memory (`BinFile::SetMemoryFile`). A Stomped saved game
still holds the whole pack, as the original's does, so each such slot is about 267 KB; in the browser the saves are kept in
IndexedDB and backed up in localStorage, where eight of them would take about 2.9 MB of the origin's space.

Still to do: a pass that compares each screen and rule with the original running (the fidelity pass), and the stores line
`BYOPOP_BUY_dynomite` in `cmake/ByopopStores.cmake`, which is a placeholder until the coordinator supplies it.

## The binary

`Dynomite.exe` is PopCap's 2006 wrapper (the Steam release). The game image inside it was linked on 2003-07-08 with
Visual C++ .NET 2002; its framework is the 2003 SexyApp, between Noah's Ark's and BookWorm's (`FRAMEWORK_2003.md`). The
port never runs the original. Most of the game's texts are string properties (`properties/default.xml`, read with
`GetString`); the few hard-coded in the exe are read from your `Dynomite.exe` by address (`src/GameText.h`), with each
text's length and hash checked, so no text is in the repository.

## Staging the game files

```bash
python games/dynomite/tools/stage_dynomite_assets.py ["<install folder>"]
```

copies images, data, sounds, music, the Stomped folder, `DEFAULT.EGG`, the properties and `Dynomite.exe` from your install
(the Steam library's `Dynomite Deluxe` folder by default) into the git-ignored `assets/dynomite/`. Nothing is written into
the install, and the DRM wrapper's files, `bass.dll` and your own high scores and saved games are not copied.

## Building and running

```bash
TARGET=dynomite tools/build.sh native        # build/native/dynomite.exe
build/native/rundesk.exe build/native/dynomite.exe
```

(`rundesk` keeps test runs invisible and silent; see CLAUDE.md. For a normal game run `dynomite.exe` directly.) Options:
`-gamedir=`, `-userdir=`, `-seed=N`, `-fast`, `-script=<file>`.

The web build is `build/web/dynomite.html` (`cmake --build build/web --target dynomite`). Its page asks for the game's
folder or a zip of it the first time and keeps the files in the browser; the preview `dynomite-web` in
`.claude/launch.json` serves `build/web` on port 18782 (`http://127.0.0.1:18782/dynomite.html?silent=1` for a silent test
run).

## Testing

```bash
tools/check.sh dynomite
```

plays a Stomped Puzzle from the title screen with five shots, logs the board, saves it into the first slot, does all that
again and checks that the screens and the saved game are the same (but for the saved game's tick counts), then continues the
saved game from the main menu and checks that the board loads as it was saved. Test scripts use the framework's `-script`
commands plus the game's own (`src/TestHooks.cpp`): `continue` and `step` for the tutorial, `board` (the board's mode,
score, egg count and a hash of its cells) and `combo <n>` (Inchy's call-out of a chain).

## Tools (`games/dynomite/tools`)

`decread.py` prints a function of the Ghidra export in a readable form, `vtscan.py` and `vtsyms.py` find and name virtual
tables, `loadtable.py` folds the loader into one line per resource and `genres.py` drafts the app's resource fields from it,
and `make_gametext.py` writes the table of text addresses `GameTextTable.inc` from `gametext_vas.txt`. They read
`re/dynomite/` (git-ignored: Ghidra's export and the carved game image). `GAME_NOTES.md` maps the game's classes and
rules.

## Mods

None. The port is the original game, unmodified and working like its exe, and adds nothing for later mods, not even hooks
(the user's rule of 2026-10-26); mods wait for the mod phase, after every PopCap game works in the browser. `MOD_IDEAS.md`
maps the 29 standard mods of `docs/MOD_ROADMAP.md` onto this game (28 fit; local achievements does not, with the reason)
and lists the ideas particular to it.
