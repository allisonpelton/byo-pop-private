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
| The tile maker started at the bottom of the grid | 0x42bd40 starts him above it (side 2) at the first column | `TileMaster` starts at `SIDE_DOWN`, cursor 0 | 5e00f97 |
| Dotted aim line, target and base marks on a fresh board | 0x422ab0 draws those only in control modes 0 and 1; the board's mode (+0x19c) is 2 from its constructor (0x407f10) and nothing else writes it. In mode 2 it draws the cursor on the hovered cell (the red one when a click does nothing) and the arrows | `Board::DrawAim` does the same | 7ff6a91 |
| The player's file (`users/<n>`) was in a layout of the port's own | 0x4391c0, 0x438450, 0x439fb0: version 4, four-byte string lengths, the temples by number (0 tutorial, 1..9) with their three flags and their records (uuid, "maxed" flag, fewest moves, second count), the records of a temple ordered by UUID as a signed number | `UserProfile::Write/Read` produce and read that layout; the bytes match the original's for the same play | b29b14e |

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

## Compared since the first report

- **The player's file after three scripted puzzles:** `users/1` is byte for byte the original's up to where the original's
  run went on one puzzle longer (it was stopped later); `users/directory.txt` and the puzzle history `users/his<n>` are
  identical. This covers the records (fewest moves, second count, "maxed") a puzzle leaves, and a profile saved by one
  program is read by the other (the port's reader follows 0x439fb0 field by field; it was not run on a file the
  original wrote beyond the one above).
- **Control modes 0 and 1** do not exist in play: the board's mode (+0x19c) is set to 2 in its constructor and no code
  writes it. The keyboard/mouse-aim paths of 0x403370 and 0x408400 for modes 0 and 1 (and the editor's grid flag
  +0xb0a1) are dead, so they are not ported.
- **87bce0a (sidebar text, World Record background):** the sidebar of the board is identical to the original's at the
  frames before, during and after a puzzle's end (shotdiff of the original run in the Sandbox against the port at the
  same updates, runs s2 to s4); no box lies in the sidebar.

## Phase two findings so far (not fixed; found by those shots)

- The original cross-fades between screens (help to island, board to the completed screen, temple to board); the port
  swaps them. At the same update the original is mid-fade.
- The completed picture's sweep: the tiles ahead of the sweep turn yellow in the original and stay stone in the port.
- The temple screen: the original's puzzle thumbnails are drawn in grey/blue/cream with a "Scroll Puzzles Down" sign; the
  port draws red crosses and a "puzzles left unsolved" line.
- Tile icons and the sliding trail are drawn slightly differently; the tile maker's pose differs by a frame.

## Open (this phase is not complete)

- **Tutorial.** The original's tutorial board ignores the bot's clicks (no hover, no move), also when they come 300
  updates later; the app's blocking flag (`+0x6a8`) and pause counter (`+0x630`) are both 0, the control mode is 2. The
  port's tutorial accepts them. The port has no tutorial lesson at all: the original's board is driven by a script of
  steps (0x4061f0, 0x406310, 0x406470, 0x4066b0 and their neighbours, using the helper widget `board+0xf4`, 0x40ece0, and
  the teacher), which decides what a click may do at each step; the port's tutorial board is an ordinary board. Porting
  the lesson is a separate piece of work (the steps, the helper widget's texts and arrows), not a harness question.
- The puzzle currently being played (`his<n>` holds its moves) was only compared after the puzzle's end, not
  continued across programs in mid-puzzle.

## Notes for the repository

`tools/identical/setup.sh` copies `re/pixelus/bin/PixelusGame.exe` (the game made by `tools/extract_game_exe.py` from the
wrapped exe) into `C:/byo-og/pix` and writes `tools/origrun/patches/pixelus.txt` (git-ignored; the two seed patches).
No script added here takes a wrapper apart, so nothing new belongs in `.publishignore`.
