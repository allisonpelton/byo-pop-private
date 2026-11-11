# What the Mac build shows (Alchemy Deluxe)

The Mac build of the game (`re/alchemy/mac/`, git-ignored, copied from the main checkout) is a PowerPC Mach-O (`AlchemyDeluxe`,
version 1.42) with its symbols left in: 5042 names, `Alchemy_Deluxe.symbols.txt`, made by `machosyms.py`. It is the same game from the
same source, built for the Mac, so its names are the original's. This comparison read the names, the order of the functions by
address and, from consecutive addresses, their sizes, and nothing else: no code and no data. The Windows exe stays the reference
for behaviour; where the two differ, the port follows `AlchemyGame.exe`.

The port's commit before the Mac files were first opened was `4e2bfa0f` (2026-11-08), and the first commit of this file records it.
Everything below is what the names showed about the port after that commit. The Windows exe has RTTI for the framework's classes only,
so the names of the game's own classes in the port (`GAME_NOTES.md`) were the decompilation's, and this is the first time they could
be checked against the original's.

## How close the decompilation was

The Mac build has sixteen game classes. Thirteen are in the port, and three are left out on purpose (the registration, the internet
scores and the update check). Six of the thirteen have the original's name and seven were named differently:

| Mac class | port class | Windows address | the name |
|---|---|---|---|
| `WinAlchApp` | `AlchemyApp` | 0x4118c9 | differs |
| `Board` | `Board` | 0x401000 .. 0x407ebe | same |
| `Piece` | `Piece` | 0x40e433 | same |
| `HighScoreWidget` | `HighScoreWidget` | 0x40bad1 | same |
| `OptionsDialog` | `OptionsDialog` | 0x40d836 | same |
| `TitleScreen` | `TitleScreen` | 0x4103e2 | same |
| `HelpWidget` | `HelpWidget` | 0x408a8a | same |
| `IntroScreen` | `SelectorScreen` | 0x40c817 | differs |
| `RecordScoreDialog` | `NameEntryDialog` | 0x40ed00 | differs |
| `HighScoreDialog` | `AllTimeDialog` | 0x4096c8 | differs |
| `BoardOverlay` | `EffectsWidget` | 0x40858e | differs |
| `Wizard` | `WizardBox` | 0x414887 | differs |
| `Sidebar` | `SidePanel` | 0x40f987 | differs |
| `RegisterDialog`, `InternetScoreDialog`, `UpdateCheckDialog` | not ported | 0x40f314, 0x41351d, 0x412139 and kin | |

The class boundaries were right everywhere that matters: every one of the Mac's classes with game logic is one class of the port, with
the same widgets drawn by it and the same parts of the game in it (the side panel's `FlashLava` is the port's `SetFlashing`, the
wizard's `Play` and `Stop` are `Talk` and `StopTalking`, `FloatPoints` is `AddScorePopup`). The inheritance, read from the sizes of the
Mac's typeinfo objects in the symbol list (an indirect reading), agrees with the port for every class but one: `IntroScreen` also has
`CheckboxListener` among its bases. Nothing in the port or in the Windows exe's selector uses it, so it changes nothing.

The methods of the two big classes were matched one by one, by name, by address order (a class's functions lie together in source
order in both builds: the Mac's `Board` runs `IsInProgress, HasSavedGame, LoadGame, SaveGame, DeleteSavedGame, Pause, Unpause,
Encode...` and the Windows exe's runs 0x4010e2, 0x4010fc, ..., 0x40111e, 0x401130, 0x40118b, 0x4011d5) and by size. Of the Mac's 55
`Board` functions all but six have a counterpart in the port: `RegisterRefused` (registration), `OrderSuffix`, `InNextButton`, the
second `DrawAliasedPiece`, `DrawCentered` and `ResetBoard` (below). Of the 51 `WinAlchApp` functions, those that carry game logic all
have one; the rest are the registration, online and update-check parts. The other classes were matched the same way (every
function of `Wizard`, `Sidebar`, `Piece`, `BoardOverlay`, `TitleScreen`, `IntroScreen`, `OptionsDialog`, `HighScoreDialog` and
`HelpWidget` is in the port, `HighScoreWidget`'s and `RecordScoreDialog`'s but the registration and online ones). The sizes agree
too: of the 39 matched `Board` pairs with a size in both builds, 36 have a Mac function between 1.2 and 2.9 times the size of the
Windows one (about 1.7 in the middle, the PowerPC code being the longer); the three outside the band are the one-line `GetColX` and
`Discard` and `AdvanceTutorial` (0.8, probably the two compilers' different expansion of its strings). No matched pair looks merged
or split by the decompilation.

What the AI got wrong, none of it with a visible effect:

- **Functions in the wrong class.** `HasSavedGame` (0x4010fc) and `DeleteSaveGame` (0x40111e) are `Board` functions (they lie inside
  the board's address range and the Mac lists them under `Board`); the port has them in `AlchemyApp`. `ShowMessage` (0x4114ff) is
  `WinAlchApp::GiveHelp(string, string, string)`, an app function taking three strings; the port made it a `Board` function taking a
  prefix and an index that it turns into the three. `DayKey` (0x409e60) is `HighScoreWidget::GetDayNum`, a member; the port has a free
  function.
- **Functions merged or left inline.** `ClearGrid` pairs with `ClearBoard` by size, and `ResetBoard` (132 bytes) has no Windows
  function that pairs with it, so it is probably inlined into its callers there. The port has one `DrawPieceImage` where the Mac has
  two `DrawAliasedPiece` overloads (one with two more arguments, which the port's pictures never need), and one `AddPanelButton`
  for the two `CreateControlButton` overloads (the port names both addresses). The name entry's box is drawn inside
  `NameEntryDialog::Draw` where the Mac has `RecordScoreDialog::DrawEditBox`, and the centred text of the statistics is done in
  place where the Mac has `Board::DrawCentered`. The Windows exe has `CheckAllLit` (0x4062e5) and `GameWon` (0x406209) as two
  functions where the Mac has one, `CheckWin`: probably one function in the source that one compiler left whole and the other split.
- **A name that led the wrong way.** `Board::mTrialPause` and `Pause(true)` look like a trial copy's pause, and the registration
  pass of this game had to stop and check them. They are the pause that covers the board (any key press in every mode, a dialog in
  the time trial), which the Mac calls plain `Pause(bool)` and `Unpause(bool)`. The notes now say so.
- **Wrapper overrides not ported.** The Windows exe's app class overrides two virtuals with a function that only passes on to the
  base: slot 8 (a five-byte jump at 0x4111cb, probably `UpdateFrames`) and slot 49 (0x4111d0, a function of one `double`, the Mac's
  `SetMusicVolume`). The Mac lists both in `WinAlchApp`. The port has the base's, which behaves the same.
- **A handler without its dialog.** The Windows exe's `ButtonDepress` (0x413eea) handles the yes of dialog 1, a change of skill while a
  game is on, which the port copied (`AlchemyApp::ButtonDepress`, `mPendingSkill`). Nothing in the Windows exe that
  I found raises that dialog: its string table has the load, new game, tutorial, quit and update questions and no skill one. The Mac has the missing piece,
  `WinAlchApp::ConfirmSkillChange(int)`. It is a Mac-only question, as Seven Seas found for its own, so the port, which follows the
  Windows exe, shows none.

## What is only in the Mac build

`Board::OrderSuffix(int)` and `HighScoreWidget::GetOrderSuffix(int)` make ordinals ("3rd"). The Windows exe has no such strings, and
its score check (0x40a57b) puts the rank into the message with a plain `%d` (`YOU_GOT_X_PLACEMENT_OF_ALL_TIME1 3
YOU_GOT_X_PLACEMENT_OF_ALL_TIME2`), which the port does. `Board::InNextButton(int, int)` is a hit test for the next-board prompt that the
Windows exe makes a link widget instead (`CLICK_HERE_TO_PLAY_NEXT_BOARD`); the port follows the Windows exe, and the two were compared
screen by screen. The registration and online parts (`RegisterDialog`, `InternetScoreDialog`, `UpdateCheckDialog`, `DoRegister*`,
`DoPregameNag`, `DoTimeTrialLimit`, `DoWebsiteInform`, `UploadScore`, `CheckForUpdates`, `RegisterRefused`, `Registered`,
`ScoreUploaded`, the browser dialogs and `URLOpen*`) are the unregistered and online game that the port leaves out
(`GAME_NOTES.md`, section 5). `DoColorDepthExplination` is the colour-depth question, which the port had until 2026-11-09 and leaves out now (`GAME_NOTES.md`, section 5).

## Faults found and fixed

The names showed no behaviour fault of the port: no function the Windows exe has and the port lacks, no class that does something the
port's class does not, and no constant to correct. The changes this comparison made to the repository are in the notes:
`GAME_NOTES.md` says what `mTrialPause` is. Naming and structure corrections wait for the readability pass: the seven class names
above, `HasSavedGame` and `DeleteSaveGame` into `Board`, `ShowMessage` into the app as `GiveHelp`, `DayKey` into
`HighScoreWidget`, and `mTrialPause` renamed for what it does. That pass made them on 2026-11-11 (`GAME_NOTES.md`, "The
readability pass"); this report keeps the names the port had when it was written. The pass also corrects one pairing above:
`tools/macmatch.py`, which did not exist when this was written, pairs the Mac's `ClearBoard` with 0x406209 (the port's `GameWon`
then) by its calls, which leaves `ResetBoard` for 0x4017dd (`ClearGrid`), and `CheckWin` is 0x4062e5 alone.

## Lessons for the next decompilation

If a Mac build with symbols exists, read its class list and each class's function list before naming anything: seven of thirteen class
names were guessed here, and the guesses were reasonable but all different, so a later reader of the Mac names pays for each. A class
can be found in the Windows exe without them, from the address ranges (the compiler lays the functions of a class together in source
order) and from the constructors' vtables, and the three misplaced functions above were the ones whose address was inside another
class's range. Function sizes are a cheap check for merged or split functions: the ratio of the Mac size to the Windows size sits in
a narrow band, so a matched pair outside it is worth a second look. A name that sounds like a feature (a "trial" pause) is a
hazard when code is removed by feature: check what the field is used for before cutting. And a handler with no code that raises it
means the Windows exe lacks something the other build has, not that the decompilation missed code.
