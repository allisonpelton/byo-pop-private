# Dynomite Deluxe port

A port of PopCap's Dynomite Deluxe (2003) that plays natively (Windows, SDL2) and in the browser (WebAssembly). It is a
translation of the original's game classes into code on the shared framework; it ships none of the game's pictures,
sounds, music, puzzle packs or texts, which come from your own copy of the game.

## Status (2026-11-12)

Both phases of the identical-play pass are done (`IDENTICAL_PLAY.md`): the same input plays the same games in the
original and the port, and the screens, the dialogs, the sound, the focus and the web build were compared with the
original's, with real-input play natively and in the browser. The pass found no fix candidate, so there is no fixes
mod. The coverage check, the performance pass, taking out the code the port does not need, the readability pass and the
check against the PopCap Game Collection's copy are done too (`GAME_NOTES.md`, sections 6 to 10). The native port now
draws its screen in software as the original does, and at 640 by 480 it is under the original's processor time
(`GAME_NOTES.md`, "The software screen"). The lite mod step found nothing worth a toggle (`LITE_MOD.md`), and the
autosplit block serves the site's Splits window (`GAME_NOTES.md`, section 11). Of the definition of done
(`docs/COORDINATION.md`, "Done means every phase but modding") only the rest of the game's features on the site are
left: its Music entries and soundboard presets, which the site's session builds.

The game is translated: the loading and title screens, the main menu with its pages (start, modes, skill, the fossil
challenge's kind, saved games, puzzle packs), the side bar with Inchy and his call-outs, the board with the slingshot, the
aim and its path, the grid of eggs with its sticks of dynamite, baby eggs, bombs, power-ups and the ceiling, and the four
modes: Stomped Puzzle (the rounds of a puzzle pack), Endless (Panic, with the whirleys), Fossil (the fragments and the
museum) and Time Trial. Then the options dialog with the high scores and credits, the tutorial with its steps and arrows,
the high-score name dialog, and the saved games: the save dialog's eight slots with their pictures, the name dialog, the
overwrite and "no slot" dialogs, and Continue from the main menu. It runs unmodified, with no mod hooks.

Left out on purpose: the unregistered copy's pieces (the register dialog, vtable 0x4fc890; the purchase pitch, 0x4ff5e0;
the dialogs 14 and 21 at 0x432740 and 0x4411d0), because the Steam copy is registered. The cursor clip of the board
(`ClipCursor`, `Dynomite.cpp`) is left out, since the platform has no call for it: only the registry value
`LockCursor` turns it on in the original, and no dialog sets it.

Two things differ from the original's files on disk. The saved games (`SLOT0.SAV` to `SLOT7.SAV`), and the high scores (the
`.HS` files) are in the user folder rather than beside the exe. Stomped Puzzle copies its pack to
`STOMPED.TMP` in the game's folder; the port keeps that copy in memory (`BinFile::SetMemoryFile`). A Stomped saved game
still holds the whole pack, as the original's does, so each such slot is about 267 KB; in the browser the saves are kept in
IndexedDB and backed up in localStorage, where eight of them would take about 2.9 MB of the origin's space.

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
the install, and the DRM wrapper's files, `bass.dll`, your own high scores and saved games and the sounds the original
caches on its first run (`sounds/cached_*.wav`) are not copied. The PopCap Game Collection's copy stages the same way
(`GAME_NOTES.md`, section 7).

## Building and running

```bash
TARGET=dynomite tools/build.sh native        # build/native/dynomite.exe
build/native/rundesk.exe build/native/dynomite.exe
```

(`rundesk` keeps test runs invisible and silent; see CLAUDE.md. For a normal game run `dynomite.exe` directly.) Options:
`-gamedir=`, `-userdir=`, `-modsdir=` (default `mods-dynomite` beside the exe; a mod with the feature `livesplit` turns
on the autosplit block, GAME_NOTES section 11), `-nomods`, `-seed=N`, `-fast`, `-script=<file>`, `-livesplitlog=<file>`.

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

The standard test commands (`docs/TEST_COMMANDS.md`) are in `src/TestHooks.cpp`, reachable only from a test, each
answered on stdout. `state` prints `STATE {...}` with the keys `screen` (title, menu or game; the tutorial's board is a
game), `tutorial` (its step) or `page` (the main menu's page), `dialog` (the topmost dialog's id), `score`, and on a
board `mode` (stomped, endless, fossil, timetrial), `paused`, `busy` (no move can be made now), `over`, `level` (the
fossil and its `round` in the Fossil Challenge, the round in Stomped, the whirley level in Endless) and `board`: `eggs`,
the `loaded` and `next` colours, `frozen` and `rows`, the grid's cells a row a string of hex colours with `.` for
empty. `goto` opens `menu`, `options`, `highscores` or `credits`, or starts `endless`, `stomped`, `fossil`,
`timetrial` or `tutorial` through the main menu's own pages (their present skill and pack). `level <n>` starts fossil
n of the Fossil Challenge from its first fragment. `set score <n>` sets the side bar's score. `win` clears the board in
Stomped and Time Trial, so the board's own end follows, and in the Fossil Challenge takes every egg and lets the fossil
fall, so the fragment's own end follows (its museum, and after fossil 15 the Rex attack); `lose` puts the eggs past the line as in play (Inchy sinks and
the eggs run off), once the first shot has started the game. `seed <n>` seeds the app's twister and its picker.
`unlock all` answers with an error: nothing is locked in the port. `move` prints the bot's next move (`src/Bot.cpp`):
in the tutorial its Continue button while it shows, then the museum's own button while the museum is open (and the side bar's museum button at the tutorial's step 93, which waits for it), at the tutorial's whirley step the shot whose flight meets the whirley, and at its steps 43 and 45 the lowest shot without a match, else the shot that joins the most eggs of the loaded colour, found
by flying every angle the mouse can give with the grid's own probes, with its `input` (a move to the point that aims it
and a click there). The bot reads the board and changes nothing. `matchmove`, the game's own, is `move` without the
low shots of steps 43 and 45: there it gives the shot that joins the most eggs, as a player would make (`tools/shots/botrec.py
--match45` uses it to see what the original does then).

`tools/realplay/play.sh <steps> <out>` plays the port natively with real input only, through Bejeweled Twist's driver
(`games/bejeweledtwist/tools/realplay/drive.py`); `tools/realplay/*.txt` are its step files (the tutorial and a game of
each mode). `tools/web/onegame.js` does the same in the player-only page through the shared test kit (`?driver=`,
`?mode=` tutorial, endless, stomped, fossil, panic or timetrial). `tools/shots/` holds phase two's comparisons with the original: `tour.sh` (screens, with shotdiff),
`sound.sh` and `soundcmp.py` (sound) and `web.sh` (the web build against the native port).

## Tools (`games/dynomite/tools`)

`decread.py` prints a function of the Ghidra export in a readable form, `vtscan.py` and `vtsyms.py` find and name virtual
tables, `loadtable.py` folds the loader into one line per resource and `genres.py` drafts the app's resource fields from it,
and `make_gametext.py` writes the table of text addresses `GameTextTable.inc` from `gametext_vas.txt`. They read
`re/dynomite/` (git-ignored: Ghidra's export and the carved game image). `GAME_NOTES.md` maps the game's classes and
rules.

## Registration and loading

The port plays as the registered original, which stays the reference (the user, 2026-11-07). The code only an
unregistered copy reaches is taken out: on the title screen the register link with its four changing texts, the free
plays left, the quit link once they ran out and the loading bar slowed by the number of plays; the registered copy's
"thanks for registering" over its dark box under the loading bar (left out since 2026-11-08: "'Thanks for registering'
should be considered part of the registration/trial code to be removed from every game", the user), with the title's
flag for it and its read of the NoReg property; in the options dialog the register button, so the high scores button keeps the full
width it has in a registered copy; the register and buy dialogs (14 and 20, with the buy button's dialogs 10020 and
20020 in the app's dialog handler) and the nag each mode showed at a game's end in their place; the unregistered
copy's refusals of the saved games and of puzzle packs other than DEFAULT.EGG on the main menu and of the side bar's
save; and the "unregistered" picture. None of these but the thanks shows in a registered copy, and the
frozen baseline checks that play is unchanged. The loading has no wait of the original's own apart from that slowed
bar: the loader runs at the machine's speed, and the title polls it every fourth update as the original does.

Nothing in the port leads out of the game (the user, 2026-11-07). The options dialog's partner links and "check for
updates" were never ported; the credits name the game's designer and its musician as links to their sites in the
original (green, underlined, ids 0 and 1), and the port draws those two names as plain text in white like the other
names, in line with them. That is the one visible change.

## Mods

None. The port is the original game, unmodified and working like its exe, and adds nothing for later mods, not even hooks
(the user's rule of 2026-10-26); mods wait for the mod phase, after every PopCap game works in the browser. `MOD_IDEAS.md`
maps the 29 standard mods of `docs/MOD_ROADMAP.md` onto this game (28 fit; local achievements does not, with the reason)
and lists the ideas particular to it.
