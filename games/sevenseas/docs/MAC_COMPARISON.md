# What the Mac build shows (Seven Seas Deluxe)

The Mac build of the game (`re/sevenseas/mac/`, git-ignored, copied from the main checkout) is a PowerPC Mach-O
(`SevenSeasDeluxe`, 32-bit big-endian, prebound) with its symbols left in: 5111 names, `Seven_Seas_Deluxe.symbols.txt` made by
`machosyms.py`. It is the same game from the same source, built for the Mac, so its names are the original's: they say what the
Windows exe's functions were called. The port reads names and the function list from it, and nothing else (no code, no data).
The Windows exe stays the reference for behaviour: where the two differ, the port follows `WinSS.exe`.

The comparison was made at the port's commit `ede4a54` and refreshed at the commit that holds this file.

## The names

The port keeps its own names (`games/sevenseas/docs/GAME_NOTES.md` has the addresses). The Mac's:

| Mac class | port class | Windows address |
|---|---|---|
| `SSApp` | `SevenSeasApp` | 0x41bc00 |
| `Board` | `Board` | 0x401000 .. 0x40b490 |
| `Sidebar` | `SidePanel` | 0x41b560 |
| `Pirate` | `TalkWidget` | 0x4199f0 |
| `IntroScreen` | `IntroWidget` | 0x416900 |
| `TitleScreen` | `TitleScreen` | 0x4256f0 |
| `HighScoreWidget` | `HighScoreWidget` | 0x40ff70 |
| `HelpWidget` | `HelpWidget` | 0x40d330 |
| `SSDialog` | `GameDialog` | 0x425540 |
| `OptionsDialog`, `NextLevelDialog`, `RecordScoreDialog`, `HighScoreDialog` | the same names | 0x418760, 0x417a30, 0x419f40, 0x40f7b0 |
| `BonusLevelDialog`, `WarningDialog` | `NagDialog` (two kinds) | 0x40c7d0, 0x4262c0 |
| `RegisterDialog`, `InternetScoreDialog` | not ported (the registration and the internet scores are taken out) | |

Board member names that help read the port: `DoInitTutorial`, `DrawTutorialBox`, `DrawArrow(x, y, x, y, bool, Graphics*)`
(the port's `DrawCellArrow`, 0x409840; the bool is the broadside's range), `DrawEnemyArrows`, `GetColAt/GetRowAt/GetXAt/GetYAt`
(pixel to cell and back; the port's `CellToPixel`), `GetPanAt` (the stereo place of a sound by the cell), `FlipWhirls`,
`PlaceShipSafe`, `GenBonusLevel`, `FoundPostAnim`, `DoCalcDists`, `CannonLife`, `CanShootAt`, `CanMoveTo`, `DoWake`, `DoWakeDim`,
`DoGhost`, `DoPixel`, `DoBlotch`, `DoWave2`, `DoSplash`, `DoSparkle`, `DoExplode`, `DoBuffer`, `DoKillGood`, `UpdateYou`,
`UpdateWater`.

## What it showed that the port had wrong or missing

These were found in this pass with the help of the names (each one was then checked against the Windows exe and fixed):

- `OptionsDialog::SetColor(int, Color const&)` exists on the Mac: the options dialog passes its button-text colour on to its own
  four buttons. That is why the Help, High Scores and "Visit PopCap.com" labels are gold in the original and white in the port
  before the fix (`Dialogs.cpp`, the options constructor sets the label colour directly).
- `HelpWidget` has `ShowPirate`, and `SSApp::DoHelp`/`KillHelp` exist: the help screen was a real screen of its own, not the
  "do nothing" the port had. It is ported now (`Help.cpp`). In the Windows exe the pirate is hidden while it is up (the test in
  `TalkWidget::Draw` is the help widget's pointer); the Mac's `ShowPirate` is a difference of the Mac, not followed.
- The Mac has no demo class at all (no `DemoWidget`, no "demo" symbol): the beginner's demo (`WinSS.exe` 0x4132b0) is a Windows
  extra. The port has it (`Demo.cpp`) because the Windows exe is the reference.
- `SSDialog` (= `GameDialog`, 0x425540) is the app's generic dialog with the skull on its top edge; the port had drawn plain
  dialogs for the new-game and quit questions.
- `HighScoreDialog` draws with the larger font of the two (`MT22`) for its title; the port had used the gold letters.

## What is still to look at

- `SSApp::ConfirmSkillChange(int)` and `ChangeSkillLevel(int)`, `IntroScreen::SetSkillLevel(int)`: a question that comes up when
  the difficulty is changed during a game. The port's menu has no such question yet; check the Windows exe's version of it
  (the strings near "Are you sure you want to end the current game?") and port it if it has one.
- `SSApp::EnforceCursor`, `DoColorDepthExplination` (a message on 16-bit colour screens: not applicable to the port).
- `SSApp::GiveHelp(string, string, string)`: the tutorial's help boxes by a key; the port's tutorial boxes match the original
  screen by screen (`build/shotreview/sevenseas/t*`), so nothing known is missing.
- The Board's effect functions `DoGhost`, `DoPixel`, `DoBlotch`, `DoWave2` have no obvious counterpart by name in the port; the
  port's animations come from `data/anims.txt` by the original's names, and every explosion and splash of the demo matched. A
  play-through of a late level against the original would show if one is missing.
