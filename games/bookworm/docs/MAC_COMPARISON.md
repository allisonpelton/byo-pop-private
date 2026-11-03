# What the Mac build shows (BookWorm Deluxe)

The Mac build of the game (`re/bookworm/mac/`, git-ignored, copied from the main checkout) is an Intel Mach-O (`Bookworm`,
i386, version 1.0.5 by its `Info.plist`) with its symbols left in, demangled in `Bookworm.symbols.txt` (10,499 lines). It is a
later build than the Windows 1.03 the port was decompiled from: it draws through OpenGL, keeps its players in property lists
and has internet high scores, an update check and ads. Its names come from the same source, so they say what the Windows exe's
functions were called; its code was not read. The Windows exe stays the authority for behaviour: where the two differ, the
port follows `BookwormGame.exe`.

The comparison started at the port's commit `d697648`, the first time the Mac files were opened (during phase one of the
identical-play pass, `IDENTICAL_PLAY.md`), and this file was written at the commit that holds it.

## How close the decompilation was

About 450 game functions have Mac names (ctors and one-line overrides included; framework classes, OpenGL, threads, the plist
tree and `AnimInfo` left out). About 150 have a port function of the same name and job, and about 95 a port function that does
the same job under another name. Four are in another place: the Mac's free functions `isPieceAdjacent` and `deselect` are the
port's `Board::IsAdjacent` and `Piece::Deselect`, its `Piece::SetDeadMovesScale` is the port's `Board::SetActionSpeed`, and
its `BookWormApp::DrawString` and `MakeClippedString` are free functions in the port. About 70 belong to Mac-only or online
features the port leaves out on purpose (the update check, registration and its trial path, internet high scores and their wait
and error dialogs, ads, the Mac's software cursor and lockable buttons, plist storage). About 15 have no port function, and
every one of them turned out to be a step the Windows build has inline or does elsewhere (below).

The class boundaries were right. Every game class the Windows exe has, the port has with the same job, and the second and
third base classes the Mac's thunks show (`ButtonListener`, `CheckboxListener`, `SliderListener`, `EditListener`,
`ListListener`) are the port's for every class: `Board`, `BWGameSelector`, `TitleScreen`, `HintDialog`, `LevelUpWidget`,
`FameWidget`, `HelpWidget`, `OptionsDialog`, `NewUserDialog`, `SelectUserDialog` and `DeleteUserDialog`. The effect hierarchy
(`Effect` with `Sparkle` and `FlyingPieceList`) is the port's. The Mac's `UserProfile` and `UserProfileMgr` are the port's
`User` and `UserManager` (a rename; the Mac stores them in a property list, the PC in the registry), and its `FameData` is the
port's `HighScoreEntry`.

The names are where the decompilation differs most. The port named functions by what they do, and the Mac's names are often
other words for the same thing: `GetPointsAtLevel` is `LevelThreshold`, `CheckForFalling` is `Refill`, `GenerateSuperBonusTiles`
is `AwardSpecialTile`, `UseBonusWord` is `BonusWordScored`, `GetNextBonusWord` is `PickBonusWord`, `BookWormApp::NewGame` is
`StartGame`, the `Do...`/`Kill...` pairs of the app are `Show...`/`Remove...`, `Read`/`Write` everywhere are `Load`/`Save`,
`StatKeeper::StatSubmit` is `AddWord`, and `WordScore::toLetterValueScore` is `CalcScore`. A few show the port reading a
thing's purpose too narrowly: what the port calls the overlay's banner the Mac calls the bonus word (`ShowBonusWord`,
`IsBonusWordMoving`), and the port's "action speed" table feeds what the Mac calls the dead-moves scale of a burning tile.

Merges and splits: the Mac has `Board::PreSubmit` and `PostSubmit` where the port has one `Submit`, separate `GameOver`,
`AddPiece`, `MakeLastPiece` and `GetAverageRow` steps that the port has inline in `Update`, `BurnTiles` and the fire checks, and
a second `GetRandLetter(int*, int)` that picks from a table where the port has one `RandomLetter`. Going the other way, the port
has functions the Mac has no symbol for (`AddLetter`, `TruncateAfter`, `BurnTiles`, `CheckBottomFire`, `StartGameOverMusic`),
which the Mac compiler inlined. The Windows exe has these as the port has them, so they are not faults; the readability pass
may take the Mac's names and splits where the Windows code allows.

Types and constants: the Mac's user values are `unsigned long` where the port reads `int`; nothing reads a value large enough
for the two to differ. The Mac's scoring tables (`WORD_VALUE`, a second `LETTER_VALUE`, the three `..._RED_FREQS` tables)
differ in size from the port's, which are read from the Windows exe and match its draws (phase one compared thousands of
board-generator draws and every score with the original).

## What it showed that the port had wrong or missing

Nothing with an effect on play. Three names contradicted what the port's comments claim, and each was checked against the
Windows exe:

- `Piece::SetDeadMovesScale` and a static `Piece::mDeadMovesScale` suggest the burn countdown is scaled. In the Windows exe the
  value `SetActionSpeed` stores (0x52e278) is written once at 0x402fd9 and never read, as the port's comment says.
- `GenerateSuperBonusTiles` keeps a static `aLastLength`, which suggests the six-letter case is gated by the previous word's
  length. In the Windows exe its counterpart (0x5347c8) is only compared, never written, so the six-letter draw stays dead, as
  the port has it.
- `HintDialog::IsHintDisabled` with a static `mHintMap` suggests hints are switched off one by one. In the Windows exe the map
  (0x534c38) is only constructed and destroyed; the lookup (0x42b3e0) uses key 0 whatever the hint, and nothing inserts, as
  the port has it.

These are differences of the later Mac version, not followed.

## What the AI got wrong, and the lessons

The decompilation got the structure right and the behaviour right (phase one found the port's play the same as the original's
in every case run); what it got wrong is naming and grouping. It invented descriptive names where the original's were terser
or used the game's own vocabulary ("bonus word", "dead moves", "super bonus tiles"), it read one or two purposes too narrowly
from where a value was used, and it merged steps the compiler had inlined. None of that had a visible effect. The lessons for
the next decompilations:

- Take a class's names from the Mac build when there is one, before naming from use: the game's own vocabulary makes the
  code easier to check against the original's texts and a later reader's memory of the game.
- Where a Mac name contradicts a comment, check the Windows exe before changing anything: three of three contradictions here
  were the Mac version's changes, not the port's faults.
- A static or global the port calls "never read" or "never written" deserves a grep of every access in the export (the three
  checks above took a minute each).

## What is still to look at

- `BWGameSelector::GetFirstName(string)` suggests the Mac greets the player by the first word of the name; the port draws the
  whole name. Phase two's screen comparison with the Windows exe will show which the PC does.
- `aNumBigMoveMessages`, `aNumSmallMoveMessages`, `aNumTimerMessages` and `aKeyLetters` are initialised data with no port
  counterpart: perhaps remarks Lex makes after big or small moves and in Action, in the Mac version only. The phase-one games
  matched the original's state and draws, so the Windows exe makes no such choice from the generators the comparison watched;
  phase two will show whether Lex says anything the port does not.
- `BookWormApp::DoColorDepthExplination` (a message on 16-bit screens) and `LoadingCompleted` have no counterpart and need none
  on the port's platforms.
