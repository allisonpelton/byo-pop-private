# Pixelus Deluxe: identical-play pass, first phase (2026-11-03)

Phase one compares the game's state, update by update, between the original (`tools/origrun`, virtual clock, scripted
cursor, the seed patched in) and the port (`-fast`). It is **not finished**: the tutorial, a saved game carried from one
program to the other, and the records table after a puzzle are not compared yet (see "Open"). The screens (phase two)
have not been started either.

## How it was compared

`games/pixelus/tools/identical/case.sh <name> <bot seed> <game seed> <puzzles> 2500 [-autoplayskip]` runs one case:

1. The port's bot plays N puzzles through the mouse (`-autoplayui`); its input is written as an origrun script
   (`-autoplayscript`) and its state, change by change with the update it came at, as a state log (`-statelog`,
   `src/testhooks/StateLog.h`): the screens that exist, moves, time, tiles' places, states and pictures, supply, cursor,
   the game's random-draw count and so on.
2. `toorig.py` turns the script into the original's (2500 updates later: the original's loading screen is longer;
   button clicks hover two updates early), `orig.sh` plays it in a copy of the original with up to four `--watch`
   values per run, and `cmp.py` compares each value's changes with the port's. A value is "same" when every change
   comes with the same value at the same update.
3. The global generator's draws were also compared per update: the original's Mersenne index (0x579d20) against the
   port's draw counter (`rand` in the state log).

Cases played: bot seed 1 / game seed 1 and bot seed 2 / game seed 7, three puzzles each, the tutorial skipped
(`-autoplayskip`), from a first run (user question, new player, help screen, story, island, temple, board, completed
screen). Both match in every value except the two listed under "Oddities".

## Changes this pass made

| Difference | Cause in the original | Port now | Commit |
|---|---|---|---|
| Skipping the tutorial went back to the island | 0x4116xx (SkipTutorial OK) shows the story, not the island | `SkipTutorialOK` shows the story | 3910d0c |
| No help screen for a new player | `UserProfile` ctor (0x439e70) sets the "show help" flag | flag defaults to true | 3910d0c |
| Tile pictures differed from the first board on | the tile constructor (0x42eee0) draws `rand % 5` on the game's own generator (0x579360, seeded in `SexyApp::Init`) for every tile made, in a puzzle's file as in a throw; the port drew with `CrtRand` after the fact at 7 sites | `Grid::NewObject` draws on `PixelusApp::mRand` (the same MT, `SeedMTRand2004`); the 7 sites are gone | 3910d0c |
| The draws after the first screens drifted | other consumers of the two generators that the port lacked: the characters of the temple, story and tutorial (0x42e110, 0x42df40; blink, pose changes, spring), their speech bubbles (0x42c4a0, 0x42c590; draw on the application's own generator, default seed 4357), the grid's wave timer (0x425d40, 0x424b30), the board's sidebar gleam timer (0x407f10, 0x409bd0) | `Character` (new), `Grid::mWaveTime`, `Board::mPulseTime`; `PixelusApp::mAppRand` | 3910d0c |
| Temple click drew 1 + tiles fewer numbers | the click (0x41162d) loads the puzzle into a grid of its own (0x427430) to get its id | `TempleScreen::MouseDown` does the same load and throws the grid away | 3910d0c |
| Help screen drew 2 + 3 fewer numbers | its demo grid (0x40ddd0, 0x40d320) makes a grid and a tile and throws a tile at frames 50, 150 and 250 of 950 | `HelpScreen` draws the same numbers; the demo itself is not drawn (phase two) | 3910d0c |
| Initial cursor on the board | the original does not aim at the board's start | the first `Aim` is gone | 3910d0c |
| Dialog width of the user dialogs | 0x190 | `SizeDialog(..., 0x190)` | 3910d0c |

With these the global generator's draw count matches the original at every update of three puzzles (temples 198 and
463 draws, boards 159, 85, 79, the help screen's frames), and so do all tile pictures. `tools/check.sh pixelus`'s full-run
hash changed from 2edb2003 to 357cb9be because the pictures now come from the original's sequence (shared commit 6ca7d85,
which also adds the check against `build/baseline-pixelus`).

## Oddities (not chased)

- **Cursor and side after the first throw.** The original's cursor/side values (`grid+0xb054`, `+0xb058`) at the first
  move are the ones the port shows after its second move (it skips one change), then both agree. The tiles' places, the
  moves and everything else are the same; the values are the aim, which the bot's hover/click pairs set in a different
  order from a player's. See it in `case.sh` output as `cursor`/`side DIFFERS at change 1`.
- **The app generator is 3 draws apart after the story is closed.** The original runs the story's last update before the
  scripted click that closes it; the port closes it before. The difference stays constant afterwards (the characters'
  three draws), the global generator is unaffected.
- Two more users of the original's generators were not matched and did not show in the runs: the main loop's draw
  (0x4943b0) and the display-mode chooser (0x47acd0).

## Deviations that remain (each without an effect on the state)

- The speech bubbles' text and picture (temple, story, tutorial) are not drawn; their state (which draws numbers) is kept.
- The story's portrait is a still head; the original renders the animated figure with its pose fades.
- The board's sidebar gleam (the colour panel at +0x1d8) is not drawn; its timer is kept.
- The help screen's demo grid (left of the slides) is not drawn; its draws are kept.

## Open (this phase is not complete)

- **Tutorial.** The original's tutorial board ignores the bot's clicks (no hover, no move), also when they come 300
  updates later; the app's blocking flag (`+0x6a8`) and pause counter (`+0x630`) are both 0, the control mode is 2. The
  port's tutorial accepts them. The cause is not found; the teacher (0x42e110 with mode 2 on the board) and the helper
  widget (`board+0xf4`, 0x40ece0) of the original are not ported.
- A saved game written by one program and continued by the other, the records table after a puzzle, and the other
  control modes (0 and 1) are not compared.
- 87bce0a (misplaced text, World Record background) against the original's screen with tools/shotdiff (Windows
  Sandbox): not done.

## Notes for the repository

`tools/identical/setup.sh` copies `re/pixelus/bin/PixelusGame.exe` (the game made by `tools/extract_game_exe.py` from the
wrapped exe) into `C:/byo-og/pix` and writes `tools/origrun/patches/pixelus.txt` (git-ignored; the two seed patches).
No script added here takes a wrapper apart, so nothing new belongs in `.publishignore`.
