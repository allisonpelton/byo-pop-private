# Rocket Mania Deluxe port

A port of PopCap and Nuclide's Rocket Mania Deluxe (2003) that plays natively (Windows, SDL2) and in the browser
(WebAssembly). It is a translation of the original's game classes into code on the shared framework; it ships none of the
game's pictures, sounds, music or texts, which come from your own copy of the game.

## Status (2026-10-26)

Done and tested: the loading screen (with the pictures the original makes from its files: tinted pipe pieces, rocket
hues, the talking head's frames, and the music joined from its segments), the menu with its hover hints, the difficulty
and continue dialogs, the quit question, the saved-game check.

Not done yet: the Board (the three game modes, 141 functions and 60 KB of the original's code, `GAME_NOTES.md` lists
them), the high score, options, record-score, level-up, money, hint and credits dialogs, the fireworks and the talking
head, saves, the autoplay test hooks, the web page. A new game currently logs `StartBoard` and stays in the menu.

## The binary

The game is inside `RocketMania.exe`, PopCap's 2006 wrapper (the Steam release). The game image is linked 2003-09-08 with
Visual C++ 6; its framework is the 2003 SexyApp (`FRAMEWORK_2003.md`). The port never runs the original and does not
need the wrapper's copy protection taken apart: the game's texts are read from the exe by address (`src/GameText.h`).

## Staging the game files

```bash
python games/rocketmania/tools/stage_rocketmania_assets.py ["<install folder>"]
```

copies images, data, sounds, music, properties and `RocketMania.exe` from your install (the Steam library's
`Rocket Mania Deluxe` folder by default) into the git-ignored `assets/rocketmania/`. Nothing is written into the install.

## Building and running

```bash
TARGET=rocketmania tools/build.sh native        # build/native/rocketmania.exe
build/native/rundesk.exe build/native/rocketmania.exe
```

(`rundesk` keeps test runs invisible and silent; see CLAUDE.md. For a normal game run `rocketmania.exe` directly.) Options:
`-gamedir=`, `-userdir=`, `-modsdir=`, `-seed=N`, `-fast`, `-script=<file>`.

## Testing

```bash
tools/check.sh rocketmania
```

runs `games/rocketmania/tools/menu_test.txt` (load, play link, menu, hover, difficulty dialog, new game) and fails on a bad
exit status, a failed load, a missing screenshot, or screenshots that do not differ. Test scripts use the framework's
`-script` commands plus `startgame` (clicks the play link once the loading screen shows it).

## Tools (`games/rocketmania/tools`)

`decread.py` prints a function of the Ghidra export in a readable form, `strfind.py` finds strings and their users,
`vtslots.py` and `ptrtable.py` read virtual tables and pointer tables from the game image, `gen_res.py` writes
`Res.h/.cpp` from the resource table, and `make_gametext.py` writes the table of text addresses `GameTextTable.inc`.
They read `re/rocketmania/` (git-ignored: Ghidra's export and the carved game image).
