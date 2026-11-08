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
| 0x40f987 the side panel (0x4e4b40, size 0x90: forge gauge, score, labels), 0x40c817 the selector (0x4e43a8, size 0xa4) | `SidePanel.cpp`, `Selector.cpp` |
| 0x40d836 the options dialog (0x4e4540, size 0x144), 0x40ed00 the name entry (0x4e4880) | `Dialogs.cpp` |
| 0x40e433 `Piece` (0x4e4798, size 200) | `Piece.cpp` |
| 0x4103e2 the title screen (0x4e4c50, size 0x88) | `TitleScreen.cpp` |
| 0x4118c9-0x414903 `AlchemyApp` (vtable 0x4e5160, 72 slots) | `AlchemyApp.cpp` (start-up, loading, registry, side-panel buttons), `AppScreens.cpp` (the screens' flow), `AlchemyApp.h` |
| 0x40858e the effects overlay (0x4e3ab0, size 0x78: floating scores, sparkles, the help arrow) | `Effects.cpp` |
| 0x414000-0x47f000 the framework (2857 functions byte-match BookWorm's) | `framework/` with `sexycompat2004` |

The unregistered game's screens (0x40f314 the register dialog and its kin, 0x413bdc and 0x413e12 the nags, 0x41351d and
0x4131ca the internet scores, 0x412139 the update check, 0x412aa0 and 0x412c68 the browser dialogs) are not ported.

## 2. The game

**The board** is 9 columns by 8 rows of 52-pixel cells at (10, 39) in a 490 x 480 widget at x = 150; the side panel is
the left 150 pixels. A piece (`Piece`) has a shape (22 symbols) and a colour (7), or is the stone (the blank block, wild:
`mSkull` in the code) or a skull (a bomb that destroys a rune: `mBomb`). The piece to place hangs from the mouse (`Piece::Update` puts it 0x30 pixels up and left
of the pointer) and the cell under its middle is the one it would go to (`Board::MouseMove`).

**Placing** (`Board::CanPlace`, 0x40153a). The grey stone starts in the middle (col 4, row 3) and the first rune must touch it.
Later a cell must touch a piece, and every piece it touches must share the new piece's colour or shape; a stone matches
anything. A skull goes on a piece instead. The reasons a click fails choose the wizard's message (`INVALID_MOVE_*`).

**Clearing.** Completing a row or a column makes it flash and go (the state `STATE_CLEARING`); a cell that was used stays lit,
and when every cell is lit the board is won: the next board comes with more colours and shapes
(`Board::SetDifficulty`: `colours = (level + 3) / 4 + 4`, `shapes = (level * 3) / 4 + 5`, the chance of a bomb or a stone per
mille `35 - 3 * level`, where level is the skill's base (0, 5 or 10) plus the boards cleared).

**The forge** (the gauge in the side panel, `Board::mProgress`) takes the piece that cannot be placed: the discard
(`Board::UseForge`, 0x405e31) melts it and raises the heat; placing a piece cools it; at 3 the game ends. The strategic mode
fills it only by discards, the time trial mode also by itself (`mProgRate`, `mProgDecay`, from the four numbers of
`data/diff.txt` and the boards cleared). The tutorial allows a discard only in step 6.

**Skill** 1 to 3 (easy, average, hard) scales the points by 0.5, 1 and 2 (`mRate`, doubled in the time trial) and picks the
level base. The rank names (`gRankNames` in `properties/default.xml`) come from the score; the cleared-board panel shows the
next rank's score.

**The tutorial** is 14 steps (`mTutStep` -1 to 13): the board is set up for each, the wizard says what to do and waits for a
button (`mTutWaiting`), a wrong placement gets a message, the last step's button starts the real game.

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
- the registry (`registry.ini` in the port): `SkillLevel`, `TimeTrial`, `LastScoreName`, `LastInternetScoreName` (kept,
  never shown), `NoPopLink`, and the framework's (volumes, full screen, custom cursors, times played).

## 4. Port decisions

- The strings are ids in `properties/default.xml`, read by `LoadProperties()` (Init); there is no GameText reader.
- The game's clock is `GameTickCount` (the virtual clock under `-fast`) where the original calls `timeGetTime`, so test runs
  repeat exactly.
- The 2004 `Dialog` places and wraps text differently from the port's: `Dialog2004` (the compat library) is used for every
  dialog; `AllTimeDialog` returns the height 0x156 as the original's does.
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

**Registration and the buttons that lead out (the coordinator's rules of 2026-11-07, checked 2026-11-08).** The port never had
the trial clock or counter, the nags, the register, unlock and buy dialogs and their checks, expiry, the update check, the
internet scores, the "more games" button (id 3 of the dialogs) or any call that opens a web page or another program (no
`OpenURL` in `src/`; the title's one link is "click here to play", which starts the game). A search of `src/` for the words
of a registered/trial split found only two leftovers, both taken out now, and no visible change comes of either (the
suite's baseline is identical):
- the picture `trialversion` (`IMAGE_TRIALVERSION`, the title screen's "buy" overlay of section 5's first paragraph), which was
  loaded and never drawn: gone from the image list and from `Res.h`, whose later ids move down by one (65 images, 9 loaded by
  the title's start-up as before, the loading task count unchanged at 101 since the picture was among the start-up's);
- the registry value `NoPopLink` and its field `mNoPopLink`, which only hid the "more games" link the port does not have.

`Board::mTrialPause` and `Pause(true)` are not registration: the name "trial" is the decompilation's, and the pause it counts is
the one that covers the board (`Board::Draw` draws the cover instead of the tiles, and the pause text when no dialog is up): any key press toggles it in every mode
(`KeyChar`), and a dialog raises it in the time trial (`AlchemyApp::ModalOpen` passes the mode). They stay. Loading has no waits (the title screen's bar follows the loading thread, which runs
once, with no pacing), so the rule on fast loading changes nothing here.

## 6. The standard test commands

The port answers the standard test commands (`docs/TEST_COMMANDS.md`) in `AlchemyApp::HandleScriptCommand`
(`src/testhooks/Commands.cpp`). They are reachable only from a test: `-script` lines (`<update> <command>`), and the live
channel (`-commands`, the web's `byopop.test.command`) once the framework has it. A game that is never given one is not changed:
the suites and the frozen baseline play as before. Each command answers with one line on stdout (`OK <command>`,
`ERROR <command>: <reason>`, `STATE {...}` or `MOVE {...}`).

| Command | What it does |
|---|---|
| `state` | `STATE {json}`: `screen` (title, menu, game, highscores, options, help), `dialog` (name, load, hint, quit, newgame, skill, tutorial, options, alltime), `updates`, `mode` (strategic, timetrial, tutorial), `skill`, `level` (the board, from 1), `score`, `forge` (0 to 3), `over` (the game has ended), `busy` (the board cannot take a piece now), `board` (`state`, the `pieces` as `[col, row, shape, colour, stone]`, `lit` as eight strings of nine digits, `litCount`, `piece` to place, `fits`, `moves`, `forged`, `boards` cleared, `shapes`, `colors`, `paused`, `cover`, and `tutorial` with `step`, `waiting`, `count`, `button`), `ready` and `link` on the title, `active`, `windowed`, `muted`, and the places of the buttons a driver presses: `panel`, `options`, `selector`, `table`. |
| `goto <screen>` | `menu` (the selector), `options`, `help`, `highscores` (the all-time table), `strategic` or `timetrial` with an optional skill 1 to 3, and `tutorial`, which start that game through the selector's own buttons. `title` and `credits` are errors: the game has neither to go to. |
| `level <n>` | Board n of the game being played: n - 1 boards cleared, an empty grid (a first board has its stone), the difficulty follows. |
| `unlock all` | `ERROR`: the game has nothing locked. |
| `set <name> <value>` | `score`, `forge` (0 to 3) or `boards` (boards cleared; the difficulty follows). |
| `win` | Every tile lit: the board is won through `CheckAllLit` (the bonus, the pieces vanish, the panel). |
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
| `rims` | `Board::RedrawCell` (0x401a82) gives a lit tile a half-opaque rim where it borders an unlit one, but tests the left and top neighbours with `1 < col` and `1 < row` and the right and bottom ones with `col < 8` and `row < 7`: the first column and row get none on their inner side, while the last ones get theirs. | The tests are `0 < col` and `0 < row`: the edge between a lit tile of the second column (or row) and an unlit tile of the first has its rim, as every other edge. Only the one-pixel line at the second column's left edge and the second row's top edge differs. |
| `continuedEnding` | `Board::LoadGame` (0x405b39) puts each piece of a continued game at its cell without the board's offset (150, 0) that every placed piece has until the next `Resize`, and the end-of-game shrink draws pieces from that position: in a game that was continued and then lost, the pieces shrink 150 pixels left of their tiles, half of them under the side panel, and the board looks emptied at once. | The pieces get the offset, and shrink away on their tiles as in a game that was not continued. |

**Looked at, not made into toggles.** The port's `Board::~Board` counts a game as played when the forge is nearly full and the
game has not ended (`mState != STATE_GAME_OVER`), but a game that is lost counts in `GameOver` and the board is in
`STATE_ENDING` for the next second or so: leaving then counts it twice. The count only decides whether the selector still shows
the tutorial's button (fewer than five games), and only a game left in that second is counted twice, so it is not a fix worth a
toggle. A name entry that is accepted empty drops its score without a word (`HighScoreWidget::SetName`): that is the game's rule
for "no name", not a mistake the port can mend without making up what the original would have shown. The game's end is slow
(about three seconds of shrinking and fading, then the table) and a click is ignored meanwhile, as in a board cleared (the
pieces vanish three at a time): those are the game's animations, part of its pace, so they stay. Loading is already as fast as
the machine: the framework runs the loading thread in one go, and the title polls it every fourth update (at most 40 ms).
