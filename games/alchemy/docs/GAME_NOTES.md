# Alchemy Deluxe: behaviour notes

Addresses are virtual addresses in `re/alchemy/bin/AlchemyGame.exe` (the game inside `Alchemy.exe`, version 1.6, linked
2004-08-23 with MSVC 7.0, image base 0x400000). The game's classes have no RTTI. Offsets such as `board+0xae8` are byte
offsets in the original object; the port's classes name the fields and keep the offset in a comment. `WORKING_NOTES.md`
has the order the analysis went in and the tools.

## 1. Where things are

| Original | Port |
|---|---|
| 0x401000-0x407ebe `Board` (vtable 0x4e32d0, size 0xe78): rules, update, mouse, tutorial, the game's end | `Board.cpp`, `BoardUpdate.cpp`, `BoardTutorial.cpp`, `BoardSave.cpp` |
| 0x404923 `Board::Draw`, 0x40400d the cleared-board panel | `BoardDraw.cpp` |
| 0x414887 the wizard's head (0x4e58b0, size 0x90), the speaking box | `Wizard.cpp` |
| 0x408a8a the help screen (0x4e3b6c) | `HelpWidget.cpp` |
| 0x4096c8 the all-time table (dialog, 0x4e3d58), 0x40bad1 the score table (0x4e417c, size 0x21c) | `Dialogs.cpp`, `HighScores.cpp` |
| 0x40f987 the side panel (0x4e4b40, size 0x90: forge gauge, score, labels), 0x40c817 the selector (0x4e43a8, size 0xa4) | `Sidebar.cpp`, `IntroScreen.cpp` |
| 0x40d836 the options dialog (0x4e4540, size 0x144), 0x40ed00 the name entry (0x4e4880) | `Dialogs.cpp` |
| 0x40e433 `Piece` (0x4e4798, size 200) | `Piece.cpp` |
| 0x4103e2 the title screen (0x4e4c50, size 0x88) | `TitleScreen.cpp` |
| 0x4118c9-0x414903 `WinAlchApp` (vtable 0x4e5160, 72 slots) | `WinAlchApp.cpp` (start-up, loading, registry, side-panel buttons), `AppScreens.cpp` (the screens' flow), `WinAlchApp.h` |
| 0x40858e the effects overlay (0x4e3ab0, size 0x78: floating scores, sparkles, the help arrow) | `BoardOverlay.cpp` |
| 0x414000-0x47f000 the framework (2857 functions byte-match BookWorm's) | `framework/` with `sexycompat2004` |

The unregistered game's screens (0x40f314 the register dialog and its kin, 0x413bdc and 0x413e12 the nags, 0x41351d and
0x4131ca the internet scores, 0x412139 the update check, 0x412aa0 and 0x412c68 the browser dialogs) are not ported; section 5
lists every function of them, with what it does in the original.

## 2. The game

**The board** is 9 columns by 8 rows of 52-pixel cells at (10, 39) in a 490 x 480 widget at x = 150; the side panel is
the left 150 pixels. A piece (`Piece`) has a shape (22 symbols) and a colour (7), or is the stone (the blank block, wild:
`mSkull` in the code) or a skull (a bomb that destroys a rune: `mBomb`). The piece to place hangs from the mouse (`Piece::Update` puts it 0x30 pixels up and left
of the pointer) and the cell under its middle is the one it would go to (`Board::MouseMove`).

**Placing** (`Board::ValidMove`, 0x40153a). The grey stone starts in the middle (col 4, row 3) and the first rune must touch it.
Later a cell must touch a piece, and every piece it touches must share the new piece's colour or shape; a stone matches
anything. A skull goes on a piece instead. The reasons a click fails choose the wizard's message (`INVALID_MOVE_*`).

**Clearing.** Completing a row or a column makes it flash and go (the state `STATE_CLEARING`); a cell that was used stays lit,
and when every cell is lit the board is won: the next board comes with more colours and shapes
(`Board::SetBoardVars`: `colours = (level + 3) / 4 + 4`, `shapes = (level * 3) / 4 + 5`, the chance of a bomb or a stone per
mille `35 - 3 * level`, where level is the skill's base (0, 5 or 10) plus the boards cleared).

**The forge** (the gauge in the side panel, `Board::mProgress`) takes the piece that cannot be placed: the discard
(`Board::SkipMove`, 0x405e31) melts it and raises the heat; placing a piece cools it; at 3 the game ends. The strategic mode
fills it only by discards, the time trial mode also by itself (`mProgRate`, `mProgDecay`, from the four numbers of
`data/diff.txt` and the boards cleared). The tutorial allows a discard only in step 6.

**Skill** 1 to 3 (easy, average, hard) scales the points by 0.5, 1 and 2 (`mRate`, doubled in the time trial) and picks the
level base. The rank names (`gRankNames` in `properties/default.xml`) come from the score; the cleared-board panel shows the
next rank's score.

**The tutorial** is 14 steps (`mTutorialStep` -1 to 13): the board is set up for each, the wizard says what to do and waits for a
button (`mTutorialWaiting`), a wrong placement gets a message, the last step's button starts the real game.

## 3. Files

- `userdata\savegame.dat`: a game in progress, written when the game is left with progress (the destructor, and the web
  build's shutdown hook), read when the player chooses to continue, deleted after it was read (`Board::SerializeGame`). The
  port's format is the original's field order with a version byte; a damaged file keeps what it could not read.
- `userdata\hiscores.dat`: for each kind of game the best score of the day (the day, a name, a score and a check) and five
  scores (a name, a score and a check); numbers are little-endian 32-bit, a name's length 16-bit. The check is
  `score ^ 0x42beef`, then a step for each character of the name (0x40a15a, 0x40a341). The default scores are 1000 down
  to 600; the default names are ten names inside the game (0x4e4108-0x4e4148, taken by the score table's constructor in the
  strategic table's order, then the time trial's). The port reads them from the player's own `Alchemy.exe` (`GameText.h`),
  and starts a table with its own placeholders when the exe is not in the game folder or is another version;
  `DEFAULT_NAME_STRATEGIC_1..5` and `DEFAULT_NAME_TIME_TRIAL_1..5` in `properties/default.xml` replace either.
- the registry (`registry.ini` in the port): `SkillLevel`, `TimeTrial`, `LastScoreName`, and the framework's (volumes, full screen, custom cursors, times played).

## 4. Port decisions

- The strings are ids in `properties/default.xml`, read by `LoadProperties()` (Init); there is no GameText reader.
- The game's clock is `GameTickCount` (the virtual clock under `-fast`) where the original calls `timeGetTime`, so test runs
  repeat exactly.
- The 2004 `Dialog` places and wraps text differently from the port's: `Dialog2004` (the compat library) is used for every
  dialog; `HighScoreDialog` returns the height 0x156 as the original's does.
- The framework's `SexyApp` had a hook for the registration; the port acts registered (`mIsRegistered`) and shows no nag.
- Where the decompiler drops the last arguments of a draw call (the selector's base picture, the dialogs' insets, the
  forge's back picture) the values come from the disassembly; each such place says so in a comment.

## 5. What the original has and the port leaves out

Two effects of `Board` are never triggered, since no code sets their flags (every access to them is the constructor's
clearing, the update and the drawing): a sweep with a flag at +0xdb4 and a counter at +0xdb8 that grows by 8 an update up
to 0x200 (`Update`, 0x4074ce), and a ring of eight pieces of picture 14 flying out from a centre (+0xdc4, +0xdc8) with a
radius at +0xdcc that grows by 3 an update up to 0x48 (`Update` 0x407ce4, `Draw` 0x40505b). They are leftovers, so the port
does not draw them. The bomb's sparks (the list at +0xddc) are the ones that do show: they leave sparkles behind them
through the effects overlay.

The unregistered game's screens and the internet scores are left out as `README.md` says. On the title screen the
second picture (0x4106bc, the "buy" overlay at y = 0x137) and the play counter belong to a copy that is not registered,
and the "thanks for registering" line (0x4108c6, y = 0x157) to one that is registered but not unlocked; the port is both
registered and unlocked, so none of them is drawn.

**Registration and the buttons that lead out (the coordinator's rules of 2026-11-07, checked 2026-11-08).** The original exe
*does* have the unregistered game: a play limit, delayed start-up, nags, a register dialog, an update check, the internet
scores and the calls that open a browser. The copy this port was made from (the Collection's `Alchemy.exe`) has no copy
protection and acts as a registered copy (the app's registered flag, +0x478, is set), so it never reaches any of them, and
the port never translated them: the ideal case, with nothing to take out afterwards. Everything below exists in the
original and is left out, by address (size in bytes), with what the export shows each does:

| function | what it does in the original |
|---|---|
| 0x4103e2 (576) `TitleScreen` constructor | besides the play link, makes the REGISTER_NOW_TO_REMOVE_THIS_DELAY text, the line of an unregistered copy's wait |
| 0x4109d8 (436) `TitleScreen::Update` | the registered branch is ported (the bar follows the loading thread). For an unregistered copy it reads the registry value `MaxPlays` (10 when missing); when the play count (+0x534) is past it, it raises the forced-registration dialog (0x41351d) at its tenth update, and otherwise holds the bar back with a timer that grows 0.01 per update (the "delay") |
| 0x410622 (950) `TitleScreen::Draw` | the registered branch is ported. The rest draws THANKS_FOR_REGISTERING (registered, not unlocked), the "YOU HAVE %d PLAYS LEFT" counter and YOU_MUST_REGISTER |
| 0x40d836 (1594) `OptionsDialog` constructor | the registered part is ported. The rest makes the buttons VISIT_POPCAP_FOR_MORE_GAMES (id 3), CHECK_FOR_UPDATES (id 5) and REGISTER (id 1) and the OptionsLinkText0 link; the port's dialog is shorter by their rows and its help and scores buttons stand side by side (`OptionsDialog::Resize`) |
| 0x40e34b (232) the options dialog's link and button handler | opens `OptionsLinkURL0` (http://www.popcap.com), starts the update check (0x41222f) for id 5, and opens the register dialog (0x410fef) for id 1 |
| 0x410fef (208) | opens the register dialog; called by 0x40e34b, 0x4109d8 and `ButtonDepress` 0x413eea |
| 0x40f314 (716) and 0x40f5e0 (732) the register dialog | its constructor (CLICK_HERE_TO_REGISTER_ONLINE, REGISTRATION, buttons with the `hiscorebtn` picture, an edit box on `hiscorebox`) and its drawing and text (LICENSE_CODE, REGISTERED_NAME, BROWSER_WINDOW_NOTICE1-3) |
| 0x40c092 (824) the internet-score dialog | constructor of dialog 9: an edit box for the name (the registry's `LastInternetScoreName`) with SUBMIT and CANCEL on `hiscorebtn`. Its destructor is not in the export under a name of its own; the function after it, 0x40c3ca, is the selector's destructor body (called by 0x40c7fb), which the port has |
| 0x40bed1 (116), 0x40bf45 (71), 0x40bfc6 (134), 0x40c076 (28) the internet-score dialog | its destructor (deletes the edit box and the two buttons), `AddedToManager` (adds them, focus on the edit box), `Resize` (places the edit box, SUBMIT and CANCEL) and scalar deleting destructor |
| 0x40efbd (141), 0x40f1bf (28), 0x40f1db (313) the register dialog | its destructor and scalar deleting destructor, and the drawing of a box from the `hiscorebox` picture (a left end, a run of middle pieces, a right end) that 0x40f5e0 calls |
| 0x4061e5 (36) `Board::RegisterRefused` (the Mac's name) | for an unregistered copy (the app's flag +0x478), a time trial board (+0xaed) with a board cleared (+0xe0c) ends the game (`GameOver`, 0x403f5c); `ButtonDepress` (0x413eea) calls it when the player answers No to dialog 5 or to the time-trial nag (dialog 0xe) |
| 0x409f8e (62) `HighScoreWidget::Registered` (the Mac's name) | sets +0x218 to 1000 and greys the internet-score SUBMIT button (+0x68, which the port does not have) |
| 0x4111e1 (25), 0x411444 (23), 0x411485 (61) | the stream helpers (`istream` get-line, a string's pointer plus an offset, `istream::get` under `ios::lock`) that the internet score upload, 0x4131ca, uses to read its page template |
| 0x413ab9 (291) | no caller in the export. For an unregistered copy it raises dialog 10 with REGISTER_NAG1 and REGISTER_NAG2; for a registered one it makes the internet-score dialog (0x40c092) |
| 0x4131ca (851) internet score upload | fills the page template `scoreul.tpl` ($NAME, $DATA) into `score%d.html` and opens it in the browser; called by `ButtonDepress` (0x413eea) with 0x413846 (252), the LICENSE_ERROR1-3 dialog |
| 0x41351d (809) forced registration | the dialog FORCE_REGISTRATION_DIALOG1-6 (and NoInternetScores) when the play limit is reached; called by 0x4109d8 |
| 0x413a2f (138) | the high-score register nag (HIGH_SCORE_REGISTER_NAG, dialog 10, modal); called by the score check 0x40a57b, whose unregistered branch it belongs to |
| 0x413bdc (566) | the pre-game nag (PREGAME_NAG with the play count, and PREGAME_NAG2-6 from 30, 40 and 60 plays), dialog 10; called by `Board::Update` (0x406f2a) |
| 0x413e12 (216) | the time-trial nag (TIME_TRIAL_NAG1, TIME_TRIAL_NAG2), dialog 0xe; called by `Board::Update` (0x406f2a) |
| 0x412139 (246) and 0x41222f (450) the update check | the question CONFIRM_UPDATE1 and 2 (called by 0x412a8a, the 22-byte override of `LoadingThreadCompleted`), and the address http://www.popcap.com/win32updatecheck.php?prod=...&referid=...&ver=... that it opens |
| 0x412aa0 (456) and 0x412c68 (257) | the dialogs that answer an attempt to open a web page: OPEN_BROWSER_DIALOG1 and 2 with URL_OPEN_ERROR1 and 2 when it fails, and URL_OPEN_DIALOG1 (virtual slots 26 and 28 of the app; slot 27 is the empty success hook, 0x410d0e) |
| 0x411a41 (258) and 0x411b43 (524) the registry | the ported methods; the original's also write and read `LastInternetScoreName`, `RegisterLink` (http://www.popcap.com/register.php?theGame=alchemy&referid=) and `ReferId` |

The framework's own `SexyApp` has the other half (the registration code and name, `register.php`, the `DontUpdate` value
and the update-check dialog at 0x476210); those are the shared code's, not the game's.

The port's own checks, made 2026-11-08, that nothing it loads is used only by those functions: a search of `src/` and of
the `properties` ids it reads for the strings and the registry values above finds none (the one that remained, `LastInternetScoreName`, is out now;
`COLOR_DEPTH_ERROR` is not on the list: it is the colour-depth question of 0x413942, which is out too since 2026-11-09, below). Of the pictures,
`trialversion` (the title's "buy" overlay, which was loaded and never drawn) and, by the pictures' places in the original's
image array, `hiscorebox` and `hiscorebtn` (read only by the register dialog and the internet-score dialog) were still
loaded; all three are gone from the image list and from `Res.h` (63 images, 9 loaded by the title's start-up, and 99 loading
tasks where the original has 101, since the two others were among the loading thread's; 59 and 95 since section 10), and the registry values
`NoPopLink` and `LastInternetScoreName` with their fields. No visible change comes of any of these (the suite's baseline is identical).
The pictures `cartoonstar` and `waitbar` that nothing in the port draws are not registration: the first is the ring of
the Board effect the first paragraph of this section calls never triggered, the second is loaded and never read by the original either
(both are out too since 2026-11-09, section 10).

`Board::mCoverCount` and `Pause(true)` are not registration: the name "trial" is the decompilation's, and the pause it counts is
the one that covers the board (`Board::Draw` draws the cover instead of the tiles, and the pause text when no dialog is up): any key press toggles it in every mode
(`KeyChar`), and a dialog raises it in the time trial (`WinAlchApp::ModalOpen` passes the mode). They stay. Loading has no waits (the title screen's bar follows the loading thread, which runs
once, with no pacing), so the rule on fast loading changes nothing here.

**The colour-depth question (taken out 2026-11-09).** `DoColorDepthExplination` (0x413942) is a dialog (id 8, the strings
COLOR_DEPTH_ERROR1 to 3 as header, text and footer, a footer button) that the options dialog's check box handler (0x40d7c2)
raises when a check box is cleared while the app's `mForceFullscreen` (+0x30f) is set, and after it the full-screen box is
checked again. That flag is the framework's, which sets it only to false: it was meant for a desktop without enough colours, and
nothing in a port raises it, so the question could not come up. It is out of the port: `OptionsDialog::CheckboxChecked`, the
`DIALOG_COLOR_DEPTH` id and its name in the `state` command are gone, and the dialog stays the listener of its two boxes only
because a `Checkbox` takes one. The three strings are in the game's own `properties` file, which is not ours to edit; the port no
longer reads them. `mForceFullscreen` is `SexyAppBase`'s, shared with the other games, and stays.

## 6. The standard test commands

The port answers the standard test commands (`docs/TEST_COMMANDS.md`) in `WinAlchApp::HandleScriptCommand`
(`src/testhooks/Commands.cpp`). They are reachable only from a test: `-script` lines (`<update> <command>`), and the live
channel (`-commands`, the web's `byopop.test.command`) once the framework has it. A game that is never given one is not changed:
the suites and the frozen baseline play as before. Each command answers with one line on stdout (`OK <command>`,
`ERROR <command>: <reason>`, `STATE {...}` or `MOVE {...}`).

| Command | What it does |
|---|---|
| `state` | `STATE {json}`: `screen` (title, menu, game, highscores, options, help), `dialog` (name, load, hint, quit, newgame, tutorial, options, alltime), `updates`, `mode` (strategic, timetrial, tutorial), `skill`, `level` (the board, from 1), `score`, `forge` (0 to 3), `over` (the game has ended), `busy` (the board cannot take a piece now), `board` (`state`, the `pieces` as `[col, row, shape, colour, stone]`, `lit` as eight strings of nine digits, `litCount`, `piece` to place, `fits`, `moves`, `forged`, `boards` cleared, `shapes`, `colors`, `paused`, `cover`, and `tutorial` with `step`, `waiting`, `count`, `button`), `ready` and `link` on the title, `active`, `windowed`, `muted`, and the places of the buttons a driver presses: `panel`, `options`, `selector`, `table`. |
| `goto <screen>` | `menu` (the selector), `options`, `help`, `highscores` (the all-time table), `strategic` or `timetrial` with an optional skill 1 to 3, and `tutorial`, which start that game through the selector's own buttons. `title` and `credits` are errors: the game has neither to go to. |
| `level <n>` | Board n of the game being played: n - 1 boards cleared, an empty grid (a first board has its stone), the difficulty follows. |
| `unlock all` | `ERROR`: the game has nothing locked. |
| `set <name> <value>` | `score`, `forge` (0 to 3) or `boards` (boards cleared; the difficulty follows). |
| `win` | Every tile lit: the board is won through `CheckWin` (the bonus, the pieces vanish, the panel). |
| `lose` | The forge overflows: the game ends through `GameOver` (the pieces shrink away, the table follows). |
| `seed <n>` | Seeds the C library generator, which seeds the board's own generator at a game's start. |
| `move` | `MOVE {json}`: the bot's next input, or `MOVE null` when it has none (the title still loads, the board is busy, a dialog the bot does not answer is up). Changes nothing, and draws none of the game's random numbers: the bot has a generator of its own, seeded from the position. |

The bot is the one `-autoplay` has always had (`src/testhooks/Bot.cpp`, shared by both; the suites' numbers did not change):
a cell that completes a row or a column is best, otherwise the cell with most neighbours, a tenth of the choices a random cell
that fits, and the forge when no cell fits. `move` also answers the title's link, the wizard's question (yes), the saved game
(yes), the name entry (three letters, then OK), the table's button, the board-cleared panel's button and the tutorial's. A MOVE's
`input` is the ordered list of steps `docs/TEST_COMMANDS.md` describes: `{"move":[x,y]}` (the pointer goes there; the piece hangs
0x30 pixels up and left of it and the board takes the cell from the piece at the last update, so a hover comes before a click),
`{"wait":n}`, `{"click":[x,y]}` (`"button":"right"` for the forge) and `{"key":"b"}`. Its `kind` is place, forge, title, dialog,
name, scores, cleared or tutorial.

## 7. The fixes mod

`games/alchemy/mods/fixes/mod.json` is the game's fixes mod (`docs/MOD_ROADMAP.md`, "The fixes mod"): one mod, each setting a
toggle for one bug of the original that the port keeps, every toggle on by default. The site serves the game with it on
(`alchemy-fixes/` beside the page); a native build finds it when the folder is copied into the mods folder. The port without
the mod is the original: `FixEnabled` (`src/Fixes.cpp`) is false for every toggle unless the mod is enabled, and a toggle is read
where its fix matters, as a live value, so it needs no restart (a picture the board already holds is redrawn at the next board).

| Toggle | The original | With the fix |
|---|---|---|
| `rims` | `Board::RedrawBackground` (0x401a82) gives a lit tile a half-opaque rim where it borders an unlit one, but tests the left and top neighbours with `1 < col` and `1 < row` and the right and bottom ones with `col < 8` and `row < 7`: the first column and row get none on their inner side, while the last ones get theirs. | The tests are `0 < col` and `0 < row`: the edge between a lit tile of the second column (or row) and an unlit tile of the first has its rim, as every other edge. Only the one-pixel line at the second column's left edge and the second row's top edge differs. |
| `continuedEnding` | `Board::LoadGame` (0x405b39) puts each piece of a continued game at its cell without the board's offset (150, 0) that every placed piece has until the next `Resize`, and the end-of-game shrink draws pieces from that position: in a game that was continued and then lost, the pieces shrink 150 pixels left of their tiles, half of them under the side panel, and the board looks emptied at once. | The pieces get the offset, and shrink away on their tiles as in a game that was not continued. |

**Looked at, not made into toggles.** The port's `Board::~Board` counts a game as played when the forge is nearly full and the
game has not ended (`mState != STATE_GAME_OVER`), but a game that is lost counts in `GameOver` and the board is in
`STATE_ENDING` for the next second or so: leaving then counts it twice. The count only decides whether the selector still shows
the tutorial's button (fewer than five games), and only a game left in that second is counted twice, so it is not a fix worth a
toggle. A name entry that is accepted empty drops its score without a word (`HighScoreWidget::NameEntered`): that is the game's rule
for "no name", not a mistake the port can mend without making up what the original would have shown. The game's end is slow
(about three seconds of shrinking and fading, then the table) and a click is ignored meanwhile, as in a board cleared (the
pieces vanish three at a time): those are the game's animations, part of its pace, so they stay. Loading is already as fast as
the machine: the framework runs the loading thread in one go, and the title polls it every fourth update (at most 40 ms).

## 8. Performance

Two passes of numbers, 2026-11-08 and 2026-11-09, the port and the original back to back on the machine as it was (other
sessions' games ran meanwhile, so the processor times move by a few seconds from run to run, see below).

**The first pass's processor figure was wrong, and the performance pass found why (2026-11-09).** The first measurement put the
original at 1.4 to 1.6 s of processor time for the scripted minute and the port at about 6 s, and this section said the port used
"about five times" the original's. The fault was in the original's runs, not in either game: `perf.py` started origrun with
`--exe ... --run alch --seed 11` and no `--patches`; origrun takes its default patch list
(`tools/origrun/patches/alchemy.txt`) by the game's name, which `--game alchemy` gives and `--exe` does not, so it applied no patch and `--seed 11` pinned nothing
(the run's log says "RNG seed to pin: 11" all the same). The original dealt its own boards, the bot's script (moves chosen for the
port's seed-11 boards, played as mouse clicks) met no move on most of them, and the copy sat drawing now and then: 549 to 565
draws in 6,451 updates (its draw counter, the app's +0x4bc) instead of on nearly every update. The same script with the list given
(`--patches tools/origrun/patches/alchemy.txt`, whose one line pins the seed) draws 5,950 to 5,970 times in the same 6,451 updates
and costs the processor what the port costs (the table below); `perf.py` now always gives the list, and refuses to run without it.

An earlier version of this section gave another cause, which I measured and then found not to explain these numbers: the original
has a screensaver-idle draw stop (a byte at 0x54d7ec, tested in `DrawDirtyStuff` at 0x46cb46, set by a once-a-second routine at
0x462a60 that asks `SystemParametersInfo` for the screensaver and its timeout and `GetLastInputInfo` for the machine's last input).
The code is there, but in every run I watched the byte read 0, and so did the routine's screensaver flag (0x51260c), while its
timeout (0x512608) stayed at its start value. With the seed pinned, the same 1,480-update script draws 991 times with the list
alone, 1,019 times with the jump at 0x46cb4d (`0f 85 eb 01 00 00`) turned into six `nop`s by a `test` line of the list, and 1,019
times with a `test` line that writes the same bytes back; without the list (seed unpinned) 192 and 190. So the earlier result, that
the nop raised the draws fivefold, compared a pinned run with an unpinned one. Whether that routine stops the draws on a machine
idle for ten minutes was not tested here: origrun's stand-in now answers `GetLastInputInfo` with the present moment in every run
(`docs/ORIGRUN.md`, "The last-input hook"), which keeps it from setting the flag whatever the machine does.

**How.** `games/alchemy/tools/profile/perf.py` starts the game and reads that process alone, every 100 ms: its working set, its
private bytes (the pagefile usage, whose peak Windows keeps) and its processor time (kernel plus user); `run.sh` alternates the
two, `exp.sh <name> VAR=VALUE...` runs the port's minute once with a changed environment (`PERF_PORT_ARGS` adds game arguments,
`PERF_ORIG_PATCHES` names another patch list for the original), `drivers.sh` repeats it per SDL render driver, and `cpuprof.py` samples the
threads' instruction pointers weighted by the cycles each used (with the stack of a sample that is outside the exe, to name the
call that made it). The port is `build/native/alchemy.exe` natively under rundesk, never `-fast`; the original is the staged copy
under origrun on its real clock (`--clock real`; origrun puts it under rundesk itself). Both play the same scripted input: the
bot's mouse game (strategic, game seed 11) from update 450 (title link, selector, board) to update 6450, which is 60 s of play at
100 updates a second, then quit (`toorig.py` turns the port's script into the original's, answering the original's first-run
"Updates" question No); the original's RNG seed is pinned (`tools/origrun/patches/alchemy.txt`, written by
`tools/identical/setup.sh`). Both run on rundesk's private desktop at below-normal priority with silent sound, so these are the
test conditions, not a player's. A run's numbers cover launch to the end of the minute (about 66 s); the processor time of "the
minute" is that of the last 60 s of the run. Game processes on the machine (any session's) were 4 to 9 during these runs.

**Loading** (launch to the first screen that takes input, the title with its play link; the original's launch is its process's
creation, its play link is read from its memory, `mLoadingThreadCompleted` and the title's link-shown byte, as origrun's `--watch`
reads it; the port's is its first state answer with `ready` true, asked every 50 ms over `-commands`):

| | runs (s) | median |
|---|---|---|
| port, native | 1.74, 2.63, 1.77 (and 0.83, 1.15 in two test runs before) | 1.74 |
| original under origrun, real clock | 2.62, 1.72, 2.77 (and 2.61, 0.82 in its two play runs; 1.45, 0.66, 1.80 in the three runs of 2026-11-09) | 1.76 |
| port, web (the Browser pane, `loadtime.js`: page navigation to the play link, silent) | 3.8 (cold), 4.8, 11.9, 13.1 | not stable |

Both load in about 1 to 3 seconds when the machine lets them (0.7 s at the quietest moments, for either), and the spread between
runs is larger than the difference between the two, so no gain or loss in loading can be claimed from these. The web's spread is the
pane's browser sharing a busy machine (its own start-up took 0.9 to 9.5 s of the time).

**Memory** over launch and the scripted minute (peak working set; peak private bytes):

| | peak working set (MB) | peak private bytes (MB) | mean working set in the last minute (MB) |
|---|---|---|---|
| port, 5 runs | 66.9 to 76.0 | 66.9 to 69.3 | 65.8 to 74.9 |
| original, 7 runs (2 with the seed unpinned, 5 pinned) | 64.3 to 77.3 | 50.9 to 56.3 | 62.8 to 74.0 |
| port, load only (3 runs) | 59.7 to 68.9 | 59.5 to 62.5 | |
| original, load only (3 runs) | 59.3 to 69.0 | 46.1 to 48.5 | |

The working sets are alike (about 65 MB in the quiet runs of both); the port holds about 12 to 15 MB more private bytes than the
original in play (67 to 69 against 51 to 56) and about 14 to 16 MB more at the title. The port's own report (the `memreport
<file>` script command, `src/testhooks/MemReport.cpp`, as Bejeweled has it; run by `tools/profile/memrun.sh`) at the title, in the
middle of the minute and at its end: 556, 577 and 577 images holding 22.6, 23.6 and 23.6 MB of pixels (4 bytes a pixel, kept after
the upload since a texture cannot be read back) and 0, 0.9 and 1.2 MB of alpha caches; 1.3, 5.4 and 6.4 MB of textures (an
estimate, held by the driver); 15 sounds of 2.1 MB; the process heap 30.7, 34.2 and 35.6 MB in 17,600 to 23,900 blocks. The
biggest images are the title and the backdrop (1.2 MB each); 17 images of 468 x 416 (0.78 MB each: the level pictures
and the layers the board makes for its light and effects) hold 12.6 MB, about half of the pixels. The original holds the same pictures; what it holds in private
bytes besides them was not measured, and the framework's part of the difference (the pixels kept after the upload, the mixer's
buffers, Direct3D's and the driver's allocations) belongs to the framework's notes of the same kind (Bejeweled's `PROFILE.md`).

**Processor time** (seconds of one core, launch to the end of the minute, and the last 60 s of it):

| | whole run | the minute | draws in the run |
|---|---|---|---|
| original as first measured, the seed unpinned (2 runs) | 1.64, 1.44 | 1.20, 0.99 | 549, 565 |
| original, seed pinned, the jump at 0x46cb4d nopped out, before the hook (3 runs) | 8.25, 5.45, 5.05 | 7.59, 4.63, 4.59 | 5407, 3427, 5862 |
| original, seed pinned, nothing nopped, the hook on (2 runs) | 6.97, 4.61 | 6.36, 4.22 | 5972, 5952 |
| port (5 runs; the first two are the first pass's) | 6.06, 5.92, 6.25, 5.22, 5.56 | 5.00, 4.92, 5.31, 4.58, 5.00 | 3,290 to 3,610 |

Over the same minute with both drawing, the original used 4.2 to 7.6 s and the port 4.6 to 5.3 s: the port is not the heavier
of the two, and no speed-up is to be had by comparison. The spread inside a column (5.2 to 6.3 s for the port in the quiet runs,
and 8.3 to 8.7 s in two more runs, made while the machine was busier) is the machine's load: the cost is the driver's spinning
and waiting, which grows when other processes compete. Read the table as "about 5 to 6 s, around 8% of a core", for either.

**Where the port's time goes (the profile).** The game's own code is under 3% of the process's processor time. The main thread
uses about 80% of the cycles and a graphics driver thread about 15%, and nearly all of both is in `ntdll.dll`, which for a 32-bit
process under 64-bit Windows is system calls and waits. Timers around the framework's SDL calls on the main thread (15.8 G cycles
in a 75 s run) put it down to the call: **`SDL_RenderPresent` took 69% of the main thread's cycles** (10.9 G, about 3.0 M cycles
or 0.8 ms for each of 3,578 frames), texture uploads 7% (1.13 G: `SDL_UpdateTexture` for the images the game redraws, 2,350 of
them in the run, 53,000 pixels each on average, 0.65 a frame), `SDL_RenderGeometry` 0.8% (0.13 G for 108,000 quads: 30 a frame,
110 at most, in 14 texture changes a frame), and the frame's other calls (`BeginFrame`, the display size, `PrepareTexture`
apart from the uploads) under 1%. The stack of a sample is no way to find this: a sample in the driver's frames seemed to be
inside `DrawTexturedQuad` for 57% of the time, because the driver's frames are kilobytes deep and the words in them are stale
return addresses of earlier calls; the cycle timers are the measurement.

What changes the cost, tried on the same minute: the SDL render driver (direct3d11, direct3d and opengl) 6.2 to 6.8 s, none
better; the software renderer 74.8 s and a run that could not keep its pace (twelve times the GPU's, since the whole screen is
blended by the processor); `SDL_RENDER_VSYNC=0` 12.1 s (the present, without the display's refresh to wait for, costs more
processor time); `SDL_RENDER_BATCHING=0` 11.5 s (every call flushed); `-renderscale=1` and `=2` (the default follows the window and the display) 8.3 and 8.7 s,
no saving. So the cost is a driver cost per presented frame, and the number of frames is
the lever.

**Do frames draw nothing new?** No. The framework draws when a top-level widget is dirty or a second has passed, and 3,602 of
4,062 draw decisions found one: `EffectsWidget::Update` (0x40844f, ported line for line) calls `MarkDirty` on every third update
for each sparkle and score popup alive (49,800 calls in the minute), the board, the side panel
and the pieces follow it, and the original does the same, which is why it too draws on nearly every update (5,950 of 6,451 in the pinned runs). Of
2,406 frames read back and compared with the one before, 4 were identical (0.2%), 28 changed under 1% of the pixels, 732 under 5%,
1,596 under 25% and 45 more; the changed pixels averaged 368,000 a frame, 13% of their bounding box (2.79 M pixels on average), so the changes are
spread over the screen. So dropping frames that show nothing new saves 0.2%, and a partial present
needs several rectangles.

**What was changed.** In the game, nothing but the `memreport` test command and the tools: the port is at the floor of what
the framework's present path allows, its processor time is the original's when the original draws, and a change that drew fewer
frames would be visible (a frame cap is the `lean` mod's setting in Bejeweled, `docs/MOD_ROADMAP.md`). The shared costs
(the present, the uploads of whole images that changed in part) went to the coordinator as a spec (2026-11-09). The frozen
baseline stays identical (`tools/check.sh alchemy`).

**Not measured.** The web build's working set, private bytes and processor time cannot be read from the Browser pane (its page
is one tab of the app's browser, with no process of its own to read), and `Module.HEAP8` is not reachable from the page: the
JS heap at the play link was 5 to 12.5 MB, which is not a process's memory. The web's load time above is what could be timed.

To repeat: `bash games/alchemy/tools/profile/run.sh` from the worktree root, after `tools/origrun/build.sh`, a port script in
`build/perf/port-play.txt` and its original's in `build/perf/orig-play.txt` (`games/alchemy/tools/profile/perf.py`, `run.sh` and
`exp.sh` say how); the original runs with `tools/origrun/patches/alchemy.txt` (the seed line; `perf.py` gives it, `PERF_ORIG_PATCHES` names
another list). A run of the original without a list is not seed pinned: look for "applied 1 patch(es)" in its `origrun.log`.

## 9. Coverage check (2026-11-08)

The coverage check of `docs/COVERAGE.md`: origrun's `--coverage` recorded which of the original's functions ran, in 21 runs on
the host (the staged copy under `C:/byo-og/alch`, the virtual clock, no sound device, the seed pinned; the hits are in
`build/coverage/alchemy/hits/`, the scripts in `games/alchemy/tools/coverage/`):

- The seven screen tours of phase two (`tools/shots/tour1.txt` to `tour7.txt`, turned into host scripts by `make.py ... host`
  and run by `tools/coverage/all.sh`): the title, selector, options, help, high scores and quit question (1); a strategic game to
  its game over, the name entry and the table (2); a time trial game with wizard messages, hints and two losses of the focus (3);
  the forge bubbles at the end of game 2 (4); the tutorial step by step (5); a saved game continued, with the side panel's
  buttons and the new game question (6); the wizard's messages (7). Each ended with the game exiting.
- One minute of a bot game on the real clock with the silent sound stand-in (`build/perf/orig-play.txt`, seed 11, 100 updates a
  second), so that the music and the mixing run. It reached nothing in the game's code that the tours had not.
- Thirteen short runs (`tools/coverage/extras.sh`) for what the tours miss. Three start from the saved game: the forge gauge
  clicked, the hint, options and help opened in a game, the tutorial question answered no and then yes, the new game question
  answered no and then yes, and the quit question answered no and then yes. Ten close the window in each state: the title, the
  selector, the options dialog, the help, the high scores, the quit question, a game in progress, the game over, the name entry and
  the table. The original answers the close of its window with no question and ends at once, in every one of them.

`games/alchemy/coverage.txt` sets the game's own code as 0x401000-0x414a7d (the rest of the exe is the framework from 0x414c30,
byte-matched to BookWorm's, the C runtime and the libraries). `tools/coverage.py` counts 257 functions there (it leaves out the
library names), and the runs reached 217 of them.

**Counts, by `tools/coverage.py report`, at the first complete set of runs and at the end:**

| | at first (the tours and the real-clock minute) | at the end (all 21 runs, after the sorting) |
|---|---:|---:|
| game functions reached | 213 of 257 | 217 of 257 |
| reached in the original, no counterpart in the port | 81 | 0 |
| defined in the port, never reached | 2 | 1 |
| neither reached nor defined | 28 | 0 |
| `none` lines in `coverage.txt` (functions they cover) | 16 (16) | 86 (119) |

**Reached in the original with no counterpart: 81 at first, 83 once the extra runs added two, 0 at the end.** No game logic was
missing. Five got their address in a comment above the port function that holds them (comment edits only; `tools/check.sh
alchemy` still matches the frozen baseline): the help widget's destructor proper (0x4089a6), the title screen's destructor body
(0x41022c), `Rect::Inflate` and the rectangle copy that the name entry's box is made with (0x40ecd8, 0x40eb64: the port does the
arithmetic inline) and the original's `WinMain` (0x410b8c, above `main`). The other 78 are `none` lines, each with its reason: the
library code the compiler instantiated in the game's functions (`std::string`, `std::vector<std::pair<int,int>>`,
`std::vector<std::string>`, `std::list`, `ios::lock` and `ios::unlock`), the scalar deleting destructors of ported classes, the
trivial destructors of the classes the port leaves implicit (`Piece`, `Wizard`, `SidePanel`, `EffectsWidget`), the
`ButtonListener` stub, the `Rect` constructor and an empty stub. The vectors of pairs are not wizard messages, as a first reading
guessed: they are the list of cells left that `Board::Update` (0x406f2a) builds in the won state, which the port's `aLeft` does.

**Defined in the port, never reached: 2 at first, 1 at the end.** `SidePanel::MouseDown` (0x40f8d3, the forge gauge is a button) and
`AlchemyApp::ConfirmTutorial` (0x411f27, the question that the help screen's tutorial link raises while a game is on) needed
clicks the tours never made; the extra runs made them. Finding the right place to click took a probe of the port's `state`
command, which gives the positions of a dialog's buttons: the quit question with a game in progress is taller than without one,
so its buttons are at y 284 instead of 204. Tours 6 and 7 click at y 204 and missed it, which went unnoticed because the port's
script ends with `quit` anyway; the tours were left as they are, and the extra runs click the right place. In a game the button
at (32, 421) is the hint button; the help screen is reached through the options dialog (a link at (318, 237)).
The one left was `DoColorDepthExplination` (0x413942; in the port, the `DoDialog` call inside `OptionsDialog::CheckboxChecked`,
0x40d7c2), which is out of the port now (section 5). The export has no function at 0x40d7c2, so the report lists no caller for it. The question appears when a check box is
cleared while the app's `mForceFullscreen` (+0x30f) is set, which the framework does on a desktop without enough colours. The
host's desktop has them, so no run reached it. The port's version was read again against the export and matches: dialog 8, the
strings COLOR_DEPTH_ERROR1 to 3 as header, text and footer, the footer button mode, then the full-screen box checked again. A
run that patches the flag was not made, because the original's options dialog is taller than the port's and the position of its
check boxes was not found without a screenshot (the host takes none).

**Neither reached nor defined: 28 at first, 26 once the extra runs reached two library helpers, 0 at the end.** Thirteen are
exception funclets and library branches no play takes: the `Catch` handlers of `std::string::_Grow_alloc`, of the vector inserts
and of the string vector copy, the branches of `_Insert_n` that an append never takes, an `operator delete` thunk, and the app's
scalar deleting destructor, which the original never calls since its app is a global object. Twelve belong to the left-out
features and are rows of section 5 now: `Board::RegisterRefused` (0x4061e5), `HighScoreWidget::Registered` (0x409f8e), the
internet-score dialog's destructor, `AddedToManager`, `Resize` and deleting destructor, the register dialog's destructor and
drawing, and the three stream helpers of the score upload. The last is 0x413942 above. The 25 others are `none` lines, and none
of them is game logic the runs should have reached.

**Differences found, none with a visible effect.** The original's `Board` constructor copies the string table `gRankNames` into a
global vector (0x516080, through `vector<string>::operator=` at 0x407dc4); the port asks the app for the table where it needs the
names (`ComputeRank`). `Board` has an `std::ostringstream` member at +0xa84, built in 0x407ebe and destroyed through 0x401001, and
a `vector<pair<int,int>>` at +0xe50, that nothing uses. The port has neither.

**Unneeded code taken out (2026-11-09).** The colour-depth question and the handler that raised it (the body of
`OptionsDialog::CheckboxChecked`, `DIALOG_COLOR_DEPTH` and its `state` name): the port's display always has the colours, and
nothing in a port sets the flag (section 5). The frozen baseline stays identical. `mForceFullscreen` and the three strings are
not the game's to remove (the framework's field; the game's own `properties` file).

**Open items.** None.

**What it cost.** The 21 runs took about 45 minutes of one game process at a time, most of it waiting for room: other sessions'
games kept the machine at three to six processes. Two things went wrong on the way and were redone. The first set of runs of the
original had no seed pin (the worktree lacked the git-ignored `tools/origrun/patches/alchemy.txt`, which `tools/identical/setup.sh`
writes), so the scripted input played another game and reached no game over. Then a second copy of `all.sh`, started by mistake,
ran at the same time in the same run folder and spoiled the next set. Both were stopped by process id and the runs repeated. The
seed also changed the original's side of the Performance numbers (section 8, corrected). One Sonnet agent sorted the 109 functions
of the two lists in 15 minutes and 278,000 tokens; this session read its table, spot-checked its verdicts against the export
(0x407dc4, 0x413942, 0x4061e5, 0x409f8e, 0x40c3ca and more), corrected one of its own earlier `none` lines (0x40c3ca is the
selector's destructor body, not the internet-score dialog's) and ran the report again with the agent's lines.

## 10. Code the port does not need (2026-11-09)

The last finishing item (`docs/COORDINATION.md`: "figure out what code does not need to be present in the decomps"). Registration
and the buttons that lead out were already out (section 5), and so was the colour-depth question (section 9). This survey looked
for what no path of the shipped game reaches or needs, by six scans of the game's `src/` (the test hooks, which tests need, apart):

1. **Functions.** The 25 object files of the native build, every defined function against every relocation in all of them
   (`llvm-nm --defined-only`, `llvm-objdump -r`): 40 had no reference. The constructors and destructors among them are the
   aliased forms of the complete-object and base-object versions; every other name was searched in the source, and all but one
   are called, inlined by the compiler. The one with no caller is in the table.
2. **Compiler warnings.** Every source compiled with `-Wall -Wextra -Wunused -Wunused-member-function -Wunused-private-field
   -Wunused-template -Wunused-macros -Wunreachable-code`: nothing in the game's files (the two deprecation warnings are the
   framework's `Rect.h` and `Insets.h`).
3. **Fields.** Every member `m...` of the game's headers by whether any code reads it: all are read (the autoplay options by the
   test hooks only), except the one below.
4. **Constants.** Every enumerator, `#define` and `const` by use: the ones below.
5. **Pictures and sounds.** Every id of `Res.h` by use in the port (all 15 sounds are named by code the game runs), and each unused picture against
   the original: the original keeps its pictures in an array of 66 at the app's +0x6b0, its slot is the port's index plus one, and
   a search of the decompilation of 0x401000-0x414a7d for each slot's offset (`4 * slot`, direct and with an index added)
   finds who draws it.
6. **Windows leftovers, switched-off code, comments.** `_WIN32` appears once (the search for the retail install folder, which
   a Windows player of the unpacked game needs), no `#if 0`, no commented-out code, no `TODO`.

| What | Why it is not needed | Verdict |
|---|---|---|
| `AlchemyApp::RemoveWidgetAndDelete` (0x410ee0) | no caller: the original's four callers (0x408a5c, 0x41123e, 0x411367, 0x4115dd) are in the screens, whose four `SafeDeleteWidget` sites the port has with `RemoveWidget` written out beside them | removed |
| `DIALOG_SKILL` (dialog 1), its yes branch in `ButtonDepress` (the original's at 0x413fa4) and the field `mPendingSkill` (+0xecc), with the name `skill` of the `state` command | nothing opens dialog 1: the original never writes +0xecc (its only access is the read at 0x413fb0) and its exe never refers to the strings CONFIRM_SKILL_CHANGE1 and 2 (they are in the game's `properties` file), so a skill change in the selector never asks | removed |
| `BUTTON_TUTORIAL` (id 11) | an enumerator no code names (the selector's tutorial button is not a side-panel button) | removed; a comment keeps the id |
| pictures `waitbar`, `bubbles7`, `selectorbutton` | loaded by the loading thread; neither the port nor the original's code refers to their slots (the original loads them and never draws them; Bejeweled's notes record the same for its `waitbar`) | removed from the list and `Res.h` |
| picture `cartoonstar` | only the board's ring of eight pieces (0x40505b), the effect of section 5 that no code triggers | removed |

The four pictures leave 59 images and 95 loading tasks (63 and 99 before; the original has 101). The only effect a player could see
is that the title's loading bar has 95 steps instead of 99, and the port loads in one go.

**Looked at and kept.** `Board::mScriptPos` (+0xa80, saved and never used) is in the save file's format. `Board::mTutWaiting` is
written by the game and read only by the test commands, which need the tutorial's state. `OptionsDialog` stays the listener of its
two check boxes, since a `Checkbox` takes one (its handler is the framework's empty one). The registry values `SkillLevel`,
`TimeTrial` and `LastScoreName` and `mRegKey` are the original's settings. `GameText` reading the
default score names from `Alchemy.exe` is how the table gets them. `Fixes.cpp` and `SiteProfiles.cpp` are the fixes mod and the site's
hooks. The overrides of `SexyAppBase`'s hooks (`ShutdownHook`, `GetShutdownSaves`, `CreateMusicInterface`, `NewDialog`, `ModalOpen`,
`ModalClose`, `LoadingThreadProc`, `EditWidgetText`, `SliderVal`) are called by the framework. Strings in the game's `properties`
file that nothing in the port reads (CONFIRM_SKILL_CHANGE1 and 2, COLOR_DEPTH_ERROR1 to 3, CONFIRM_UPDATE1 and 2 and the
registration texts) are the game's data, not ours to edit.

**Doubtful, and how each was settled (the coordinator, 2026-11-09).**
- *The test hooks in the shipped game.* `src/testhooks/` (about 1,550 lines: the test commands, `-autoplay`, the bot, `memreport`) is
  compiled into the one executable that players get, reachable only from a test (`docs/TEST_COMMANDS.md`). They stay: the suites and
  the frozen baseline run the very executable the site serves, and the rule is only that a test command is reachable from a test.
- *Shared code.* `mForceFullscreen` and the registration members of `SexyApp` are the framework's (section 5); whether the
  framework still needs them is the coordinator's, who has them in the queue.

**Proof.** `tools/check.sh build alchemy` after the removals: the build passes and the suite's baseline is identical (strategic 12206,
time trial 14641, the tutorial, the saved game, the rerun). The suite plays the game's logic, so the pictures were checked as well,
with a control: the source changes were put aside as a patch, the game built as it was before them, and the screen tours 1, 2, 5 and 7
(`tools/shots`: the title, selector, options, help, high scores and quit question; a strategic game to its table; the tutorial;
the wizard's messages; same script, same seed, `-fast`) run on it, then the patch applied, the game built and the tours run again.
All 106 screenshots are byte for byte the same (17, 40, 35 and 14). A script also reads `Res.h` and the name table in
`AlchemyApp.cpp` before and after and finds every id still at the same file name, in the same order (63 against 63, 59 against 59),
and the only arithmetic on ids (`IMAGE_LEVEL2 + n`, 43 to 50, and `IMAGE_WIZARDHEAD + n`, 21 to 25) stays inside runs that no
removal cuts.

## 11. The readability pass (2026-11-11)

The pass of `docs/CODE_STANDARDS.md`. Nothing the port does changed (the proof is at the end), and
`python tools/namecheck.py games/alchemy` finds nothing, with no exception needed. Sections 1 to 7 of these notes use the new
names; sections 8 to 10, `IDENTICAL_PLAY.md` and `MAC_COMPARISON.md` are reports and keep the names of their time.

**Names from the Mac build.** The Windows exe has no names for the game's classes, so the port's were the decompilation's guesses;
the Mac build has the original's (`MAC_COMPARISON.md`), and `tools/macmatch.py` pairs 120 of the game's functions with them. Seven
classes changed: `AlchemyApp` is `WinAlchApp`, `SelectorScreen` `IntroScreen`, `NameEntryDialog` `RecordScoreDialog`,
`AllTimeDialog` `HighScoreDialog`, `EffectsWidget` `BoardOverlay`, `WizardBox` `Wizard` and `SidePanel` `Sidebar`, and their files
follow (`WinAlchApp.cpp`, `IntroScreen.cpp`, `BoardOverlay.cpp`, `Sidebar.cpp`, with their headers), as do the fields named after
them (`mIntroScreen`, `mBoardOverlay`, `mSidebar`, the board's `mWizard`) and `DIALOG_RECORD_SCORE`. The methods:

| class | was | is |
|---|---|---|
| `Board` | `HasProgress`, `Resume`, `SetDifficulty`, `NewPiece`, `NewCurrentPiece` | `IsInProgress`, `Unpause`, `SetBoardVars`, `MakeRandomPiece`, `NextMove` |
| | `RecordMove`, `RecordPlacement`, `RecordForge` | `EncodeMoveNum`, `EncodeMove`, `EncodeDiscard` |
| | `ColToX`, `RowToY`, `XToCol`, `YToRow` | `GetColX`, `GetRowY`, `GetColAt`, `GetRowAt` |
| | `CanPlace`, `ComputeValid`, `CountValid`, `ShowHint` | `ValidMove`, `DetermineValidMoves`, `GetNumPossibleMoves`, `ShowPossibleMoves` |
| | `PlacePiece`, `PlaceCurrentPiece`, `UseForge`, `UseForgeClicked` | `PutPiece`, `MakeMove`, `SkipMove`, `Discard` |
| | `ClearGrid`, `CheckAllLit`, `GameWon`, `BoardCleared`, `StartNextBoard` | `ResetBoard`, `CheckWin`, `ClearBoard`, `ShowBoardComplete`, `NextBoard` |
| | `ComputeRank`, `AdvanceTutorial`, `ShowHelpBox` | `GetGameStats`, `NextTutorial`, `ShowWizard` |
| | `RedrawCell`, `RedrawAllCells`, `DrawPieceImage`, `DrawStatLine` | `RedrawBackground` (both), `DrawAliasedPiece`, `DrawColumn` |
| `WinAlchApp` | `StartGame`, `ShowSelector`, `TitleScreenDone`, `ConfirmLoad` | `NewGame`, `ShowIntroScreen`, `LoadingCompleted`, `TryConfirmLoad` |
| | `ShowHelp`, `HideHelp`, `ShowOptions`, `ShowAllTimeScores`, `ShowNameEntry`, `ShowHighScores` | `DoHelp`, `RemoveHelpWidget`, `DoOptions`, `DoHighScoreDialog`, `DoRecordScoreDialog`, `DoGameOver` |
| | `SetSkillLevel`, `SetTimeTrial`, `LoadImage`, `AddPanelButton` | `ChangeSkillLevel`, `ChangeTimeTrial`, `DoLoadImage`, `CreateControlButton` |
| `HighScoreWidget` | `Load`, `Save`, `CheckScore`, `RankOf`, `SetName` | `ReadFromFile`, `WriteToFile`, `ProcessScoreStuff`, `GetScorePlace`, `NameEntered` |
| others | `SelectorScreen::SetSkill`, `EffectsWidget::AddScorePopup`, `WizardBox::Talk` and `StopTalking`, `SidePanel::SetFlashing` | `IntroScreen::SetSkillLevel`, `BoardOverlay::FloatPoints`, `Wizard::Play` and `Stop`, `Sidebar::FlashLava` |

One pairing of `MAC_COMPARISON.md` is corrected on the way: it took 0x4017dd for the Mac's `ClearBoard` by size, and 0x406209 with
0x4062e5 for the two halves of `CheckWin`. macmatch pairs `ClearBoard` with 0x406209 by its calls (it is what `CheckWin` calls when
every tile is lit: the board's pieces and tiles then vanish), so 0x4017dd, which empties the grid, is `ResetBoard`. Its size
fits (132 bytes on the Mac against 82); `ClearBoard`'s 168 against 220 is below the usual ratio, so that pairing rests on the calls. macmatch had the two one-line cell functions the wrong way round (0x4014e6 adds the grid's x, 10, so it is `GetColX`;
0x401510 adds its y, 0x27); the port follows the code.

**Structure, as the original has it.** `HasSavedGame` (0x4010fc) and `DeleteSavedGame` (0x40111e) are `Board`'s, static since
neither uses an object (the port had them in the app). The wizard's message is `WinAlchApp::GiveHelp` (0x4114ff), which takes the
three texts; `Board::GiveHelp(prefix)` is the port's own helper that looks the three up, where each of the original's calls
spells them out. `HighScoreWidget::GetDayNum` (0x409e60), `HighScoreWidget::DrawColumn` (0x409f01) and
`RecordScoreDialog::DrawEditBox` (0x40eb9f) are members; the port had free functions.

**Fields named for what they hold** (the Mac build carries no field names, so these come from their use). `Board`: `mPause` and
`mTrialPause` are `mPauseCount` and `mCoverCount` (how many pauses hold the game, and how many of them cover the board: a key
press in any game, a dialog in the time trial; `Pause` and `Unpause` take `isCovered`), `mScriptPos` is `mSavedZero` (0 throughout,
saved and loaded, nothing reads it), `mRecord` is `mEncodedGame` (the game move by move for the original's internet scores: saved,
nothing reads it), `mFxImage` and `mFxPiece` are `mDiscardImage` and `mDiscardPiece`, `mTutStep`, `mTutCount` and `mTutWaiting` are
spelled out, `mOverFade` and `mOverTimer` are `mGameOverFade` and `mGameOverTimer`. `WinAlchApp`: `mSelectorFlag` is
`mBeforeFirstGame` and `mDiff` is `mForgeRates` (the four numbers of `data\diff.txt`). The short locals of the test commands, of
`GameText.cpp` and of the score table's drawing are words.

**Numbers.** The board area's rectangle, written out in thirteen places (and its width in three more), is `BOARD_X`,
`BOARD_WIDTH` (new) and `SCREEN_HEIGHT`;
the 50 pixels of a piece's picture and of a frame of the symbol and skull sheets are `SYMBOL_SIZE`. The explosion sheet's 50-pixel
frames in `Piece::Draw` are another thing and keep their number. Other coordinates are used once, beside the call that draws
with them.

**Addresses and comments.** Of the 35 functions without an address one came out (below); the others now name it, the original
function they are part of (the save
file's field helpers are part of `Board::SaveGame` and `LoadGame`, the score file's of `HighScoreWidget::WriteToFile`,
`FlashColor` of `Board::Draw`), or carry the port marker; the test hooks' three files, `GameText.cpp` and `Fixes.cpp` carry it in
their first comment. Comments point to these notes by section name, the app's header no longer names a file that does not exist
(`AppButtons.cpp`), and two comments that named a pass of the work say what the thing is for instead.

**What came out.** `BoardOverlay::Clear`, the port's own function that nothing called (section 10's scan had taken a call of
`MemoryImage::Clear` for a call of it).

**Left as it is.** The texts that test scripts and logs carry (`selector`, `name`, `alltime`, `cover` in the `state` command's
answer) are formats and keep their spelling. `main.cpp` has the locals every game's `main.cpp` has. Flag parameters keep the
framework's own `is` and `want` forms.

**Proof.** Everything but the structure corrections and the removal was made first and compared by the object code: every one of
the 25 units compiles to the same instructions as before the pass (`llvm-objdump -d` of each object, symbol names taken out, since a
renamed class changes every mangled name). After the corrections seven units differ, the ones they touch (`WinAlchApp`,
`AppScreens`, `Board`, `BoardSave`, `Dialogs`, `BoardOverlay`, `HighScores`), and the rest are the same. For those,
`tools/check.sh build alchemy` passes with the baseline identical (strategic 12206, time trial 14641, the tutorial, the saved game
loaded, the rerun), and the screen tours 1, 2, 5 and 7 (the title, selector, options, help and scores; a strategic game
with its name entry and table; the tutorial; the wizard's messages), run before the pass and after it with the same script and
seed, give 106 screenshots that are byte for byte the same.

## 12. The software path (2026-11-11)

**The question.** What would a port cost if it drew through SDL's software renderer, with no Direct3D device
(`SDL_RENDER_DRIVER=software`), instead of the default path? And the user's idea: nothing in these games looks better drawn
at 4K, so draw at the game's own size and scale the finished frame at the end. This is a test; no default changed. The tools
are `games/alchemy/tools/profile/softpath.py` (one measured run), `softpath.sh` (the matrix), `softpath_table.py` (the tables
below), `softshots.sh` (the pictures), and the shared `tools/scaletest`, `tools/memmap.py` and the `memreport` test command.
Every run is under rundesk, on a fresh user folder, seed 11, with the screen mode written into its registry.ini: windowed (2x,
1280 x 960) and full screen on the 4K screen (3840 x 2160). Four ways of drawing: **default** (the Direct3D renderer, the
frame drawn at the window's scale), **default at 640** (`-renderscale=1`: drawn at 640 x 480 and scaled by the renderer's
final copy), **software at 640** (the same with the software renderer) and **software at the window's scale**.

**Memory and cost, Alchemy.** Private bytes (MB) from `memreport` at start (before the game opens a file: the runtime, the
libraries and the device), at the title, in play (update 3000) and at the end of the bot's game; the peak; the processor
cycles of a real-time game (65 s, a scripted bot game) and the frames it drew; the frame time, from two `-fast` runs of the
same game that differ only in how often it draws (the difference in wall time over the difference in frames, 5390 of them;
`*` marks the short pair, 935 frames, where the long pair was not run).

| drawing | window | private: start / title / play / end | peak | real-time game: G cycles, frames | one frame: ms, M cycles |
|---|---|---|---|---|---|
| default | window | 30.1 / 60.1 / 68.7 / 71.3 | 71.3 | 11.5, 3607 | 0.47, 1.2 |
| default | full screen | 30.0 / 59.5 / 67.4 / 69.6 | 69.6 | 13.4, 3345 | 0.31, 0.9 |
| default at 640 | window | 29.8 / 59.7 / 66.5 / 68.0 | 68.0 | 11.6, 2875 | 0.68, 0.9 |
| default at 640 | full screen | 30.5 / 61.8 / 73.8 / 76.0 | 76.0 | 12.6, 3369 | 0.50, 1.0 |
| software at 640 | window | 8.3 / 37.4 / 43.7 / 45.3 | 54.8 | 83.6, 2044 in 73 s | 24.7, 39.9 |
| software at 640 | full screen | 8.3 / 64.3 / 70.9 / 72.3 | 97.4 | 126.2, 810 | 80.8, 167.1 |
| software at the window's scale | window | 11.9 / 40.9 / 47.3 / 48.8 | 50.7 | 104.9, 3152 | 14.9*, 29.9* |
| software at the window's scale | full screen | 11.9 / 84.8 / 91.3 / 92.6 | 117.6 | 181.4, 673 in 93 s | 156.4*, 290.7* |

In a window the software renderer saves 12 to 25 MB of private bytes in play, depending on how it is counted (the reports:
66.2 against 49.9 in the real-time game, 71.3 against 45.3 in the `-fast` one; the peaks: 66.8 against 54.8 and 71.3 against
50.0; at 640 its peak lies 5 MB above its reports, in full screen 24 MB). The saving is the device: 30 MB before any file
against 8 to 12. In full screen it is gone: the window's own surface at 3840 x 2160 is 33 MB, and the peak is 97 to 118 MB
against 70. The price is time: 24.7 ms a frame against 0.5 (the game keeps about 28 frames a second in real time where the
default path keeps 55) and 40 M cycles a frame against 1, so a real-time game costs 7 to 9 times the processor cycles in a window
and 9 to 14 times in full screen. Drawing at the window's scale is cheaper per frame in a window (14.9 ms) than at 640
(24.7), because the 640 path ends with SDL's linear scale of the finished frame, which `tools/scaletest` puts at 10.8 ms in a
window and 56 to 75 ms at 4K; that is also why full screen takes 81 to 156 ms a frame. The software drawing itself is about 13
to 14 ms a frame at 640.

**Bookworm Adventures** (the larger game; the script only waits on the first cutscene with the new-player dialog, so "play"
is that screen, and the frame time is the short pair, 1350 frames):

| drawing | window | private: start / title / play / end | peak | real-time game: G cycles, frames | one frame: ms, M cycles |
|---|---|---|---|---|---|
| default | window | 29.6 / 160.7 / 165.8 / 166.3 | 168.1 | 16.0, 3482 | 0.52, 0.4 |
| default | full screen | 29.6 / 164.6 / 169.5 / 170.3 | 171.0 | 15.2, 3472 | 0.50, 0.6 |
| default at 640 | window | 29.7 / 167.5 / 170.6 / 170.7 | 170.9 | 15.6, 3401 | 0.12, 0.6 |
| default at 640 | full screen | 30.5 / 165.5 / 171.1 / 172.1 | 172.6 | 14.1, 3597 | (noise) |
| software at 640 | window | 12.2 / 144.3 / 150.5 / 150.5 | 161.8 | 131.7, 1882 | 28.0, 54.8 |
| software at 640 | full screen | 12.2 / 165.5 / 171.8 / 171.9 | 202.6 | 131.4, 970 | 102.0, 200.3 |
| software at the window's scale | window | 17.7 / 149.6 / 156.3 / 156.3 | 160.1 | 101.5, 816 in 71 s | 58.8, 106.3 |
| software at the window's scale | full screen | 17.7 / 193.2 / 199.4 / 200.0 | 230.1 | 250.8, 671 in 124 s | 451.0, 609.8 |

The saving is 6 to 16 MB in a window (the pictures dominate a bigger game: 160 MB at the title; by the peak only 6 to 8 MB),
none in full screen, and the frame costs 28 to 59 ms in a window and 102 to 451 ms in full screen against 0.5.

**The memory map** (`tools/memmap.py`, 30 s into the real-time bot game; MB). The original under origrun is the reference.

| | images | mapped | stacks | private | process private bytes | working set |
|---|---|---|---|---|---|---|
| original (origrun) | 89.0 | 29.6 | 1.1 | 47.4 | 52.2 | 73.4 |
| port, default, window | 139.3 | 10.0 | 2.6 | 42.1 | 66.8 | 66.9 |
| port, default, full screen | 169.4 | 29.6 | 3.0 | 43.3 | 69.0 | 75.5 |
| port, software at 640, window | 87.4 | 29.4 | 0.9 | 50.1 | 53.9 | 73.1 |
| port, software at 640, full screen | 87.4 | 29.4 | 0.9 | 95.2 | 99.2 | 118.2 |

The default path maps the graphics driver's libraries (77 MB in two files on this machine; shared, and not counted as private
bytes), and the process's private bytes exceed stacks plus private by 22 MB, most likely the image pages the driver's
libraries changed (not broken down further). The software path loads none of it: its map (87 MB of images, 54 MB private
bytes) is the original's (89, 52), so the port's 15 MB over the original in a window is the device. Full screen with the
software renderer adds one window-sized surface (33 MB at 4K) and SDL's present copy.

**Scaling the finished frame, each way** (`tools/scaletest`, Alchemy's 640 x 480 frame, 200 frames each; milliseconds to
scale / to hand to the window, and the process's processor cycles a frame; "exact" fills the window keeping the frame's shape,
"whole" is the largest whole-number factor with borders; the hand-over is measured on rundesk's invisible desktop, and a GDI
call into memory is the processor's work alone).

| way | 2x window, exact (1280 x 960) | 4K, exact (2880 x 2160) | 4K, whole factor 4 (2560 x 1920) |
|---|---|---|---|
| SDL software renderer, nearest | 1.6 / 1.3, 4.9 M | 7.7 / 11.3, 25.5 M | 7.2 / 9.8, 25.3 M |
| SDL software renderer, linear (what the framework does now) | 10.8 / 1.2, 24.1 M | 55.9 / 5.2, 126.1 M | 75.2 / 5.8, 128.7 M |
| SDL_BlitScaled to the window's surface | 0.6 / 1.7, 2.4 M | 5.0 / 11.6, 21.5 M | 8.7 / 15.2, 25.8 M |
| GDI `StretchDIBits`, nearest, into memory | 0.7, 1.4 M | 4.5, 9.2 M | 7.0, 11.3 M |
| GDI `StretchDIBits`, nearest, into the window | 1.4, 0.4 M | 3.6, 0.4 M | 1.7, 0.6 M |
| GDI `StretchDIBits`, halftone, into the window | 7.8, 14.3 M | 33.5, 67.2 M | 47.2, 88.9 M |

`StretchDIBits` with nearest sampling into the window is the cheapest: 1.4 ms at 2x, 3.6 ms at 4K and 1.7 ms at the whole
factor. Its cycles are only the process's own (the display driver or the compositor does the rest outside the process), so the
milliseconds are the figure to compare. SDL's own nearest copy is 1.6 ms in a window and 7.7 at 4K; linear is the expensive
one, and the framework's final copy uses it (`SDL_SetTextureScaleMode` in `DDInterface::CreateScreenTexture`).

**The pictures** (`softshots.sh`, with the `tools/shots` tours 1, 2, 5 and 7: 106 screenshots each, windowed, by the default
path and by the software renderer, at the window's scale and at `-renderscale=1`). A nearest scale of the 640 x 480 frame, by
SDL or by GDI, is **byte for byte** the default path's picture at scale 2 (window) and at scale 4 (full screen): drawing at 4K
gives this game nothing the game-size frame does not already hold. Linear and halftone scaling differ from it in 81 to 84% of
the pixels (mean 3.1 to 3.6 levels). The software renderer's own pictures differ from the default path's in every screenshot of
the tours, by at most 4 levels of 255 in a channel (median 3.1 to 3.9% of the pixels, at most 6.8%): its blending rounds
differently from the GPU's. At the whole factor 4 the nearest picture is clean and leaves borders (640 px at the sides, 120
above and below); at the exact fit of 4.5 it makes pixels of uneven width (4 and 5 wide).

**What leaving the scaling to Windows would take.** Draw each frame at the game's own size with the software renderer
(`-renderscale=1`, about 13 ms a frame in Alchemy, about 17 in Bookworm Adventures), and end the frame with one
`StretchDIBits` (COLORONCOLOR) of that frame into the window's device context instead of SDL's linear copy. The measured
parts add up to about 15 ms a frame in a window and about 17 at 4K for Alchemy (24.7 and 80.8 now; 0.5 by the default path);
that sum is not built or run in the game. The code is a Windows-only present step in `DDInterface`: the window handle from
`SDL_GetWindowWMInfo`; the frame's pixels from the render target with `SDL_RenderReadPixels` (a copy of its own, not measured);
and the software renderer must not also own the window's surface, which has to be tried. The cheaper change, with no new
code, is the final copy's scale mode: nearest where the factor is whole (1.6 ms against 10.8 in a window, 7.2 against 75 at
4K whole), which the framework could choose per factor. Neither changes what the software path is: its drawing alone is about
13 ms a frame against 0.5 for the default path, so it is a fallback for machines with no usable 3D device, not a way to lower
the default path's memory (that is the driver's).

**Not measured.** The software path's sound or loading time; Bookworm Adventures' pictures (compared on Alchemy only); the
long pair for the software path at the window's scale and for Bookworm Adventures; the visible desktop's hand-over cost.
