# The 2003 SexyApp framework (Seven Seas Deluxe) and how the port maps it

Seven Seas Deluxe (WinSS.exe 1.1, linked 2003-01-29 with Visual C++ 6.0) was built on the same first generation of
PopCap's SexyApp as Noah's Ark Deluxe (`games/noahsark/docs/FRAMEWORK_2002.md` describes that one): a `SexyApp` class
that is also the `ButtonListener`, top-level widgets owned by the game, hard-coded image and sound tables. This note
records what the port had to know beyond that. The shared framework (`framework/`, SexyAppBase 1.3 API with
`compat2004`) needed no change for this game; the differences are handled in `games/sevenseas/src`.

## What differs from Noah's Ark

- `Widget` has 43 virtual slots here and 44 in Noah's: a slot number of a game override is one lower from about slot 3
  on (Draw is slot 16, Update 17). The numbers matter only when the two exes are compared; the port overrides by name.
- There is no RTTI for the game's own classes (compiled without /GR), only for the 50 framework and library classes.
  Their vtables were found from the constructors and ~125 virtual functions were missing from Ghidra's list
  (`tools/gapfuncs.py`, README).
- The widget manager keeps the keys in an array of flags (`mKeyDown`). The board reads the cursor keys, the number pad
  and the fire keys from it once per update (`Board::UpdateMove`, 0x4035d0) instead of handling `KeyDown`; the port
  reads the same array.
- Dialogs are built with the 2003 `Dialog` constructor (header, lines, footer, button mode) and the game's own subclass
  (0x425540). Their buttons arrive at the app as `2000 + dialog id` and `3000 + dialog id`, which is what
  `Dialog2004` of `compat2004` does, so `SevenSeasApp::ButtonDepress` (0x421a10) handles the answers of dialogs and the
  sidebar's buttons in one switch, as in Noah's Ark.
- The app owns its screens (title, menu, board, map, high-score screen, side panel, the pirate's head) and adds and
  removes them from the widget manager itself. The port's destructor therefore removes every widget it still holds from
  the manager before deleting it (an AddressSanitizer report at shutdown found the deletion of a widget the manager
  still pointed to).
- Fonts are `ImageFont`s made from a picture and a descriptor in `data\` (font1, scorefont, yesno, mediumfont, tutfont)
  and two Italian Old Style fonts. The five cursors are set with `SetCursorImage(0..4)`; cursor 4 is the shovel of
  the bonus screen.
- The music is one module (`music\game.it`) loaded into two song slots. The game starts a piece by its *order*
  (`SevenSeasApp::SetMusicOrder`, 0x420550; a level uses `(level - 1) % 11`, a bonus screen `(level / 10) % 4 + 0x1f`,
  the tutorial 0x14), alternating the two slots so that one can fade out as the other fades in. The port's mixer takes
  the order in the low 16 bits of the offset.
- Sounds are played with `PlaySample(id)` or `PlaySample(id, pan)`; the pan comes from the cell's column
  (`Board::Pan`, 0x40a440).
- The game has an animation list of its own on top of the framework: `data\anims.txt` (0x41c960) names strips of
  frames with delays; the board runs them as `AnimInstance`s (`Board::AddAnim`, 0x4032f0).

## Random numbers and time

The game draws from the C library's `rand()`; the port uses `CrtRand`/`CrtSRand` of `compat2004`, so `-seed=N` repeats a
game, and `GameTickCount` replaces `GetTickCount` so that `-fast` test runs stay deterministic. The generators draw in the
same order as the original's functions (the level generator's placement loops keep their shape for that reason).

## Saves

The original writes `data\savegame.dat` next to the exe (version 1: the internet score check's two strings, then the
board's fields in a fixed order) and `hiscores.dat` (the best score of the day, then the five best; each a name, a score and
a check value `score ^ 0x42beef` with every character multiplied by 0x11 xored in). The port writes the same fields in
the same order into the user folder (`%APPDATA%\wasm-bejeweled\Seven Seas Deluxe\`, `/sspersist` in the browser); the
score check's strings are empty. Options and the nag flags (`BonusNag`, `WarnBlueNag`, `WarnGhostNag`, `WarnSerpNag`,
`WarnWhirlNag`, `LastScoreName`) are in the registry file (`PopCap\SevenSeas`).

## Texts

Every text of the game is read from the player's `WinSS.exe` by address (`src/GameText.cpp`, `GT(0x4a2ec0)`), through the
table in `src/GameTextTable.inc` that `tools/make_gametext.py` writes (addresses, lengths and hashes only, no text). The
numeric layout tables (map dots, rank thresholds) are embedded because they are numbers, not text.
