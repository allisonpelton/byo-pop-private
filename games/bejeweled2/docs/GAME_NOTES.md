# Bejeweled 2 Deluxe: notes on the port

Bejeweled 2 Deluxe (Bejeweled2Game.exe, `Sexy::WinDMApp : SexyApp`, the global app object `gApp` at 0x67ccd8) is ported
from our own Ghidra export in `re/bejeweled2/` (git-ignored). The PC exe is the authority for behaviour; the Mac build's
symbol names (`re/bejeweled2/mac/Bejeweled_2.symbols.txt`) are used for names. The tool `games/bejeweled2/tools/dec.py`
decompiles a function of the export again (`<hex>`, `-r lo hi [minsize]` for a range, `-l` for a list, `-a` for the
assembly). Source files carry the original addresses in their comments, and every member of a translated class has its
original offset in a comment.

This file collects what the port does differently from the exe and what it found odd. It is written as the port is made;
a section that says "not yet" is still to be done.

## Deviations from the original

Each of these has no visible effect on play unless the entry says so.

- **Registration, trial and online code is left out.** No registration or unlock path, no trial counters (TitleScreen's
  trial logic, the nag screen before play, `DoPregameNag`), no announcement or update check, no "More Games" button
  (MainMenu), no PopCap pages (OptionsDialog's Register, PopCap page and Update Check buttons and the Automatic Update
  checkbox; `mAlwaysUpdate` is no longer set). OptionsDialog's Main Menu button with no board calls Shutdown in the original
  (a branch that cannot be reached); the port has no such branch.
- **Loading is done in line.** The original loads backgrounds, resources and the galaxy map on worker threads
  (`WorkerThread`) while the screen keeps drawing; the port runs those tasks in one go on the main thread (the framework runs
  a SexyAppFramework game's loading thread in one go in the same way). `backdrops.lst` is not rewritten, and the reload
  thread of the original runs in line. Board's image preparation (`m15f60` and the like) is in line too.
- **No Vista migration** of the old save folders; saved games use "/" as the path separator.
- **No game-state recorder** (GameStateEngine's recording), no write-only app byte (+0xf24), no `OutputDebugString` in
  UpdateBonusPenalty.
- **Dead functions are left out**: 0x59bb4f, 0x59bbc1, 0x59c390, 0x59dd3c, 0x594c71 and 0x5963ba are never called.
- **The 640 by 480 scaled-image cache is not ported.** The port always draws at 1024 by 768 and the framework scales the
  window (`ScaleNum` is kept, with the 1024 case only taken).
- **Memory leaks of the original are fixed**: Lightning's bolt array, `mPoints` when CheckForHints clears them, the old
  backdrop that the XMAS cheat replaces, and PuzzleStateOverlay's dome image (`mDomeImage`) are freed.
- **Uninitialised state is zeroed**: the ranges of a `Piece` that the original never sets, a ShockWave's grow flag (set
  true, as the original's uninitialised byte happens to be), `PuzzlePod::mIndex` (0 until the overlay's constructor sets it)
  and the tunnel rings' size, and points' x and y in Hyperspace (DoUpdate sets them before they are read).
- **The board's memory is zeroed** (`Board::operator new`). The original's constructor leaves some sixty members unset (for
  example `mChainCount`, the swap and hint phases, the slide values), which the first game reads as zero from fresh memory and a
  later game reads as the last board's leftovers; the port always reads zero.
- **A gem colour table copy**: `gOrigGemColors` is copied into `gGemColors` as the original's code does at start.
- **The four colour tables are one block** (`gColorBlock` in BoardEffects.cpp): the lightning, gem, score and original gem
  colours (0x6797d0 to 0x6799cf) are one run of memory in the original, and the gem types past 7 (rock, bomb, hyper cube) index
  past the end of a table into the next, so the port keeps them in the same order and the lookups give the same colours.
- **`m1728` is never set** (the original's code never does either; kept only for the offsets).
- **EffectOverlay.** The destructor frees the text picture (`mTextImage`), which the original leaks. The constructor zeroes
  what the original leaves uninitialised (the text's alpha, scale, stretch, age and hold, the portal and face timers, the wild
  gem's place, the fade) and sets `mHasAlpha`, which the first stub missed. The portal's wait for the backdrop's task
  (FUN_005e3594) is left out, since tasks run in line, and a missing backdrop is skipped. The face's data files (edata) are read
  with `ReadBufferFromFile` and an emulation of the C runtime's text-mode `fgets`/`feof` (CRLF to LF, Ctrl-Z ends, lines of 255
  characters); a line without a space, a polygon colour outside 0 to 9 and a polygon of more than 20 points (the data has at
  most 6) are skipped or clamped, where the original would read past its arrays. `powf`, `sinf` and `cosf` are libm calls
  rounded to float, where the original calls the C runtime's x87 versions: a last-bit difference is possible in the wild gem's
  curve and in the hint arrow's cosine.
- **GalaxyMap.** The map picture (`mMapImage`) that every Setup overwrites without freeing is freed first; `mSelectedPuzzle` and
  `mPuzzleBoard`, uninitialised in the original's constructor, are zeroed. The setup that the original runs on its worker
  thread runs in line (`mSettingUp` is never seen set), and the `Sleep` calls and the wait for the worker in Draw and in the
  destructor are left out.
- **Hyperspace's 3D tunnel needs texture addressing.** The original sets Direct3D's WRAP (u and v past 1) and CLAMP (v below
  0) mode for the tunnel's triangles; the framework's `DDInterface::DrawTrianglesTex` has no such control, so the picture is
  clamped. A spec for the framework follows once the port runs (`SetTextureAddress` in Hyperspace.cpp marks the place).
  This is a visible difference until then.

## Test commands

The standard commands of `docs/TEST_COMMANDS.md` are in `src/TestCommands.cpp` (`WinDMApp::HandleTestCommand`, reached from
`HandleScriptCommand`); a command that is never given changes nothing. Done: `state` (screen, dialogs and the buttons of the
top one, menu state, mode, level, level index, score, phase, tick, busy, the board as eight strings of gem types, the selected
gem), `goto <mode>` (a latch that `WinDMApp::UpdateFrames` steps once an update through whatever screen is in front, as a
player's clicks would: the title's play link, the first user's name ("Tester"), the welcome and other messages, the menu's
mode button, so it works from launch, from the menu and from a game; `goto menu` from the title clicks the play link, from a
game it runs `DoMainMenu`), `unlock all` (the counters that open the secret modes; given before a user exists it waits for
the goto that creates one), `seed`, `set score`, `lose` (the game's own ending), `win` and
`level <n>` (the level's goal is met, so the board's own update starts the whirlpool; in the puzzle modes `win` is
`PuzzleCompleted`), and `move` (the first of the moves `Board::FindMove` finds, picked by a hash of the board and the score;
the swap as two clicks in game pixels). `move` also answers for the situations around the play: while a goto is on its way, or a
game starts or ends, it asks to wait; with a message open (a one-time hint, the How To Play screen) it answers with the click
on the message's button, once the dialog has stopped sliding; in the puzzle modes (puzzles and Cognito) the move is the first
valid one of the puzzle's own solution file (`FindPuzzleValidMoves`, the moves the hint shows). Not yet: `set` for other keys,
`goto` for the options, help, high scores and credits screens. `tools/playbymove/playbymove.py` plays every mode but the puzzle
ones with `--first "unlock all" --first "goto <mode>"`; `games/bejeweled2/tools/realplay/soak.py` is a first driver that plays the
bot's moves as real input from the title on (the name typed, the dialogs clicked).

## Oddities (skipped, not chased)

- **PuzzlePackSelector's double click** calls `ButtonDepress(1000)`.
- **A missing galaxy file** (`GalaxyParser`): the "Unable to open galaxy.xml file" error is overwritten by the empty parse
  that follows it, so a missing file parses to an empty galaxy that has not failed.
- **galaxy.xml's case mismatch**: one file name is "26. green right.bpz" where the file is "26. Green right.bpz" (Windows
  does not care).
- **PuzzleStateOverlay::DrawFront** draws the unlit pods picture on `g` (colorize off) while it sets the alpha on a copy of
  the Graphics, so the alpha does not apply. The port repeats it.
- **NewBoard reads freed gems** (0x597efd): it frees the 64 gems of the old board without clearing their pointers and
  then makes the new gems one square at a time, asking HasSet after every colour it draws, so HasSet looks at the
  freed gems of the squares that are not made yet. Their memory holds what the allocator left there. The port clears
  the pointers (those squares read as empty); whether the original's HasSet ever counted a stale gem, and so drew one
  more colour, is for the identical-play comparison of a level change to show.
- **The tunnel's triangles are drawn with tx = 1.0**: the original's wrapper passes a byte that stands for "blend" to the
  argument that moves the vertices in x, so every vertex is one pixel to the right. The port repeats it.

## Fix candidates (for the fixes mod)

None found yet.
