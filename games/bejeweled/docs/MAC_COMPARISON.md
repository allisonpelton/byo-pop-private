# What the Mac build shows (Bejeweled Deluxe)

The Mac build of the game (`re/bejeweled/mac/`, git-ignored) is a PowerPC Mach-O (`BejeweledDeluxe`, 32-bit big-endian,
prebound) with its symbols left in: 4770 names, `Bejeweled_Deluxe.symbols.txt` made by `machosyms.py`. It is the same game
from the same source, built for the Mac with the same framework, so its names are the original's. The port reads names,
signatures and the function list from it, and nothing else (no code, no data). The Windows exe stays the reference for
behaviour: where the two differ, the port follows `WinBej.exe`.

The comparison was made at the port's commit `e6b16ad` (the commit before the Mac folder was first opened in the
identical-play pass, `IDENTICAL_PLAY.md`) and holds at the commit that adds this file: the pass changed no port code.

## The classes

The port's classes have the Mac's names: `WinDMApp`, `Board`, `Piece`, `TitleScreen`, `HighScoreWidget`, `HighScoreDialog`,
`OptionsDialog` and `RecordScoreDialog` (`games/bejeweled/src`). Every member name of `OptionsDialog` (11), `RecordScoreDialog`
(9), `TitleScreen` (8), `HighScoreDialog` (4) and `Piece` is in the port. The signatures the Mac gives agree with the port's
reading of the Windows exe where they carry information: `Board(WinDMApp*, bool)` (the bool is timed mode),
`FindMove(int*, int, bool, bool)` (a move out of the list, horizontal and vertical), `GameOver(int)` (the shake ticks),
`Pause(bool)` and `Unpause(bool)` (the user's pause or the focus), `KeyChar(char)`, `GetPanPosition(int)` and
`DoBouncyText(string, int)` (text and duration).

Classes the Mac has and the port does not: `RegisterDialog`, `UpdateCheckDialog`, `InternetManager`, `HTTPTransfer`, `BigInt`
(the registration key's arithmetic) and `AdInstance`, all of them registration, the update check and online features, which
the port leaves out (the Steam exe runs registered, so the nags never show; `GAME_NOTES.md`, the dialog table); `FModSoundManager`,
`OSXSurface`, `OSXFont`, `MacOSXPasteboard` and `CursorWidget` are the Mac's platform layer (the port's custom cursors are the
framework's, and they matched the original's in the comparison).

## Names that differ

The port kept its own names where it had named a function before the Mac build was looked at. The Mac's, with the port's
and the Windows address:

| Mac | port | Windows |
|---|---|---|
| `Board::HasSet`, `FindSets` | `HasMatch`, `ProcessMatches` | 0x4c9cf5, 0x4cd287 |
| `Board::SwapPieces`, `UpdateSwapping` | `StartSwap`, `UpdateSwap` | 0x4c9ff8, 0x4cd84f |
| `Board::DoBouncyText`, `CreateBouncyTextDraw` | `ShowText` (the drawing is in `Draw`) | 0x4ca1c8 |
| `Board::DoWhirlpool`, `UpdateWhirlpool` | `StartBlackHole`, `UpdateBlackHole` | 0x4ca58d, 0x4cb931 |
| `Board::UpdateGameOverAnim`, `UpdateLevelTransition`, `UpdateBoardDropping` | `UpdateGameOver`, `UpdateLevelWipe`, `UpdateFallOut` | 0x4cab82, 0x4cb284, 0x4cb3bf |
| `Board::TryShowHint` | `DoHint` | 0x4cad2f |
| `Board::SetupLevelData`, `NewBoard` | `SetupLevel`, `FillBoard` | 0x4cae29, 0x4cb182 |
| `Board::GetPanPosition` | `GetPanForX` | 0x4c9d94 |
| `Board::IsInProgress`, `HasSavedGame`, `DeleteSavedGame` | `IsGameInProgress`, `SaveGameExists`, `DeleteSaveGame` | 0x4c9bbe, 0x4cca37, 0x4cd130 |
| `WinDMApp::DoOptions`, `DoHighScoreDialog`, `DoRecordScoreDialog` | `ShowOptionsDialog`, `ShowHighScoreDialog`, `ShowRecordScoreDialog` | 0x4d483e, 0x4d48b6, 0x4d45e3 |
| `WinDMApp::DoInstructionDialogette`, `DoFullInstructions` | `ShowInstructionsDialog`, `ShowFullInstructionsDialog` | 0x4d59cc, 0x4d5ac1 |
| `WinDMApp::DoColorDepthExplination`, `TryConfirmLoad` | `ShowColorDepthDialog`, `ConfirmLoadGame` | 0x4d5bef, 0x4d550d |
| `WinDMApp::DoLoadImage`, `CreateControlButton` | `LoadAppImage`, `MakeButton` | 0x4d62b8, 0x4d6e71 |
| `WinDMApp::DoRegister`, `DoRegisterError`, `DoRegisterScoreNag`, `DoPregameNag` | `ShowRegisterDialog` (its "already registered" branch only); the others not ported | 0x4d5272, 0x4d5386, 0x4d5486, 0x4d5cdf |
| `WinDMApp::CheckForUpdates`, `ConfirmCheckForUpdates` | not ported (the update check, `GAME_NOTES.md`) | 0x4d68da |
| `HighScoreWidget::GetScorePlace`, `GetDayNum`, `GetOrderSuffix` | `GetRank`, `GetDayNumber`, `GetOrdinalSuffix` | 0x4ced82, 0x4ced59, 0x4cf4a7 |
| `HighScoreWidget::ReadFromFile`, `WriteToFile`, `ProcessScoreStuff` | `LoadHighScores`, `SaveHighScores`, `DoCheckForHighScore` | 0x4ceec2, 0x4cf1b4, 0x4d0e4b |

The Board's seven update functions are the port's seven states one to one (falling 0, swapping 2 and 3, clearing, game over 5,
whirlpool 7, level transition 8, dropping 9), so the Mac knows no state the port lacks.

## What it showed that the port had wrong or missing

Nothing. Every Mac function of the game's own classes has its counterpart in the port, or belongs to the registration, the
update check, the online scores or the Mac's platform layer, which the port leaves out on purpose. No behaviour was changed
because of the Mac build.

## What is still to look at

- `WinDMApp::DoGameOver` and `LoadingCompleted` have no function of their own in the port: in the Windows exe the game over
  is handled in `Board::UpdateGameOver` and the app's `CheckForHighScore`, and `LoadingThreadCompleted` (0x4d4591) is empty.
  They may be Mac-only splits of the same code; the game over and the name entry matched the original shot for shot
  (`IDENTICAL_PLAY.md`), so nothing known is missing.
- `HighScoreWidget::NameEntered` is the record dialog's answer, which the port handles in `AddHighScore` (0x4d06d8); it was
  compared in the timed and normal game-over runs.
