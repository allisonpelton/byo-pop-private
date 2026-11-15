# Big Money Deluxe: game-code behaviour spec

This is the behavioural specification of Big Money Deluxe's game code: 0x401000-0x421830 of the game
PE carved from `WinBM.exe`, linked 2005-11-14 with MSVC 7.0 on SexyAppFramework 1.2. It plays the same
role as Bejeweled's `games/bejeweled/docs/GAME_NOTES.md`. Translators use it together with:
- the decompilation in `re/bigmoney/ghidra/` (git-ignored; regenerate as in [README.md](README.md));
- the framework slot and field reference [FRAMEWORK_12.md](FRAMEWORK_12.md);
- the function list [INVENTORY.md](INVENTORY.md).

Function, field and data names are in `symbols/*.jsonl` (for `ApplySymbols.java`), and the port's
headers in `games/bigmoney/src/` use the same names.

The spec was written in five module analyses, which are Parts 1-5 below:
- **Part 1, Board logic**: the rules, update, input and save game.
- **Part 2, Board drawing**: Board::Draw, plus MrMoney, Points and OverlayWidget.
- **Part 3, the application**: WinBMApp, the resources, the dialogs, MoneyDialog and LockableButton.
- **Part 4, the screens**: TitleScreen, StartButton, GameSelector, HelpWidget and CreditsDialog.
- **Part 5, the score and option dialogs**: HighScoreWidget, HighScoreDialog, RecordScoreDialog,
  OptionsDialog and `hiscores.dat`.

Every coordinate, colour, constant and x87 formula in them was read from the disassembly; Ghidra hides
many stack arguments and `_ftol` operands in this binary. Each part ends with its own cross-module
references and open questions. This overview ties them together. Where a part's reconciliation notes
disagree with another part, the decision is recorded in [section 0.9](#09-reconciled-decisions).

Contents:
- [0. Overview](#0-overview)
- [Part 1: Board logic](#part-1-board-logic)
- [Part 2: Board drawing, MrMoney, Points, OverlayWidget](#part-2-board-drawing-mrmoney-points-overlaywidget)
- [Part 3: The application (WinBMApp, resources, dialogs)](#part-3-the-application-winbmapp-resources-dialogs)
- [Part 4: Screens (title, selector, help, credits)](#part-4-screens-title-selector-help-credits)
- [Part 5: High scores and options](#part-5-high-scores-and-options)
- [Part 6: The port's test commands, fixes, removals and drivers](#part-6-the-ports-test-commands-fixes-removals-and-drivers)
- [Coverage check (2026-11-08)](#coverage-check-2026-11-08)

---

## 0. Overview

### 0.1 Conventions (all parts)

- **Offsets** are absolute from the object start, in hex. Game classes derived from Widget start their
  own data at +0x80, or at +0x84 when a ButtonListener vfptr sits at +0x80; Dialogs at +0x148; WinBMApp
  at +0x6d0. The port keeps the field order and gives each field's original offset in a comment.
- **Ticks**: `Update()` runs at a fixed 100 Hz. `mUpdateCnt` (WidgetContainer +0x24) counts a widget's
  updates. Board updates its own only while not paused, so its cadences freeze during a pause.
- **Arithmetic**:
  - `(int)` is x87 `_ftol`, which truncates toward zero.
  - Integer `/` and `%` are C signed operations. Exception: `MTRand::Next() % n` is an unsigned modulo.
  - Doubles are computed on the x87 with 53-bit precision control, which the port's SSE2 doubles
    reproduce.
  - `fsin`/`fcos` match C `sin`/`cos` for every value the game produces; this was checked against every
    truncation boundary.
- **Strings**: `GetString(id)` reads `properties/default.xml`, and `&cr;` is a newline. Strings marked
  "literal" are hard-coded in the exe.
- **Colours**: `Color(r,g,b)` has alpha 255.

### 0.2 Structure

```
WinMain (0x41b670) -> gWinBMApp (static global, 0x828 bytes): Init, Start, Shutdown
  TitleScreen          loading bar, logo, Mr. Moneybags, "CLICK HERE TO PLAY"
  -> TitleScreenIsFinished: side buttons (New Game, Options, Quit), MrMoney widget, GameSelector,
     "Continue?" dialog when savegame.dat exists
  GameSelector         game type (Action / Strategy / Puzzle) and skill (1..3); slides in and out
  -> StartGame: HelpWidget (per-mode how-to-play, first time per run unless "don't show again")
  -> NewGame: Board (0x810 bytes) covering the screen; music by mode
  Board                the coin grid (14 x 15), rules, scoring, levels, game over
  -> game over: HighScoreWidget slides up inside the board; RecordScoreDialog for a new high score;
     "CLICK HERE TO PLAY AGAIN" -> NewGame (same mode)
  Dialogs              MoneyDialog subclasses (options, high scores, name entry) and plain Dialogs
                       (confirmations); CreditsDialog from Options
```

The side panel (left bar) and Mr. Money stay on screen from the title screen onwards. The Board is
frozen (state 8) behind the GameSelector when the player picks a new game during play.

### 0.3 Game modes and levels

| | Action (0) | Strategy (1) | Puzzle (2) |
|---|---|---|---|
| Coins | 4 colours (3 at level 0, skill 1) | as Action | 5 colours (4 at level 0, skill 1) |
| Minimum group | 3 | 3 | 2 |
| New rows | rise over time (`mMint`, accelerating) | rise per valid click; also when no moves are left | none; a fixed board from a constructive generator |
| Money bags | drop when the money meter fills; collect one by removing the coin under it | as Action | already on the board; drop them off the bottom row |
| Dollar bombs | random drops (level >= 1) clear one colour | no | no |
| Level won | bank `mBagsNeeded` bags | as Action | release `mBagsNeeded` bags |
| Lost | coins reach the top: a 3-second countdown, then game over | a row must enter while the top row is occupied | no group of 2 left while bags are still needed |

Level parameters use `L = level + {0, 4, 7}[skill - 1]`:
- meter capacity: `300 + 20L` points;
- points multiplier: `0.5 + 0.1L`, tripled in Puzzle;
- bags needed: `clamp((int)(2.6 + 0.26L), 1, 8)`;
- mint speed: `0.0025 + 0.001L`, and its acceleration `1e-6 + 5e-8L`.

Part 1 section 1 has the full table.

### 0.4 Board states (`Board::mState`, +0xa0)

0 playing, 1 level done (waiting for the button), 2 level-complete fanfare, 3 Puzzle row sweep,
4 board falls away, 5 game over, 6 high scores / play again, 7 dollar bomb detonating, 8 frozen behind
the GameSelector. The transition table is Part 1 section 5.1, and the per-tick update order is Part 1
section 5.

### 0.5 Scoring

- **Clicks**:
  - A click on a group of at least the minimum size removes it.
  - It scores by group size times `mPointsMult`; all colours are worth the same.
  - The points fill the money meter, except in Puzzle.
- **Bonuses** have string ids in default.xml under "Board.cpp":
  - the combos: Double/Triple, Mega, Hyper, Excellent, Awesome, Big Money;
  - Color Combo, Speed Bonus and Column Clearing (each also in a Super version);
  - Excess Baggage, Speed Bagger and Tower Toppler.
- Their exact rules are Part 1 sections 6.8 and 6.9. The floating "+N" texts are Points widgets (Part 2
  section 4).
- **Ranks**: 27 rank names (`gRankNames`) with thresholds from 0 to 500,000 points; see Part 1
  section 10.
- **Stats** (panel at level end and game over): game time, levels completed, biggest coin combo,
  biggest bonus, coins cleared.

### 0.6 Random numbers

There are two generators, and the framework's global `Sexy::Rand()` is not used by game code:

| Generator | Owner | Seeded | Used for |
|---|---|---|---|
| Board MTRand (`Board::mRand`) | each Board | `rand() * 0xffff` in the Board ctor; restored from `savegame.dat` | everything that shapes play: board generation, rows, bag and bomb drops, shakes, flings |
| MSVC CRT `rand()` | process (main thread) | `srand(GetTickCount())` in SexyAppBase::Init | the Board seed, cosmetic effects, Puzzle bag columns, the flying-coin image choice, Mr. Money's idle animations, the GameSelector voice line |

- Part 1 section 7 lists every draw site in call order.
- The port reproduces MSVC's `rand()` with `CrtRand`/`CrtSRand` (`games/bigmoney/src/Compat12.h`), so
  it is identical on native and web. `-seed=N` seeds both it and the framework.

### 0.7 Files and settings

| File / store | Owner | Format |
|---|---|---|
| `savegame.dat` | Board (SaveGame 0x403300, LoadGame 0x4075d0) | Part 1 section 9: 0x14-byte header (version 0, game type, skill, state, 0x9c0), the board RNG's 624 MT words, then plain field writes; no checksum. Written only by the app's destructor for a game in progress; loaded (then deleted) from the "Continue?" dialog |
| `hiscores.dat` | HighScoreWidget | Part 5 section 5: three tables (Action, Strategy, Puzzle), each a day number plus today's record plus 5 all-time records; each record carries a name, a score and a checksum |
| Registry `PopCap\BigMoney` | WinBMApp + framework | Part 3 section 7: LastScoreName, SkillLevel, GameType, NoPopLink, DontShowHelp0..2, plus the framework's own values |
| `properties/partner.xml` | framework | Steam: NoReg, DontUpdate, one options link (not ported) |

In the port, files and the registry live in the user dir; the byte formats are unchanged.

### 0.8 Resources

There is no `resources.xml`; every name is hard-coded:
- 71 images, all loaded by Init before the title screen;
- 48 sounds (Ogg, plus three Sun `.au`), loaded by the loading thread;
- 14 songs, all ids on the one `music/QualityTime_BigMoney.it` module, each starting at an order
  position;
- 13 ImageFonts from 5 descriptors, using the noshadow, outline and underlined tag combinations;
- derived images built at load time: tinted button tiles, mirrored arrows, scaled bills, Mr. Money's
  mouth frames and the bright coins.

The full tables are Part 3 section 3 (the port's `games/bigmoney/src/Res.h`).

### 0.9 Reconciled decisions

The five analyses ran in parallel; where they disagreed, this is what holds:
- **Skill level** is 1..3: GameSelector stores button id - 10, the default is 1, and Board's label row
  `(type*3 + skill)*15 - 15` needs it. Part 3 adopted this.
- **App session flags +0x81c..+0x81f** are `mBagCollectedOnce`, `mBombSeenOnce`, `mDangerHintShown`
  (the coins-near-the-top hint, not a moves count) and `mHasClickedGroup`. +0x825 is `mShowInstructor`.
- **`DrawPiece`** takes three arguments; the third is a y offset (0, or `mMintPixels` for the row rising
  from below). The **MrMoneyAnim** enum is defined once, in `MrMoney.h`.
- **Shared functions** (identical-COMDAT folding) keep one name each:
  - 0x401c70 is `Board::ButtonPress`, also HighScoreWidget's;
  - 0x40ee80 is `GetPreferredHeight` returning 396, for CreditsDialog and HighScoreDialog;
  - 0x412c50 is `Resize`, for HighScoreWidget and TitleScreen.
- **Module ranges** corrected against the inventory's first estimate:
  - WinBMApp.cpp starts at 0x41b670; WinMain and 11 app helpers sit before the old boundary 0x41bd90;
  - HighScoreWidget.cpp starts at 0x412b80 (GetDayNumber, GetRank);
  - the functions at 0x419630, 0x40fb90, 0x410d80 and 0x40ee10 are destructors.
- **Framework sizes**: `operator new(0x4c)` for MemoryImage and `new(0xcc)` for DDImage are 4 bytes
  smaller than FRAMEWORK_12.md's computed layouts. One 1.2 Image field is absent in this build. Game code
  does not touch those fields.

### 0.10 Not ported

- The registration and trial paths. The Steam build is registered (NoReg), so the port always takes
  the registered branch. RegisterDialog is not ported.
- Update checks (UpdateCheckDialog, dialog 6 and 7).
- The internet high-score submission, which is dead code even in the original.
- InternetManager and BetaSupport.
- The Zylom and RealArcade branches.

Each part notes these where they occur.

---

## Part 1: Board logic

Scope: the rules half of `Board.cpp` (0x401000-0x40ee10) plus `Piece`. Drawing (`Board::Draw` 0x405480,
`DrawPiece` 0x402060, the stats/rank panel 0x404950, `ShowContinueButton` 0x4045e0 internals) belongs
to board_draw; this file names every Board field (board_draw uses these names) and describes the
calls into those functions. Header: `board_logic.h`. Symbols: `board_logic_symbols.jsonl`.

Corrections to the brief:
- `0x403300` **writes** `savegame.dat` (`fopen "wb"`) and `0x4075d0` **reads** it.
- The save body is **not encrypted**: after a 0x14-byte header (version, game type, skill, state,
  RNG length) comes the raw 2496-byte MT19937 state of the Board's MTRand, then plain `fwrite`s of
  the fields (section 9). Verified byte-exactly against a real `savegame.dat`.
- Game type values: 0 = Action, 1 = Strategy, 2 = Puzzle (`GameType` registry value, app+0x818; the
  GameSelector plays sound 0x2a+type = action/strategy/puzzle). Skill = 1..3 (`SkillLevel`,
  app+0x814, default 1).

---

### 0. Conventions

- Offsets are absolute from the object start, hex. `Board` derives Widget (0x80 bytes) +
  ButtonListener (vfptr at +0x80); own data from +0x84. **sizeof(Board) = 0x810**
  (`operator new(0x810)` in WinBMApp::NewGame 0x41bfb0). **sizeof(Piece) = 0xa8**.
- Ticks: `Board::Update` runs at 100 Hz. `mUpdateCnt` (+0x24, WidgetContainer) is incremented by
  `Widget::Update`, which Board calls only when not paused (and not frozen), so all `% N` cadences
  freeze while paused.
- `(int)` = x87 `_ftol` = truncation toward zero. Integer `/`, `%` are C signed ops. All `double`.
- `Next()` = the Board's own `MTRand` (`mRand`, +0x88) `Next()` (SAF 1.2 MT19937, 31-bit result,
  `& 0x7fffffff`). Every `Next() % n` is an **unsigned** 32-bit modulo. `rand()` = MSVC CRT
  `rand()` (LCG `h = h*214013 + 2531011; return (h>>16) & 0x7fff`, seeded by
  `srand(GetTickCount())` in SexyAppBase::Init). Board never calls the global `Sexy::Rand()`.
- `ColX(c) = 29*c + 177` (0x4015e0), `RowY(r) = 29*r + 3` (0x4015f0): pixel top-left of grid cell
  (col c, row r), widget coords. Coins are 29x29. Row 0 is the top, row 14 the bottom.
- `GetString(id)` = `SexyAppBase::GetString` (properties/default.xml). `PlaySample(n)` =
  app vtable slot 49 (`+0xc4`). Sound / image / song ids: section 8.
- "Repaint" = `MarkDirty()` (vtable slot 4, +0x10).

### 1. Modes, skill and level parameters

The Board is constructed with only the app pointer; it reads `app->mGameType` (+0x818) and
`app->mSkillLevel` (+0x814) directly whenever needed (set by GameSelector 0x410340 / registry).
The level index `mLevel` (+0x5b8) starts at 0 and increments per level.

`SetupLevelParams()` 0x401490, called by the ctor and `NextLevel`:
- `L = mLevel + (skill==1 ? 0 : skill==2 ? 4 : 7)` (any other skill value also adds 7).
- Puzzle: `mNumColors = 5`; `mBagsNeeded = clamp((int)(L*0.26 + 2.6), 1, 8)` (uses the puzzle pair
  +0x740/+0x738). Action/Strategy: `mNumColors = 4`; same formula with +0x730/+0x728 (also 0.26 /
  2.6). If `L < 1`: `mNumColors -= 1` (only level 0 at skill 1: 3 colours, Puzzle 4).
- `mMintSpeed = L*0.001 + 0.0025`, `mMintAccel = L*5e-8 + 1e-6`, `mMeterDrainRate = L*7e-5 + 0`,
  `mMeterCapacity = L*20 + 300`, `mPointsMult = L*0.1 + 0.5` (then x3.0 in Puzzle),
  `mMintPerClick = L*0.011 + 0.24501`. (Each is `L*k + c` in x87 double, `k`/`c` the tuning
  doubles at +0x6d8..+0x750, constants set by the ctor.)
- `mMinGroup` (+0x9c) = 3, or 2 in Puzzle (ctor).

| Mode | How rows appear | Win a level | Lose |
|---|---|---|---|
| Action (0) | time: `mMint` grows every 2nd tick; full row pushes in from the bottom | bank `mBagsNeeded` money bags (bags drop when the money meter fills; collect by removing the coin directly under a bag) | coins reach the top row: 3 s countdown ("three, two, one") then game over |
| Strategy (1) | per move: each valid click (with no bag above the group) adds `mMintPerClick` to `mMintTarget`; auto-rises when no moves are left | as Action | a row must be pushed while a coin is in the top row: immediate game over |
| Puzzle (2) | none (fixed generated board) | drop `mBagsNeeded` bags off the bottom row | no group >= 2 left and not all needed bags released |

Dollar bombs drop only in Action (`Next()%15000==0` per playing tick, level >= 1).

Coins have no individual value: a click scores by group size (OnClick, section 6.8) times
`mPointsMult`, and all five colours are equivalent. There are no gold bars in the Board logic (the
"goldbar" image, index 6, is not used by Board.cpp). The money meter fills from every scored point
(`AddPoints(.., toMeter)`), not per coin; in Puzzle it never fills (AddPoints skips it).

### 2. Class layouts

#### 2.1 Piece (sizeof 0xa8, vtable 0x4f5430: slot 0 = scalar deleting dtor)

A coin, money bag, dollar bomb, or a dying/flying/flung remnant. Grid pieces live in
`Board::mGrid`; pieces that left the grid live in `Board::mLoosePieces`. `Piece::Piece` 0x401000
zeroes everything except `mGroupSize = 1` and increments the live counter `gPieceCount`
(0x52d860); `~Piece` 0x4010b0 decrements it.

| Off | Type | Name | Meaning (init 0 unless stated) | Saved |
|---|---|---|---|---|
| 0x00 | vfptr | | 0x4f5430 | |
| 0x08 | double | mX | pixel x (top-left) | y |
| 0x10 | double | mY | pixel y | y |
| 0x18 | double | mVX | x velocity (column slide; flung coins; dropping specials 0.5) | y |
| 0x20 | double | mVY | y velocity (gravity 0.14/tick) | y |
| 0x28 | int | mShakeX | draw offset set while the Action overflow countdown shakes the board; never reset | y |
| 0x2c | int | mShakeY | " | y |
| 0x30 | int | mCol | grid column | y |
| 0x34 | int | mRow | grid row (next-row pieces: 15 from GenerateBoard, 14 from PushRow; level end: rows+2) | y |
| 0x38 | bool | mHighlighted | part of the hovered group (>= mMinGroup) | n |
| 0x3c | int | mGroupSize | size of its same-colour group (FindGroups); init 1 | n |
| 0x40 | bool | mDanger | column-top danger flash phase (SetDangerFlash) | n |
| 0x41 | bool | mBlinkHidden | blink phase while `mLifeTimer < 200` (DrawPiece skips the piece when set) | n |
| 0x48 | double | mSpin | spin phase of a flung coin | n |
| 0x50 | double | mSpinSpeed | spin speed (game-over fling) | n |
| 0x58 | int | mColor | coin colour 0..4 = red, blue, yellow, green, magenta (gCoinColors 0x52d8c8); also the bomb's colour | y |
| 0x5c | int | mMark | flood-fill scratch mark | n |
| 0x60 | bool | mIsSpecial | not a clickable coin (bag, bomb, poof, explosion) | y |
| 0x61 | bool | mFallThrough | special that leaves the board when it lands (bag collected / bomb detonated) | y |
| 0x64 | int | mSpecialType | PieceType: 0 money bag, 1 poof, 2 dollar bomb, 3 exploding coin (valid if mIsSpecial) | y |
| 0x68 | int | mImageSet | 1 = draw from "horzcoins" (flung coins, first 3/4 of a meter flight), else "coins" | y |
| 0x6c | int | mFrame | animation frame | y |
| 0x70 | bool | mFlying | flies from mStart to mTarget (combo coins to the meter, bags to the holder) | y |
| 0x78 | double | mFlyT | flight progress 0..1 | y |
| 0x80 | double | mFlySpeed | per-tick increment of mFlyT | y |
| 0x88 | int | mStartX | flight start (`(int)mX` at launch) | y |
| 0x8c | int | mStartY | | y |
| 0x90 | int | mTargetX | flight target | y |
| 0x94 | int | mTargetY | | y |
| 0x98 | bool | mDying | removed coin playing its vanish animation | y |
| 0x9c | int | mLifeTimer | if > 0: counts down in UpdatePhysics, deletes at 0, blinks below 200. Never set > 0 by the game (dead feature) | y |
| 0xa0 | int | mUnusedA0 | only ctor/save/load touch it | y |
| 0xa4 | - | (pad) | | |

#### 2.2 List payloads (std::list nodes: next +0, prev +4, payload +8; offsets below are payload-relative)

`Particle` (list `mParticles`, node 0x40):

| Off | Type | Name | Meaning |
|---|---|---|---|
| +0x00 | int | mFrameDelay | advance `mFrame` when `mUpdateCnt % mFrameDelay == 0` |
| +0x08 | double | mX | |
| +0x10 | double | mY | |
| +0x18 | double | mVX | |
| +0x20 | double | mVY | |
| +0x28 | double | mGravity | added to mVY every tick |
| +0x30 | int | mFrame | 0..13; the node is erased when it reaches 14 |
| +0x34 | bool | mBright | 1 for flight/burst sparkles, 0 for bag/drop trails (Draw decides; probably additive) |

`Bill` (list `mBills`, node 0x28): `double mX` +0, `double mY` +8, `double mVY` +0x10,
`int mFrame` +0x18 (0..15, +1 every 4 ticks), `int mType` +0x1c (0..4). Erased when `mY > mHeight`.

`TubeCoin` (list `mTubeCoins`, node 0x20): `double mY` +0 (32 at spawn, +15/tick), `double mDist`
+8 (distance to the stack top, -15/tick; lands when < 0), `int mFrame` +0x10
(`(int)(mDist*9.0*(1/450))`, recomputed every tick).

#### 2.3 Board (sizeof 0x810, vtables 0x4f54e4 (54 slots) and 0x4f54c4 at +0x80 (ButtonListener))

"Users" names functions by their short names (section 4). "Draw" = read by Board::Draw / DrawPiece /
the stats panel (board_draw). "Saved" = in savegame.dat.

| Off | Type | Name | Meaning | Init (ctor) | Saved | Users |
|---|---|---|---|---|---|---|
| 0x000 | Widget | (base) | Board covers the whole app (Resize(0,0,w,h)) | | | |
| 0x080 | vfptr | | ButtonListener (0x4f54c4) | | | |
| 0x084 | WinBMApp* | mApp | | arg | | all |
| 0x088 | MTRand* | mRand | board RNG | `new MTRand(rand()*0xffff)` | state | see section 7 |
| 0x08c | DWORD | mCreateTick | | GetTickCount | | never read |
| 0x090 | DWORD | mTimerTick | start of the current timed stretch | GetTickCount | | Pause, StartNextLevel, state 4, ShowContinueButton |
| 0x094 | int | mStatsSeconds | `mPlaySeconds + (now-mTimerTick)/1000` snapshot | 0 | | ShowContinueButton W, stats panel R ("%d min %02d sec") |
| 0x098 | int | mPlaySeconds | seconds banked at pause and at level end | 0 | | Pause, state 4 |
| 0x09c | int | mMinGroup | 3, Puzzle 2 | `(type!=2)+2` | y | FindGroups, hover, OnClick, tick 600 |
| 0x0a0 | int | mState | BoardState | 0 | header | everywhere |
| 0x0a4 | bool | mPlayStarted | a valid click was made (or a game was loaded) | 0 | | OnClick, LoadGame, CanSaveGame |
| 0x0a8 | Piece*[15][14] | mGrid | `mGrid[row][col]` | zeroed by GenerateBoard | y | |
| 0x3f0 | Piece*[14] | mNextRow | coins being minted below the bottom row | GenerateBoard | y | PushRow, Draw |
| 0x428 | int | mRiseOffset | pieces are drawn this many px low after a push; +29 per push, -1/tick | 0 | y | PushRow, Update, UpdatePhysics floor |
| 0x42c | bool | mHasMoves | some group >= mMinGroup exists | | y | FindGroups, Update |
| 0x430 | int | mCols | 14 | 14 | y | |
| 0x434 | int | mRows | 15 | 15 | y | |
| 0x438 | int | mMintPixels | `(int)((1-mMint)*29)` = hidden px of the next row (29..0); also +1 per 2 ticks in state 2 and per 3 ticks in states 5/6 | 0 (29 Generate) | y | Update, PushRow, Draw |
| 0x440 | double | mMint | next-row progress; > 1 pushes | 0 | y | Update, PushRow |
| 0x448 | double | mMintTarget | mMint eases up to it at 0.015/tick; 0.25 at level start; -0.75 per push; Strategy +mMintPerClick per click | 0 | y | |
| 0x450 | double | mMintSpeed | Action mint rate | level | y | |
| 0x458 | double | mMintAccel | added to mMintSpeed each mint step | level | y | |
| 0x460 | double | mMintPerClick | Strategy | level | y | |
| 0x468 | Color | mMovesColor | Strategy moves-left lamp colour, eased to red/yellow/green | (128,128,128,255) | y | Update, Draw |
| 0x478 | int | mLastPieceHitTick | throttles sound 3 (piecehit) | 0 | | UpdatePhysics |
| 0x47c | int | mLastBagTick | for Speed Bagger | 0 | | UpdatePhysics |
| 0x480 | int | mLastColor | colour-combo colour | 0 | | OnClick |
| 0x484 | int | mColorStreak | | 0 | | OnClick |
| 0x488 | int | mLastGroupSize | previous click's group size | 0 | | OnClick |
| 0x48c | int | mPrevGroupSize | the one before | 0 | | OnClick |
| 0x490 | int | mScore | | 0 | y | AddPoints, HighScoreWidget, HighScoreDialog, stats panel |
| 0x494 | int | mDisplayScore | rolls toward mScore | 0 | y | Update, Draw |
| 0x498 | double | mPointsMult | score multiplier | 1.0, then level | y | AddPoints, ClearBoardForLevelEnd, Update |
| 0x4a0 | list<Points*> | mPointsWidgets | live floating texts (head +0x4a4, size +0x4a8) | | | AddPoints, ShowBonusText, Update, Pause |
| 0x4ac | Points* | mLastBonusText | stacking of bonus texts | 0 | | ShowBonusText, Update |
| 0x4b0 | int | mMouseX | | 0 | | MouseMove, UpdateHover |
| 0x4b4 | int | mMouseY | | 0 | | |
| 0x4b8 | int | mLastWarningTick | sound 0x15 throttle | 0 | | Update |
| 0x4bc | int | mMeterFlash | set to 16 by AddPoints; never read | 0 | y | |
| 0x4c0 | double | mMeterJustFull | set 1.0 at a bag drop; never read | 0 | y | |
| 0x4c8 | double | mMeterFraction | sum of p/mMeterCapacity since the last bag (tone pitch) | 0 | y | AddPoints, OnClick, meter |
| 0x4d0 | double | mMeterTarget | meter fill in 1/64-meter units (+64p/cap) | 0 | y | AddPoints, Update |
| 0x4d8 | int | mMeterCoins | tube coins landed | 0 | y | meter, Draw |
| 0x4e0 | double | mMeterDrained | units consumed (drain, bag drops, level end) | 0 | y | meter, Update, Draw |
| 0x4e8 | int | mMeterPending | tube coins still to launch | 0 | y | AddPoints, meter |
| 0x4f0 | double | mMeterLaunchTimer | launch spacing | 0 | y | meter |
| 0x4f8 | double | mMeterEmptying | units still to drain after a bag drop (64) | 0 | y | meter |
| 0x500 | double | mMeterCapacity | points per full meter | level | y | AddPoints |
| 0x508 | double | mMeterDrainRate | Action: drain per 2 ticks (x64 units) | level | y | meter |
| 0x510 | list<Piece*> | mLoosePieces | pieces outside the grid (head +0x514, size +0x518) | | y | many, Draw |
| 0x51c | list<Particle> | mParticles | sparkles (head +0x520, size +0x524) | | | Update, Draw |
| 0x528 | list<Bill> | mBills | falling bills (head +0x52c, size +0x530) | | | Update, LevelComplete, Draw |
| 0x534 | list<TubeCoin> | mTubeCoins | coins falling into the meter (head +0x538, size +0x53c) | | y | meter, Draw |
| 0x540 | int | mBillsToSpawn | one bill per tick while > 0; never set > 0 (dead) | 0 | y | Update |
| 0x544 | bool | mDangerFlash | toggling danger flash phase | 0 | | Update |
| 0x548 | int | mMeterLevel | `(int)min(6*fill, 5)`, Action only | 0 | | meter |
| 0x54c | int | mBagsReleased | bags that left the board (holder slot index) | 0 | y | UpdatePhysics, NextLevel, Update |
| 0x550 | int | mBagsBanked | bags arrived in the holder | 0 | y | UpdateLoosePieces, Draw |
| 0x554 | int | mMeterFlashFrame | 0..3 | 0 | y | Update, Draw |
| 0x558 | int | mMeterFlashTimer | 160 at a bag drop | 0 | y | meter, Update |
| 0x55c | int | mBagsNeeded | | level | y | |
| 0x560 | bool | mLevelUpActive | "level up" sign shown | 0 | y | LevelComplete, StartNextLevel, Update, Draw |
| 0x568 | double | mLevelUpY | sign y | 0 | y | Update, Draw |
| 0x570 | double | mLevelUpAngle | `atan2(rightY-leftY, levelup.width)` | 0 | y | Update, Draw |
| 0x578 | double | mLevelUpUnused | 0.03, never read | 0.03 | y | |
| 0x580 | double | mLevelUpStretch | 189 -> 150 in state 4 (Draw row-scale divisor) | | y | Update, Draw |
| 0x588 | bool | mLevelUpBouncing | | | y | |
| 0x590 | double | mLevelUpLeftY | -150 at start | | y | |
| 0x598 | double | mLevelUpLeftVY | | | y | |
| 0x5a0 | double | mLevelUpRightY | -174 at start | | y | |
| 0x5a8 | double | mLevelUpRightVY | | | y | |
| 0x5b0 | int | mMintDelay | Action minting paused while > 0 | 0 (160 Generate, 200 Load) | y | |
| 0x5b4 | bool | mPiecesMoving | a grid piece is still falling (or, state 4, still on screen) | 0 | y | UpdatePhysics, Update |
| 0x5b8 | int | mLevel | 0-based level | 0 | y | |
| 0x5bc | int | mLevelEndDelay | state 2 duration (160) | 0 | y | |
| 0x5c0 | int | mPauseCount | nested pause count | 0 | y | Pause, input |
| 0x5c8 | double | mPauseFade | 0..1 (+-0.05/tick) | 0 | y | Update, Draw |
| 0x5d0 | int | mPauseAnim | counter while the pause overlay shows | 0 | y | Draw |
| 0x5d4 | bool | mPauseClickable | no dialog is open ("click here to continue" shown) | 0 | y | Draw |
| 0x5d8 | int | mNoMovesY | state 6: +5/tick up to 500 (Puzzle "nomoremoves" image at y = this+80) | 0 | | Draw |
| 0x5dc | int | mGoTimer | ticks to "Go!" (140 first level, 80 later levels) | 140 | y | Update |
| 0x5e0 | bool | mBannerVisible | blink phase of the "collect N bags" banner | 0 | y | Draw |
| 0x5e4 | int | mBannerTimer | 480 (level 0) / 384 | 0 | y | Update, ShowBonusText |
| 0x5e8 | int[14] | mColWarning | per-column danger arrow frame 0..3 or -1 | -1 (Generate) | | SetDangerFlash, Draw |
| 0x620 | int | mOverflowTicks | Action overflow countdown (300 = game over) | 0 | y | Update |
| 0x624 | int | mFlingDelay | game over: ticks until FlingAllCoins | 0 | y | |
| 0x628 | bool | mCountdownActive | | 0 | y | Update, GameOver, Draw |
| 0x630 | double | mCountdownValue | 0..2.99 | 0 | y | Draw |
| 0x638 | int | mGameOverRiseDelay | 100 | 0 | y | |
| 0x63c | DialogButton* | mContinueButton | "CLICK HERE TO PLAY NEXT LEVEL / AGAIN" | 0 | flag | |
| 0x640 | int | mButtonBlink | 0..299 | | | Update |
| 0x644 | int | mLevelCoinsCleared | | 0 | y | stats panel |
| 0x648 | int | mGameCoinsCleared | | 0 | y | |
| 0x64c | int | mLevelBiggestBonus | biggest single AddPoints | 0 | y | |
| 0x650 | int | mGameBiggestBonus | | 0 | y | |
| 0x654 | int | mLevelBiggestCombo | biggest group | 0 | y | |
| 0x658 | int | mGameBiggestCombo | | 0 | y | |
| 0x65c | - | (pad) | | | | |
| 0x660 | double[14] | mColTowerHeight | smoothed tower heights (Tower Toppler) | 0 | | |
| 0x6d0 | double | mSpeedBonus | +0.16 per click, -0.0025/tick | 0 | | |
| 0x6d8..0x750 | double[16] | tuning | 0x6d8 0.0025, 0x6e0 0.001, 0x6e8 1e-6, 0x6f0 5e-8, 0x6f8 300, 0x700 20, 0x708 0, 0x710 7e-5, 0x718 0.5, 0x720 0.1, 0x728 2.6, 0x730 0.26, 0x738 2.6, 0x740 0.26, 0x748 0.24501, 0x750 0.011 | ctor | | SetupLevelParams |
| 0x758 | 16 bytes | (unused) | | | | |
| 0x768 | int | mBombColor | colour being detonated | 0 | y | |
| 0x76c | int | mBombIdle | state 7 linger counter | 0 | y | |
| 0x770 | bool | mHoverBag | mouse over a money bag | 0 | | UpdateHover |
| 0x774 | int | mRepaintTimer | counts down, repaint at 0; only ever set to 0 (dead) | 0 | | |
| 0x778 | list<8-byte> | mUnusedList | never used (head +0x77c, size +0x780) | | | |
| 0x784 | int | mNumColors | | level | NOT saved | |
| 0x788 | int | mMovesLeft | Strategy clicks until the next push | | | UpdateMovesLeft, OnClick |
| 0x78c | bool | mMoveDigitActive | Strategy "3/2/1" digit | 0 | y | Draw |
| 0x790 | int | mMoveDigit | 3-movesLeft (0..2) | | y | Draw |
| 0x798 | double | mMoveDigitFade | 1.0 -> 0 (-0.01/tick) | | y | Draw |
| 0x7a0 | int | mClipExtra | 0, Draw only | 0 | | |
| 0x7a4 | int | (unused) | | 0 | | |
| 0x7a8 | SoundInstance* | mExplodeSound | `mSoundManager->GetSoundInstance(0x17)` | | | state 7, dtor (Release) |
| 0x7ac | bool | mGlintActive | random glint (Draw) | 0 | | Update |
| 0x7b0 | int | mGlintFrame | | | | |
| 0x7b4 | int | mGlintTick | | | | |
| 0x7b8 | int | mSweepTone | Puzzle sweep tone index | 0 (Generate) | | |
| 0x7bc | MemoryImage* | mStatsImage | Draw cache (396x263) | 0 | | deleted by ShowContinueButton/LoadGame/dtor |
| 0x7c0 | MemoryImage* | mStatsFadeImage | Draw cache | 0 | | |
| 0x7c4 | bool | mStatsDirty | rebuild mStatsFadeImage | 0 | | |
| 0x7c8 | int | mHighScoreDelay | 180 | | | state 6 |
| 0x7cc | int | mHighScoreSlide | 0 -> -146 (-2/tick) | 0 | | state 6, Draw |
| 0x7d0 | double | mStatsFade | 0..1 (+0.015/tick) | 0 | | |
| 0x7d8 | MemoryImage* | mLevelUpImage | Draw cache | 0 | | |
| 0x7dc | HighScoreWidget* | mHighScoreWidget | | 0 | | |
| 0x7e0 | double | mBagGlow | 0..1 phase, +0.02/tick while a bag is in the grid | 0 | | UpdatePhysics, DrawPiece |
| 0x7e8 | bool | mHintVisible | | 0 | | |
| 0x7e9 | bool | mHintAlt | set with hint 5 | | | Draw |
| 0x7ec | int | mHintTimer | 0 = never times out | | | |
| 0x7f0 | int | mHintId | HintId | | | |
| 0x7f4 | int | mHintX | | 0 | | |
| 0x7f8 | int | mHintY | | 0 | | |
| 0x7fc | Piece* | mHintPiece | piece followed by the hint | | | |
| 0x800 | bool | mHintFollowMeter | hint 1 tracks the meter top | | | |
| 0x804 | int | mBagFlashCount | 3 per banked bag | 0 | | Draw |
| 0x808 | double | mBagFlash | 0..1 phase | 0 | | Draw |

App fields used (WinBMApp, see section 10): +0x338 mMusicInterface, +0x47c mSoundManager,
+0x498 mHasFocus, +0x6bc mIsRegistered, +0x6c0 mTimesPlayed, +0x6d4 MrMoney*, +0x6e4 Image**
(image table), +0x6e8 mBrightCoinsImage, +0x6ec mFont1 / +0x700 mFont3 / +0x718 mFloatFont,
+0x814 mSkillLevel, +0x818 mGameType, +0x81c..+0x81f and +0x825 (mShowInstructor) instructor-hint flags.

#### 2.4 Enums (header has the full lists)

- `BoardState` (+0xa0): 0 PLAYING, 1 LEVEL_DONE (waiting on "click here to play next level"),
  2 LEVEL_COMPLETE (fanfare, 160 ticks), 3 PUZZLE_SWEEP (rows cleared one by one), 4 LEVEL_CLEAR_FALL
  (board falls away, sign settles), 5 GAME_OVER (fling + delay), 6 GAME_OVER_SCORES (high scores,
  "play again"), 7 BOMB (dollar bomb detonating one colour), 8 FROZEN (Update returns at once; set
  when the GameSelector is shown over a running board).
- `PieceType` (+0x64): 0 MONEYBAG, 1 POOF, 2 DOLLAR_BOMB, 3 EXPLODING.
- `HintId` (+0x7f0): 1 METER (at (610,0), follows the meter top), 2 DANGER (at (380,8)), 3 BOMB,
  5 BAG (on the coin under a landed bag), 6 CLICK_GROUP, 7 CLICK_GROUP_PUZZLE, 8 PUZZLE_BAG.

### 3. Construction, destruction, widget overrides

#### 3.1 `Board::Board(WinBMApp* theApp)` 0x40e760 (called only by WinBMApp::NewGame 0x41bfb0)

Order (it matters for the RNG):
1. Widget ctor; vtables; `mMovesColor = Color()`; allocate the six list heads (0x4a0, 0x510, 0x51c,
   0x528, 0x534, 0x778); `mHintX = mHintY = 0`; `mApp = theApp`.
2. Fill the global rank-name vector `gRankNames` (0x52d864, vector<string>) from
   `GetStringVector("gRankNames")` (27 names, default.xml).
3. `mCols = 14`, `mRows = 15`.
4. `seed = rand()` (CRT), `mRand = new MTRand(seed * 0xffff)` (32-bit wrap; seed 0 -> 4357; SAF 1.2
   Knuth-69069 initialiser, see FRAMEWORK MTRand), `mCreateTick = mTimerTick = GetTickCount()`.
5. Zero the fields listed in the layout (0x94..0x5e4 etc.), `mLevelUpUnused = 0.03`,
   `mPointsMult = 1.0`, `mBagsNeeded = 1` (0x55c), `mColTowerHeight[] = 0`, tuning doubles
   (table above), `mGoTimer = 140`, `mLastBonusText = 0`.
6. `mMinGroup = (gameType != Puzzle) + 2`; `mExplodeSound = mSoundManager->GetSoundInstance(0x17)`;
   `mGlintActive = false`.
7. `SetupLevelParams()` (level 0), `GenerateBoard()` (all initial MTRand draws).
8. Zero again: 0x7bc, 0x7c0, 0x7d0, 0x7d4, 0x7d8, meter fields 0x4d0..0x4fc, 0x540, 0x78c, 0x7c4,
   0x7cc, 0x7dc, 0x7e0, 0x5d0, `mHintVisible`; `mMovesColor = (128,128,128)`; 0x7a0, 0x7a4, 0x5d8,
   0x488, 0x48c, 0x47c, 0x480, 0x484, 0x804, 0x808.

WinBMApp then calls `Resize(0, 0, appWidth, appHeight)`, `AddWidget`, `BringToBack`, `SetFocus`.

#### 3.2 `~Board` 0x403f10 (scalar deleting dtor 0x4045a0)

`mExplodeSound->Release()` (slot 1); delete mStatsImage, mStatsFadeImage, mLevelUpImage; free the
list nodes of 0x778, 0x534, 0x528, 0x51c, 0x510, 0x4a0 (the Piece objects in mLoosePieces, mGrid and
mNextRow are **not** deleted - leak in the original); `Widget::~Widget`.

#### 3.3 Widget overrides

- slot 8 `RemovedFromManager(wm)` 0x403aa0: base; remove `mContinueButton` and
  `app->SafeDeleteWidget` it (null it); remove and `delete` `mHighScoreWidget` (null it); remove
  every Points widget and SafeDelete it; clear `mPointsWidgets`.
- slot 15 `OrderInManagerChanged()` 0x401460: `PutInfront(mHighScoreWidget, this)` and
  `PutInfront(mContinueButton, this)` when non-null.
- slot 31 `KeyChar(c)` 0x403050: `' '` -> `Pause(mPauseCount == 0)` (toggle).
- slot 35 `MouseLeave()` 0x401fa0: `Widget::MouseLeave()` (empty), then `UpdateHover()` (mouse
  position not changed, so the highlight stays).
- slot 36 `MouseMove(x, y)` 0x401fb0: base (empty); `mMouseX = x; mMouseY = y; UpdateHover()`.
- slot 38 `MouseDown(x, y, clickCount)` 0x40c5f0: `Widget::MouseDown(x,y,cc)`; if `mPauseCount > 0`
  -> `Pause(false)` and return; if `mState == 0` -> `ClickAt(x, y)`. Any mouse button clicks.
- ButtonListener slot 1 `ButtonPress(id)` 0x401c70: `PlaySample(10)` (buttonclick).
- ButtonListener slot 2 `ButtonDepress(id)` 0x40e580 (the only button is mContinueButton):
  - `mState == 6`: `app->NewGame()` (0x41bfb0; deletes this board).
  - Puzzle and `!app->mIsRegistered`: `app->ShowPuzzleNextLevelLocked()` (0x41d4c0, dialog
    THE_NEXT_LEVEL_IS_LOCKED...).
  - else `StartNextLevel()`.
- slot 9 `Update()`: section 5. slot 13 `Draw`: board_draw.

#### 3.4 Other entry points used by WinBMApp

- `CanSaveGame()` 0x4010d0: `mPlayStarted && mState != 6`. App dtor saves when true; 0x420560
  (quit flow) checks it too.
- `SaveGame()` 0x403300, `LoadGame()` 0x4075d0: section 9. `SaveGameExists()` 0x4010f0 (fopen "rb"),
  `DeleteSaveGame()` 0x401440 (DeleteFileA) are free functions.
- `Pause(bool)` 0x402930 is also called by WinBMApp ModalOpen/ModalClose (vf73/vf74: pause /
  unpause), GotFocus/LostFocus (vf75/vf76) and 0x41b6f0 / 0x41b7f0.
- `Freeze()` 0x403b80: `RemovePointsWidgets()` (0x403a20) then `mState = 8`. Called by 0x41b840
  when the GameSelector is shown over a running game (the board stays behind it, frozen).

### 4. Grid helpers (all integer unless noted)

- `ResetMarks()` 0x401600: every grid piece `mMark = 0`.
- `FloodFill(col, row, mark, sameColor, int* count, int sum[2])` 0x401720, recursive. Let p =
  mGrid[row][col] (must be non-null). If p is special: return (nothing counted). If `sum`:
  `sum[0] = (int)(sum[0] + p.mX)`, `sum[1] = (int)(sum[1] + p.mY)` (double add, truncate).
  `(*count)++`, `p.mMark = mark`. Then neighbours in the order (-1,0), (+1,0), (0,-1), (0,+1)
  (left, right, up, down): recurse if in bounds, non-null, not special, (same mColor or
  !sameColor) and `mMark != mark`. The start cell's own mark is not checked.
- `FindGroups()` 0x401840: `ResetMarks()`; `mHasMoves = false`; for rows 0..14, cols 0..13: for
  each piece with `mMark == 0`: `n=0; FloodFill(c, r, -1, true, &n)`; `m=0;
  FloodFill(c, r, n, true, &m)`; if `mMinGroup <= m`: `mHasMoves = true`. Then every piece:
  `mGroupSize = mMark`. The marks are **left** equal to the group sizes (specials 0).
  Quirk: OnClick/Compact/UpdateHover flood with `mark = mUpdateCnt`; a same-colour neighbour whose
  leftover mark equals mUpdateCnt (possible only while mUpdateCnt <= ~210, i.e. the first seconds of
  a board, and only if no MouseMove ran UpdateHover since) stops the flood. Keep this behaviour.
- `PieceAt(x, y)` 0x401960: first grid piece (rows 0..14, cols 0..13) with
  `mX <= x && mY <= y && x < mX+29.0 && y < mY+29.0` (doubles), else null. Specials included.
- `CountCoinColumns()` 0x401a90: columns containing at least one non-special piece.
- `GetTopCoinRow()` 0x401af0: smallest row index containing a non-special piece; mRows if none.
- `GetAverageCoinRow()` 0x401b40 (x87, returns long double): mean row index over non-special pieces
  (`(long double)sumRows / count`); mRows if none.
- `GetBagSlotPos(Point* out, n)` 0x401e30: `perRow = (mBagsNeeded > 6) + 3`; `row = n / perRow`;
  `col = n % perRow`; `inRow = min(mBagsNeeded - row*perRow, perRow)`;
  `x = col*37 - (inRow*37 - 4)/2 + 85`, `y = row*40 + 232` (holder slot of the n-th bag; also used by
  Draw).
- `UpdateMovesLeft()` 0x401bd0 (Strategy only): `x = mMintTarget; r = GetTopCoinRow();
  while (x >= 1.0) { x -= 0.75; r--; }`; `mMovesLeft = (int)ceil(((1.0 - x) + r*0.75) /
  mMintPerClick)` (x87; `ceil` = 0x4d8810).
- `SetDangerFlash(bool on)` 0x401660: per column c: `flag = on; top = mRows`; for rows r = 0..14:
  piece -> `top = min(top, r)`, `piece.mDanger = flag`; empty cell with `r >= 1` -> `flag = false`.
  Then `mColWarning[c] = top==0 ? (on ? 2 : 3) : top==1 ? (on ? 0 : 1) : -1`.
- `UpdateHover()` 0x401e90: `p = PieceAt(mMouseX, mMouseY)`; `hb = p && p.mIsSpecial &&
  p.mSpecialType == 0`; if `hb != mHoverBag` set it and repaint. `n = 0`; if p is a coin and
  `mState == 0`: `FloodFill(p.mCol, p.mRow, mUpdateCnt, true, &n, null)`. Then every grid piece:
  `h = (n >= mMinGroup && piece.mMark == mUpdateCnt)`; on change set `mHighlighted = h` and
  repaint; `piece.mMark = 0`.
- `ClickAt(x, y)` 0x40c5c0: `p = PieceAt(x, y)`; if p is a coin -> `OnClick(p)`.
- Hints: `HideHint()` 0x401a80: `mHintVisible = false`, repaint.
  `ShowHintAt(id, const Point* pt, duration)` 0x401a20: only if `app->mSkillLevel == 1`:
  `mHintVisible = 1, mHintAlt = 0, mHintX/Y = pt, mHintId = id, mHintTimer = duration,
  mHintPiece = null, mHintFollowMeter = 0`.
  `ShowHint(id, Piece* p, duration)` 0x401fe0: only if `app->mShowInstructor` (+0x825):
  `mHintVisible = 1, mHintAlt = 0, mHintX = (int)(p.mX+14.0), mHintY = (int)(p.mY+14.0),
  mHintPiece = p, mHintId = id, mHintTimer = duration, mHintFollowMeter = 0`.
  A duration of 0 never expires (the timer is pre-decremented and tested for 0).
- `AddPoints(int p, int x, int y, bool toMeter)` 0x407ea0:
  `pts = (int)(p * mPointsMult)`; `life = clamp((int)(p * (2.0/3.0)), 100, 600)` (constant
  0.6666666666666666 at 0x4f5748); text = sprintf("+%d", pts); new `Points(app, app->mFloatFont
  (+0x718), text, mX + x, mY + y, life, 0 /*centred*/)` (0xac bytes), pushed on mPointsWidgets and
  `mWidgetManager->AddWidget`. `mLevelBiggestBonus = max(.., pts)`, `mGameBiggestBonus = max(.., pts)`,
  `mScore += pts`. If not Puzzle and toMeter: `old = (int)mMeterTarget; d = p / mMeterCapacity;
  mMeterTarget += 64.0*d; mMeterFraction += d; mMeterPending += (int)mMeterTarget - old;
  mMeterFlash = 16;` repaint. (`p` is the raw value, before mPointsMult.)
- `ShowBonusText(const string& s)` 0x408110: `dy = mBannerTimer > 0 ? 148 : 180`; if mLastBonusText
  is alive (`+0x84 life > 0`) and `last.mY + 36 > dy`: `dy = last.mY + 36`. New `Points(app,
  app->mFont1 (+0x6ec), s, 212, mY + dy, 200, -1 /*left*/)`, pushed and added;
  `mLastBonusText = it`.
- `RemovePointsWidgets()` 0x403a20: RemoveWidget + SafeDeleteWidget each, clear the list.

### 5. `Board::Update()` (slot 9, 0x40c9f0): per-tick order

1. `mState == 8` -> return.
2. Pause overlay: if `mPauseCount > 0`: `mPauseClickable = (app->GetDialogCount() == 0)`; if
   `mPauseFade < 1.0`: `+= 0.05`, clamp 1.0, repaint. Else if `mPauseFade > 0`: `-= 0.05`, clamp 0,
   repaint.
3. If `mPauseFade > 0`: `mPauseAnim++`; repaint when `mPauseAnim % 8 == 0`.
4. If paused: `MrMoney.mTargetX = 0, mTargetY = 290` (+0x98/+0x9c); return.
5. `Widget::Update()` (`mUpdateCnt++`).
6. `mUpdateCnt == 100 && Puzzle && !app->mIsRegistered`: `app->ShowPuzzleTrialDialog()` (0x41d2e0,
   dialog 15 PUZZLE_MODE_LOCKED / YOU_CAN_ONLY_PLAY_ONE_LEVEL...).
7. `mUpdateCnt == 300`: MrMoney target (0, 290).
8. `mUpdateCnt == 600 && !app->mHasClickedGroup` (+0x81f): two scans over rows 10..14, cols 3..10:
   first a piece with `mGroupSize == mMinGroup`, then `mGroupSize >= mMinGroup`; each found piece
   (while `!mHintVisible`) -> `ShowHint(Puzzle ? 7 : 6, piece, 0)`. (Only the first one sticks.)
9. `mBillsToSpawn > 0` (dead): spawn a Bill: `x = rand()%500 + 80`, `y = -50.0`,
   `vy = (rand()%1000)*0.001*1.2 + 1.6`, `frame = rand()%16`, `type = rand()%5` (draw order x, vy,
   frame, type); decrement.
10. `mFlingDelay > 0`: decrement; at 0 -> `FlingAllCoins()`.
11. Glint: if `!mGlintActive`: `if (rand()%2000 == 0) { mGlintActive = 1; mGlintTick = 0;
    mGlintFrame = 0; repaint }`. Else: every `mUpdateCnt % 10 == 0`: `mGlintFrame++`, `> 14` ->
    `mGlintActive = 0`, repaint; every `% 4 == 0`: `mGlintTick++`, repaint.
12. `mRepaintTimer > 0`: decrement, repaint at 0 (dead).
13. `mSpeedBonus > 0`: `-= 0.0025`, clamp 0.
14. State 7 (bomb), every `mUpdateCnt % 8 == 0`: scan rows 0..14, cols 0..13 for the first coin
    with `mColor == mBombColor`: `mExplodeSound->Play(false, false)`; if it is the hint piece ->
    HideHint; `mIsSpecial = 1, mSpecialType = 3, grid cell = null`, push on mLoosePieces; stop the
    scan. Then `Compact()`. Found -> `mBombIdle = 8`; else `if (--mBombIdle < 1) mState = 0`.
15. `mState == 0` -> `TowerToppler()` (return value ignored here).
16. `mUpdateCnt == 1`: MrMoney target (350, 24); `PlaySample(8)` (getready); `MrMoney.PlayAnim(13)`.
17. `mGoTimer > 0`: decrement; at 0: `PlaySample(9)` (go); `PlayAnim(14)`;
    `mBannerTimer = mLevel != 0 ? 384 : 480`.
18. `mBannerTimer > 0`: decrement; `mBannerVisible = ((mBannerTimer/12) % 4 != 0)`; repaint on change.
19. `mMeterFlashTimer > 0`: decrement; `mMeterFlashFrame = 3 - abs(3 - ((t/2) % 6))` (t = new
    value); repaint on change.
20. `UpdateLoosePieces()` 0x409520 (5.2).
21. `UpdateMeter()` 0x40c640 (5.3).
22. `UpdatePhysics()` 0x408a60 (5.4).
23. State block:
    - **0 (playing)**:
      a. Action: `if (Next() % 15000 == 0 && mLevel > 0) DropSpecial(2)` (the draw happens every
         playing tick in Action).
      b. `top = GetTopCoinRow()`.
      c. `top < 4 && !app->mDangerHintShown (+0x81e) && !Puzzle`: set it; `ShowHintAt(2, {380, 8},
         1000)`.
      d. Danger: Action: if `top < 2`: every `mUpdateCnt % 12 == 0`: `mDangerFlash = !mDangerFlash;
         SetDangerFlash(mDangerFlash)`, repaint; `iv = (int)(200.0 - ((2 - top) - mMintPixels *
         (1/29.0)) * 0.5 * 160.0)`; if `mUpdateCnt - mLastWarningTick >= iv`: `PlaySample(0x15)`
         (warning), `mLastWarningTick = mUpdateCnt`. Strategy/Puzzle: if `top == 0`: same flash
         every 24 ticks, warning every 150 ticks. Otherwise (no danger) if mDangerFlash:
         `SetDangerFlash(false); mDangerFlash = false`.
      e. `avg = GetAverageCoinRow()`; `f = clamp((0.6 - top/mRows) * 2, 0, 1)` (x87:
         `(0.6 - top/(double)rows)`, doubled by `fadd st,st`); `s = (top*f + (1-f)*avg)/mRows - 0.5 +
         1.0`.
      f. Action: `if (mMintDelay > 0) mMintDelay--; if (mMintDelay == 0 && mUpdateCnt % 2 == 0)
         { mMint += s * mMintSpeed; mMintSpeed += mMintAccel; }`.
      g. `if (mMint < mMintTarget) { mMint += 0.015; if (mMint > mMintTarget) mMint = mMintTarget; }`.
      h. Strategy: `if (!mHasMoves || mMint >= 1.0) mMint += 0.015`.
      i. `m = (int)((1.0 - mMint) * 29.0)` (truncation: mMint up to 1.0344 still gives 0).
         - `m == mMintPixels`: `mOverflowTicks = 0; mCountdownActive = false`.
         - `m >= 0`: `mMintPixels = m`, repaint.
         - `m < 0` (a row must enter): if `top != 0` -> `PushRow()`, repaint. Else if not Action ->
           `GameOver()`, repaint. Else (Action overflow):
           `mOverflowTicks++`; `q = min(mOverflowTicks*0.01, 2.99)`; if `!mCountdownActive ||
           (q != mCountdownValue && (int)q != (int)mCountdownValue)`:
           `MrMoney.PlayAnim((int)(q + 4.0))`, `PlaySample((int)(q + 32.0))` (anims 4/5/6 and sounds
           three/two/one); then (whenever q changed or it was inactive) `mCountdownValue = q;
           mCountdownActive = true`, repaint.
           Every `mOverflowTicks % 4 == 0` shake: `n = mRows*mOverflowTicks/300`;
           `w[c] = 1.0` for columns whose row-0 cell is a coin (other columns: **uninitialised
           stack**, see open questions); then for columns with `w[c] == 0.0` and a neighbour
           `w[c-1] == 1.0` or `w[c+1] == 1.0`: `w[c] = 0.6` (in place, c ascending). For c =
           0..13, rows r = 0..n-1 with a piece: `amp = (mOverflowTicks*(1/300.0)*0.5 + 0.6)*w[c]`;
           `mShakeX = (int)(((Next()%100)*0.01 - 0.5)*amp*5.0)`; `mShakeY = (int)(((Next()%100)*0.01 -
           0.5)*amp*2.0)`; repaint. If `mOverflowTicks == 300` -> `GameOver()`; repaint.
    - **2 (level complete)**: `if (mUpdateCnt % 2 == 0) mMintPixels++`; repaint; `if (mLevelEndDelay
      > 0) mLevelEndDelay--; else ClearBoardForLevelEnd()`.
    - **4 (board falling away)**: if `!mPiecesMoving && !mLevelUpBouncing`: `if (mLevelUpY > 16.0)
      mLevelUpY -= 1.0` (repaint); `if (mLevelUpStretch > 150.0) mLevelUpStretch -= 0.75`
      (repaint); if `mLevelUpY == 16.0` exactly (bit test hi==0x40300000, lo==0): `mState = 1;
      ShowContinueButton(); mPlaySeconds += (GetTickCount() - mTimerTick)/1000`.
    - **3 (Puzzle sweep)**, every `mUpdateCnt % 16 == 0`: rows r = 0..14 until one row had pieces:
      for c = 0..13: piece p: grid cell = null; 3 particles k = -1, 0, 1 {delay 2, (p.mX+14,
      p.mY+14), vx 1.5k, vy -3.0, g 0.2, frame 0, bright 1}; coin -> `mDying = 1, mFrame = 0`, push on
      mLoosePieces, `pts += 3`; special -> `delete`. If `pts > 0`: `AddPoints(pts, 308, RowY(r)+14,
      false)`; `PlaySample(11 + min(mSweepTone, 8))` (tone1..tone9); `mSweepTone++`. If no row had
      pieces: `LevelComplete()`.
    - **1 (level done)**: if `mStatsFade < 1.0`: `mStatsDirty = 1; mStatsFade += 0.015` (clamp 1),
      repaint.
    - **6 (game over scores)**: `if (mNoMovesY < 500) mNoMovesY += 5` (repaint); `if
      (mHighScoreDelay > 0) mHighScoreDelay--; else { if (mHighScoreSlide > -146) mHighScoreSlide -=
      2; mHighScoreWidget->Resize(mX+182, mHighScoreSlide*3 + 664 + mY, 396, 236);
      mContinueButton->Resize(212, hsw.mY + 200, 336, 29); repaint }`.
24. Tail (all states):
    - `(mState == 4 || mState == 1) && mMeterDrained < mMeterTarget`: `mMeterDrained += 0.5`, repaint.
    - Level-up sign, if `mLevelUpActive && mLevelUpBouncing`: `lvy += 0.15; ly += lvy; if (ly > 80)
      { ly = 80; lvy *= -0.6; if (lvy > -0.5) lvy = 0 }`; same for the right end; `mLevelUpY =
      (ry + ly)*0.5`; `mLevelUpAngle = atan2(ry - ly, (double)images[43 levelup].mWidth)` (x87
      `fpatan`); if both velocities are 0: `mLevelUpBouncing = false; mLevelUpAngle = 0`; repaint.
    - Strategy: `v = (int)((1.0 - mMintTarget) / mMintPerClick)`; target colour = v == 0 ? (255,0,0)
      : v == 1 ? (255,255,0) : (0,255,0); for each channel i = 0..3 of mMovesColor: if cur > tgt:
      `cur += (tgt-cur)/8 - 1` (C division), clamp up to tgt, repaint; elif cur < tgt: `cur +=
      (tgt-cur)/8 + 1`, clamp down to tgt, repaint.
    - States 5/6: `mStatsFade` fade-in as state 1 (also `mStatsDirty = 1`); `if (mGameOverRiseDelay
      > 0) mGameOverRiseDelay--`; `if (mUpdateCnt % 3 == 0 && mGameOverRiseDelay == 0)
      mMintPixels++`.
    - `mRiseOffset > 0`: `UpdateHover(); mRiseOffset--`; repaint.
    - Points widgets: each: MarkDirty; `+0xa8 = (+0xa8 + 7) % 255` (colour phase); every 2nd tick
      if `w.mY > 0`: `w.Move(w.mX, w.mY - 1)`; `if (--w.life(+0x84) < 1)`: clear mLastBonusText if
      it, erase, RemoveWidget, SafeDeleteWidget.
    - Score roll: if `mDisplayScore != mScore`: repaint; if lower: `mDisplayScore += (int)(((mScore -
      mDisplayScore)/mPointsMult*0.02 + 2.0) * mPointsMult)`; clamp to mScore.
    - Particles: if `mUpdateCnt % mFrameDelay == 0`: MarkDirtyFull (slot 6); `if (++mFrame >= 14)`
      erase (no move this tick). Otherwise (and on frame ticks that did not erase): `mVY += mGravity;
      mX += mVX; mY += mVY`; repaint.
    - Bills: every 4 ticks `mFrame = (mFrame+1) % 16`; `mY += mVY`; erase if `mY > mHeight`; repaint.
    - Hint: if visible: `if (--mHintTimer == 0) HideHint()`; if mHintPiece: `mHintX =
      (int)(p.mX+14.0), mHintY = (int)(p.mY+14.0)`; if mHintFollowMeter: `t = (int)(440.0 -
      (mMeterCoins - mMeterDrained)*6.0)`; `if (mHintY > 160) mHintY = mHintY - (mHintY - t)/3 - 1`;
      `if (mHintY < t) mHintY = t`.
    - Continue button: `mButtonBlink = (mButtonBlink + 1) % 300`; colour = `mButtonBlink <= 275 ?
      (0,255,0) : (255,255,255)`; SetColor(0, c) if different from GetColor(0).
    - `mMoveDigitActive`: `mMoveDigitFade -= 0.01`; `< 0` -> inactive; repaint.
    - Puzzle game over: `Puzzle && !mHasMoves && !mPiecesMoving && mBagsReleased < mBagsNeeded &&
      mState == 0` -> `GameOver()`.
    - Bag flash: `if (mBagFlashCount > 0 || mBagFlash != 0) { mBagFlash += 0.02; if (> 1.0) {
      mBagFlash = 0; mBagFlashCount--; } }`.

#### 5.1 State transitions summary

| From | Event | To | Function |
|---|---|---|---|
| (ctor/NextLevel) | | 0 | GenerateBoard |
| 0 | mBagsBanked reaches mBagsNeeded (Action/Strategy) | 2 | LevelComplete |
| 0 | same, Puzzle | 3 | (UpdateLoosePieces) |
| 3 | no rows left | 2 | LevelComplete |
| 2 | mLevelEndDelay (160) elapsed | 4 | ClearBoardForLevelEnd |
| 4 | board gone, sign at y 16 | 1 | (Update) + ShowContinueButton |
| 1 | continue button released | 0 | StartNextLevel -> NextLevel |
| 0 | dollar bomb falls through | 7 | UpdatePhysics |
| 7 | 8 idle checks without that colour | 0 | Update |
| 0 | overflow / no moves | 5 | GameOver |
| 5 | fling done, loose list empty | 6 | UpdateLoosePieces + ShowContinueButton |
| 6 | button released | (new Board) | app->NewGame |
| any | GameSelector shown | 8 | Freeze |

#### 5.2 `UpdateLoosePieces()` 0x409520

If `mLoosePieces` is empty: if `mFlingDelay == 0 && mState == 5`: `PlaySample(0x18)` (gameover
voice); `MrMoney.PlayAnim(12)`; `mState = 6`; `mHighScoreDelay = 180`; `mHighScoreWidget = new
HighScoreWidget(app)` (0x304 bytes, 0x414640), `AddWidget`; `ShowContinueButton()` (PLAY AGAIN);
`mContinueButton->Resize(0,0,0,0)` (placed later by state 6). Return.

Otherwise `coinFrames = images[23 coins].mWidth / 29`, `horzFrames = images[24 horzcoins].mWidth /
29`, and for each node (list order = insertion order) with piece p, flags `erase = delete = false`:

1. **Flying** (`p.mFlying`): `p.mFlyT += p.mFlySpeed`; if `mFlyT >= 1.0`: if p is a bag (special):
   `if (mBagsBanked < mBagsNeeded) { mBagsBanked++; mBagFlashCount = 3; }`; if `mBagsBanked ==
   mBagsNeeded && mState == 0`: `HideHint()`; Puzzle -> `mState = 3`, else `LevelComplete()`.
   erase = delete = true (coins too).
   Coin (not special): `t4 = mFlyT*4.0`; if `t4 >= 3.0`: `mImageSet = 0`;
   `mFrame = (int)((t4 - (int)t4) * horzFrames)`.
   Position (both): `mX = (double)(int)((1-t)*mStartX + mTargetX*t)`, `mY = (double)(int)((1-t)*mStartY
   + mTargetY*t)` (t = mFlyT).
   Sparkle: bags always, coins only if `rand()%15 == 0`: Particle {delay 3, x = rand()%8 + mX + 14 -
   4, y = rand()%8 + mY + 14 - 4, vx = (rand()%1000 - 500)*0.002, vy -2.0, g 0.1, frame 0, bright 1}
   (draw order: [%15], x, y, vx).
2. **Not flying**: if `mState == 7` -> skip to 3 (frozen).
   - Special with `mSpecialType == 1` (poof): -> 3.
   - Other special (dropping-in bag/bomb, or exploding coin): `mVY += 0.14; nx = mX + mVX; mX = nx;
     mY += mVY`. Landing floor: `floorRow = mRows, floorY = RowY(mRows)`; scan column mCol from row 0
     down for the first coin (non-special) q: `floorRow = q.mRow, floorY = (int)q.mY`. Sway:
     `tx = ColX(mCol)`; `if (nx < tx) mX = nx + 1.0; else if (nx > tx) mX = nx - 1.0`.
     If `mState == 0` and `mY >= floorY - 29`: erase = true; if `floorRow <= 0` delete = true (no
     room) else: `PlaySample(6)` (baghit); `mRow = floorRow - 1; mVY = 0; mY = RowY(mRow);
     mGrid[mRow][mCol] = p`; if `!app->mBagCollectedOnce (+0x81c) && type == 0 && mRow < mRows-1`:
     `ShowHint(5, mGrid[mRow+1][mCol], 0); mHintAlt = 1`; if `!app->mBombSeenOnce (+0x81d) && type ==
     2`: `ShowHint(3, p, 1000)`. If `mState != 0` and `mY > mHeight`: erase = delete = true.
     Trail: `if (rand()%2 == 0)` Particle {delay 2, x = rand()%28 + mX, y = mY + 20, v 0, g 0.035,
     frame 0, bright 0} (draw order: %2, %28).
   - Coin (removed/dying or flung): if `!mDying`: `mVY += 0.14; mY += mVY; mX += mVX`. If `mX < 84.0`
     or `mX > mWidth`: erase = delete = true. If `mSpinSpeed > 0`: `mSpin += mSpinSpeed; mFrame =
     (int)((mSpin - (int)mSpin) * horzFrames)`. If `mY >= RowY(mRows-1)`: bounce: `mVY *= -0.8; if
     (mVY > -4.0) mVY = -4.0 - (Next()%100)*0.01`; `mY = RowY(mRows-1)`; `if (mVX < 0) mVX -= 0.5;
     if (mVX > 0) mVX += 0.5`.
3. Common: if `mDying && !mFlying`: `if (++mFrame >= coinFrames)` erase = delete = true.
   `type 1`: every 2nd tick `if (++mFrame >= 14)` erase = delete. `type 3`: every `% 3 == 0` tick
   `if (++mFrame >= 14)` erase = delete. `type 2`: every `% 3` tick `mFrame = (mFrame+1) % 32`.
   (Type checks apply to coins too, whose type is 0.) Delete, then erase the node.

Repaint at the end.

#### 5.3 `UpdateMeter()` 0x40c640 (money meter)

Units: the meter is 64 units; `fill = (mMeterCoins - mMeterDrained) / 64`.
1. Launch: if `mMeterLaunchTimer <= 0 && mMeterPending > 0`: `top = (int)(440.0 - (((uint)(count +
   mMeterCoins)) - mMeterDrained + 3.0) * 6.0)` with count = `mTubeCoins.size()`; push TubeCoin
   {mY = 32.0 (= top - (top-32)), mDist = top - 32, mFrame = 0}; `mMeterPending--;
   mMeterLaunchTimer += 1.0`.
2. If `mMeterLaunchTimer > 0`: `mMeterLaunchTimer -= mMeterPending * 0.125`.
3. Each TubeCoin: `mDist -= 15; mY += 15; mFrame = (int)(mDist*9.0*(1/450.0))`; if `mDist < 0`:
   erase, `mMeterCoins++`, `PlaySample(0x24)` (cointube); repaint.
4. `fill > 1.0`: if `mMeterFlashTimer != 0` return (the rest is skipped). If `mMeterEmptying ==
   0.0`: `mMeterEmptying += 64; mMeterPending = 0; mMeterFlashFrame = 0; mMeterFlashTimer = 160;
   mMeterJustFull = 1.0; mMeterFraction = 0`; if `mState == 0`: `PlaySample(5)` (bonusdrop),
   `DropSpecial(0)` (money bag).
5. If `mMeterFlashTimer == 0`: if `mMeterEmptying > 0`: move 0.5 (or the remainder if < 0.5) from
   mMeterEmptying to mMeterDrained. If `fill > 0 && mState == 0 && Action`: every 2nd tick
   `mMeterDrained += mMeterDrainRate*64`; clamp `mMeterDrained >= 0`; `mMeterLevel =
   (int)min(fill*6.0, 5.0)`; repaint.

#### 5.4 `UpdatePhysics()` 0x408a60

`mPiecesMoving = false; removed = false; bagSeen = false`.
1. For rows 14..0, cols 0..13, grid piece p:
   - `mLifeTimer > 0` (never in practice): decrement; 0 -> delete, cell = null, removed = true,
     next; `< 200` -> `mBlinkHidden = ((t/4) % 2 == 0)` (repaint on change).
   - Special type 0 (bag): bagSeen = true; `if (mFrame < 3) { mFrame++; repaint }`; if not Puzzle
     and `rand()%10 == 0`: Particle {delay 5, x = rand()%28 + mX, y = mY + 20, v 0, g 0.035,
     frame 0, bright 0}.
   - Special type 2 (bomb): every `% 3` tick `mFrame = (mFrame+1) % 32`, repaint.
   If bagSeen: `mBagGlow += 0.02; if (> 1.0) -= 1.0`; repaint.
2. `bottom = (double)(RowY(mRows-1) + mRiseOffset)`. If `mState != 7`:
   a. Gravity, rows r = 14..0, cols 0..13, grid piece p: `old = mY; mVY += 0.14; mY += mVY`.
      - State 4: `if (mY < mHeight) mPiecesMoving = true; else { delete p; cell = null }`.
      - Else: `floor = bottom`; if `r < 14` and `q = mGrid[r+1][c]`: `floor = q.mY - 29.0`.
        `mY <= floor` -> mPiecesMoving = true (still falling). Otherwise (reached the floor):
        - `mFallThrough` (special leaves the board): if it is the hint piece -> HideHint.
          Bag: `PlaySample(7)` (gotbag); cell = null; `pts = 100`; if `mUpdateCnt - mLastBagTick <
          500`: `ShowBonusText(SPEED_BAGGER_BONUS)`, `PlayAnim(10)`, `PlaySample(0x27)`, `pts = 150`.
          `AddPoints(pts, (int)(mX+14), (int)(mY+14), false)`. `GetBagSlotPos(&pt,
          mBagsReleased)`; `app->mBagCollectedOnce = 1`; `mBagsReleased++`; `mFlying = 1; mTarget =
          pt; mStartX = (int)mX; mStartY = (int)mY; mFlySpeed = min(15.0/sqrt(dx*dx+dy*dy), 0.4)`
          (int dx, dy, x87 fsqrt); `mFlyT = 0`; push on mLoosePieces; `mLastBagTick = mUpdateCnt`;
          removed = true.
          Bomb: `app->mBombSeenOnce = 1; mSpecialType = 3; mFrame = 0`; cell = null; push on
          mLoosePieces; if the hint is visible with id 3 -> HideHint; `if (mState == 0) mState =
          7`; `mBombColor = mColor`; removed = true.
          (Other types: removed = true only.)
        - Normal landing: `if (mVY > 4.0 && mUpdateCnt - mLastPieceHitTick >= 8) { PlaySample(3)
          (piecehit); mLastPieceHitTick = mUpdateCnt; }`. No q (floor) -> `mY = floor; mVY *=
          -0.15`. q falling (`q.mVY > 0`) -> `mY = floor; mVY *= 0.85`. q not falling: if `old <=
          floor` -> `mY = floor; mVY *= -0.15; q.mVY *= -0.15`; else (already overlapping) `d = old
          - floor`: `d >= 0.05` -> `mY = old - d*0.6; mVY = 0`, else `mY = floor; mVY = 0`.
   b. Column slide, cols c = 0..13: `tx = (double)ColX(c)`, leader = true; rows r = 14..0, grid piece
      p:
      - leader: `d = clamp(tx - mX, -16, 16)` (x87: `d > 0 ? min(d,16) : max(d,-16)`); `mVX += d *
        0.004`; snap if `(tx <= mX && mX + mVX <= tx) || (tx >= mX && mX + mVX >= tx)` -> `mX = tx; mVX
        = 0`; otherwise leader = false.
      - follower: `q = mGrid[r+1][c]`; if q: `t = q.mX*0.5 + q.mX*0.5` (sic: the same piece twice when
        `r < mRows-2`); if `r > 0 && a = mGrid[r-1][c]`: `t = t*0.67 + a.mX*0.33`; `mVX = mVX*0.7 + (t
        - mX)*0.45499999999999996`; if `abs((int)mVX) > abs((int)(t - mX))`: `mVX = t - mX`; if
        `abs((int)(mX - t)) < 0.005 && abs((int)(tx - t)) < 0.005 && mVX < 0.005` (signed mVX):
        `mX = tx; mVX = 0`.
      - then `mX += mVX`.
3. Repaint; if removed -> `Compact()`.

### 6. Board mechanics

#### 6.1 `Compact()` 0x402b70 (gravity + closing empty columns)

1. `map[c] = c`.
2. Per column c: dest = 14; scan src = 14..0; `gap` = the cell scanned just before was empty. Each
   piece at src != dest moves to dest (`mRow = dest`); Puzzle: `mFallThrough = (mIsSpecial && dest ==
   14)`; else `mFallThrough |= (mIsSpecial && gap)` (only the coin directly under the special was
   removed counts); if `mFallThrough && type == 0`: `mRepaintTimer = 0`. Then dest--, gap = false.
   Rows 0..dest are cleared.
3. For each column c whose bottom cell (row 14) is empty (left to right; the grid changes as it goes):
   `l` = nearest column <= c with a bottom piece; `ls = FloodFill(l, 14, mUpdateCnt, false)` size
   (any colour); `r` = nearest column >= c with a bottom piece, `rs` likewise. If both exist: `g = r -
   l - 1; mr = ml = g/2`; odd g: `ls < rs ? mr++ : ml++`. `ResetMarks()`. Shift the whole left part
   right `mr` times: step s shifts columns `c+s-1 .. 0` one to the right (grid cells, `mCol++`,
   `map`, `mColTowerHeight`; vacated column 0 cleared). Then shift the whole right part left `ml`
   times: columns `k = c+mr .. 12` take `k+1` (`mCol--`, map, tower heights).
4. If anything shifted: `inv[map[i]] = i` (others -1); every mLoosePieces piece with `inv[mCol] !=
   -1` gets `mCol = inv[mCol]`.
5. `FindGroups()`. (Pixel positions are not changed: UpdatePhysics animates them.)

#### 6.2 `PushRow()` 0x4088a0 (a minted row enters)

`PlaySample(4)` (moveup); `mMintPixels = 0`. For each column c = 0..13: if `p = mGrid[0][c]` is
special: `mIsSpecial = 1, mSpecialType = 1` (poof), push on mLoosePieces. Shift the column up
(`mGrid[r][c] = mGrid[r+1][c]`, `mRow--`, r = 0..12); `mGrid[14][c] = mNextRow[c]` (`mRow = 14`);
new next coin: `mCol = c, mRow = 14, mX = ColX(c), mY = RowY(mRows) + 6 (= 444), mColor = Next() %
mNumColors`. Then `mMintPixels = 29; mMintTarget -= 0.75; mRiseOffset += 29; mMint = 0;
FindGroups()`.

#### 6.3 `DropSpecial(int type)` 0x40aea0 (money bag 0 / dollar bomb 2)

Candidate columns: no piece in row 0, no special anywhere in the column, and no special in
mLoosePieces with that mCol. For a bag (type 0) when `!app->mBagCollectedOnce` and not Puzzle,
remove column 13 from the candidates. If any: `c = cand[Next() % n]`; new Piece: `mY = -29.0, mVX =
0.5, mCol = c, mX = ColX(c), mIsSpecial = 1, mSpecialType = type`; bomb: `mColor = Next() %
mNumColors`; push on mLoosePieces. (Falls in via UpdateLoosePieces.)

#### 6.4 `TowerToppler()` 0x409e00 -> bool

Puzzle: false. `top[c]` = first coin row of column c (mRows if none). For c: `left = c==0 ? mRows :
top[c-1]`, `right = c==13 ? mRows : top[c+1]`; `h = min(max(left-top[c],0), max(right-top[c],0))`
(a column standing above both neighbours). `a = mColTowerHeight[c]`: if `a <= 0 || a - h < 4.2`:
`a = h*0.0035 + a*0.9965`; else (the tower lost > 4.2 rows at once): `PlayAnim(10)`,
`PlaySample(0x27)`, `ShowBonusText(TOWER_TOPPLER_BONUS)`, result = true, `a = h`. Store a.
Called every playing tick (result ignored) and by OnClick (+125 points when true).

#### 6.5 `GenerateBoard()` 0x40a110

Zero `mMeterCoins, mMeterDrained, mMeterTarget, mMeterPending, mMeterLaunchTimer, mMeterEmptying`;
null the whole grid (pieces not deleted).
**Action / Strategy**: for c = 0..13: `y = -58`; rows r = 14..9: `y += (int)(((Next()%10)*(1/3.0) +
1.2) * -29.0)`; new coin `mY = y, mCol = c, mRow = r, mX = ColX(c), mColor = Next() % mNumColors`
(2 draws per coin, 168 total).
**Puzzle** (constructive generator; `pieces` below means grid pieces, all coins until the end):
Loop:
1. `cand` = columns c (ascending) whose row-6 cell is empty and where some k in {c-1, c, c+1} (in
   0..13) has a row-14 piece. If every column has a piece at row 6: stop.
2. `c = cand.empty() ? Next() % 14 : cand[Next() % cand.size()]`.
3. `e` = number of empty cells above the top piece of column c (15 if empty). If `e <= 1`: next loop.
4. `startRow = (int)ceil(14.0 - (15-e) * pow(Next()%100000 * 5e-6, 2.0) * 4.0)`
   (x87 `_CIpow` 0x4d93e0 = pow(ST1, ST0), ceil 0x4d8810); `color = Next() % mNumColors` (overwritten
   before use); `ResetMarks()`; `target = (int)(pow(Next()%100000 * 5e-6, 4.0) * 16.0 * 15.0)`
   (0..14).
5. `placed = 0, first = null`; for attempt a = 0, 1, ...: stop when `a >= target && placed >= 2`.
   - No coin yet: `if (a > 4 && startRow < 14) startRow++`; `(x, y) = (c, startRow)`; `color =
     Next() % mNumColors`.
   - Else: list the (col,row) of all pieces with `mMark == 1` (rows 0..14, cols 0..13 order);
     `i = Next() % count`; `d = Next() & 3`; `(x, y) = pos[i] + dir[d]` with dir = {(0,0), (-1,0),
     (+1,0), (0,+1)} (a 5-entry table {(0,0),(-1,0),(1,0),(0,1),(0,-1)} indexed 0..3: (0,-1) is never
     used).
   - Place if `0<=x<14, 0<=y<15`, supported (`y == 14 || mGrid[y+1][x]`), `mGrid[2][x]` empty and ok:
     ok is false if any of the cells (y,x), (y,x-1), (y,x+1), (y+1,x) holds a piece of `color` with
     `mMark != 1`; also, walking up r = y-1, y-2, ... while `mGrid[r+1][x]` is a group piece (mark 1),
     any of (r,x-1), (r,x), (r,x+1) holding such a piece clears ok. Placement shifts rows 1..y of
     column x up one (`mRow--`), then new coin at (x,y): `mX = ColX(x), mColor = color, mMark = 1`
     (`mY` set later); `first = first ? first : it; placed++`.
   - After the attempt: if `placed == 1 && a > 8`: delete `first`, clear `mGrid[startRow][c]`, compact
     column c downward (pieces keep order, `mRow` updated), abandon this group.
6. Loop again.
Then: `cols = [0..13]`; remove `14 - (mBagsNeeded - skill + 4)` entries: each `cols.erase(rand() %
cols.size())` (**CRT rand**). For each remaining column (ascending): money bag (`mIsSpecial = 1,
type 0, mX = ColX`) at the first empty row above the column's top piece.
Start heights: for c = 0..13: `y = -58 - (int)(((Next()%10)*(1/3.0) + 1.2) * 29.0)`; rows r = 14 up
while a piece exists: `mY = y; y += (int)(((Next()%10)*(1/3.0) + 1.2) * -29.0)` (1 + n draws per
column).
**All modes**, next row: for c = 0..13: `mColTowerHeight[c] = 0; mColWarning[c] = -1`; new coin
`mCol = c, mRow = 15, mY = RowY(15) + 6 = 444, mX = ColX(c), mColor = Next() % mNumColors`.
Finally: `mMintDelay = 160; FindGroups(); mMint = 0; mMintTarget = 0.25; mMintPixels = 29;
mLastGroupSize = mPrevGroupSize = 0; mLastColor = mColorStreak = 0; mSweepTone = 0;
UpdateMovesLeft()`.

Quirk: if `cand` is empty and `Next()%14` picks a column of height 13 (e = 2, row 2 occupied) no coin
can ever be placed and step 5 never ends (hang in the original; needs a full column of 13 with no
eligible candidate column, very unlikely).

#### 6.6 Level flow

- `LevelComplete()` 0x4083d0: music (Action: FadeOut(3, false, 0.02), PlayMusic(4, 0x28, false);
  Strategy: song 6 -> 7 at 0x3f; Puzzle: 0 -> 1 at 0x17); `PlaySample(0x19)` (levelup);
  `PlayAnim(0x11)`; 60 bills, i = 0..59: `x = rand()%500 + 80`, `vy = (rand()%1000)*0.001*0.4 +
  i*0.02 + 1.6`, `y = -50 - rand()%92`, `frame = rand()%16`, `type = 4 - (5i)/60`; level-up sign
  `mLevelUpY = -150, mLevelUpAngle = 0, mLevelUpUnused = 0.03, mLevelUpBouncing = 1, mLevelUpLeftY =
  -150, mLevelUpRightY = -174, velocities 0, mLevelUpStretch = 189, mStatsFade = 0, mStatsDirty = 1,
  mLevelUpActive = 1, mState = 2, mLevelEndDelay = 160, mMeterFraction = 0`; repaint.
- `ClearBoardForLevelEnd()` 0x408650 (end of state 2): `mMeterFraction = 0`; HideHint; for rows 0..14,
  cols 0..13 (grid piece p): `mRow = mRows + 2; mVY = (Next()%200)*0.01`; not Puzzle and p is a bag:
  every bag after the first: `AddPoints((int)(mPointsMult*500.0), (int)(mX+14), (int)(mY+14),
  false)` (AddPoints multiplies by mPointsMult again). If `bags >= 3`:
  `ShowBonusText(EXCESS_BAGGAGE_BONUS)`, `PlayAnim(10)`, `PlaySample(0x27)`. `mState = 4`.
- `ShowContinueButton()` 0x4045e0 (board_draw details): `mStatsSeconds = (now - mTimerTick)/1000 +
  mPlaySeconds`; `mContinueButton = new DialogButton(images[33 dialogbutton], 0, this+0x80)`,
  font app+0x700, colour 0 = (0,255,0), label `CLICK_HERE_TO_PLAY_AGAIN` (state 6) else
  `CLICK_HERE_TO_PLAY_NEXT_LEVEL`, `Resize(197, 444, 366, 27)`, mouse insets (-10,-10,-10,-10),
  `mDoFinger = 1`, AddWidget; `mButtonBlink = 0`; delete the three Draw caches; `mStatsFade = 0;
  mStatsDirty = 1`; repaint.
- `StartNextLevel()` 0x40e4b0: music (Action FadeOut(4, true, 0.02), FadeIn(3, -1, 0.004, false);
  Strategy 7 -> 6; Puzzle 1 -> 0); `mLevelUpActive = 0; mTimerTick = GetTickCount(); NextLevel()`.
- `NextLevel()` 0x40ae20: `mLevel++; mState = 0`; remove + SafeDelete mContinueButton;
  `mLevelCoinsCleared = mLevelBiggestBonus = mLevelBiggestCombo = 0; mBagsReleased = mBagsBanked =
  0; mGoTimer = 80; SetupLevelParams(); GenerateBoard()`. Score, meter fill, mint speed and the
  game totals carry over.
- `GameOver()` 0x401d30: music FadeOut(3/6/0, true, 0.02) all three, PlayMusic(9, 0x43, false);
  `PlaySample(0x16)` (gameovertone); `SetDangerFlash(false); mCountdownActive = 0; mMeterFraction =
  0; mStatsFade = 0; mState = 5; mGameOverRiseDelay = 100; mStatsDirty = 1; mFlingDelay = Puzzle ? 200
  : 1; HideHint()`.
- `FlingAllCoins()` 0x408220: rows 0..14, cols 0..13, grid piece p: cell = null; `mVX =
  (Next()%300)*0.01 - 1.5; mImageSet = 1; mSpin = 0; mSpinSpeed = (rand()%1000)*4e-5; k = r/mRows +
  0.1; mVY = (double)(-(int)(Next()%500)) * k * 0.01; mVX += mVX > 0 ? 0.5 : -0.5`; special -> `type
  1, frame 0`; push on mLoosePieces.

#### 6.7 `Pause(bool pause)` 0x402930

Only in state 0 (`mState != 0` -> return). `old = mPauseCount; mPauseCount += pause ? 1 : -1`,
clamp >= 0. Every Points widget: `SetVisible(mPauseCount == 0)`. Then:
- now unpaused (`mPauseCount < 1`) and `old >= 1`: `mTimerTick = GetTickCount()`; music Action
  FadeOut(5, true, 0.008) + FadeIn(3, -1, 0.008, false); Strategy 8 -> 6; Puzzle 2 -> 0.
- now paused and `old <= 0` and `app->mHasFocus`: `mPlaySeconds += (GetTickCount() - mTimerTick) /
  1000` (unsigned); music Action FadeOut(3, false, 0.008) + FadeIn(5, 0x29, 0.008, false); Strategy
  6 -> 8 at 0x40; Puzzle 0 -> 2 at 0x18.
- Repaint.
`SaveGame` calls `Pause(true); Pause(false)` first to bank the seconds.

#### 6.8 `OnClick(Piece* p)` 0x40b120 (a group is clicked; called only in state 0)

`voiced = (mState != 0)` (false): at most one Mr. Money line per click ("voice" = `PlayAnim(a)` +
`PlaySample(s)`, skipped once voiced; every bonus below sets voiced). Return if p is special.
1. `n = 0, sum = (0,0)`; `FloodFill(p.mCol, p.mRow, mUpdateCnt, true, &n, sum)`. `n < mMinGroup` ->
   repaint, return (no penalty, nothing else).
2. First valid click of this board: `app->mTimesPlayed++`, `mPlayStarted = 1`.
   `app->mHasClickedGroup = 1`.
3. Tutorial: if the visible hint is 6 or 7: HideHint; Puzzle -> `ShowHint(8, first grid bag found
   (rows/cols order), 1000)`; else `ShowHintAt(1, {610, 0}, 1000); mHintFollowMeter = 1`.
4. `bagsAbove` = number of group cells (rows 1..14) whose upper neighbour is a special.
5. Base score: n = 2..15 -> {5, 10, 15, 20, 30, 40, 50, 65, 80, 95, 115, 135, 155, 180}[n-2]; n >= 16
   -> `(n-2)*25 - 145`. `bagsAbove > 0` -> score *= 2.
6. Combo (c0 = n, c1 = mLastGroupSize, c2 = mPrevGroupSize; first match wins):

| Condition | Text id | Voice anim, sound | Bonus |
|---|---|---|---|
| c0>=13 && c1>=13 && c2>=13 | TRIPLE_HYPER_COMBO | 7, 0x1d awesome | +450 |
| c0 >= 20 | AWESOME_COMBO | 7, 0x1d awesome | +400 |
| c0>=10 && c1>=10 && c2>=10 | TRIPLE_MEGA_COMBO | 21, 0x1e unbelievable | +350 |
| c0>=13 && c1>=13 | DOUBLE_HYPER_COMBO | 21, 0x1e unbelievable | +350 |
| c0>=10 && c1>=10 | DOUBLE_MEGA_COMBO | 16, 0x1c incredible | +250 |
| c0 >= 15 | BIG_MONEY_COMBO | 8, 0x1f bigmoney | +200 |
| c0>=7 && c1>=7 && c2>=7 | TRIPLE_COMBO | 7, 0x1d awesome | +100 |
| c0>=7 && c1>=7 | DOUBLE_COMBO | 15, 0x1a goodjob | +75 |
| c0 >= 10 | EXCELLENT_COMBO | 11, 0x1b excellent | +25 |

   Each shows `ShowBonusText(GetString(id))`.
7. Colour combo: `p.mColor == mLastColor` -> `++mColorStreak`: == 4 -> COLOR_COMBO_BONUS (voice 10,
   0x27 bonus) +20; == 6 -> SUPER_COLOR_COMBO_BONUS (voice 20, 0x28 superbonus) +50. Else
   `mLastColor = p.mColor; mColorStreak = 1`.
8. Speed bonus (Action only): `mSpeedBonus += 0.16`; `>= 1.0` -> `mSpeedBonus = 0`, SPEED_BONUS
   (voice 19, 0x26 speedbonus) +40.
9. Stats: `mLevelCoinsCleared += n; mGameCoinsCleared += n; mLevelBiggestCombo = max(.., n);
   mGameBiggestCombo = max(.., n)`.
10. Sounds: `PlaySample(n <= 4 ? 0 : n <= 9 ? 1 : 2)` (clearcoins 1/2/3); then Puzzle `PlaySample(0x14)`
    (tonepuzzle), else `PlaySample(11 + (int)min(mMeterFraction*9.0, 8.0))` (tone1..tone9 by meter
    fill).
11. `colsBefore = CountCoinColumns()`.
12. Remove the group: for rows 0..14, cols 0..13, piece q with `mMark == mUpdateCnt`: hint piece ->
    HideHint; track `minRow/maxRow`; `mDanger = 0, mGroupSize = 0, mHighlighted = 0, mVX = mVY = 0`;
    - `bagsAbove > 0`: `mIsSpecial = 1, type 1 (poof), mFrame = 0`;
    - else `n >= 7`: fly to the meter: `mFlying = 1, mTarget = (611, 25), mStart = ((int)mX,
      (int)mY), mFlySpeed = min(9.0/sqrt(dx*dx+dy*dy), 0.2), mFlyT = 0, mImageSet = rand()%2 + 1
      (CRT), mDying = 1`;
    - else: if `n >= 5`: 3 particles k = -1, 0, 1 {delay 2, (mX+14, mY+14), vx 1.5k, vy -3.0, g 0.2,
      frame 0, bright 1}; `mDying = 1, mFrame = 0`;
    push on mLoosePieces; grid cell = null.
13. `mMintDelay = max(mMintDelay, (maxRow - minRow)*8 + 24)`.
14. `Compact()`. `TowerToppler()` true -> +125.
15. `d = colsBefore - CountCoinColumns()`: `d >= 6` -> SUPER_COLUMN_CLEARING_BONUS (voice 20, 0x28)
    +200; `d >= 3` -> COLUMN_CLEARING_BONUS (voice 10, 0x27) +50.
16. `AddPoints(score, sum[0]/n + 14, sum[1]/n + 14, true)` (C int division).
17. Strategy and `bagsAbove == 0`: `mMintTarget += mMintPerClick; UpdateMovesLeft()`; if
    `1 <= mMovesLeft <= 3`: `PlayAnim(7 - m)`, `PlaySample(35 - m)` (three/two/one; not subject to
    voiced), `mMoveDigitActive = 1; mMoveDigit = 3 - m; mMoveDigitFade = 1.0`.
18. `mPrevGroupSize = mLastGroupSize; mLastGroupSize = n`; repaint.

A click with a special directly above the group therefore releases bags/bombs (Action/Strategy:
Compact sets `mFallThrough`), doubles the base score, plays no flying/burst animation, and in
Strategy does not advance the rows.

#### 6.9 Bonus summary (other than OnClick)

| Bonus | Where | Points |
|---|---|---|
| Money bag collected | UpdatePhysics | AddPoints(100) (no meter) |
| SPEED_BAGGER_BONUS | bag collected < 500 ticks after the previous one | AddPoints(150) instead |
| TOWER_TOPPLER_BONUS | TowerToppler in OnClick | +125 to the click score |
| EXCESS_BAGGAGE_BONUS | level end, each bag left on the board after the first | AddPoints((int)(500*mult)) each; text when >= 3 bags |
| Puzzle sweep | state 3 | AddPoints(3 per coin per row) |

### 7. RNG draws in call order

Two generators. **B** = `mRand->Next()` (Board MTRand, saved in savegame.dat). **C** = CRT `rand()`
(shared, not saved, seeded from GetTickCount). Within a tick the order is the Update order of
section 5; OnClick runs from MouseDown between ticks.

| Site | When | Draws (in order) |
|---|---|---|
| ctor | Board creation | C: seed; then GenerateBoard |
| GenerateBoard A/S | level start | per column 0..13, rows 14..9: B%10 (start y), B%colors; then next row: 14 x B%colors |
| GenerateBoard Puzzle | level start | generator loop: column pick (B%14 or B%cand), [e>1:] B%100000 (start row), B%colors (unused), B%100000 (target); per attempt: first coin B%colors, later B%count, B&3; then C%size per removed bag column; then per column B%10 x (1 + height); then next row 14 x B%colors |
| Update step 9 | mBillsToSpawn > 0 (never) | C x4 |
| Update step 10 / FlingAllCoins | game over + delay | per piece rows 0..14 / cols 0..13: B%300, C%1000, B%500 |
| Update step 11 | glint inactive, every tick | C%2000 |
| UpdateLoosePieces | per loose piece, list order | flying: [coin: C%15] then C%8, C%8, C%1000 if sparkling; dropping special: C%2, [C%28]; flung coin floor bounce: B%100 when the rebound speed is below 4 |
| UpdateMeter | meter overflow, state 0 | DropSpecial(0): B%cand |
| UpdatePhysics | per grid bag rows 14..0 / cols 0..13, not Puzzle | C%10, [C%28] |
| Update state 0 | every playing tick, Action | B%15000; if 0 and level>0: DropSpecial(2): B%cand, B%colors |
| Update state 0 | Action overflow, every 4th overflow tick | per column 0..13, rows 0..n-1 with a piece: B%100 (x), B%100 (y) |
| PushRow | row pushed | 14 x B%colors |
| LevelComplete | level won | 60 bills x (C%500, C%1000, C%92, C%16) |
| ClearBoardForLevelEnd | end of state 2 | per grid piece rows 0..14 / cols 0..13: B%200 |
| OnClick | group of >= 7, no bag above | per removed coin (rows/cols order): C%2 |

Notes for a port: B is fully deterministic from its saved state; to reproduce a live game both
generators and the frame-exact input timing are needed. `Next() % 15000` is consumed every playing
tick in Action even at level 0 (the level test comes after the draw). The CRT generator is also used
by other modules between and within ticks: MrMoney::Update draws 2-3 `rand()` per tick (idle
animations, board_draw section on MrMoney), GameSelector::Update, the framework.

### 8. Sounds, music, Mr. Money

Sound ids (index into the name table 0x5286d8, files `sounds/<name>.ogg/.au`): 0 clearcoins,
1 clearcoins2, 2 clearcoins3, 3 piecehit, 4 moveup, 5 bonusdrop, 6 baghit, 7 gotbag, 8 getready,
9 go, 10 buttonclick, 11..19 tone1..tone9, 20 tonepuzzle, 21 warning, 22 gameovertone, 23 explode,
24 gameover, 25 levelup, 26 goodjob, 27 excellent, 28 incredible, 29 awesome, 30 unbelievable,
31 bigmoney, 32 three, 33 two, 34 one, 35 bad, 36 cointube, 37 welcome, 38 speedbonus, 39 bonus,
40 superbonus, 41 megamoney, 42 action, 43 strategy, 44 puzzle, 45 wakeup, 46 anewhighscore,
47 loveit. Board plays 0..34 except 23 via PlaySample (10 only from ButtonPress), 23 through
mExplodeSound, and 36, 38, 39, 40; never 35, 37, 41..47.

Music (one module `music/QualityTime_BigMoney.it`, song ids loaded by WinBMApp; Board's use):

| Mode | Main | Pause (offset) | Level complete (offset) |
|---|---|---|---|
| Action | 3 | 5 (0x29) | 4 (0x28) |
| Strategy | 6 | 8 (0x40) | 7 (0x3f) |
| Puzzle | 0 | 2 (0x18) | 1 (0x17) |

Game over: PlayMusic(9, 0x43, false). Fade speeds: pause 0.008; level complete FadeOut 0.02;
next level FadeOut 0.02 / FadeIn 0.004; game over FadeOut 0.02. `FadeIn(id, -1, ...)` resumes.
MusicInterface slots used: 2 PlayMusic, 11 FadeIn, 12 FadeOut.

Mr. Money (`app->mMrMoney`, +0x6d4, MrMoney.cpp): `PlayAnim(id)` 0x415200 (sets +0xc0 anim, resets
+0xc4/+0xc8, length from the 400-byte table entry at 0x4f6818 + id*400) and target position fields
+0x98/+0x9c (set to (350,24) at tick 1, (0,290) at tick 300 and while paused). Anim ids used by Board:
4/5/6 countdown 3/2/1, 7 awesome, 8 big money, 10 bonus, 11 excellent, 12 game over, 13 get ready,
14 go, 15 good job, 16 incredible, 17 level up, 19 speed bonus, 20 super bonus, 21 unbelievable.

Images used by the logic (index into app->mImages, +0x6e4): 23 coins (frames = width/29), 24
horzcoins, 33 dialogbutton (button), 43 levelup (sign width). Fonts: app+0x6ec (bigmoneyfont1,
bonus texts), +0x700 (bigmoneyfont3, button), +0x718 (floatfont, "+%d").

### 9. savegame.dat (working directory; little-endian, packed, plain fwrite)

Written by `SaveGame()` 0x403300 (only from the WinBMApp destructor, when `CanSaveGame()`), read by
`LoadGame()` 0x4075d0 (dialog 5 "Continue?" YES: WinBMApp::NewGame, then LoadGame, then the file is
deleted; NO deletes it).

Header (0x14 bytes): `u32 version = 0` (load fails unless 0), `i32 gameType`, `i32 skill` (both
written into the app on load), `i32 state` (mState), `u32 rngLen` (0x9c0), then `rngLen` bytes =
the 624 MT words (MTRand::Serialize; mti is not saved; on load SRand_state copies the words and
keeps the current mti, see open questions).

Piece record (108 bytes): `d mX, d mY, d mVX, d mVY, i mShakeX, i mShakeY, i mCol, i mRow, i mColor,
u8 mIsSpecial, u8 mFallThrough, i mSpecialType, i mImageSet, i mFrame, u8 mFlying, d mFlyT, d
mFlySpeed, i mStartX, i mStartY, i mTargetX, i mTargetY, u8 mDying, i mLifeTimer, i mUnusedA0`
(0x401120 read / 0x4012b0 write).

Body:
1. Grid, rows 0..14, cols 0..13: `u8 present`, then a Piece record if present.
2. mNextRow[0..13]: 14 Piece records (always present).
3. `i 0x9c mMinGroup, i 0x428 mRiseOffset, u8 0x42c mHasMoves, i 0x430, i 0x434, i 0x438, d 0x440,
   d 0x448, d 0x450, d 0x458, d 0x460, Color 0x468 (4 x i32), i 0x490, i 0x494, d 0x498, i 0x4bc,
   d 0x4c0, d 0x4c8, d 0x4d0, i 0x4d8, d 0x4e0, i 0x4e8, d 0x4f0, d 0x4f8, d 0x500, d 0x508`.
4. `u32 count` + count Piece records (mLoosePieces).
5. `u32 count` + count x {d mY, d mDist, i mFrame} (mTubeCoins).
6. `i 0x540, i 0x54c, i 0x550, i 0x554, i 0x558, i 0x55c, u8 0x560, d 0x568, d 0x570, d 0x578,
   d 0x580, u8 0x588, d 0x590, d 0x598, d 0x5a0, d 0x5a8, i 0x5b0, u8 0x5b4, i 0x5b8, i 0x5bc,
   i 0x5c0, d 0x5c8, i 0x5d0, u8 0x5d4, i 0x5dc, u8 0x5e0, i 0x5e4, i 0x620, i 0x624, u8 0x628,
   d 0x630, i 0x638, i 0x644, i 0x648, i 0x64c, i 0x650, i 0x654, i 0x658, i 0x768, i 0x76c,
   u8 0x78c, i 0x790, d 0x798, u8 (mContinueButton != 0)`.
No checksum, no trailing data. Not saved: mNumColors, mTimerTick/mPlaySeconds, particles, bills,
Points widgets, hints, stats images, mColTowerHeight, mColWarning, mSpeedBonus, combo memory
(0x478..0x48c), mMovesLeft. A real save from the Steam install parses to exactly its file size with this layout.

LoadGame, after reading: if the button flag is set -> `ShowContinueButton()`; `mPlayStarted = 1`;
delete the three Draw caches; `mHintVisible = 0; mMintDelay = 200; UpdateMovesLeft();
FindGroups()`. Returns true. (Loaded pieces are new Piece objects; existing grid/next-row pieces are
deleted first; existing loose pieces are only unlinked.)

### 10. Cross-module references

WinBMApp fields Board reads/writes (names proposed; WinBMApp agent to reconcile):

| App offset | Type | Proposed name | Board use |
|---|---|---|---|
| +0x338 | MusicInterface* | mMusicInterface (SAF) | FadeOut/FadeIn/PlayMusic (section 8) |
| +0x47c | SoundManager* | mSoundManager (SAF) | GetSoundInstance(0x17) |
| +0x498 | bool | mHasFocus (SAF) | Pause banks time only with focus |
| +0x6bc | bool | mIsRegistered (SAF) | Puzzle trial: tick-100 dialog, next level locked |
| +0x6c0 | int | mTimesPlayed (SAF) | ++ on the first valid click of a board |
| +0x6d0 | Board* | mBoard | (owner) |
| +0x6d4 | MrMoney* | mMrMoney | PlayAnim; +0x98/+0x9c target x/y |
| +0x6e4 | Image** | mImages | 71-entry table (names 0x5285b8): 23 coins, 24 horzcoins, 33 dialogbutton, 43 levelup |
| +0x6e8 | MemoryImage* | mBrightCoinsImage | DrawPiece (hovered/danger coins) |
| +0x6ec | ImageFont* | mFont1 (bigmoneyfont1) | ShowBonusText |
| +0x700 | ImageFont* | mFont3 (bigmoneyfont3) | continue button, stats panel |
| +0x718 | ImageFont* | mFloatFont (floatfont) | AddPoints "+%d" |
| +0x814 | int | mSkillLevel (reg SkillLevel, 1..3) | SetupLevelParams, ShowHintAt, Puzzle bags; written by LoadGame |
| +0x818 | int | mGameType (reg GameType) | everywhere; written by LoadGame |
| +0x81c | bool | mBagCollectedOnce | session flag: set when a bag is collected; suppresses hint 5 and the column-13 rule |
| +0x81d | bool | mBombSeenOnce | set when a bomb detonates; suppresses hint 3 |
| +0x81e | bool | mDangerHintShown | hint 2 once per session |
| +0x81f | bool | mHasClickedGroup | set by OnClick; tick-600 hint |
| +0x825 | bool | mShowInstructor | ShowHint works only when set (WinBMApp 0x41c170: help screen shown for this mode) |

(+0x81c..+0x81f are zeroed only in the WinBMApp ctor, so they are per session.)

App methods called: slot 32 SafeDeleteWidget, slot 49 PlaySample(int), slot 72 GetDialogCount,
GetString, GetStringVector, `NewGame` 0x41bfb0 (ButtonDepress in state 6),
`ShowPuzzleTrialDialog` 0x41d2e0, `ShowPuzzleNextLevelLocked` 0x41d4c0.

Other modules: `Points::Points(app, font, text, x, y, life, justify)` 0x417630 (life at +0x84,
colour phase at +0xa8; justify 0 centred / 1 right / other left), `MrMoney::PlayAnim` 0x415200,
`HighScoreWidget::HighScoreWidget(app)` 0x414640, `DialogButton::DialogButton(img, id, listener)`
0x429c10, `MTRand` 0x45bc80/0x45bcb0/0x45fd90/0x45c750, `ceil` 0x4d8810, `_CIpow` 0x4d93e0, CRT
`rand` 0x4d9372.

Readers of Board fields elsewhere: HighScoreWidget 0x4133b0 / vf55 0x414530 (mScore), WinBMApp
0x420f10 (mHighScoreWidget, RecordScore dialog result), 0x4207e0 (mState != 6), app dtor
(CanSaveGame, SaveGame), 0x420560 (CanSaveGame), ModalOpen/Close, focus (Pause), 0x41b840 (Freeze).

Naming reconciliation: app.md's provisional names mHint81C / mHint81D / mHint81E / mMovedThisSession
for +0x81c..+0x81f should become mBagCollectedOnce / mBombSeenOnce / mDangerHintShown /
mHasClickedGroup (+0x81e is the "coins within 4 rows of the top" danger hint, not a moves count);
+0x825 keeps app.md's mShowInstructor (the hint art is images 63/64 instructoarrows/instructotext).
MrMoney 0x415200 is `PlayAnim` (board_draw); its full id enum lives in board_draw.h.

Globals: `gPieceCount` 0x52d860 (int), `gRankNames` 0x52d864 (vector<string>, filled by the ctor),
`gRankThresholds` 0x4f53c0 (int[27]: 0, 1000, 2000, ..., 10000, 11500, 13000, 15000, 20000, 30000,
40000, 50000, 60000, 70000, 80000, 100000, 125000, 150000, 200000, 300000, 500000; rank = highest i
with threshold <= mScore, "next rank at" = threshold[i+1]; read by the stats panel),
`gCoinColors` 0x52d8c8 (Color[5]: (255,0,0), (0,148,255), (255,255,0), (0,255,0), (255,0,255)),
`gCoinLightColors` 0x52d878 (Color[5]: (255,128,128), (128,128,255), (240,240,64), (87,255,87),
(240,64,240); Draw).

### 11. Open questions and quirks

1. **Uninitialised shake weights** (Update, Action overflow): `w[c]` is only written (1.0) for
   columns whose row-0 cell is a coin; other columns read stale stack doubles (the 0.6 rule only fires
   when the stale value is exactly 0.0). Behaviour is undefined in the original; a port should
   initialise `w[] = 0.0` (the evident intent) and note it.
2. **RNG after LoadGame**: `SRand_state` copies the 624 words but keeps the constructor's `mti`
   (the number of draws GenerateBoard made at construction: 182 for Action/Strategy, variable for
   Puzzle), so a restored game continues from that index of the saved block, not from where the
   saved game was. mNumColors is also not saved: it keeps the constructor value (level 0 of the app's
   mode/skill at load time), so after a load, minted coins use 3 colours at skill 1 (4 at skill 2/3,
   5 Puzzle) until the next level. Both are original behaviour; confirm in-game if possible.
3. Shake offsets (`mShakeX/Y`) are never reset after a survived countdown; DrawPiece always adds them.
4. Follower column-slide target uses `mGrid[r+1]` twice (`0.5*a + 0.5*a`); probably meant r+2.
5. Puzzle generator can hang (section 6.5 quirk). Puzzle dir table never uses (0,-1).
6. `mLifeTimer`, `mBillsToSpawn`, `mRepaintTimer`, `mUnusedList`, `mMeterFlash`, `mMeterJustFull`,
   `mLevelUpUnused`, 0x758..0x767, 0x7a4, Piece +0xa0 are written/saved but never drive anything.
7. Leftover flood marks (section 4) can make an early click (first ~2 s of a board) fail.
8. Exact MrMoney anim semantics (table 0x4f6818) and the MusicInterface song/order mapping belong to
   the MrMoney / WinBMApp analyses.
9. The bomb timing: a type-3 exploding piece still in mLoosePieces when state 7 ends would then take
   the "dropping special" path (fall, land and be put back into the grid as a special). With 8-tick
   detonation spacing, a 42-tick explosion and 64 idle ticks this cannot normally happen.
10. Stats panel (board_draw): it shows level totals (0x644/0x64c/0x654) in state 1 and game totals
    (0x648/0x650/0x658) in state 6, plus mStatsSeconds and the rank of mScore. The string
    `LEVELS_COMPLETED` is never referenced by the code (unused string).

### 12. Function index (Board.cpp, logic side)

| Address | Name | Notes |
|---|---|---|
| 0x401000 | Piece::Piece() | |
| 0x4010b0 | Piece::~Piece() | |
| 0x401d10 | Piece::`scalar deleting dtor` (slot 0) | |
| 0x4010c0 | std::char_traits<char>::assign | |
| 0x4010d0 | Board::CanSaveGame() | |
| 0x4010f0 | SaveGameExists() | |
| 0x401120 | ReadPiece(FILE*, Piece*) | 108-byte record |
| 0x4012b0 | WritePiece(FILE*, Piece*) | |
| 0x401440 | DeleteSaveGame() | |
| 0x401460 | Board::OrderInManagerChanged() slot 15 | |
| 0x401490 | Board::SetupLevelParams() | |
| 0x4015e0 | Board::ColX(int) (static) | 29c+177 |
| 0x4015f0 | Board::RowY(int) (static) | 29r+3 |
| 0x401600 | Board::ResetMarks() | |
| 0x401660 | Board::SetDangerFlash(bool) | |
| 0x401720 | Board::FloodFill(int,int,int,bool,int*,int*) | |
| 0x401840 | Board::FindGroups() | |
| 0x401960 | Board::PieceAt(int,int) | |
| 0x401a20 | Board::ShowHintAt(int, const Point*, int) | |
| 0x401a80 | Board::HideHint() | |
| 0x401a90 | Board::CountCoinColumns() | |
| 0x401ae0 | ButtonListener::ButtonPress(int,int) | framework thunk |
| 0x401af0 | Board::GetTopCoinRow() | |
| 0x401b40 | Board::GetAverageCoinRow() | |
| 0x401bd0 | Board::UpdateMovesLeft() | |
| 0x401c70 | Board::ButtonPress(int) (+0x80 slot 1) | PlaySample(10) |
| 0x401d30 | Board::GameOver() | |
| 0x401e30 | Board::GetBagSlotPos(Point*, int) | |
| 0x401e90 | Board::UpdateHover() | |
| 0x401fa0 | Board::MouseLeave() slot 35 | |
| 0x401fb0 | Board::MouseMove(int,int) slot 36 | |
| 0x401fe0 | Board::ShowHint(int, Piece*, int) | |
| 0x402060 | Board::DrawPiece(Graphics*, Piece*, int theYOffset) | board_draw (ret 0xc) |
| 0x402930 | Board::Pause(bool) | |
| 0x402b70 | Board::Compact() | |
| 0x403050 | Board::KeyChar(char) slot 31 | |
| 0x403300 | Board::SaveGame() | |
| 0x403a20 | Board::RemovePointsWidgets() | |
| 0x403aa0 | Board::RemovedFromManager(WidgetManager*) slot 8 | |
| 0x403b80 | Board::Freeze() | |
| 0x403f10 | Board::~Board() | |
| 0x4045a0 | Board::`scalar deleting dtor` (slot 0) | |
| 0x4045e0 | Board::ShowContinueButton() | board_draw details |
| 0x404950 | Board::DrawStatsPanel(Graphics*) | board_draw |
| 0x405480 | Board::Draw(Graphics*) slot 13 | board_draw |
| 0x4075d0 | Board::LoadGame() | |
| 0x407ea0 | Board::AddPoints(int,int,int,bool) | |
| 0x408110 | Board::ShowBonusText(const string&) | |
| 0x408220 | Board::FlingAllCoins() | |
| 0x4083d0 | Board::LevelComplete() | |
| 0x408650 | Board::ClearBoardForLevelEnd() | |
| 0x4088a0 | Board::PushRow() | |
| 0x408a60 | Board::UpdatePhysics() | |
| 0x409520 | Board::UpdateLoosePieces() | |
| 0x409e00 | Board::TowerToppler() | |
| 0x40a110 | Board::GenerateBoard() | |
| 0x40ae20 | Board::NextLevel() | |
| 0x40aea0 | Board::DropSpecial(int) | |
| 0x40b120 | Board::OnClick(Piece*) | |
| 0x40c5c0 | Board::ClickAt(int,int) | |
| 0x40c5f0 | Board::MouseDown(int,int,int) slot 38 | |
| 0x40c640 | Board::UpdateMeter() | |
| 0x40c9f0 | Board::Update() slot 9 | |
| 0x40e4b0 | Board::StartNextLevel() | |
| 0x40e580 | Board::ButtonDepress(int) (+0x80 slot 2) | |
| 0x40e760 | Board::Board(WinBMApp*) | |

The remaining small functions in the range are std:: helpers (string assign/append/erase/_Grow,
list node allocation per payload size 0xc/0x40/0x28/0x20/0x10, list _Incsize, erase, vector<int>,
vector<Point>, vector<string> insert/copy/assign; see the symbols file).

---

## Part 2: Board drawing, MrMoney, Points, OverlayWidget

Scope: the drawing half of `Board.cpp` (Board::Draw 0x405480 and its helpers), plus `MrMoney.cpp`
(0x415140-0x415560), `Points.cpp` (0x4174f0-0x417730) and `OverlayWidget.cpp` (0x417400-0x4174f0).
Everything below was read from the disassembly (`llvm-objdump`), not only from Ghidra; every x87
expression that feeds `_ftol` was recovered from the instructions. Board field names are proposals
from the drawing side (board_logic owns the Board layout; see section 6 for the reconciliation list).

### 0. Conventions

- Offsets are absolute from the object start (hex). `app` = `Board::mApp` (+0x84) = the WinBMApp.
- `img[N]` = `app->mImages[N]`, the Image* table at `app+0x6e4` (an `operator new(0x11c)` array of 71
  pointers filled by `WinBMApp::LoadImage(i)` 0x41d7d0 from the name table at 0x5285b8, file
  `images\<name>`; byte offset in the table = N*4, which is what the code uses).
- `(int)x` of a double is `_ftol`: truncation toward zero. Integer `/` and `%` are C signed
  (truncate toward zero) unless marked unsigned. x87 precision control is assumed 53-bit (CRT
  default), as for Bejeweled.
- `DrawImage(img, x, y)` = Graphics::DrawImage(Image*, int, int) 0x43af80;
  `DrawImage(img, x, y, src)` = 0x43b030; `DrawImage(img, dest, src)` = 0x43b110 (stretch);
  `DrawImageRotated(img, x, y, rot, NULL)` = 0x43f820 (rotation in radians about the image centre,
  w/2, h/2, int division); `DrawString(s, x, y)` = 0x4325f0 (y is the baseline);
  `DrawImageBox(rect, img)` = 0x441f70. Rects are written (x, y, w, h).
- `SW(s)` = `g->GetFont()->StringWidth(s)` (ImageFont slot 7). Draw mode 0 = NORMAL, 1 = ADDITIVE.
- Colours: `Color(r,g,b)` alpha 255; `Color(r,g,b,a)`.
- Ticks: Update runs at 100 Hz (framework).

### 1. Resources used by the drawing code

#### 1.1 Images (index in `app->mImages`, file, size, cell layout)

Dimensions are from the files in `images/` (an `_name` file is the alpha mask of `name`).

| N (byte off) | file | size | cells used by the code |
|---|---|---|---|
| 16 (0x40) | level.gif | 51x21 | whole ("LEVEL" label) |
| 17 (0x44) | leftbar.jpg + _leftbar.gif | 640x480 | whole (full-screen HUD frame, drawn over the playfield) |
| 18 (0x48) | backdrop.jpg | 406x465 | whole (playfield background) |
| 19 (0x4c) | rightbar1.jpg | 56x479 | whole |
| 20 (0x50) | rightbar2.jpg + _rightbar2.gif | 80x480 | whole |
| 21 (0x54) | coin.gif + _coin.gif | 400x40 | 10 frames of 40x40 (coins flying into the stack) |
| 22 (0x58) | stack.gif | 40x372 | 62 coin slices of 6 px; the code takes the top n*6 rows |
| 23 (0x5c) | coins.gif + _coins.gif | 580x145 | 20 frames x 5 colours, 29x29 cells (x = frame*29, y = colour*29) |
| 24 (0x60) | horzcoins.gif + _horzcoins.gif | 464x145 | 16 frames x 5 colours, 29x29 |
| 25 (0x64) | bag.gif + _bag.gif | 132x36 | 4 cells of 33x36 (W/4) |
| 26 (0x68) | explosion.gif | 675x91 | 15 cells of 45x91 (W/15) |
| 27 (0x6c) | dollar.gif + _dollar.gif | 1056x33 | 32 cells of 33x33 (W/32) |
| 28 (0x70) | sparkle.gif | 280x20 | 14 cells of 20x20 (W/14; the list sparkles use a fixed 20) |
| 29 (0x74) | olddialog.gif + _olddialog.gif | 192x192 | DrawImageBox component image (stats panel frame) |
| 33 (0x84) | dialogbutton.gif | 96x36 | DialogButton component image (continue button) |
| 38 (0x98) | rankstrip.gif | 100x2700 | 27 cells of 100x100 (rank pictures) |
| 39 (0x9c) | gameover.gif + _gameover.gif | 333x181 | whole |
| 40 (0xa0) | countdown.jpg + _countdown.gif | 160x480 | 3 cells of 160x160 (H/3): "3", "2", "1" |
| 41 (0xa4) | barbagdisplay.jpg | 168x43 | 4 cells of 42x43 |
| 43 (0xac) | levelup.gif + _levelup.gif | 300x189 | whole ("LEVEL UP!") |
| 45 (0xb4) | bagholder.gif + _bagholder.gif | 41x34 | whole (empty bag slot) |
| 46 (0xb8) | glowbag.gif + _glowbag.gif | 64x64 | whole (additive glow) |
| 53 (0xd4) | bottomline.jpg + _bottomline.gif | 436x18 | whole (colourised "danger line") |
| 54 (0xd8) | coinglow.gif + _coinglow.gif | 45x45 | whole (additive, colourised selection glow) |
| 57 (0xe4) | bill.gif + _bill.gif | 1920x50 | 16 cells of 120x50 (scaled copies in app+0x7e0.., 1.4) |
| 58 (0xe8) | gamedifftype.gif + _gamedifftype.gif | 100x135 | 9 rows of 100x15: easy/normal/hard x action/strategy/puzzle |
| 60 (0xf0) | floatlogo.gif + _floatlogo.gif | 156x76 | whole (top-left logo) |
| 61 (0xf4) | pausebackdrop.gif | 489x374 | whole, tiled vertically |
| 63 (0xfc) | instructoarrows.gif | 50x250 | 5 cells of 50x50: 0 bent, 1 up, 2 right, 3 down, 4 left |
| 64 (0x100) | instructotext.gif | 200x450 | 9 rows of 200x50 (tutorial sentences) |
| 70 (0x118) | nomoremoves.gif | 333x181 | whole |
| 55 (0xdc) / 56 (0xe0) | mrmoney.gif 184x136 / mouths.gif 184x816 | | used only to build the MrMoney frames (1.4) |

Name table entry 42 is also "mouths" (same string as 56); the drawing code never uses index 42.

#### 1.2 Fonts (WinBMApp fields, built in WinBMApp::Init 0x41d9a0)

| app field | font | used for |
|---|---|---|
| +0x6ec mFont1 | `ImageFont(app, "data\\bigmoneyfont1.txt")` (base, no tags) | pause texts, "collect bags" message, stats labels/values, Points messages |
| +0x700 mFont3 | `ImageFont(app, "data\\bigmoneyfont3.txt")` | rank name, next-rank line, continue button label |
| +0x710 mScoreFont | `ImageFont(app, "data\\scorefont.txt")` | score "$%d" |
| +0x714 mLevelFont | legacy `ImageFont(img[14] levelfont.gif, "data\\levelfont.txt")` | level number |
| +0x718 mFloatFont | `ImageFont(app, "data\\floatfont.txt")` | floating score Points (created by board logic) |

(Other copies, not used here: +0x6f0 font1+"noshadow", +0x6f4 font1+"outline"+"noshadow", +0x6f8
font1+"underlined", +0x6fc font1+"outline"+"underlined", +0x704/+0x708/+0x70c font3 with
"outline"/"underlined"/both, +0x71c scorefont duplicate with SetPointSize(20).)

#### 1.3 Colour tables (Board.cpp statics, built by dynamic initialisers)

- `gCoinLightColors[5]` @0x52d878 (init 0x4f32a0), indexed by piece colour:
  (255,128,128), (128,128,255), (240,240,64), (87,255,87), (240,64,240).
- `gCoinColors[5]` @0x52d8c8 (init 0x4f3220), indexed by piece colour:
  (255,0,0), (0,148,255), (255,255,0), (0,255,0), (255,0,255).
- An identical copy of gCoinColors at 0x52d918 (init 0x4f3320, another translation unit) has no
  code references.
- `gRankThresholds[27]` @0x4f53c0 (.rdata): 0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
  10000, 11500, 13000, 15000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 100000, 125000,
  150000, 200000, 300000, 500000.
- `gRankNames` @0x52d864: `std::vector<std::string>` filled in Board's ctor (0x40e760) from
  `app->GetStringVector("gRankNames")` (properties/default.xml, 27 entries, Pauper .. Infinite Power);
  first-element pointer at 0x52d868, 28-byte strings.

The coin colour order (rows of coins.gif) is therefore red, blue, yellow, green, purple.

#### 1.4 Composite images built by the app (WinBMApp::LoadingThreadProc 0x41efd0, Init 0x41d9a0)

The drawing code uses these app-owned images; their construction is the app's, summarised here:
- `app+0x6e8` mBrightCoinsImage: MemoryImage copy of img[23] coins, every pixel channel c (r, g, b)
  becomes `min(255, c*280/255 + 48)` (unsigned int maths), alpha kept. Used for highlighted coins.
- `app+0x7c8 + i*4`, i = 0..5, mMrMoneyImages[6]: MemoryImage 184x136 (size of img[55]),
  SetImageMode(true,true); draws img[55] mrmoney at (0,0), then img[56] mouths
  src (0, 136*i, 184, 136) at (0,0); for i == 5 also mouths src (0, 0, 184, 136) at (0,0). So frames
  0..4 are mouth shapes, frame 5 = row 5 of mouths.gif (closed eyes) plus mouth row 0.
- `app+0x7e0 + k*4`, k = 0..3, mBillImages[4]: MemoryImage (int)(1920*s) x (int)(50*s) for
  s = 0.9, 0.8, 0.7, 0.6 = 1728x45, 1536x40, 1344x35, 1152x30, SetImageMode(true,true), and
  `DrawImage(img[57], Rect(0,0,w',h'), Rect(0,0,1920,50))` (stretched, mFastStretch false).
  The Board indexes them as `app+0x7dc + scale*4` for scale 1..4 (scale 0 = img[57] itself).

### 2. Board drawing

#### 2.1 Board fields read by the drawing code

Field names are board_logic.h's (reconciled; see 6.1 for the differences that remain).
sizeof(Board) = 0x810 (`operator new(0x810)` in 0x41bfb0; the Board is Resize(0, 0, app->mWidth,
app->mHeight), i.e. 640x480 at the origin). Initial values are from the ctor 0x40e760. Writers are
the functions that store the field (RW = read and written).

| off | type | name (board_logic.h) | meaning (draw side) | init | writers |
|---|---|---|---|---|---|
| 0x84 | WinBMApp* | mApp | | ctor arg | ctor |
| 0x94 | int | mStatsSeconds | game time in seconds; set by ShowContinueButton to `(timeGetTime()-mTimerTick)/1000 + mPlaySeconds` | 0 | 4045e0, 408a60, 40b120 |
| 0xa0 | int | mState | BoardState; Draw tests 1 STATE_LEVEL_DONE, 5 STATE_GAME_OVER, 6 STATE_GAME_OVER_SCORES | 0 | logic |
| 0xa8 | Piece*[15][14] | mGrid | grid, row 0 = top, `[row][col]` at 0xa8 + row*0x38 + col*4 | | logic |
| 0x3f0 | Piece*[14] | mNextRow | the row rising from below (drawn with y offset mMintPixels) | | logic |
| 0x430 | int | mCols | 14 | 14 | ctor |
| 0x434 | int | mRows | 15 | 15 | ctor |
| 0x438 | int | mMintPixels | extra y added to mNextRow pieces (hidden part of the minting row) | 0 | 4088a0, 40a110, 40c9f0 |
| 0x468 | Color | mMovesColor | colour of the bottomline image; Update animates it | (128,128,128,255) | ctor, 40c9f0 |
| 0x490 | int | mScore | real score (rank) | 0 | 407ea0 |
| 0x494 | int | mDisplayScore | rolling score shown as "$%d" | 0 | 40c9f0 |
| 0x4a0 | std::list<Points*> | mPointsWidgets | live Points widgets (section 4) | | 407ea0, 408110, 40c9f0 |
| 0x4d8 | int | mMeterCoins | coins in the stack meter | 0 | 40a110, 40c640 |
| 0x4e0 | double | mMeterDrained | drained part: the meter shows `mMeterCoins - mMeterDrained` coins; Update adds 0.5/tick while it is below mMeterTarget (+0x4d0, double) in states 1 and 4 | 0 | 40a110, 40c640, 40c9f0 |
| 0x510 | std::list<Piece*> | mLoosePieces | pieces outside the grid (falling, dropped bags...) | | logic |
| 0x51c | std::list<Particle> | mParticles | 0x38-byte elements (2.3) | | logic |
| 0x528 | std::list<Bill> | mBills | 0x20-byte elements (2.3) | | logic |
| 0x534 | std::list<TubeCoin> | mTubeCoins | 0x18-byte elements (2.3), coins flying into the stack | | logic |
| 0x550 | int | mBagsBanked | | 0 | 409520, 40ae20 |
| 0x554 | int | mMeterFlashFrame | barbagdisplay cell 0..3 | 0 | 40c640, 40c9f0 |
| 0x55c | int | mBagsNeeded | number of bag slots | 1 | 401490 |
| 0x560 | bool | mLevelUpActive | "LEVEL UP!" visible | 0 | 4083d0 |
| 0x568 | double | mLevelUpY | | 0 | 4083d0, 40c9f0 |
| 0x570 | double | mLevelUpAngle | rotation while bouncing (radians) | 0 | 4083d0, 40c9f0 |
| 0x578 | double | mLevelUpUnused | not read by draw | 0.03 | |
| 0x580 | double | mLevelUpStretch | rows of the squashed image (189 = unsquashed); 189 at start, Update lowers it by 0.75 to 150 | 0 | 4083d0, 40c9f0 |
| 0x588 | bool | mLevelUpBouncing | bounce phase: draw rotated; else squashed | | 4083d0, 40c9f0 |
| 0x5b8 | int | mLevel | 0-based, shown +1 | 0 | 40ae20 |
| 0x5c8 | double | mPauseFade | 0..1, Update +-0.05/tick | 0 | 40c9f0 |
| 0x5d0 | int | mPauseAnim | +1 per tick while mPauseFade > 0 | 0 | 40c9f0 |
| 0x5d4 | bool | mPauseClickable | Update: `app->GetDialogCount() == 0` while paused | 0 | 40c9f0 |
| 0x5d8 | int | mNoMovesY | +5/tick to 500 in state 6 | 0 | 40c9f0 |
| 0x5e0 | bool | mBannerVisible | show COLLECT_X_MONEY_BAGS_TO_ADVANCE | 0 | 40c9f0 |
| 0x628 | bool | mCountdownActive | "3, 2, 1" warning countdown | 0 | 401d30, 40c9f0 |
| 0x630 | double | mCountdownValue | integer part = digit cell, fraction drives the zoom | 0 | 40c9f0 |
| 0x63c | DialogButton* | mContinueButton | | | 4045e0 |
| 0x640 | int | mButtonBlink | cleared by ShowContinueButton | | 4045e0 |
| 0x644 / 0x648 | int | mLevelCoinsCleared / mGameCoinsCleared | stats panel shows the game value when mState == 6, else the level value | | logic |
| 0x64c / 0x650 | int | mLevelBiggestBonus / mGameBiggestBonus | same selection | | logic |
| 0x654 / 0x658 | int | mLevelBiggestCombo / mGameBiggestCombo | same selection | | logic |
| 0x78c | bool | mMoveDigitActive | one-shot countdown digit, set at 0x40c567 (OnClick) when mMovesLeft (+0x788) is 1..3 | 0 | 40b120, 40c9f0 |
| 0x790 | int | mMoveDigit | countdown cell 0..2 (= 3 - mMovesLeft) | | 40b120 |
| 0x798 | double | mMoveDigitFade | 1.0 at start, -0.01/tick, flag cleared at <= 0 | | 40b120, 40c9f0 |
| 0x7a0 | int | mClipExtra | added to the clip width and the right-hand HUD x; only ever 0 | 0 | ctor |
| 0x7bc | MemoryImage* | mStatsImage | stats panel, rendered once (2.7) | NULL | Draw, 4045e0 |
| 0x7c0 | MemoryImage* | mStatsFadeImage | wobbled copy actually drawn | NULL | Draw, 4045e0 |
| 0x7c4 | bool | mStatsDirty | recompute mStatsFadeImage this frame | 0 | 4045e0, 4083d0, 40c9f0, Draw |
| 0x7c8 | int | mHighScoreDelay | (not read by Draw) ticks before mHighScoreSlide starts moving | | 40c9f0 |
| 0x7cc | int | mHighScoreSlide | game over slide: 0, then -2/tick to -146 in state 6 (also positions mHighScoreWidget, +0x7dc) | 0 | 40c9f0 |
| 0x7d0 | double | mStatsFade | 0..1, +0.015/tick in states 1, 5, 6 | 0 | 401d30, 4045e0, 4083d0, 40c9f0 |
| 0x7d8 | MemoryImage* | mLevelUpImage | squashed levelup copy | NULL | Draw, 4045e0, 4075d0 |
| 0x7e0 | double | mBagGlow | 0..1 phase of the bag-piece glow | 0 | 408a60 |
| 0x7e8 | bool | mHintVisible | tutorial arrow + text | 0 | 401a20, 401a80, 401fe0, 4075d0 |
| 0x7e9 | bool | mHintAlt | use the bent arrow | | 401a20, 401fe0, 409520 |
| 0x7f0 | int | mHintId | row of instructotext.gif (HintId 1..8) | | 401a20, 401fe0 |
| 0x7f4 | int | mHintX | target point | 0 | 401a20, 401fe0, 40c9f0 |
| 0x7f8 | int | mHintY | | 0 | 401a20, 401fe0, 40c9f0 |
| 0x804 | int | mBagFlashCount | pulses left (Update) | 0 | 40c9f0 |
| 0x808 | double | mBagFlash | 0..1, +0.02/tick while mBagFlashCount > 0 or phase != 0 | 0 | 40c9f0 |

The widget fields used: mWidth (+0x34), mWidgetManager (+0x10).

#### 2.2 Piece fields read by the drawing code (sizeof(Piece) = 0xa8, `operator new(0xa8)` at 0x40771c; owned by board_logic)

| off | type | meaning (draw side) |
|---|---|---|
| 0x08 | double | mX: screen x of the top-left corner (Board coordinates) |
| 0x10 | double | mY |
| 0x28 | int | mShakeX: shake offset (Update sets it from MTRand jitter at 0x40d65b) |
| 0x2c | int | mShakeY |
| 0x38 | bool | mHighlighted (hovered group): coin glow + bright coin image |
| 0x40 | bool | mDanger (danger flash phase): bright coin image without glow |
| 0x41 | bool | mBlinkHidden: DrawPiece draws nothing |
| 0x58 | int | mColor 0..4 (coins row, colour tables) |
| 0x60 | bool | mIsSpecial (not a coin) |
| 0x64 | int | mSpecialType: 0 money bag, 1 poof (sparkle), 2 dollar bomb, 3 explosion |
| 0x68 | int | mImageSet: 1 = horzcoins strip, else coins |
| 0x6c | int | mFrame (cell index) |
| 0x70 | bool | mFlying: (mLoosePieces only) drawn unclipped |

#### 2.3 List element layouts seen from Draw (std::list node = next, prev, value at +8)

- `Particle` (0x38 bytes, node 0x40): +0x08 double mX, +0x10 double mY, +0x30 int mFrame, +0x34 bool
  mBright (read and tested by Draw but both branches draw the same thing).
- `Bill` (0x20 bytes, node 0x28): +0x00 double x, +0x08 double y, +0x18 int frame (0..15),
  +0x1c int scale (0 = full size, 1..4 = app+0x7dc + scale*4).
- `TubeCoin` (0x18 bytes, node 0x20): +0x00 double mY, +0x10 int mFrame (0..9).

#### 2.4 `void Board::Draw(Graphics* g)` (0x405480, slot 13)

Unless stated, everything is drawn on `g`. Draw mode NORMAL and colourize off unless stated.

1. `Graphics gc(*g); gc.ClipRect(175, 4, mClipExtra + 409, 467);` (the playfield clip; with
   mClipExtra = 0 the rect is x 175..583, y 4..470).
2. **Playfield**, only if `mPauseFade < 1.0`:
   1. `DrawImage(img[18] backdrop, 178, 4)`.
   2. `SetColor(mMovesColor); SetColorizeImages(true); DrawImage(img[53] bottomline, 156, 432);
      SetColorizeImages(false)`. (The colour stays set on g.)
   3. Coin glows, row-major (row 0..mRows-1, col 0..mCols-1 inner), for every non-NULL
      `p = mGrid[row][col]`: `x = (int)((double)p->mShakeX + p->mX); y = (int)((double)p->mShakeY + p->mY)`
      (computed for every piece); if `p->mHighlighted`: `SetColorizeImages(true); SetDrawMode(ADDITIVE);
      SetColor(gCoinLightColors[p->mColor]); DrawImage(img[54] coinglow, x-8, y-8);
      SetDrawMode(NORMAL); SetColorizeImages(false)`. Neither the hidden flag (+0x41) nor the item
      flag (+0x60) is tested here, and mNextRow / mLoosePieces get no glow.
   4. Pieces, on `gc`, column-major: for col = 0..mCols-1 { for row = mRows-1 down to 0:
      if `mGrid[row][col]` `DrawPiece(&gc, mGrid[row][col], 0)`; then if `mNextRow[col]`
      `DrawPiece(&gc, mNextRow[col], mMintPixels)` }.
   5. `mLoosePieces` in list order: `DrawPiece(p->mFlying ? g : &gc, p, 0)`.
   6. `mBills` in list order (`scale` = Bill::mType): `im = scale == 0 ? img[57] : app[0x7dc + scale*4]`; `W = im->mWidth`,
      `x0 = W*frame/16`, `x1 = W*(frame+1)/16` (signed int, values >= 0);
      `DrawImage(im, (int)b.x, (int)b.y, Rect(x0, 0, x1-x0, im->mHeight))`.
3. **Pause overlay**, if `mPauseFade > 0.0`:
   1. `SetColorizeImages(true); SetColor(Color(255,255,255,(int)(mPauseFade*255.0)))`.
   2. `w,h` = size of img[61] pausebackdrop (489x374); `off = (mPauseAnim/8) % h` (both signed);
      draw at x = mWidth - w (151) and y = -off, -off+h, -off+2h (three DrawImage calls).
   3. If mPauseClickable: `SetFont(app+0x6ec)`;
      `s = GetString("GAME_PAUSED")`, `DrawString(s, 380 - SW(s)/2, 210)`;
      `s = GetString("CLICK_HERE_TO_CONTINUE")`, `DrawString(s, 380 - SW(s)/2, 250)`.
      The text uses the current colour (white with the fade alpha).
   4. `SetColorizeImages(false)`.
4. **HUD** (always):
   1. `DrawImage(img[17] leftbar, 0, 0)`.
   2. `r = (app->mGameType(+0x818)*3 + app->mSkillLevel(+0x814))*15 - 15`;
      `DrawImage(img[58] gamedifftype, 35, 161, Rect(0, r, 100, 15))` (game type 0 action,
      1 strategy, 2 puzzle; skill 1..3 easy/normal/hard).
   3. `DrawImage(img[60] floatlogo, 15, 8)`.
   4. `DrawImage(img[19] rightbar1, mWidth - 56 + mClipExtra, 0)` (584 at 640 wide).
   5. Coin stack meter: `v = (double)mMeterCoins - mMeterDrained`; `a = (int)v`;
      `y = (int)((1.0 - (v - a)) * 6.0 + 440.0)` (x87: ((1 - (v - (double)a)) * 6) + 440).
      While `v > 0.0`: `n = (int)v + 1; if (n >= 372/6 = 62) n = 62` (the bound is `stackH/6`,
      int); `y -= n*6`; `DrawImage(img[22] stack, 595, y, Rect(0, 0, 40, n*6))`; `v -= n`.
      So a fractional stack rises smoothly by 6 px per coin; the strip is cut into chunks of at most
      62 coins.
   6. `mTubeCoins`: `DrawImage(img[21] coin, 595, (int)c.y, Rect(c.frame*40, 0, 40, 40))`.
   7. `DrawImage(img[20] rightbar2, mWidth - 80 + mClipExtra, 0)` (560).
   8. Score: `SetColor(255,255,255); SetFont(app+0x710 scorefont)`; `s = sprintf("$%d", mDisplayScore)`
      (no thousands separators); `DrawString(s, 140 - SW(s), 110)` (right-aligned at x 140).
   9. `DrawImage(img[41] barbagdisplay, 592 + mClipExtra, 4, Rect(mMeterFlashFrame*42, 0, 42, 43))`.
   10. `DrawImage(img[16] level, 589, 441)`; `SetFont(app+0x714 levelfont); SetColor(255,255,255)`;
       `s = sprintf("%d", mLevel+1)`; `DrawString(s, 616 - SW(s)/2, 471)`.
   11. Bag slots, i = 0..mBagsNeeded-1, `(px,py) = GetBagSlotPos(i)` (2.6):
       - i < mBagsBanked: `DrawImage(img[25] bag, px-2, py-8, Rect(99, 0, 33, 36))` (cell 3);
         then if `mBagFlash != 0.0`: `c = (int)((1.0 - 2*|mBagFlash - 0.5|) * 255.0)`
         (x87: |phase-0.5| doubled with fadd st,st, then 1.0 - that, times 255);
         `SetColorizeImages(true); SetColor(c,c,c); SetDrawMode(ADDITIVE);
         DrawImage(img[46] glowbag, px-17, py-17); SetDrawMode(NORMAL); SetColorizeImages(false)`.
         (All collected bags pulse together.)
       - else: `DrawImage(img[45] bagholder, px + (29-41)/2, py + (29-34)/2)` = (px-6, py-2),
         computed from the image size with int division.
5. **Overlays**, only if `mPauseFade == 0.0` (exactly); otherwise skip to step 6:
   1. If mLevelUpActive:
      - mLevelUpBouncing: `DrawImageRotated(img[43] levelup, 238, (int)mLevelUpY, mLevelUpAngle, NULL)`.
      - else (squash pixel effect): `src` = img[43] as MemoryImage (`__RTDynamicCast` Image to
        MemoryImage), `w,h` = its size (300x189). If mLevelUpImage == NULL: `new MemoryImage(app)`,
        `Create(w, h)`, `SetImageMode(true, true)`, `SetVolatile(true)`. `d = mLevelUpImage->GetBits()`,
        `s = src->GetBits()`. For r = 0..h-1: `sy = (int)((double)(r*h) / mLevelUpStretch)`
        (the numerator is an int accumulator stepping by h); if `sy < h` copy the w pixels of source
        row sy to the next destination row (the destination pointer advances only when a row is
        copied, so rows are packed from the top). Rows not written keep their previous contents
        (zero on a fresh image). `mLevelUpImage->BitsChanged()`;
        `DrawImage(mLevelUpImage, 238, (int)mLevelUpY)`. The effect squashes the image vertically
        to about mLevelUpStretch rows, top-aligned.
   2. If mCountdownActive (zooming digit): `w = 160` (img[40] width), `h3 = 480/3 = 160`,
      `cx = 380 - w/2 = 300`; `f = (int)mCountdownValue`;
      `k = (0.5 - |0.5 - (mCountdownValue - f)|) * 8.0`, `if (!(k < 1.0)) k = 1.0`;
      `dw = (int)(w*k) | 1; dh = (int)(h3*k) | 1`; `SetFastStretch(true)`;
      `DrawImage(img[40], Rect(cx + (w-dw)/2, 124 + (h3-dh)/2, dw, dh), Rect(0, f*h3, w, h3))`;
      `SetFastStretch(false)`. So each digit zooms in during the first 1/8 of its second and out
      during the last 1/8, centred on (380, 204).
   3. If mMoveDigitActive: same as 2 with `k = (0.5 - |mMoveDigitFade - 0.5|) * 8.0` (clamped to 1) and
      source cell `mMoveDigit` (Rect(0, mMoveDigit*h3, w, h3)).
   4. If mBannerVisible: `fmt = GetString("COLLECT_X_MONEY_BAGS_TO_ADVANCE")` ("Collect %d Money
      Bag%s to Advance!"); `sprintf(buf, fmt, mBagsNeeded, mBagsNeeded > 1 ? "s" : "")`;
      `SetFont(app+0x6ec); SetColor(255,255,255)`; `DrawString(buf, 380 - SW(buf)/2, 180)`.
   5. `mParticles` in list order: `SetDrawMode(ADDITIVE)`;
      `DrawImage(img[28] sparkle, (int)(s.x - 10.0), (int)(s.y - 10.0), Rect(s.frame*20, 0, 20, 20))`;
      `SetDrawMode(NORMAL)` (per sparkle). The bool at element +0x34 selects between two identical
      code paths.
   6. If mHintVisible (tutorial), with X = mHintX, Y = mHintY, choose
      (arrow cell a, arrow dx, arrow dy, text dx, text dy):
      - Y < 100: (1 up, -22, -6, -100, +38)
      - else X < 254: (4 left, -2, -24, +11, -25)
      - else X > 506: (2 right, -48, -24, -208, -25)
      - else mHintAlt: (0 bent, -40, -40, -100, -85)
      - else: (3 down, -22, -47, -100, -92)
      `DrawImage(img[63] instructoarrows, X+adx, Y+ady, Rect(0, a*50, 50, 50))`;
      `DrawImage(img[64] instructotext, X+tdx, Y+tdy, Rect(0, mHintId*50, 200, 50))`.
   7. If mState == 6: `DrawImage(img[39] gameover, 221, (int)((double)mHighScoreSlide * 1.5))`.
   8. If (mState == 5 || mState == 6) && app->mGameType == 2:
      `DrawImage(img[70] nomoremoves, 216, mNoMovesY + 80)`.
   9. If mState == 6 || mState == 1: the stats panel.
      - If mStatsImage == NULL: `mStatsImage = new MemoryImage(app); Create(396, 263);
        SetImageMode(true, true)`; `Graphics sg(mStatsImage); DrawStatsPanel(&sg)` (2.7);
        `mStatsFadeImage = new MemoryImage(app); Create(396, 263); SetImageMode(true, true);
        SetVolatile(true)`. (The panel content is rendered once and then frozen.)
      - If mStatsDirty (wave pixel effect): `s = mStatsImage->GetBits(); d = mStatsFadeImage->GetBits();
        a = (int)(mStatsFade * 256.0)`. For y = 0..263-1: `srow = s + 396*y; drow = d + 396*y;
        off = (int)(sin((double)y*0.08 + mStatsFade*8.0) * (1.0 - mStatsFade) * 64.0)`
        (x87 order: y*0.08 + t*8.0, fsin, times (1.0 - t), times 64.0; `sin` is the x87 `fsin`
        instruction, not a CRT call); if off < 0 { srow += -off; n = 396 + off } else
        { drow += off; n = 396 - off }; for n pixels `*drow = ((((*srow >> 24) * a) >> 8) << 24) |
        (*srow & 0xffffff)` (unsigned). Pixels of drow not covered keep the previous frame's
        values (zero at first). `mStatsFadeImage->BitsChanged(); mStatsDirty = false`.
        The panel therefore fades in (alpha a/256) while a horizontal sine wave of amplitude
        64*(1-t) settles; at t = 1.0 it is an exact copy.
      - `DrawImage(mStatsFadeImage, 182, 162 + mHighScoreSlide)` (the code tests mHighScoreSlide == 0
        separately but draws the same position).
6. `gc` is destroyed.

#### 2.5 `void Board::DrawPiece(Graphics* g, Piece* p, int theYOffset)` (0x402060)

Only caller: Draw. `sx = p->mShakeX, sy = p->mShakeY` (ints).
- If `p->mBlinkHidden` (hidden): return.
- If `p->mIsSpecial` (item), by `p->mSpecialType`:
  - 0, money bag: `cw = img[25].W/4` (33), `ch = img[25].H` (36);
    `x = (int)((double)((29-cw)/2 + sx) + p->mX)`, `y = (int)((double)((29-ch)/2 + sy + theYOffset) + p->mY)`
    (x87: fild of the int part then fadd of the double); `(29-33)/2 = -2`, `(29-36)/2 = -3`.
    `g->DrawImage(img[25], x, y, Rect(p->mFrame*cw, 0, cw, ch))`.
    Then, unless `app->mGameType == 2`: `c = (int)(|mBagGlow - 0.5| * 510.0)`;
    `SetColorizeImages(true); SetColor(c,c,c); SetDrawMode(ADDITIVE);
    DrawImage(img[46] glowbag, (int)(p->mX - 17.0), (int)(p->mY - 12.0)); SetDrawMode(NORMAL);
    SetColorizeImages(false)` (the glow ignores shake and theYOffset).
  - 2, dollar bomb: `cw = img[27].W/32` (33), `ch = 33`; `SetColorizeImages(true);
    SetColor(gCoinColors[p->mColor])`; `x = (int)((double)((29-cw)/2 + sx) + p->mX)`,
    `y = (int)((double)((29-ch)/2 + sy) + p->mY + theYOffset)` (here the double is added before the
    int offset); `DrawImage(img[27], x, y, Rect(p->mFrame*cw, 0, cw, ch)); SetColorizeImages(false)`.
  - 3, explosion: `cw = img[26].W/15` (45), `ch = 91`; `SetDrawMode(ADDITIVE)`;
    x, y as for the dollar ((29-45)/2 = -8, (29-91)/2 = -31);
    `DrawImage(img[26], x, y, Rect(p->mFrame*cw, 0, cw, ch)); SetDrawMode(NORMAL)`.
  - 1 PIECE_POOF (and any other value), sparkle: `cw = img[28].W/14` (20), `ch = 20`; offsets (29-20)/2 = 4;
    `DrawImage(img[28], x, y, Rect(p->mFrame*cw, 0, cw, ch))`, NORMAL mode.
- Else (coin): `im = img[23]` coins; if `p->mImageSet == 1` `im = img[24]` horzcoins; else if
  `p->mHighlighted || p->mDanger` `im = app+0x6e8` bright coins.
  `x = (int)((double)sx + p->mX)`, `y = (int)((double)sy + p->mY + theYOffset)`;
  `DrawImage(im, x, y, Rect(p->mFrame*29, p->mColor*29, 29, 29))`.

#### 2.6 `void Board::GetBagSlotPos(Point* thePos, int i)` (0x401e30)

Writes the Point to thePos and returns thePos in eax (`ret 8`). Callers: Draw and 0x408a60 (logic: flying bag
target). `perRow = mBagsNeeded > 6 ? 4 : 3; row = i / perRow; col = i % perRow;
inRow = min(mBagsNeeded - row*perRow, perRow)`;
`x = col*37 - (inRow*37 - 4)/2 + 85; y = row*40 + 232` (signed int maths). The slots are 37 px
apart; with 29-px cells each row is centred on x of about 83 (for 3 bags: x = 32, 69, 106).

#### 2.7 `void Board::DrawStatsPanel(Graphics* g)` (0x404950)

Only caller: Draw (once per panel, into the 396x263 mStatsImage). `k = (mState == 6) ? 1 : 0`.
1. `g->DrawImageBox(Rect(0, 0, 396, 263), img[29] olddialog)`.
2. `SetFont(app+0x6ec); SetColor(255,163,0)`; labels at x 46:
   `GetString("GAME_TIME")` y 50 (both branches of a `mState == 6` test fetch the same id),
   `"BIGGEST_COIN_COMBO"` y 72, `"BIGGEST_BONUS"` y 94, `"COINS_CLEARED"` y 116.
3. `SetColor(255,70,39)`; values right-aligned at x 355 (`DrawString(s, 355 - SW(s), y)`):
   `sprintf("%d min %02d sec", mStatsSeconds/60, mStatsSeconds%60)` y 50; `"%d"` (k ? mGameBiggestCombo : mLevelBiggestCombo) y 72;
   `"%d"` (k ? mGameBiggestBonus : mLevelBiggestBonus) y 94; `"%d"` (k ? mGameCoinsCleared :
   mLevelCoinsCleared) y 116 (the code indexes the level/game pair with k).
4. Rank: `next = 0; for (r = 26; r >= 0; r--) { if (mScore >= gRankThresholds[r]) break; next =
   gRankThresholds[r]; }`, rank = r (0 if the loop runs out). So `next` is the smallest threshold above
   the score, 0 at the top rank.
   `DrawImage(img[38] rankstrip, 53, 125, Rect(0, rank*100, 100, 100))`.
5. `SetFont(app+0x700 bigmoneyfont3); SetColor(0,212,15)`; `sprintf("%s", gRankNames[rank])` at
   (168, 167).
6. If next != 0: `SetColor(0,141,0)`; `sprintf(GetString("NEXT_RANK_AT_X_PTS"), next)` ("Next Rank
   at %d pts", plain %d) at (168, 195).

#### 2.8 `void Board::ShowContinueButton()` (0x4045e0)

Called by 0x4075d0 (load), 0x409520 (level complete) and Update (game over). Not a draw function
but it prepares the panel:
1. `mStatsSeconds = (timeGetTime() - mTimerTick)/1000 + mPlaySeconds` (unsigned divide of the tick difference).
2. `mContinueButton = new DialogButton(img[33] dialogbutton, 0, this+0x80 (ButtonListener))`
   (0x130 bytes); `SetFont(app+0x700)` (ButtonWidget slot 55);
   `SetColor(0 /*COLOR_LABEL*/, Color(0,255,0))` (slot 19);
   `mLabel = GetString(mState == 6 ? "CLICK_HERE_TO_PLAY_AGAIN" : "CLICK_HERE_TO_PLAY_NEXT_LEVEL")`;
   `Resize(197, 444, 366, 27)`; `mMouseInsets = Insets(-10,-10,-10,-10)`; `mDoFinger = true`;
   `mWidgetManager->AddWidget(mContinueButton)` (top level, not a Board child).
3. `mButtonBlink = 0`; delete mStatsImage, mStatsFadeImage, mLevelUpImage (virtual deleting dtors) and set
   them NULL; `mStatsFade = 0.0; mStatsDirty = true; MarkDirty()`.

### 3. MrMoney (Mr. Moneybags, `MrMoney.cpp`)

Widget subclass, no ButtonListener. sizeof 0xd8 (`operator new(0xd8)` in WinBMApp 0x4217a3).
Created once by WinBMApp::TitleScreenIsFinished (slot 96, 0x4215c0): `app->mMrMoney (+0x6d4) = new MrMoney(app)`,
`Resize(0, 0, 184, 136)`, `Move(-200, 0)`, added to the WidgetManager. vftable 0x4fb63c.

#### 3.1 Layout

| off | type | name | meaning | init | readers / writers |
|---|---|---|---|---|---|
| 0x80 | WinBMApp* | mApp | | ctor arg | |
| 0x88 | double | mRot | current rotation (radians) | 0.0 | Update, Draw |
| 0x90 | double | mRotVel | angular velocity | 0.004 | Update |
| 0x98 | int | mTargetX | spring target x | 0 | Move; written directly by Board Update (0x40cb0e, 0x40cb72, 0x40d00b) and WinBMApp::ShowGameSelector (0x41b840: 0, 290) |
| 0x9c | int | mTargetY | spring target y | 0 | Move; Board Update (290; 24), ShowGameSelector (290) |
| 0xa0 | double | mPosX | current x | 0.0 | Move, Update |
| 0xa8 | double | mPosY | current y | 0.0 | Move, Update |
| 0xb0 | double | mVelX | | 0.0 | Update |
| 0xb8 | double | mVelY | | 0.0 | Update |
| 0xc0 | int | mAnim | playing animation, -1 none | -1 | PlayAnim, Update; read by GameSelector (0x40fd7e) |
| 0xc4 | int | mAnimTick | ticks in the current step | 0 | PlayAnim, Update |
| 0xc8 | int | mAnimStep | step index | 0 | PlayAnim, Update |
| 0xcc | int | mFrame | frame 0..5 (app+0x7c8 image) | 0 | PlayAnim, Update, Draw |
| 0xd0 | bool | mIdleWoke | the 120 s idle wake-up already fired | true | Update |
| 0xd1-0xd7 | | padding | | | |

Widget fields set in the ctor: mMouseVisible (+0x4d) = false, mHasAlpha (+0x3c) = true,
mHasTransparencies (+0x52) = true.

#### 3.2 Functions

- `MrMoney(WinBMApp* theApp)` (0x415140): Widget(), the inits above.
- `~MrMoney()` (0x4151f0, body only: vftable, then Widget::~Widget 0x424d80); deleting dtor 0x415540 (slot 0).
- `void PlayAnim(int theAnim)` (0x415200): `mAnim = theAnim; mAnimStep = 0; mAnimTick = 0;
  mFrame = gMrMoneyAnims[theAnim][0].mFrame; MarkDirty()`. Callers: Board logic (0x4083d0,
  0x408650, 0x408a60, 0x409520, 0x409e00, 0x40b120, 0x40c9f0), GameSelector
  (0x40fcb0, 0x40fe90), HighScoreWidget (0x4133b0).
- `void Move(int x, int y)` (0x415240, slot 26): `Widget::Move(x, y); mTargetX = mX; mTargetY = mY;
  mPosX = (double)mX; mPosY = (double)mY` (teleport: the spring is at rest at the new place, the
  velocities are kept).
- `void Update()` (0x415290, slot 9; Ghidra's function size of 1 byte is wrong, the body runs to
  0x4154ce):
  1. `Widget::Update()`.
  2. Idle wake-up: `idle = mWidgetManager->mUpdateCnt (+0x24) - mWidgetManager->mLastInputUpdateCnt (+0xc8)`
     (unsigned). If `idle > 12000` (120 s without input): if !mIdleWoke { `app->PlaySample(45 "wakeup")`
     (slot 49); `PlayAnim(22)`; `mIdleWoke = true` }. Else `mIdleWoke = false`.
  3. Rotation spring: `mRot = mRot + mRotVel; mRotVel = mRotVel - mRot*0.0004` (the new mRot).
  4. Position spring (x87 order):
     `ax = ((double)mTargetX - mPosX) * 0.0025; mVelX = (mVelX + ax) * 0.94;`
     `mRot = mRot*0.99 + ax*0.01;` (the swing follows the horizontal acceleration)
     `mVelY = (mVelY + ((double)mTargetY - mPosY) * 0.0025) * 0.94;`
     `mPosX += mVelX; mPosY += mVelY`.
  5. `if ((app->mHasFocus (+0x498) && mUpdateCnt % 8 == 0) || |mVelX| > 1.0 || |mVelY| > 1.0)
     MarkDirtyFull()` (slot 6).
  6. `Resize((int)mPosX, (int)mPosY, mWidth, mHeight)` (slot 25) every tick.
  7. Animation: if mAnim != -1: `mAnimTick++`; if `mAnimTick >= gMrMoneyAnims[mAnim][mAnimStep].mDuration * 2`:
     `mAnimStep++; mAnimTick = 0; mFrame = gMrMoneyAnims[mAnim][mAnimStep].mFrame;
     if (mFrame == -1) { mAnim = -1; mFrame = 0; } MarkDirty()`.
     So one duration unit is 2 ticks (20 ms).
  8. Random idle animations, CRT `rand()` (0x4d9372, the MSVC LCG `holdrand*214013+2531011`,
     `>>16 & 0x7fff`), in this order every tick:
     `r1 = rand(); if (r1 % 3000 == 0 && mAnim == -1) PlayAnim(24);`
     `r2 = rand(); if (r2 % 6000 == 0 && mAnim == -1) PlayAnim(rand() % 25 + 25);`
     (the third rand() is drawn only when the second condition holds). Two or three CRT rand()
     draws per MrMoney tick, interleaved with the Board's own rand() users (its ctor seeds its
     MTRand with `rand()*0xffff`), so the CRT stream is shared.
- `void Draw(Graphics* g)` (0x4154d0, slot 13): `im = app->mMrMoneyImages[mFrame]` (app+0x7c8 +
  mFrame*4). If `|mRot| < 0.005`: `g->DrawImage(im, 0, 0)`; else `g->DrawImageRotated(im, 0, 0,
  mRot, NULL)` (centre 92, 68).

#### 3.3 `gMrMoneyAnims[50][50]` (0x4f6818, .rdata, 0x190 bytes per animation)

Each step is `{int mFrame; int mDuration;}`; frame -1 ends the animation (its duration is 0).
Written as frame:duration (duration in units of 2 ticks):

```
 0: 0:3 1:7 4:9 1:10 2:8
 1: 3:4 4:6 0:5 3:5 4:5 0:6 2:9
 2: 2:2 1:8 2:2 4:7 3:6 0:5 2:13
 3: 0:3 1:8 2:5 3:6 4:10 2:6 1:5 3:8 4:6 2:5 1:5 4:6 4:10 2:25
 4: 2:3 1:5 2:2 3:11 2:6
 5: 1:11 2:2 3:3 2:6
 6: 2:3 1:15 3:3 2:6
 7: 0:3 1:16 2:7 3:4 4:4 3:4 2:11
 8: 0:1 2:3 1:17 4:7 2:14 3:7 4:15 0:6 2:25
 9: 0:1 1:9 2:4 0:4 4:5 3:9 2:4 0:4 1:7 2:5 3:4 0:10 4:10 0:4 1:4 3:3 4:4 0:4 2:9 4:6 0:9 2:19
10: 1:1 2:4 1:10 2:4 3:5 4:7 0:5 2:16
11: 4:7 3:5 2:4 1:6 3:6 0:3 3:6 2:17
12: 3:3 1:8 3:4 4:9 3:3 0:3 2:4 1:11 2:43
13: 2:5 1:10 2:9 0:6 3:4 4:5 0:4
14: 2:4 3:4 4:3 1:15 4:3 0:4 2:21
15: 1:22 2:11 3:5 1:10 4:3 3:3 2:18
16: 2:4 1:8 2:9 3:5 4:6 2:7 0:4 3:5 1:6 0:5 2:22
17: 0:1 2:2 0:2 1:6 2:2 3:4 4:5 0:4 2:11
18: 2:8 1:9 2:5 3:6 1:10 2:7 1:7 4:9 3:10 0:7 2:3
19: 3:3 4:4 1:10 3:4 0:3 2:3 3:2 4:2 1:6 0:3 4:6 0:2 2:15
20: 2:2 1:4 3:2 2:2 1:5 0:3 2:2 1:5 3:4 0:2 2:5 3:7 0:2 2:19
21: 1:7 2:4 1:6 2:3 3:3 4:3 3:3 0:3 1:6 0:2 2:6
22: 2:3 1:6 0:3 3:3 0:3 2:7
23: 1:7 3:5 4:6 1:10 2:6 1:10 2:7 1:7 3:6 4:9 0:8 1:6 3:7 4:7 0:5 2:29
24: 5:5
25: 5:5 0:5 5:5
26: 5:5 2:82 5:5
27: 5:5 4:2 3:2 2:9 3:2 4:2 5:3
28: 3:8 4:13 5:8
29: 1:18 2:18 3:10 5:6
30: 3:6 2:20 1:11 3:14 5:5
31: 3:39 5:8
32: 4:3 1:6 4:2 5:4
33: 3:35 2:26 5:7
34: 3:55 2:45 5:7 0:6 5:5 0:19 2:48 5:6 3:5 3:3
35: 5:21 4:11
36: 4:3 3:3 2:26 3:3 4:3 5:4
37: 1:50 2:15 5:5
38: 5:5 0:3 5:4 0:3 5:4
39: 5:25 1:6 2:26 5:4 0:3 5:4 0:3 5:4
40: 2:119 4:2 0:2 4:2 5:4 0:3 5:4
41: 4:14 3:20 5:5
42: 5:3 0:2 5:3 0:2 5:3 0:2 5:3 3:28
43: 1:9 0:18 4:6 5:3
44: 3:55 5:5 0:3 5:4 0:3 5:3
45: 3:7 1:17 3:5 2:6 5:3 0:2 5:2
46: 5:15 0:20 5:16
47: 1:7 2:20 5:12
48: 5:29 0:3 5:11 0:4 5:7 0:5 5:5 0:2 5:4 0:6 5:2
49: 5:9 0:5 5:3 0:2 5:3
```

Animations 0..23 are lip-sync sequences (frames 0..4) for the speech samples, 24 is a blink, 25..49
are idle expressions (rolled by Update as 25 + rand()%25). Every PlayAnim call site plays its
sample next to it (sound name in brackets): 0/1/2 game type (action/strategy/puzzle, GameSelector,
anim = sample - 42), 3 (anewhighscore, HighScoreWidget 0x413434), 4/5/6 (three/two/one: Board
0x40c550 `PlayAnim(7 - mMovesLeft)` with sample `35 - mMovesLeft`, and 0x40d518
`PlayAnim((int)(t + 4.0))` with sample `(int)(t + 32.0)`), 7 (awesome), 8 (bigmoney), 9 (loveit),
10 (bonus), 11 (excellent), 12 (gameover), 13 (getready), 14 (go), 15 (goodjob), 16 (incredible),
17 (levelup), 18 (no caller; probably megamoney), 19 (speedbonus), 20 (superbonus),
21 (unbelievable), 22 (wakeup, MrMoney's own idle wake), 23 (welcome, GameSelector).
When the last step ends, mFrame returns to 0.

### 4. Points (floating text, `Points.cpp`)

Widget subclass; sizeof 0xac (`operator new(0xac)` at 0x407f23 and 0x408160). vftable 0x4fba54.
Points has no Update override: Board::Update moves and ages them (below).

| off | type | name | meaning | init |
|---|---|---|---|---|
| 0x80 | WinBMApp* | mApp | | arg 1 |
| 0x84 | int | mLife | ticks left; alpha = min(mLife/18, 1) | arg 6 |
| 0x88 | std::string | mText | | arg 3 |
| 0xa4 | Font* | mFont | | arg 2 |
| 0xa8 | int | mHue | HSL hue 0..254 | 0 |

- `Points(WinBMApp* theApp, Font* theFont, const std::string& theText, int theX, int theY,
  int theLife, int theJustify)` (0x417630, `ret 0x1c`): Widget(); mMouseVisible = false; stores
  the fields; `w = theFont->StringWidth(theText)`; `x = theX - w/2` if theJustify == 0,
  `theX - w` if theJustify == 1, else theX; `y = theY - theFont->GetAscent()`;
  `Resize(x, y, w, theFont->GetHeight())` (Widget::Resize 0x424500 called directly).
- `~Points()` (0x4175c0): frees mText, Widget::~Widget; deleting dtor 0x417610 (slot 0).
- `void Draw(Graphics* g)` (0x4174f0, slot 13): `Widget::Draw(g)` (empty, folded stub 0x476c80);
  `a = (double)mLife * (1/18.0)` (constant 0.0555..., multiplied, not divided), `if (a > 1.0) a = 1.0`;
  `rgb = app->HSLToRGB(mHue, 255, 128) & 0xffffff`; `g->SetColor(Color(rgb | ((int)(a*255.0) << 24)))`
  (Color(int) ctor 0x42b470: an alpha byte of 0 would become 255); `SetFont(mFont)`;
  `DrawString(mText, 0, mFont->GetAscent())`.
- Board side (read for context; board_logic owns it): Board::Update (0x40dff6) for each Points
  in mPointsWidgets: `MarkDirty(); mHue = (mHue + 7) % 255; if (Board mUpdateCnt % 2 == 0 && mY > 0)
  Move(mX, mY - 1); if (--mLife <= 0) { if (mLastBonusText == it) mLastBonusText = NULL; erase;
  mWidgetManager->RemoveWidget(it); app->SafeDeleteWidget(it) (slot 32); }`.
  Creation sites: 0x407ea0 (score popups: `pts = (int)((double)base * mPointsMult)`, text
  `"+%d"` of pts, floatfont app+0x718, centred (justify 0) at (Board mX + argX, Board mY + argY),
  life `clamp((int)((double)base * 0.6666666666666666), 100, 600)` (base = the int argument, kept
  in st(1) across the first `_ftol`)) and 0x408110 (messages: font app+0x6ec, x 212,
  `y = Board mY + max(mBannerTimer > 0 ? 148 : 180, last->mY + 36)` where `last` = mLastBonusText if
  it is non-NULL with mLife > 0, life 200, justify -1 = left).

### 5. OverlayWidget (`OverlayWidget.cpp`)

Widget subclass; sizeof 0x90 (`operator new(0x90)` in GameSelector 0x410b11); stored at
GameSelector+0xdc. vftable 0x4fb96c.

| off | type | name | meaning | init |
|---|---|---|---|---|
| 0x80 | WinBMApp* | mApp | | arg |
| 0x84 | | padding | | |
| 0x88 | double | mAlpha | fade of the HUD frame, written by GameSelector | 1.0 |

- `OverlayWidget(WinBMApp* theApp)` (0x417400): Widget(); mMouseVisible = false; mHasAlpha = true.
- deleting dtor 0x4174d0 (slot 0, plain Widget dtor).
- `void Draw(Graphics* g)` (0x417440, slot 13): `SetColorizeImages(true);
  SetColor(Color(255,255,255,(int)(mAlpha*255.0))); DrawImage(img[17] leftbar, 0, 0);
  SetColorizeImages(false); DrawImage(img[60] floatlogo, 15, 8)` (the logo always opaque).

### 6. Cross-module references

WinBMApp fields read (names from app.md): +0x498 mHasFocus (SexyAppBase); +0x6d4 mMrMoney;
+0x6e4 mImages[71] (ImageId, name table 0x5285b8, loader 0x41d7d0); +0x6e8 mBrightCoinsImage;
+0x6ec mFont1, +0x700 mFont3, +0x710 mScoreFont, +0x714 mLevelFont, +0x718 mFloatFont;
+0x7c8 mMrMoneyImages[6]; +0x7e0 mBillImages[4] (read as +0x7dc + scale*4); +0x814 mSkillLevel;
+0x818 mGameType (0 action, 1 strategy, 2 puzzle, registry "GameType"). **mSkillLevel range:**
Board::Draw's gamedifftype row `(mGameType*3 + mSkillLevel)*15 - 15` is only valid for skill
1..3 (easy, normal, hard), which agrees with screens.md (selector stores 1..3) and not with
app.md's "0..2". WinBMApp virtuals called: GetString
(0x4822a0), PlaySample(int) slot 49, SafeDeleteWidget slot 32 (Points), HSLToRGB (0x478280).

Board fields (board_logic): all of 2.1. Notable couplings: mStatsDirty/mStatsFade (Update and
ShowContinueButton drive the panel effect), mLevelUpStretch/mLevelUpBouncing (Update 0x40d7e5..,
0x40dc16..), mPauseFade/mPauseAnim/mPauseClickable (Update 0x40ca0f..), mBagFlashCount/
mBagFlash (Update 0x40e451..), mMoveDigit* (0x40c529..), mCountdown* (0x40d4c9..). Piece fields:
2.2. GameSelector writes OverlayWidget::mAlpha and reads MrMoney::mAnim; Board and ShowGameSelector
write MrMoney::mTargetX/mTargetY directly.

Framework: Graphics copy ctor 0x440630, Graphics(Image*) 0x4406a0, ~Graphics 0x440730,
ClipRect 0x43b250, SetColor 0x432550, SetColorizeImages 0x4325b0, SetDrawMode 0x432590,
SetFastStretch 0x4325d0, SetFont 0x432540, GetFont 0x432530, Color(r,g,b) 0x42b4b0,
Color(r,g,b,a) 0x42b4e0 (also used as the 4-int Insets ctor by COMDAT folding), Color(int) 0x42b470,
MemoryImage(app) 0x44c820, DialogButton ctor 0x429c10, WidgetContainer::AddWidget 0x4292d0,
Widget::Move 0x421c20, Widget::Update 0x421c70, Widget::Resize 0x424500.

#### 6.1 Reconciliation with board_logic.h

Board/Piece names in this file are board_logic.h's. Points to settle there:
- `DrawPiece` takes three arguments (`ret 0xc`): `DrawPiece(Graphics* g, Piece* thePiece, int theYOffset)`;
  Draw passes 0, or `mMintPixels` for `mNextRow` pieces. board_logic.h declares two.
- `GetBagSlotPos(Point* thePos, int theIndex)` matches (hidden result pointer, returned in eax).
- Colour tables: board_logic.h has `gCoinColors[5][3]` (0x52d8c8) and `gCoinLightColors[5][3]`
  (0x52d878) as int triples; the values agree with 1.3. In the binary they are `Color` objects
  (16 bytes, alpha 255) built by dynamic initialisers, passed by address to `SetColor`. This file and
  board_draw.h now use those names and do not define the tables again.
- `gRankThresholds` (0x4f53c0) = this file's rank table; the loop in 2.7 picks the highest index
  with `score >= threshold`.
- MrMoney animation ids: board_logic.h's `MrMoneyAnim` (`MRANIM_*`, "SetAnim") is a subset of
  board_draw.h's full enum (same names, same values; the function is `MrMoney::PlayAnim` 0x415200).
  Only one of the two headers should keep the enum.
- `mMovesColor` (+0x468) is what Draw uses to colourise the bottomline image (156, 432).
- `mBagGlow` (+0x7e0) drives the money-bag piece glow; `mBagFlash`/`mBagFlashCount` the HUD bag-slot
  pulse.

### 7. Files and formats

Nothing is written. Read: the images and fonts in section 1 (through the app), strings
GAME_PAUSED, CLICK_HERE_TO_CONTINUE, COLLECT_X_MONEY_BAGS_TO_ADVANCE (format "%d ... %s"),
GAME_TIME, BIGGEST_COIN_COMBO, BIGGEST_BONUS, COINS_CLEARED, NEXT_RANK_AT_X_PTS (format "%d"),
CLICK_HERE_TO_PLAY_AGAIN, CLICK_HERE_TO_PLAY_NEXT_LEVEL, string array gRankNames.

### 8. Open questions

1. mStatsFadeImage and mLevelUpImage keep stale pixels where a row is shifted or not written
   (2.4 step 5). The port must keep persistent pixel buffers to reproduce the slight smear; the
   levelup bottom rows are probably transparent, so the smear there may be invisible. Not verified
   at runtime.
2. `fsin` is the x87 instruction (not CRT `sin`); the arguments stay below 30 (262*0.08 + 8), where
   it differs from libm `sin` by a few ulps at most. Checked offline for every row and every
   mStatsFade value Update can produce (0, 0.015, 0.03, ... then 1.0): the closest pre-truncation
   value is 1.9e-6 from an integer, so `std::sin` gives identical offsets. (Resolved, unless the
   x87 precision assumption of item 7 is wrong.)
3. The Particle element bool mBright (+0x34) selects between two identical draw paths; probably an
   optimised-away difference in the source. Its meaning is board_logic's.
4. Both arms of DrawStatsPanel's `mState == 6` test load "GAME_TIME"; maybe one was meant to be a
   level-time string. mStatsSeconds itself is the same in both cases.
5. mClipExtra (+0x7a0) is written only by the ctor (0); it may be a leftover of a sliding HUD.
6. (Resolved with board_logic.h) mState 1 = STATE_LEVEL_DONE (stats panel, next-level button),
   5 = STATE_GAME_OVER, 6 = STATE_GAME_OVER_SCORES.
7. x87 precision: everything assumes 53-bit precision control (as for Bejeweled). The scaled bill
   sizes (1344x35, 1152x30) depend on it: a 64-bit control word would give 1343 for 1920*0.7.

---

## Part 3: The application (WinBMApp, resources, dialogs)

Behaviour spec of the application module of the carved game PE (`re/bigmoney/bin/BigMoneyGame.exe`, image
base 0x400000, MSVC 7.0), written from the Ghidra export (`re/bigmoney/ghidra/`) and checked against the
disassembly wherever Ghidra lost stack arguments (every DoDialog/NewDialog/CopyImage/DrawImage call of this
module was re-read in the disassembly). Framework: SexyAppFramework 1.2 (`docs/bigmoney/FRAMEWORK_12.md`,
sources `E:\retools\refs\saf12`). Header: `app.h`. Symbols: `app_symbols.jsonl`.

### 0. Conventions and extent

- Offsets are absolute from the object start, hex. `IMG[n]` = `mImages[n]` (`app+0x6e4` is an `Image**`,
  `IMG[n]` is the pointer at `*(app+0x6e4) + 4*n`). Image/sound/song ids are the enums of `app.h`.
- Slot numbers are WinBMApp's primary vtable (0x4fc6e4, 97 slots; 0..89 = SexyAppBase, 90..95 = SexyApp,
  96 = new). `+0x4` vtable 0x4fc6d8 is DialogListener (2 slots, SexyAppBase's: YES -> `ButtonPress/
  ButtonDepress(2000 + dialogId)`, NO -> `3000 + dialogId`).
- **Module extent.** The inventory puts WinBMApp.cpp at 0x41bd90-0x421830, but 0x41b670-0x41bd90 (listed
  under TitleScreen.cpp) is also app code: 0x41b670 is `WinMain` and 0x41b6a0..0x41bd00 are `WinBMApp`
  methods (all `__thiscall` on the app, no TitleScreen use). The TitleScreen ctor is 0x41b120 (the
  inventory's 0x419630 is ~TitleScreen). So WinBMApp code = 0x41b670-0x421830 (WinMain may come from a
  separate main.cpp linked between TitleScreen.obj and WinBMApp.obj; it does not matter for the port).
- 100 updates per second (framework). Double arithmetic is x87 at 53-bit precision (CRT default, also for
  the loading thread: `_beginthread` resets the FPU); `_ftol` truncates toward zero.
- **No RNG**: nothing in this module draws a random number or seeds one (the only CRT `rand()` near it is
  in GameSelector::Update, see screens.md). Seeding is the framework's, inside `SexyApp::Init` ->
  `SexyAppBase::Init` (SAF 1.2): `mRandSeed = GetTickCount(); SRand(mRandSeed)` (Sexy::Rand / gMTRand),
  then after InitPropertiesHook `srand(GetTickCount())` (CRT rand). SexyAppBase::Init also `chdir`s to
  the exe's folder, so the relative files (savegame.dat, hiscores.dat, BigMoney.ini) live there.
- Button modes (Dialog): 0 NONE, 1 YES_NO, 2 OK_CANCEL, 3 FOOTER. Dialog colour ids: 0 HEADER, 1 LINES,
  2 FOOTER, 3 BUTTON_TEXT. `Insets` and `Color` share one 4-int ctor in this binary (0x42b4e0), so
  Ghidra's `Color_rgba(..)` stored at Dialog+0x114 is `mContentInsets = Insets(l,t,r,b)`.

### 1. Global objects and entry

| Address | What |
|---|---|
| 0x52ddc8 | `WinBMApp gWinBMApp` (static object, 0x828 bytes; the next global starts at 0x52e5f0). Dynamic initializer 0x4f3a20: `gWinBMApp.WinBMApp()` then `atexit(0x4f4150)`; 0x4f4150 = `gWinBMApp.~WinBMApp()` (0x41c240). |
| 0x41b670 | `int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)` (called from the CRT at 0x4d9a43): `gHInstance (0x563f78) = hInstance; gWinBMApp.Init()` (direct call to 0x41d9a0, not virtual); `gWinBMApp.Start(); gWinBMApp.Shutdown(); return 0`. |
| 0x563f78 | `HINSTANCE gHInstance` (framework global, used by MakeWindow). |
| 0x565044 | `SexyAppBase* gSexyAppBase` (framework, = &gWinBMApp). |
| 0x5285b8 | `const char* gImageNames[71]` (section 3.1). |
| 0x5286d8 | `const char* gSoundNames[48]` (section 3.2). |
| 0x528914 / 0x528930 / 0x52894c / 0x528968 | framework `std::string DIALOG_YES_STRING / NO / OK / CANCEL` (Init overwrites them). |

Fields the ctor does not initialise (`mBoard` 0x6d0, `mTitleScreen` 0x6d8, `mHelpWidget` 0x6e0,
`mBrightCoinsImage` 0x6e8, 0x825) rely on the static zero-init of the global: **the port must zero them**.

### 2. WinBMApp layout

`class WinBMApp : public SexyApp` (SexyApp ends at 0x6c9, own data from 0x6d0). **sizeof 0x828**
(last field 0x825; 8-byte aligned; static object, no `operator new`).

| Off | Type | Name | Meaning | Ctor | Writers / readers |
|---|---|---|---|---|---|
| 0x6d0 | Board* | mBoard | current board widget | (not set: 0) | NewGame w, RemoveBoard w; read everywhere (HighScoreWidget, HelpWidget, focus handlers) |
| 0x6d4 | MrMoney* | mMrMoney | Mr. Money widget (184x136) | NULL | TitleScreenIsFinished w; ShowGameSelector writes its mTargetX/Y (+0x98/+0x9c); Board, GameSelector, HighScoreWidget r |
| 0x6d8 | TitleScreen* | mTitleScreen | | (not set) | Init w, TitleScreenIsFinished w |
| 0x6dc | GameSelector* | mGameSelector | | NULL | ShowGameSelector w, RemoveGameSelector w |
| 0x6e0 | HelpWidget* | mHelpWidget | | (not set) | ShowHelp w, RemoveHelp w |
| 0x6e4 | Image** | mImages | `new Image*[71]` indexed by ImageId | NULL | Init w; everyone r |
| 0x6e8 | MemoryImage* | mBrightCoinsImage | brightened copy of IMG[23] coins (580x145) | (not set) | Init w; Board 0x402060 r (highlighted coins); never deleted |
| 0x6ec | ImageFont* | mFont1 | data\bigmoneyfont1.txt | NULL | Init |
| 0x6f0 | ImageFont* | mFont1NoShadow | copy of mFont1 + tag "noshadow" | NULL | Init |
| 0x6f4 | ImageFont* | mFont1OutlineNoShadow | copy + "outline" + "noshadow" | NULL | Init |
| 0x6f8 | ImageFont* | mFont1Underlined | copy + "underlined", Prepare() | NULL | Init |
| 0x6fc | ImageFont* | mFont1OutlineUnderlined | copy + "outline" + "underlined", Prepare() | NULL | Init |
| 0x700 | ImageFont* | mFont3 | data\bigmoneyfont3.txt | NULL | Init |
| 0x704 | ImageFont* | mFont3Outline | copy + "outline", Prepare() | NULL | Init |
| 0x708 | ImageFont* | mFont3Underlined | copy + "underlined", Prepare() | NULL | Init |
| 0x70c | ImageFont* | mFont3OutlineUnderlined | copy + "outline" + "underlined", Prepare() | NULL | Init |
| 0x710 | ImageFont* | mScoreFont | data\scorefont.txt (default point size 25) | NULL | Init |
| 0x714 | ImageFont* | mLevelFont | legacy font: `ImageFont(IMG[14], "data\\levelfont.txt")` | NULL | Init |
| 0x718 | ImageFont* | mFloatFont | data\floatfont.txt | NULL | Init |
| 0x71c | ImageFont* | mScoreFontSmall | copy of mScoreFont, `SetPointSize(20)` | NULL | Init |
| 0x720 | std::list\<ButtonWidget*\> | mButtonList | side buttons (MakeButton); `_Myhead` 0x724, `_Mysize` 0x728 | empty | MakeButton |
| 0x72c | std::list\<Image*\> | mButtonImageList | over/down images of the side buttons (deleted by the dtor); head 0x730, size 0x734 | empty | MakeButton |
| 0x738 | Image* | mCheckboxCheckedImage | CopyImage(IMG[37] checkboxes, Rect(22,0,22,23)) | NULL | LoadingThreadProc; OptionsDialog (Checkbox checked image) |
| 0x73c | Image* | mCheckboxUncheckedImage | CopyImage(IMG[37], Rect(0,0,22,23)) | NULL | LoadingThreadProc; OptionsDialog |
| 0x740 | Image* | mStartArrowsMirrored | CopyImage(IMG[62] startarrows) mirrored | NULL | LoadingThreadProc; StartButton::Draw |
| 0x744 | Image*[3][4] | mGameTypeButtonImages | [type][0 normal, 1 over, 2 down, 3 unused] | NULL x12 | LoadingThreadProc; GameSelector ctor (LockableButtons) |
| 0x774 | Image*[3][4] | mSkillButtonImages | [skill][0 normal, 1 over, 2 down, 3 unused] | NULL x12 | LoadingThreadProc; GameSelector ctor |
| 0x7a4 | Image*[3][3] | mSelectorButtonImages | [GameSelector button 0..2][button, over, down]; only [1][0] (0x7b0) is ever set | NULL x9 | LoadingThreadProc; GameSelector ctor |
| 0x7c8 | Image*[6] | mMrMoneyImages | Mr. Money with mouth frame 0..5 (184x136) | NULL x6 | LoadingThreadProc; MrMoney::Draw (`+0x7c8 + 4*frame`) |
| 0x7e0 | Image*[4] | mBillImages | IMG[57] bill scaled 0.9/0.8/0.7/0.6 | NULL x4 | LoadingThreadProc; Board::Draw reads `app+0x7dc + 4*n` for n = 1..4 (n = 0 uses IMG[57]) |
| 0x7f0 | Image* | mCobrandImage | GetImage("images\\cobrand") if property Cobranded | NULL | Init; TitleScreen::Draw |
| 0x7f4 | bool[3] | mHelpShownThisSession | per game type: help screen already shown before a game this run | false x3 | StartGame |
| 0x7f7 | | (pad) | | | |
| 0x7f8 | std::string | mLastScoreName | registry LastScoreName; prefills RecordScoreDialog | "" | Read/WriteToRegistry, ButtonDepress (dialog 3) |
| 0x814 | int | mSkillLevel | **1..3**, last selected skill (registry SkillLevel); GameSelector stores `id - 10` of skill buttons 11..13 and defaults an unset skill to 1 | 1 | GameSelector::StartGame w; Board, GameSelector r |
| 0x818 | int | mGameType | 0 Action, 1 Strategy, 2 Puzzle (registry GameType) | 0 | GameSelector::StartGame w; Board, HighScoreWidget, focus handlers r |
| 0x81c | bool | mHint81C | session-once instructor hint flag: set by Board 0x408a60 (piece kind 0 event), tested by Board 0x409520 and 0x40aea0 | false | Board |
| 0x81d | bool | mHint81D | session-once instructor hint flag: set by Board 0x408a60 (piece kind 2 -> 3), tested by Board 0x409520 | false | Board |
| 0x81e | bool | mHint81E | session-once "fewer than 4 moves left" hint, not in Puzzle (Board::Update sets/tests) | false | Board |
| 0x81f | bool | mMovedThisSession | set by Board 0x40b120 together with `++mTimesPlayed` and Board+0xa4 = true (first move of a game); Board::Update shows an idle hint at its update 600 while still false | false | Board |
| 0x820 | bool | mNoPopLink | registry NoPopLink, read only, never used | false | ReadFromRegistry |
| 0x821 | bool | mWelcomePlayed | GameSelector update 100: first time plays sound 37 "welcome" + Mr. Money talk, later 10% chance of 47 "loveit" | false | GameSelector::Update |
| 0x822 | bool[3] | mDontShowHelp | per game type "don't show this screen again" (registry DontShowHelp0..2) | false x3 | HelpWidget::ButtonPress w (button 2), StartGame r |
| 0x825 | bool | mShowInstructor | the help screen was shown before the current game: the board shows its instructor arrows/text | (not set) | StartGame w; Board 0x401fe0 r |
| 0x826 | | (pad to 0x828) | | | |

Framework fields of note set/used by this module: `mProdName` (+0x28) "BigMoney", `mTitle` (+0x44)
"Big Money Deluxe " + mProductVersion, `mRegKey` (+0x60) "PopCap\\BigMoney", `mWidth/mHeight` (+0x88/+0x8c,
640x480 from partner.xml), `mWidgetManager` (+0x2e4), `mDialogMap` size (+0x2f0), `mIsWindowed` (+0x307),
`mMusicInterface` (+0x338), `mSoundManager` (+0x47c), `mHasFocus` (+0x498), `mCustomCursorsEnabled`
(+0x4c8, ctor false), `mNumLoadingThreadTasks` (+0x4cc), `mCompletedLoadingThreadTasks` (+0x4d0),
`mLoadingFailed` (+0x4c5), `mInternetManager` (+0x5e0), `mReferId` (+0x5e8), `mUserName` (+0x668),
`mRegCode` (+0x6a0), `mIsRegistered` (+0x6bc), `mTimesPlayed` (+0x6c0).

#### Vtable overrides

| Slot | Address | Method |
|---|---|---|
| 1 | 0x41bfa0 | ButtonPress(int theId) |
| 2 | 0x420f10 | ButtonDepress(int theId) |
| 16 | 0x421450 | LoadingThreadCompleted() |
| 17 | 0x41c9d0 | scalar deleting dtor (-> ~WinBMApp 0x41c240) |
| 22 | 0x41efd0 | LoadingThreadProc() |
| 23 | 0x41c9f0 | WriteToRegistry() |
| 24 | 0x41cb60 | ReadFromRegistry() |
| 25 | 0x41bd90 | NewDialog(...) |
| 33 | 0x41fe80 | URLOpenFailed(const std::string&) |
| 34 | 0x41bf80 | URLOpenSucceeded(const std::string&) |
| 35 | 0x420140 | OpenURL(const std::string&, bool) |
| 45 | 0x41d9a0 | Init() |
| 71 | 0x41c1c0 | KillDialog(int) |
| 73 | 0x41bf00 | ModalOpen() |
| 74 | 0x41bf20 | ModalClose() |
| 75 | 0x41bf40 | GotFocus() |
| 76 | 0x41bf60 | LostFocus() |
| 96 | 0x4215c0 | **new** `virtual void TitleScreenIsFinished()` (called by TitleScreen::ButtonDepress) |

Not overridden (notably): Shutdown (40, SexyAppBase), CreateMusicInterface (18: SexyAppBase's, BASS),
UpdateFrames (8), PreDisplayHook (26). Slot 0 is ButtonListener::ButtonPress(id, clicks) -> slot 1.

### 3. Resources (there is no resources.xml)

#### 3.1 Images (`gImageNames` @0x5285b8, 71 entries)

All 71 are loaded **synchronously by Init, in index order, before the title screen**, by
`LoadAppImage(i)` = `GetImage("images\\" + gImageNames[i], true)` (SexyAppBase::GetImage -> ImageLib:
extension search, alpha mask `_name` / `name_` merged automatically; a lone `_name` file gives a white image
with that alpha). Then `GenerateDDSurface()`. Init also calls `Palletize()` on each (no visual effect) and
logs "%s - PALLETIZED!" / "%s - 32 Bit Only" with OutputDebugString.

| n | +off | name | file(s) | size | users |
|---|---|---|---|---|---|
| 0 | 0x00 | cursor_pointer | .gif + _ | 54x54 | cursor 0 (CURSOR_POINTER) |
| 1 | 0x04 | cursor_hand | .gif + _ | 58x58 | cursor 1 (HAND) |
| 2 | 0x08 | cursor_dragging | .gif + _ | 37x37 | cursor 2 (DRAGGING) |
| 3 | 0x0c | cursor_text | .gif | 9x21 | cursor 3 (TEXT) |
| 4 | 0x10 | titleback | .jpg | 640x480 | TitleScreen |
| 5 | 0x14 | barandlogo | .gif + _ | 540x149 | TitleScreen |
| 6 | 0x18 | goldbar | .jpg + _.gif | 370x76 | TitleScreen |
| 7 | 0x1c | mrmoneybags | .jpg + _.gif | 613x403 | TitleScreen |
| 8 | 0x20 | titlecigartip | .gif + _ | 35x50 | TitleScreen |
| 9 | 0x24 | titleeyeglow | .gif | 163x99 | TitleScreen |
| 10 | 0x28 | bigmoneylogo | .jpg + _.gif | 472x239 | TitleScreen |
| 11 | 0x2c | deluxor | .gif + _ | 155x150 | TitleScreen |
| 12 | 0x30 | dollarstrip | .gif + _ | 640x20 | TitleScreen |
| 13 | 0x34 | trialversion | .jpg + _.gif | 124x126 | TitleScreen |
| 14 | 0x38 | levelfont | .gif + _ | 164x19 | mLevelFont image |
| 15 | 0x3c | floatfont | .gif + _ | 223x20 | **unused** (floatfont.txt loads its own) |
| 16 | 0x40 | level | .gif + _ | 51x21 | Board |
| 17 | 0x44 | leftbar | .jpg + _.gif | 640x480 | Board, HelpWidget, OverlayWidget |
| 18 | 0x48 | backdrop | .jpg | 406x465 | Board |
| 19 | 0x4c | rightbar1 | .jpg | 56x479 | Board |
| 20 | 0x50 | rightbar2 | .jpg + _.gif | 80x480 | Board |
| 21 | 0x54 | coin | .gif + _ | 400x40 | Board |
| 22 | 0x58 | stack | .gif | 40x372 | Board |
| 23 | 0x5c | coins | .gif + _ | 580x145 | Board (29x29 cells); source of mBrightCoinsImage |
| 24 | 0x60 | horzcoins | .gif + _ | 464x145 | Board |
| 25 | 0x64 | bag | .gif + _ | 132x36 | Board |
| 26 | 0x68 | explosion | .gif | 675x91 | Board |
| 27 | 0x6c | dollar | .gif + _ | 1056x33 | Board |
| 28 | 0x70 | sparkle | .gif | 280x20 | Board |
| 29 | 0x74 | olddialog | .gif + _ | 192x192 | NewDialog (all DoDialog dialogs), CreditsDialog, UpdateCheckDialog, Board/HighScoreWidget |
| 30 | 0x78 | dialog | .jpg + _.gif | 192x192 | MoneyDialog subclasses (HighScore/Options/RecordScore), RegisterDialog |
| 31 | 0x7c | dialoglight | .gif | 14x14 | MoneyDialog::Draw lights |
| 32 | 0x80 | dialogglow | .gif | 22x22 | MoneyDialog::Draw glow (additive) |
| 33 | 0x84 | dialogbutton | .gif | 96x36 | every dialog's button component image |
| 34 | 0x88 | buttonstates | .jpg | 108x174 | side buttons (6 rows of 29 px) |
| 35 | 0x8c | slidertrack | .gif | 209x20 | OptionsDialog |
| 36 | 0x90 | sliderthumb | .png | 36x31 | OptionsDialog |
| 37 | 0x94 | checkboxes | .jpg | 44x23 | -> mCheckbox*Image |
| 38 | 0x98 | rankstrip | .gif | 100x2700 | Board (0x404950) |
| 39 | 0x9c | gameover | .gif + _ | 333x181 | Board |
| 40 | 0xa0 | countdown | .jpg + _.gif | 160x480 | Board |
| 41 | 0xa4 | barbagdisplay | .jpg | 168x43 | Board |
| 42 | 0xa8 | mouths | .gif + _ | 184x816 | **unused** (same file as 56) |
| 43 | 0xac | levelup | .gif + _ | 300x189 | Board |
| 44 | 0xb0 | editbox | .png | 96x32 | RecordScoreDialog |
| 45 | 0xb4 | bagholder | .gif + _ | 41x34 | Board |
| 46 | 0xb8 | glowbag | .gif + _ | 64x64 | Board |
| 47 | 0xbc | waitbar | .gif | 32x16 | UpdateCheckDialog (not ported) |
| 48 | 0xc0 | selectback | .jpg | 467x480 | GameSelector, StartButton; source of selector tiles |
| 49 | 0xc4 | gametypestext | .gif + _ | 465x248 | selector tiles (3 columns of 154) |
| 50 | 0xc8 | illustrations | .jpg + _.gif | 452x128 | selector tiles (3 x 144, pitch 154) |
| 51 | 0xcc | skillstext | .gif + _ | 467x52 | skill tiles |
| 52 | 0xd0 | skillspressed | .jpg | 467x52 | skill tiles (down) |
| 53 | 0xd4 | bottomline | .jpg + _.gif | 436x18 | Board |
| 54 | 0xd8 | coinglow | .gif + _ | 45x45 | Board |
| 55 | 0xdc | mrmoney | .gif + _ | 184x136 | -> mMrMoneyImages |
| 56 | 0xe0 | mouths | .gif + _ | 184x816 | -> mMrMoneyImages (6 frames of 136) |
| 57 | 0xe4 | bill | .gif + _ | 1920x50 | Board (16 frames of 120); -> mBillImages |
| 58 | 0xe8 | gamedifftype | .gif + _ | 100x135 | Board |
| 59 | 0xec | starttext | .gif + _ | 280x100 | StartButton |
| 60 | 0xf0 | floatlogo | .gif + _ | 156x76 | Board, HelpWidget, OverlayWidget |
| 61 | 0xf4 | pausebackdrop | .gif | 489x374 | Board |
| 62 | 0xf8 | startarrows | .gif + _ | 30x60 | StartButton; -> mStartArrowsMirrored |
| 63 | 0xfc | instructoarrows | .gif | 50x250 | Board |
| 64 | 0x100 | instructotext | .gif | 200x450 | Board |
| 65 | 0x104 | register | .jpg + _.gif | 381x385 | RegisterDialog (not ported) |
| 66 | 0x108 | register_overlay | .gif | 381x385 | RegisterDialog (not ported) |
| 67 | 0x10c | helpcircles | .jpg | 104x660 | HelpWidget |
| 68 | 0x110 | clouds | **only _clouds.jpg** | 512x512 | CreditsDialog (white image, alpha from the mask) |
| 69 | 0x114 | bigarrow | .gif | 372x87 | HelpWidget |
| 70 | 0x118 | nomoremoves | .gif + _ | 333x181 | Board |

"+ _" = an `_name.gif` alpha mask of the same size exists. Not in the table: `images\cobrand` (Init, only
if Cobranded: cobrand.jpg 228x87 + cobrand_.gif, cobrand.gif 121x50 + _cobrand.gif; which one ImageLib picks
depends on its extension order). Font descriptor images (fontlefont, Outlinefont1, fontunderline,
bigmoneyfont3, scorefont_top/_blur, floatfont...) are loaded by the descriptors themselves and are mostly
alpha-only `_x.gif` files; names in the .txt files differ in case from the files (case-insensitive lookup).

**Derived images** (sections 4.2, 4.3 and 5.2 give the exact construction): mBrightCoinsImage (Init), grey
illustrations (temporary), mGameTypeButtonImages, mSkillButtonImages, mSelectorButtonImages[1][0],
mCheckbox*Image, mStartArrowsMirrored, mMrMoneyImages, mBillImages (LoadingThreadProc), side-button
over/down images (TitleScreenIsFinished). No HSL colourising in this module (the TitleScreen's deluxe badge
is HSL, see screens.md).

#### 3.2 Sounds (`gSoundNames` @0x5286d8, 48 entries)

Loaded by LoadingThreadProc: `mSoundManager->LoadSound(i, "sounds\\" + name)` for i = 0..47. Files: `.ogg`
except 3 piecehit, 5 bonusdrop, 23 explode (`.au`). Played with `PlaySample(id)` (slot 49).

| id | name | id | name | id | name | id | name |
|---|---|---|---|---|---|---|---|
| 0 | clearcoins | 12 | tone2 | 24 | gameover | 36 | cointube |
| 1 | clearcoins2 | 13 | tone3 | 25 | levelup | 37 | welcome |
| 2 | clearcoins3 | 14 | tone4 | 26 | goodjob | 38 | speedbonus |
| 3 | piecehit | 15 | tone5 | 27 | excellent | 39 | bonus |
| 4 | moveup | 16 | tone6 | 28 | incredible | 40 | superbonus |
| 5 | bonusdrop | 17 | tone7 | 29 | awesome | 41 | megamoney |
| 6 | baghit | 18 | tone8 | 30 | unbelievable | 42 | action |
| 7 | gotbag | 19 | tone9 | 31 | bigmoney | 43 | strategy |
| 8 | getready | 20 | tonepuzzle | 32 | three | 44 | puzzle |
| 9 | go | 21 | warning | 33 | two | 45 | wakeup |
| 10 | buttonclick | 22 | gameovertone | 34 | one | 46 | anewhighscore |
| 11 | tone1 | 23 | explode | 35 | bad | 47 | loveit |

Played by this module: 10 buttonclick (every app ButtonPress), 35 bad (invalid registration code).

#### 3.3 Fonts

Five descriptor files: `data\bigmoneyfont1.txt`, `data\bigmoneyfont3.txt`, `data\scorefont.txt`,
`data\levelfont.txt` (legacy "width list" format, image IMG[14]), `data\floatfont.txt`. Thirteen ImageFont
objects (layout table, 0x6ec..0x71c). The tags are matched case-insensitively (AddTag and the descriptor's
LayerRequireTags/LayerExcludeTags upper-case them): bigmoneyfont1 has layers Outline (requires OUTLINE),
UnderlineOutline (UNDERLINED+OUTLINE), Shadow (excludes NOSHADOW), ShadowUnderline, Main, MainUnderline.
`Prepare()` only pre-builds the active layer list (no visible effect). bigmoneyfont2.txt,
new_bigmoneyfont1.txt and _floatfont.txt ship but are not loaded.

#### 3.4 Music

The music interface is the framework default (`SexyAppBase::CreateMusicInterface`, BASS). Init loads the
same module **14 times**: `LoadMusic(id, "music\\QualityTime_BigMoney.it")` for id = 0..13. A song id is
one channel of that module; the `theOffset` of PlayMusic/FadeIn is the **order position** (BASS
`MusicPlayEx(pos = MAKELONG(order, 0))`), loop on (`noLoop = false` everywhere). The module
("Quality Time - Big Money", 101 orders, 82 patterns, speed 6, tempo 135) holds several sub-songs separated
by `+++` (254) markers; their loops are pattern jumps inside the module.

| Song id | Start order | Used for (who) |
|---|---|---|
| 0 | 0 | Puzzle (type 2) game music (NewGame FadeIn; Board pause/resume FadeIn(0, -1 = continue)) |
| 1 | 23 | Puzzle level-up jingle (Board 0x4083d0: FadeOut(0, false, 0.02), PlayMusic(1, 23), sound 25 levelup; 0x40e4b0 later FadeOut(1, true, 0.02), FadeIn(0, -1, 0.004)) |
| 2 | 24 | Puzzle pause music (Board::Pause 0x402930: FadeOut(0, false, 0.008), FadeIn(2, 24, 0.008); unpause FadeOut(2, true, 0.008), FadeIn(0, -1, 0.008)) |
| 3 | 26 | Action (type 0) game music |
| 4 | 40 | Action level-up jingle (as song 1) |
| 5 | 41 | Action pause music (as song 2) |
| 6 | 44 | Strategy (type 1) game music |
| 7 | 63 | Strategy level-up jingle |
| 8 | 64 | Strategy pause music |
| 9 | 67 | game over (Board 0x401d30: FadeOut(3/6/0, true, 0.02), PlayMusic(9, 67), sound 22 gameovertone) |
| 10 | - | loaded, never played |
| 11 | 87 | title (Init: `PlayMusic(11, 87, false)`); keeps playing into the game selector |
| 12 | 89 | game selector (ShowGameSelector: `if (!IsPlaying(11)) FadeIn(12, 89, 0.005)`) |
| 13 | - | loaded, never played |

(Board's calls are listed for reconciliation; the Board spec owns them.) Fade speeds used here: 0.005
(NewGame, selector) and 0.01 (ShowGameSelector fade-outs). BASS FadeIn semantics (SAF 1.2): stop the
channel, play from the order, volume ramps by `speed` per music update; FadeOut ramps down by `speed` and
stops when silent if `stopSong`. Music volume itself is the framework's (OptionsDialog slider).

#### 3.5 Cursors

Init: `SetCursorImage(0, IMG[0]); (1, IMG[1]); (2, IMG[2]); (3, IMG[3])`. `mCustomCursorsEnabled = false`
in the ctor; the framework registry value and the Options checkbox (ButtonDepress dialog 1) change it.

### 4. Construction, Init, loading, destruction

#### 4.1 `WinBMApp()` - 0x41c790

1. `SexyApp()`; vtables 0x4fc6e4 / 0x4fc6d8.
2. Construct mButtonList (0x720) and mButtonImageList (0x72c) (empty), mLastScoreName = "".
3. NULL: the 13 fonts (0x6ec..0x71c), mImages, mGameSelector, mMrMoney, mCobrandImage, mCheckbox*,
   mStartArrowsMirrored, and all of 0x744 (12), 0x774 (12), 0x7a4 (9), 0x7c8 (6), 0x7e0 (4).
4. `mProdName = "BigMoney"`; `mTitle = "Big Money Deluxe " + mProductVersion`;
   `mRegKey = "PopCap\\BigMoney"`.
5. `mCustomCursorsEnabled = false`; `mGameType = 0`; `mSkillLevel = 1`; bytes 0x81c..0x824 = false;
   0x7f4..0x7f6 = false.
6. Not set (rely on zero-init): mBoard, mTitleScreen, mHelpWidget, mBrightCoinsImage, mShowInstructor.

(partner.xml then overrides through the framework: ProdName "BigMoney", RegistryKey "PopCap\BigMoney",
Width 640, Height 480, NoReg true -> `mIsRegistered = mBuildUnlocked = true`, DontUpdate true,
DefaultWindowed false; DisplayName is not read by 1.2, so the title stays "Big Money Deluxe <version>".)

#### 4.2 `Init()` - 0x41d9a0, slot 45

1. `SexyApp::Init()` (SexyAppBase::Init: InitPropertiesHook/partner.xml, **ReadFromRegistry** (section 7), window,
   PreDisplayHook; ++mTimesExecuted).
2. `LoadProperties()` (properties\default.xml, the string table).
3. `DIALOG_YES_STRING = GetString("DIALOG_BUTTON_YES")`, likewise NO, OK, CANCEL.
4. RealArcade (not ported): `if (GetBoolean("RealArcade", false))` (absent from partner.xml -> false):
   `mIsRegistered = false`; read `[Registry] Key` and `[Registry] ValueName` from the full path of
   `BigMoney.ini` (GetPrivateProfileStringA, 2048-byte buffer); the first 5 characters of Key select the
   root ("HKLM\\" HKEY_LOCAL_MACHINE, "HKCU\\" HKEY_CURRENT_USER, "HKCR\\" HKEY_CLASSES_ROOT, else nothing);
   `RegOpenKeyA(root, Key.substr(5))`; if `RegQueryValueExA(ValueName)` (256-byte buffer) succeeds:
   RegCloseKey and `mIsRegistered = true`.
5. `mImages = new Image*[71]` (all NULL). For i = 0..70: `if (!LoadAppImage(i)) { Shutdown(); return; }`,
   then `mImages[i]->Palletize()` + debug log.
6. **mBrightCoinsImage**: `m = new MemoryImage(this); m->Create(w(IMG[23]), h(IMG[23]))` (580x145);
   for every pixel of `IMG[23]->GetBits()`: each of R, G, B `c' = min(255, c * 280 / 255 + 48)` (unsigned
   integer division), alpha unchanged; written to `m->GetBits()`; `m->BitsChanged()`.
   (So black -> 48, 255 -> 255, 100 -> 157.)
7. Fonts, in this order (copies are `new ImageFont(const ImageFont&)` of the base font):
   mFont1 = ImageFont(this, "data\\bigmoneyfont1.txt"); mFont1NoShadow (+noshadow);
   mFont1OutlineNoShadow (+outline, +noshadow); mFont1Underlined (+underlined, Prepare);
   mFont1OutlineUnderlined (+outline, +underlined, Prepare); mFont3 = ImageFont(this,
   "data\\bigmoneyfont3.txt"); mFont3Outline (+outline, Prepare); mFont3Underlined (+underlined, Prepare);
   mFont3OutlineUnderlined (+outline, +underlined, Prepare); mScoreFont = ImageFont(this,
   "data\\scorefont.txt"); mScoreFontSmall = copy, `SetPointSize(20)`; mLevelFont = ImageFont(IMG[14],
   "data\\levelfont.txt"); mFloatFont = ImageFont(this, "data\\floatfont.txt").
8. `mTitleScreen = new TitleScreen(this)` (0x118 bytes, ctor 0x41b120); `Resize(0, 0, mWidth, mHeight)`;
   `mWidgetManager->AddWidget(mTitleScreen)`.
9. Music: LoadMusic 0..13 (3.4); `PlayMusic(11, 87, false)`.
10. Cursors (3.5).
11. `if (GetBoolean("Cobranded", false)) mCobrandImage = GetImage("images\\cobrand", true)` (partner.xml:
    false).

The loading thread is started later by SexyAppBase::Start (mAutoStartLoadingThread).

**`LoadAppImage(int i)` - 0x41d7d0** (bool): `mImages[i] = GetImage("images\\" + gImageNames[i], true)`;
if NULL: `Popup("Failed to load images\\" + name)`, `mLoadingFailed = true`, return false; else
`((DDImage*)mImages[i])->GenerateDDSurface()`, return true.

#### 4.3 `LoadingThreadProc()` - 0x41efd0, slot 22 (loading thread)

1. `mNumLoadingThreadTasks = 72`.
2. Sounds: if `mSoundManager->Initialized()`: for i = 0..47: `if (!LoadSound(i, "sounds\\" + name))
   { Popup("Failed to load sounds\\" + name); mLoadingFailed = true; return; }`, ++completed.
   Else completed += 48.
3. Grey illustrations (temporary): `src = dynamic_cast<MemoryImage*>(IMG[50])`; `grey = new
   MemoryImage(this); grey->Create(w(src), h(src))` (452x128); per pixel `v = (R + G + B) / 3`
   (unsigned), `out = (A << 24) | v * 0x010101`; `grey->BitsChanged()`.
4. Game-type tiles, for column i = 0..2, `x = 154 * i` (++completed per column; `CopyImage(img, r)` =
   SAF CopyImage: new DDImage of r's size, image drawn at (-r.x, -r.y), CopyAttributes):
   - `[i][0]` normal: `CopyImage(IMG[48], Rect(x + 5, 58, 154, 266))`; on it: `FillRect(0, 0, 154, 266)`
     with Color(0,0,0,128); colorize on; Color(120,120,120): `DrawImage(IMG[49], 0, 10, Rect(x, 0, 154,
     248))`; Color(120,120,120): `DrawImage(grey, 10, 42, Rect(x, 0, 144, 128))`.
   - `[i][1]` over: same copy; FillRect with Color(0,0,0,64); colorize; Color(220,220,0): IMG[49] as above;
     Color(180,180,180): grey as above.
   - `[i][2]` down: same copy, no fill; colorize; Color(120,255,0): IMG[49] as above; Color(255,255,255):
     `DrawImage(IMG[50], 10, 42, Rect(x, 0, 144, 128))` (the coloured illustration).
   - `[i][3]` stays NULL. Then `delete grey`.
5. Skill tiles, for i = 0..2 with `x = 29, 186, 339` (++completed each):
   - `[i][0]`: `CopyImage(IMG[48], Rect(x, 359, 113, 52))`; colorize; Color(120,120,120);
     `DrawImage(IMG[51], 0, 0, Rect(x, 0, 113, 52))`.
   - `[i][1]`: same copy; colorize; Color(220,220,0); same draw.
   - `[i][2]`: `CopyImage(IMG[52], Rect(x, 0, 113, 52))`; colorize; Color(120,255,0);
     `DrawImage(IMG[51], 1, 1, Rect(x, 0, 113, 52))` (text offset 1 px when pressed).
6. `mSelectorButtonImages[1][0] (0x7b0) = CopyImage(IMG[48], Rect(7, 429, 455, 38))`.
7. `mCheckboxUncheckedImage (0x73c) = CopyImage(IMG[37], Rect(0, 0, 22, 23))`;
   `mCheckboxCheckedImage (0x738) = CopyImage(IMG[37], Rect(22, 0, 22, 23))`.
8. `mStartArrowsMirrored = CopyImage(IMG[62])`; `MirrorImage(it)` (horizontal flip of the bits).
9. Mr. Money, for j = 0..5 (++completed each): `w, h = size(IMG[55])` (184x136);
   `m = new MemoryImage(this); m->Create(w, h); m->SetImageMode(true, true)`; `Graphics g(m)`:
   `DrawImage(IMG[55], 0, 0)`; `DrawImage(IMG[56], 0, 0, Rect(0, h * j, w, h))`; if j == 5 also
   `DrawImage(IMG[56], 0, 0, Rect(0, 0, w, h))` (frame 5 = mouth 5 with mouth 0 on top);
   `mMrMoneyImages[j] = m`.
10. Bills, for k = 0..3 with `s = 0.9, 0.8, 0.7, 0.6` (doubles; ++completed each): `W, H = size(IMG[57])`
    (1920x50); `sw = (int)(W * s)`, `sh = (int)(H * s)` (int x double, truncated) = 1728x45, 1536x40,
    1344x35, 1152x30 at 53-bit precision; `m = new MemoryImage(this); Create(sw, sh);
    SetImageMode(true, true)`; `Graphics(m).DrawImage(IMG[57], Rect(0, 0, sw, sh), Rect(0, 0, W, H))`
    (stretch, mFastStretch false); `mBillImages[k] = m`.

Completed tasks reach 48 + 3 + 3 + 6 + 4 = **64 of 72**, so `GetLoadingThreadProgress()` tops out at
0.888... until `mLoaded` makes it 1.0 (the TitleScreen's loading bar relies on that).

**`LoadingThreadCompleted()` - 0x421450, slot 16**: `if (ShouldCheckForUpdate()) ConfirmCheckForUpdates()`
(slot 90; DontUpdate=true -> false). Does not call the (empty) base.

#### 4.4 `~WinBMApp()` - 0x41c240 (slot 17 = 0x41c9d0 deleting wrapper)

1. `if (mBoard && mBoard->IsGameInProgress()) mBoard->SaveGame()` (Board 0x4010d0 / 0x403300). This is
   where a game in progress is saved on exit (Shutdown is not overridden).
2. mTitleScreen, mGameSelector, mMrMoney: each `mWidgetManager->RemoveWidget` + `delete` if set.
3. delete fonts 0x6ec..0x710, 0x71c, 0x714, 0x718; 0x738, 0x73c, 0x740; mCobrandImage; the 12 + 12 + 9 +
   6 + 4 derived images; every image in mButtonImageList; the 71 mImages and the array.
4. Destroy mLastScoreName and both lists (the side ButtonWidgets themselves are not deleted), ~SexyApp.
   mBoard, mHelpWidget and mBrightCoinsImage are not deleted.

### 5. Screen flow

#### 5.1 Overview

```
Init: TitleScreen (music 11 @87)
 TitleScreen link "play" -> TitleScreenIsFinished(): side buttons, Mr. Money, ShowGameSelector(),
                            ConfirmContinueGame() (dialog 5 if savegame.dat exists)
 GameSelector "start"    -> GameSelector::StartGame (vf54): mGameType/mSkillLevel = selection,
                            app->StartGame():
                               first time this type this run and not "don't show": ShowHelp()
                                 HelpWidget button -> NewGame() (or RemoveHelp() if a board exists)
                               else NewGame()
 NewGame()               -> music, GameSelector slides out (vf56; it removes itself at the end),
                            RemoveBoard(), new Board
 Board game over (state 6): the Board shows its own HighScoreWidget (Board+0x7dc);
                            HighScoreWidget::CheckForHighScore -> ShowRecordScoreDialog (dialog 3)
                            or (unregistered) ShowRegisterToRecordScoreDialog (dialog 15);
                            clicking the board in state 6 -> NewGame() (same type);
 side button 0 "New Game"-> ConfirmNewGame(): board active -> dialog 0 -> YES -> ShowGameSelector();
                            board over / none -> ShowGameSelector() (or StartGame if the selector is up)
 ShowGameSelector()      -> board (if any) frozen (state 8) behind the selector, selector slides in
                            (vf55) and calls RemoveBoard() when in place
 side button 2 "Quit"    -> dialog 12 -> YES -> Shutdown(); the dtor saves a game in progress
```

#### 5.2 `TitleScreenIsFinished()` - 0x4215c0, new slot 96

Called by TitleScreen::ButtonDepress (play link, not expired).
1. `mWidgetManager->RemoveWidget(mTitleScreen); SafeDeleteWidget(mTitleScreen); mTitleScreen = NULL`.
2. Side buttons, `MakeButton(id, rect, over, down)` with over/down = `CopyImage(IMG[34] buttonstates, r)`:

| Id | Button | Rect (x,y,w,h) | over image rect | down image rect |
|---|---|---|---|---|
| 0 | New Game | (30, 126, 108, 29) | (0, 0, 108, 29) | (0, 87, 108, 29) |
| 1 | Options | (31, 189, 108, 29) | (0, 29, 108, 29) | (0, 116, 108, 29) |
| 2 | Quit | (30, 444, 108, 29) | (0, 58, 108, 29) | (0, 145, 108, 29) |

   (The down copy is made first, then the over copy.)
3. `mMrMoney = new MrMoney(this)` (0xd8 bytes, ctor 0x415140); `Resize(0, 0, 184, 136)`;
   `Move(-200, 0)`; `mWidgetManager->AddWidget(mMrMoney)`.
4. `ShowGameSelector()`; `ConfirmContinueGame()`.

The title music (song 11) is not stopped here.

**`MakeButton(int id, const Rect& r, Image* over, Image* down)` - 0x421470** (returns ButtonWidget*):
`mButtonImageList.push_back(over); push_back(down)`; `b = new ButtonWidget(id, this)`; `b->Resize(r)`;
`b->mDoFinger = true`; `b->mOverImage = over; b->mDownImage = down` (mButtonImage stays NULL: invisible
until hovered/pressed, the left bar art shows through); `b->mMouseInsets = Insets(1, 1, 3, 3)`;
`mWidgetManager->AddWidget(b)`; `mButtonList.push_back(b)`.

#### 5.3 Game selector

**`ShowGameSelector()` - 0x41b840.**
1. `KillDialog(3)`.
2. For song 0..13 except 11: `FadeOut(song, true, 0.01)`.
3. If mGameSelector: `SafeDeleteWidget(mGameSelector); mWidgetManager->RemoveWidget(mGameSelector)`.
4. `mMrMoney->mTargetX (+0x98) = 0; mMrMoney->mTargetY (+0x9c) = 290` (spring target, board_draw.md
   section 3; Board writes the same pair at its update 300).
5. `mGameSelector = new GameSelector(this)` (0xe0, ctor 0x4107c0); `Resize(0, 0, mWidth, mHeight)`;
   AddWidget; `BringToBack(mGameSelector)`.
6. If mBoard: `mBoard->0x403b80()` (removes the board's child widgets, state = 8); `BringToBack(mBoard)`;
   `mGameSelector->vf55()` (slide in over the board; at its end GameSelector::Update calls RemoveBoard).
7. `if (!mMusicInterface->IsPlaying(11)) FadeIn(12, 89, 0.005, false)`.

**`RemoveGameSelector()` - 0x41b9c0** (GameSelector::Update, end of slide-out): if mGameSelector:
RemoveWidget, SafeDeleteWidget, NULL.

**`StartGame()` - 0x41c170** (GameSelector::StartGame vf54, after it stored mGameType/mSkillLevel):
`t = mGameType`; if `!mDontShowHelp[t] && !mHelpShownThisSession[t]`: `mShowInstructor = true; ShowHelp();
mHelpShownThisSession[t] = true`; else `mShowInstructor = false; NewGame()`.

#### 5.4 Help

**`ShowHelp()` - 0x41b6f0** (StartGame, OptionsDialog button 4):
1. `KillDialog(1)` (Options).
2. If mHelpWidget already exists: nothing more.
3. If mGameSelector: `mGameSelector->vf56()` (slide out).
4. `mHelpWidget = new HelpWidget(this)` (0xa0, ctor 0x410f20); `Resize(0, 0, mWidth, mHeight)`; AddWidget;
   if mBoard: `PutInfront(mHelpWidget, mBoard)` else `BringToBack(mHelpWidget)`.
5. If mBoard: `mBoard->Pause(true)`.

**`RemoveHelp()` - 0x41b7f0** (HelpWidget, NewGame): if mHelpWidget: RemoveWidget, SafeDeleteWidget,
NULL; if mBoard: `mBoard->Pause(false)`.

#### 5.5 Board

**`NewGame()` - 0x41bfb0** (StartGame, HelpWidget, Board click in state 6, ButtonDepress dialog 5):
1. `KillDialog(3)`; `RemoveHelp()`.
2. Music: `FadeOut(s, true, 0.005)` for s = 0..13; then, by mGameType:
   type 0: `FadeIn(3, 26, 0.005, false)` else `FadeOut(3, true, 0.005)`;
   type 1: `FadeIn(6, 44, 0.005, false)` else `FadeOut(6, ...)`;
   type 2: `FadeIn(0, 0, 0.005, false)` else `FadeOut(0, ...)`.
3. If mGameSelector: `mGameSelector->vf56()` (slide out).
4. `RemoveBoard()`.
5. `mBoard = new Board(this)` (0x810 bytes, ctor 0x40e760); `Resize(0, 0, mWidth, mHeight)`; AddWidget;
   `BringToBack(mBoard)`; `SetFocus(mBoard)`.

**`RemoveBoard()` - 0x41b6a0** (NewGame, GameSelector::Update): if mBoard: RemoveWidget, SafeDeleteWidget,
`mBoard = NULL`, `KillDialog(3)`.

#### 5.6 Focus and modal pausing

- `ModalOpen()` 0x41bf00 (slot 73): `if (mBoard && mGameType == 0) mBoard->Pause(true)`.
- `ModalClose()` 0x41bf20 (slot 74): `if (mBoard && mGameType == 0) mBoard->Pause(false)`.
- `GotFocus()` 0x41bf40 (slot 75): `SexyAppBase::GotFocus()` (empty); `if (mBoard) mBoard->Pause(false)`.
- `LostFocus()` 0x41bf60 (slot 76): base (empty); `if (mBoard) mBoard->Pause(true)`.

`Board::Pause(bool)` (0x402930) is a counter (+0x5c0), so modal dialogs pause only Action (timed); losing
focus pauses every type. The framework calls ModalOpen/ModalClose when a modal dialog is added/killed.
MoneyDialog lights also stop while `!mHasFocus`.

### 6. Dialogs

#### 6.1 `NewDialog(int id, bool modal, header, lines, footer, int mode)` - 0x41bd90, slot 25

`d = new Dialog(IMG[29] olddialog, IMG[33] dialogbutton, id, modal, header, lines, footer, mode)`
(0x148 bytes); `SetButtonFont(mFont3); SetHeaderFont(mFont1); SetLinesFont(mFont3)`; `SetColor(0,
Color(255,255,255)); SetColor(1, Color(0,255,0)); SetColor(3, Color(0,255,0))`; `mContentInsets =
Insets(36, 36, 36, 36)`; `Resize(206, 42, 348, GetPreferredHeight(348))`. Not added (SexyAppBase::DoDialog
= KillDialog(id) + NewDialog + AddDialog(id), returns the dialog). The game's own dialogs (Options, High
Scores, Record Score, Register) are MoneyDialog/Dialog subclasses created directly.

#### 6.2 Dialog ids

A FOOTER button is `ID_FOOTER = ID_YES = 1000`, so it arrives as 2000+id; "-" = no such button.

| Id | Dialog | Created by | Mode | YES / OK / footer (2000+id) | NO / CANCEL (3000+id) |
|---|---|---|---|---|---|
| 0 | confirm end current game | ConfirmNewGame | YES_NO | KillDialog(0); ShowGameSelector() | KillDialog(0) |
| 1 | OptionsDialog | ShowOptionsDialog | FOOTER "Done" | apply fullscreen/cursors; KillDialog(1) | - |
| 2 | RegisterDialog (not ported) | ShowRegisterDialog | NONE (own buttons 2002/3002) | validate | KillDialog(2) |
| 3 | RecordScoreDialog (non-modal) | ShowRecordScoreDialog | OK_CANCEL | record name | KillDialog(3) |
| 4 | (never created) | - | - | KillDialog(4); ShowRegisterDialog() | KillDialog(4) |
| 5 | continue saved game? | ConfirmContinueGame | YES_NO | KillDialog(5); NewGame(); mBoard->LoadGame(); DeleteSaveGame() | KillDialog(5); DeleteSaveGame() |
| 6 | check for updates? | ConfirmCheckForUpdates | YES_NO | KillDialog(6); DoUpdateCheck() | KillDialog(6) |
| 7 | UpdateCheckDialog (not ported) | DoUpdateCheck | FOOTER (Cancel) | KillDialog(7) | - |
| 8 | invalid license code | ShowInvalidCodeDialog | FOOTER | KillDialog(8) | - |
| 10 | opening browser | OpenURL | NONE | - | - |
| 11 | URL open failed | URLOpenFailed | FOOTER | KillDialog(11) | - |
| 12 | confirm quit | ConfirmQuit | YES_NO | KillDialog(12); Shutdown() | KillDialog(12) |
| 13 | HighScoreDialog | ShowHighScoreDialog | FOOTER | KillDialog(13) | - |
| 14 | CreditsDialog | ShowCreditsDialog | (screens.md) | KillDialog(14) | - |
| 15 | register prompts (3 variants) | ShowOneLevelOnlyDialog, ShowNextLevelLockedDialog, ShowRegisterToRecordScoreDialog | YES_NO | KillDialog(15); ShowRegisterDialog() | KillDialog(15) |
| 16 | info: already registered / enter a name | ShowRegisterDialog, ButtonDepress(2003) | FOOTER | KillDialog(16) | - |
| 10007 | update available (UpdateCheckDialog result, mId+10000) | UpdateCheckDialog::Update | YES_NO | KillDialog(10007); KillDialog(7); OpenUpdateURL() | KillDialog(10007); KillDialog(7) |
| 20007 | up to date (mId+20000) | UpdateCheckDialog::Update | FOOTER | KillDialog(20007); KillDialog(7) | - |

#### 6.3 Dialog creators

| Function | Dialog | Content / layout |
|---|---|---|
| ConfirmNewGame 0x4207e0 | 0 | If `!mBoard`: if `mGameSelector && !mGameSelector->+0xa1` (not sliding out) `mGameSelector->vf54()` (StartGame) and return; otherwise ShowGameSelector(). If `mBoard->mState (+0xa0) == 6` (game over): ShowGameSelector(). Else `DoDialog(0, true, GetString("NEW_GAME"), GetString("ARE_YOU_SURE_YOU_WANT_TO_END_THE_CURRENT_GAME"), "", YES_NO)`. |
| ShowOptionsDialog 0x41bbe0 | 1 | `new OptionsDialog(this)` (0x198, ctor 0x416720); SetColor(0 white, 1 green, 3 green); `mContentInsets = Insets(36,36,36,36)`; `Resize(186, 26, 388, GetPreferredHeight(388))`; `AddDialog(1, d)`. |
| ShowRegisterDialog 0x41cd60 | 2 / 16 | If `mIsRegistered` (always on Steam): `DoDialog(16, true, GetString("ALREADY_REGISTERED"), GetString("YOU_HAVE_ALREADY_REGISTERED_BIG_MONEY"), GetString("DIALOG_BUTTON_OK"), FOOTER)`. Else `new RegisterDialog(this)` (0x170, ctor 0x4188e0); colours as above; insets 36; `Resize((mWidth - 381) / 2, 32, 381, 385)` (C division; fixed height); `AddDialog(2, d)`. |
| ShowRecordScoreDialog(const string& lines) 0x41bd00 | 3 | `new RecordScoreDialog(this, lines)` (0x168, ctor 0x417a30); `Resize(250, 32, 316, GetPreferredHeight(316))`; `AddDialog(3, d)`. Called by HighScoreWidget::CheckForHighScore. |
| ConfirmContinueGame 0x420370 | 5 | If `Board::SaveGameExists()` (0x4010f0: fopen("savegame.dat","rb") succeeds): `DoDialog(5, true, GetString("CONTINUE"), GetString("DO_YOU_WANT_TO_CONTINUE_YOUR_LAST_GAME"), "", YES_NO)`. |
| ConfirmCheckForUpdates 0x420a30 | 6 | `UpdateCheckQueried()` (slot 91); `d = DoDialog(6, true, GetString("UPDATES"), GetString("DO_YOU_WANT_TO_CHECK_FOR_UPDATES_TO_BIG_MONEY"), "", YES_NO)`; `d->Resize(146, 50, 348, GetPreferredHeight(348))`. Not ported. |
| DoUpdateCheck 0x420c50 | 7 | `KillDialog(7)`; `d = new UpdateCheckDialog(IMG[29], IMG[33], IMG[47] waitbar, 7)` (0x158); fonts mFont3/mFont1/mFont3, colours, insets 36; `Resize(146, 50, 348, prefH(348))`; `AddDialog(7, d)`; `mInternetManager->0x473120("http://www.popcap.com/win32updatecheck.php?prod=" + mProdName + "&ver=" + mProductVersion + "&referid=" + mReferId)`. Not ported. |
| ShowInvalidCodeDialog 0x41d090 | 8 | `DoDialog(8, true, GetString("INVALID_CODE"), GetString("THE_LICENSE_CODE_YOU_ENTERED_IS_NOT_VALID__"), GetString("DIALOG_BUTTON_OK"), FOOTER)`; `PlaySample(35)`. Not ported. |
| OpenURL 0x420140 (slot 35) | 10 | `d = NewDialog(10, true, "", GetString("OPENING_BROWSER"), "", NONE)`; insets 36; `Resize(164, 140, 312, 98)`; `AddDialog(10, d)`; `return SexyAppBase::OpenURL(url, shutdownOnOpen)`. |
| URLOpenSucceeded 0x41bf80 (slot 34) | - | `SexyApp::URLOpenSucceeded(url)`; `KillDialog(10)`. |
| URLOpenFailed 0x41fe80 (slot 33) | 11 | `SexyAppBase::URLOpenFailed(url)`; `KillDialog(10)`; `d = NewDialog(11, true, GetString("OPEN_BROWSER"), GetString("PLEASE_OPEN_THE_FOLLOWING_URL_IN_YOUR_BROWSER") + url, GetString("OK"), FOOTER)` (the string ends in `&cr;`); `Resize(100, 50, 440, GetPreferredHeight(440))`; `AddDialog(11, d)`. |
| ConfirmQuit 0x420560 | 12 | `lines = GetString("ARE_YOU_SURE_YOU_WANT_TO_QUIT_THE_GAME")`; if `mBoard && mBoard->IsGameInProgress()`: `lines += GetString("YOUR_CURRENT_GAME_IN_PROGRESS_WILL_BE_SAVED_SO_YOU_CAN_CONTINUE_NEXT_TIME")` (starts with two `&cr;`); `DoDialog(12, true, GetString("QUIT"), lines, "", YES_NO)`. |
| ShowHighScoreDialog 0x41ba00 | 13 | `new HighScoreDialog(this)` (0x160, ctor 0x412310); SetColor(0 white, 1 green, 3 green); `Resize(196, 16, 388, GetPreferredHeight(388))`; `AddDialog(13, d)`. (OptionsDialog button 2.) |
| ShowCreditsDialog 0x41baf0 | 14 | `new CreditsDialog(this)` (0x168, ctor 0x40ef30); same colours; `Resize(196, 16, 388, GetPreferredHeight(388))`; `AddDialog(14, d)`. (OptionsDialog button 6.) |
| ShowOneLevelOnlyDialog 0x41d2e0 | 15 | `DoDialog(15, true, GetString("PUZZLE_MODE_LOCKED"), GetString("YOU_CAN_ONLY_PLAY_ONE_LEVEL__DO_YOU_WANT_TO_REGISTER_NOW_TO_ENABLE_THE_FULL_GAME"), "", YES_NO)`. Called by Board::Update (trial). |
| ShowNextLevelLockedDialog 0x41d4c0 | 15 | same header, lines `THE_NEXT_LEVEL_IS_LOCKED_UNTIL_YOU_REGISTER__DO_YOU_WANT_TO_REGISTER_NOW`. Called by Board::ButtonDepress when `mGameType == 2 && !mIsRegistered`. |
| ShowRegisterToRecordScoreDialog(const string& lines) 0x41d6a0 | 15 | `DoDialog(15, true, GetString("HIGH_SCORE"), lines, "", YES_NO)`. Called by HighScoreWidget::CheckForHighScore (unregistered). |

Colours: "white" = Color(255,255,255), "green" = Color(0,255,0); indices 0 header, 1 lines, 3 button text.

#### 6.4 `ButtonPress(int id)` - 0x41bfa0, slot 1 and `ButtonDepress(int id)` - 0x420f10, slot 2

ButtonPress: `PlaySample(10)` (buttonclick) for every id, including dialog presses (2000+/3000+id).

ButtonDepress: `r = id % 10000`.
- `2000 <= r < 3000` (YES/OK/footer), `d = id - 2000`:
  Compare order (disassembly): `d > 10007` -> (`d == 20007` ? KillDialog(20007), KillDialog(7) :
  KillDialog(d)); `d == 10007` -> KillDialog(10007), KillDialog(7), `OpenUpdateURL()` (slot 95);
  `d > 15` -> KillDialog(d); else the switch below. (Id 22007 has r = 2007, so the 20007 case is reached.)
  - d = 0: KillDialog(0); ShowGameSelector().
  - d = 1 (Options Done): `o = GetDialog(1)`; `windowed = !o->+0x16c(fullscreen Checkbox)->IsChecked()`;
    `if (mIsWindowed != windowed) SwitchScreenMode(windowed)` (slot 63); `EnableCustomCursors(o->+0x170
    (custom cursors Checkbox)->IsChecked())`; KillDialog(1).
  - d = 2 (Register, not ported): `rd = GetDialog(2)`; `name = rd->+0x150 (EditWidget)->mString`,
    `code = rd->+0x154->mString`; if `code` is empty: `rd->ButtonDepress(0)` (its own "register online"
    button: OpenRegisterPage + leave fullscreen); else if `!Validate(name, code)`:
    ShowInvalidCodeDialog() (dialog 2 stays); else `mUserName = name; mRegCode = code; mIsRegistered =
    true; KillDialog(2); mWidgetManager->MarkAllDirty(); if (mBoard && mBoard->+0x7dc (HighScoreWidget*))
    it->CheckForHighScore()` (slot 54).
  - d = 3 (Record score): `name = GetDialog(3)->+0x160 (EditWidget)->mString`; if empty:
    `DoDialog(16, true, GetString("ENTER_NAME"), GetString("IF_YOU_WANT_TO_RECORD_YOUR_SCORE_ENTER_YOUR_NAME_IN_THE_HIGH_SCORE_BOX"), GetString("OK"), FOOTER)` (dialog 3 stays open); else
    `mBoard->+0x7dc->AddHighScore(name)` (HighScoreWidget slot 55, no NULL check); `mLastScoreName = name`;
    KillDialog(3).
  - d = 4: KillDialog(4); ShowRegisterDialog().
  - d = 5: KillDialog(5); NewGame(); `mBoard->LoadGame()` (0x4075d0); `Board::DeleteSaveGame()` (0x401440:
    DeleteFileA("savegame.dat")).
  - d = 6: KillDialog(6); DoUpdateCheck().
  - d = 12: KillDialog(12); Shutdown() (slot 40).
  - d = 15: KillDialog(15); ShowRegisterDialog().
  - any other d (7..11, 13, 14, 16..10006): KillDialog(d).
- `3000 <= r < 4000` (NO/CANCEL), `d = id - 3000`: d == 5: KillDialog(5); DeleteSaveGame(). d == 10007:
  KillDialog(10007); KillDialog(7). Else KillDialog(d).
- Other r: nothing.

Then, for every id, a second switch: id 0 -> ConfirmNewGame(); 1 -> ShowOptionsDialog(); 2 ->
ConfirmQuit(). (Only the side buttons have ids 0..2 with the app as listener.)

#### 6.5 `KillDialog(int id)` - 0x41c1c0, slot 71

`if (!SexyAppBase::KillDialog(id)) return false;` then if the dialog map is now empty
(`mDialogMap.size() == 0`, +0x2f0) `mWidgetManager->SetFocus(mBoard)` (mBoard may be NULL); return true.

#### 6.6 Registration, trial and update branches (not ported)

The Steam build's partner.xml has `NoReg=true` (SexyApp::InitPropertiesHook sets mIsRegistered and
mBuildUnlocked) and `DontUpdate=true`; `RealArcade` and `Cobranded` are absent/false. Consequences:
- **Registered path** (the one to port): ShowRegisterDialog only ever shows dialog 16 "Already registered"
  (OptionsDialog's Register button 1 is left unsized when mBuildUnlocked and Check Updates (5) when
  DontUpdate, dialogs.md; TitleScreen calls it only when its trial logic wants registration); HighScoreWidget always
  takes the ShowRecordScoreDialog branch; Board never calls the puzzle-lock dialogs (they need
  `!mIsRegistered`); TitleScreen hides the register link and trial badge (screens.md).
- Dead on Steam: RegisterDialog (dialog 2) and its validation in ButtonDepress(2002), dialogs 8 and 15,
  the RealArcade registry probe in Init, LockableButton's lock flag (never set anyway), the update check
  (LoadingThreadCompleted -> ShouldCheckForUpdate is false; OptionsDialog's button 5 is not laid out when
  DontUpdate), dialogs 6/7/10007/20007, and
  `OpenRegisterPage` (RegisterDialog only).
- OpenURL is reached only from OptionsDialog: the partner links ("OptionsLinkURL0", a Steam store page)
  and button 3 (never created, a PopCap page). Port: neither is ported (the user's decision, 2026-10-24),
  so nothing opens a URL; the dialog is 308 high, as with no links, and WinBMApp::OpenURL stays, unused.

### 7. Registry, properties and files

**Registry** (the game's own values use `HKEY_LOCAL_MACHINE\SOFTWARE\` + mRegKey =
`HKLM\SOFTWARE\PopCap\BigMoney` directly, RegOpenKeyExA; the framework's own values go through
SexyAppBase::RegistryRead/Write). Neither function closes the key (handle leak).

| Value | Type | Field | Read | Written |
|---|---|---|---|---|
| LastScoreName | REG_SZ | mLastScoreName | yes (256-byte buffer) | only if non-empty; cbData = length (no NUL) |
| SkillLevel | REG_DWORD | mSkillLevel | yes | yes |
| GameType | REG_DWORD | mGameType | yes | yes |
| NoPopLink | REG_DWORD | mNoPopLink (!= 0) | yes | no |
| DontShowHelp0/1/2 | REG_DWORD | mDontShowHelp[0..2] (!= 0) | yes | yes (0/1) |

`WriteToRegistry()` 0x41c9f0 (slot 23): `SexyApp::WriteToRegistry()`; `RegOpenKeyExA(HKLM, "SOFTWARE\\" +
mRegKey, 0, KEY_WRITE (0x20006))`; on success the writes above in table order. `ReadFromRegistry()`
0x41cb60 (slot 24): `SexyApp::ReadFromRegistry()`; open with KEY_READ (0x20019); each value read
independently (missing values keep their current value). port: keep them with the framework's other
registry values (HKLM writes need admin on modern Windows).

**Properties read by the app**: `GetBoolean("RealArcade", false)` (Init; also TitleScreen/OptionsDialog),
`GetBoolean("Cobranded", false)` (Init). Framework partner.xml properties: ProdName, RegistryKey, Width,
Height, NoReg, DontUpdate, DefaultWindowed, ReferId, RegisterLink, Title (absent). OptionsDialog reads
NumOptionsLinks / OptionsLinkText%d / OptionsLinkURL%d / DontUpdate; TitleScreen MaxPlays. Strings: every
`GetString(id)` named in section 6 plus DIALOG_BUTTON_YES/NO/OK/CANCEL.

**Files**: `properties\default.xml`, `properties\partner.xml` (framework), `images\*`, `sounds\*`,
`music\QualityTime_BigMoney.it`, `data\{bigmoneyfont1,bigmoneyfont3,scorefont,levelfont,floatfont}.txt`,
`BigMoney.ini` (RealArcade only), `savegame.dat` (Board writes/reads; the app only tests existence via
0x4010f0 and deletes via 0x401440), `hiscores.dat` (HighScoreWidget 0x412eb0 load / 0x413100 save; not
app code, see its spec). The app writes no file itself.

### 8. MoneyDialog (MoneyDialog.cpp 0x414e00-0x415140)

`class MoneyDialog : public Dialog` - the framed dialog with the running lights, base of HighScoreDialog,
OptionsDialog, RecordScoreDialog. Vtables 0x4f670c (60 slots) / 0x4f66ec (ButtonListener at +0x80).
**sizeof 0x158** (subclasses put their next vtable at +0x158).

| Off | Type | Name | Meaning | Ctor |
|---|---|---|---|---|
| 0x148 | WinBMApp* | mApp | | ctor arg 1 |
| 0x14c | | (pad) | | |
| 0x150 | double | mLightPhase | 0..1 position of the light chain | 0.0 |

**`MoneyDialog(WinBMApp* app, Image* comp, Image* btnComp, int id, bool modal, header, lines, footer, int
mode)` - 0x414e00** (9 args): `Dialog(comp, btnComp, id, modal, header, lines, footer, mode)`;
`mLightPhase = 0`; vtables; `mApp = app`. All three subclasses pass IMG[30] dialog and IMG[33]
dialogbutton (HighScoreDialog: id 13, modal, FOOTER; OptionsDialog: id 1, modal, FOOTER; RecordScoreDialog:
id 3, **not modal**, OK_CANCEL). `~MoneyDialog` 0x414e60 = ~Dialog; 0x414ee0 deleting wrapper.

**`Update()` - 0x414e80, slot 9**: `Dialog::Update()`; if `mApp->mHasFocus` (+0x498): `mLightPhase +=
0.0005; if (mLightPhase > 1.0) mLightPhase -= 1.0; MarkDirty()`. (No focus: nothing, no repaint.) One
lap = 2000 updates = 20 s.

**`Draw(Graphics* g)` - 0x414f00, slot 13** (`W/H` = mWidth/mHeight; called by the subclasses' Draw):
1. `g->SetColor(Color(0,0,0)); g->FillRect(8, 8, W - 16, H - 16)`.
2. 40 lights, i = 0..39: `t = i * 0.025 + mLightPhase; if (t > 1.0) t -= 1.0;` then (doubles, `_ftol`):
   - `t < 0.25`: `x = (int)(t * 4.0 * (W - 39)) + 18`, `y = 14` (top edge, left to right)
   - `t < 0.5`: `x = W - 21`, `y = (int)((t - 0.25) * 4.0 * (H - 34)) + 14` (right edge, downward)
   - `t < 0.75`: `x = (int)((0.75 - t) * 4.0 * (W - 39)) + 18`, `y = H - 20` (bottom, right to left)
   - else: `x = 18`, `y = (int)((1.0 - t) * 4.0 * (H - 34)) + 14` (left, upward)
   (exact operation order: `t*4.0` first, then times the int converted to double.) Store (x, y);
   `g->DrawImage(IMG[31] dialoglight, x - 6, y - 6)`.
3. `Dialog::Draw(g)` (frame, texts, buttons over the black panel and lights).
4. `g->SetDrawMode(1)` (additive); for each stored light: skip if `28 <= y <= 54` or `H - 56 <= y <=
   H - 30`; else `g->DrawImage(IMG[32] dialogglow, x - 10, y - 10)`. `g->SetDrawMode(0)`.
   (So the glow is drawn after the frame and hidden in two horizontal bands, where the side lights pass the
   header and the button rows.)

No RNG. Light spacing: 40 lights evenly in t, 0.025 apart.

### 9. LockableButton (LockableButton.cpp 0x414d80-0x414e00)

`class LockableButton : public ButtonWidget`, vtable 0x4f6604 (57 slots). **sizeof 0x120**
(`operator new(0x120)` in the GameSelector ctor 0x4107c0, the only user: the three game-type buttons).

| Off | Type | Name | Ctor |
|---|---|---|---|
| 0x118 | WinBMApp* | mApp | ctor arg 1 |
| 0x11c | bool | mLocked | false |

- `LockableButton(WinBMApp* app, int id, ButtonListener* listener)` 0x414d80: `ButtonWidget(id,
  listener)`; vtable; `mApp = app; mLocked = false`.
- `~LockableButton` 0x414dc0 = ~ButtonWidget; 0x414de0 deleting wrapper.
- `Draw(Graphics*)` 0x414dd0, slot 13: `jmp ButtonWidget::Draw` - no lock overlay. Nothing in the binary
  reads or sets mLocked or mApp after the ctor: the class is a registration leftover; the port can use a
  plain ButtonWidget (keep sizeof only if layouts are mirrored).

### 10. RegisterDialog surface (not ported)

`class RegisterDialog : public Dialog, public EditListener` (EditListener vfptr at +0x148), **sizeof 0x170**
(`operator new(0x170)` in ShowRegisterDialog). Only created when `!mIsRegistered`, which never happens on
the Steam build (NoReg=true). What the app relies on:
- ctor 0x4188e0 `RegisterDialog(WinBMApp*)`: `Dialog(IMG[30] dialog, IMG[33] dialogbutton, 2, true,
  GetString("REGISTRATION"), "", "", NONE)`; +0x14c mApp; fonts mFont3/mFont1/mFont3; +0x150 name EditWidget
  (id 0), +0x154 code EditWidget (id 1; tab links between the two); buttons +0x15c ButtonWidget(3002, app)
  (cancel -> app NO branch -> KillDialog(2)), +0x160 ButtonWidget(2002, app) (OK -> app validates),
  +0x158 ButtonWidget(0, this) ("register online"); each button image is a CopyImage of IMG[65] register.
  Draw (0x4181a0) uses IMG[65]/IMG[66]; Update (0x418140) animates a phase at +0x168 every 8 updates.
- EditListener::EditWidgetText 0x418240: id 0 (name, Enter) -> focus the code field; else
  `mDialogListener->ButtonDepress(mId + 2000)` (= OK).
- ButtonDepress 0x419160: id 0 -> `app->OpenRegisterPage(defines)` (slot 93) and, if fullscreen,
  `SwitchScreenMode(true)`; other ids -> Dialog::ButtonDepress.
- GetPreferredHeight 0x417f90 returns 0x160 (352).

### 11. Cross-module references

#### 11.1 Other classes' members this module uses

| Class.member | Use |
|---|---|
| Board+0xa0 (int, mState) | `== 6` game over (ConfirmNewGame); IsGameInProgress |
| Board+0xa4 (bool) | game started (IsGameInProgress 0x4010d0 = `+0xa4 && +0xa0 != 6`) |
| Board+0x7dc (HighScoreWidget*) | the game-over high score widget: CheckForHighScore (slot 54 = jmp 0x4133b0), AddHighScore(name) (slot 55 0x414530) |
| GameSelector+0xa1 (bool) | sliding out (ConfirmNewGame) |
| GameSelector slots 54 / 55 / 56 | StartGame (0x410340) / slide in over the board (0x4103e0) / slide out (0x410590) |
| MrMoney+0x98, +0x9c (int mTargetX, mTargetY) | set to 0 and 290 by ShowGameSelector (spring target, board_draw.md section 3) |
| OptionsDialog+0x16c / +0x170 (Checkbox*) | fullscreen / custom cursors (IsChecked = slot 55) |
| RegisterDialog+0x150 / +0x154 (EditWidget*) | name / code; EditWidget+0x84 = mString |
| RecordScoreDialog+0x160 (EditWidget*) | name |
| Dialog+0x114 (Insets mContentInsets) | set to 36,36,36,36 |
| ButtonWidget +0x74 mDoFinger, +0x64 mMouseInsets, +0xac mOverImage, +0xb0 mDownImage | MakeButton |

Functions of other modules called: Board ctor 0x40e760 `Board(WinBMApp*)`, Board::Pause(bool) 0x402930,
IsGameInProgress 0x4010d0, SaveGame 0x403300, LoadGame 0x4075d0, static SaveGameExists 0x4010f0, static
DeleteSaveGame 0x401440, 0x403b80 (remove child widgets + state 8); TitleScreen ctor 0x41b120; GameSelector
ctor 0x4107c0; HelpWidget ctor 0x410f20; MrMoney ctor 0x415140; OptionsDialog ctor 0x416720;
HighScoreDialog ctor 0x412310; CreditsDialog ctor 0x40ef30; RecordScoreDialog ctor 0x417a30; RegisterDialog
ctor 0x4188e0; UpdateCheckDialog ctor 0x42acd0 (framework); InternetManager 0x473120 (framework).

#### 11.2 App functions called by other modules

| Caller | App function |
|---|---|
| TitleScreen::ButtonDepress 0x419730 | Shutdown (slot 40), TitleScreenIsFinished (slot 96) |
| TitleScreen::Update 0x41a810 | ShowRegisterDialog 0x41cd60 |
| GameSelector::Update 0x40fe90 | RemoveBoard 0x41b6a0, RemoveGameSelector 0x41b9c0 |
| GameSelector::StartGame (vf54) 0x410340 | StartGame 0x41c170 |
| HelpWidget::ButtonPress 0x410eb0 | NewGame 0x41bfb0, RemoveHelp 0x41b7f0 (writes mDontShowHelp[mGameType]) |
| OptionsDialog::ButtonDepress 0x416210 | 1 ShowRegisterDialog, 2 ShowHighScoreDialog, 3 OpenURL("http://www.popcap.com"), 4 ShowHelp, 5 DoUpdateCheck, 6 ShowCreditsDialog, 100..199 OpenURL(GetString("OptionsLinkURL%d", "http://www.popcap.com")) |
| Board::ButtonDepress 0x40e580 | NewGame (state 6), ShowNextLevelLockedDialog 0x41d4c0 |
| Board::Update 0x40c9f0 | ShowOneLevelOnlyDialog 0x41d2e0 |
| HighScoreWidget 0x4133b0 | ShowRecordScoreDialog 0x41bd00, ShowRegisterToRecordScoreDialog 0x41d6a0 |
| (framework, 6 callers) | 0x41c200 `operator==(const std::string&, const char*)` (COMDAT) |

#### 11.3 App fields used by other modules (my names)

From a scan of every `mApp`-relative access in the game code (0x401000-0x421830) plus the other analyses
(section 11.4 lists their files):

| App offset | Name | Used by |
|---|---|---|
| 0x6d0 | mBoard | HelpWidget ctor 0x410f20 and ButtonPress 0x410eb0, HighScoreWidget CheckForHighScore 0x4133b0 / AddHighScore 0x414530 (mBoard->mScore +0x490) |
| 0x6d4 | mMrMoney | Board (0x4083d0, 0x408650, 0x408a60, 0x409520, 0x409e00, 0x40b120, Update), GameSelector (Update, vf55, vf56, 0x40fcb0), HighScoreWidget 0x4133b0 |
| 0x6e4 | mImages | everyone (Board, CreditsDialog, GameSelector, HelpWidget, HighScoreWidget, MoneyDialog, OverlayWidget, StartButton, TitleScreen, Options/Record/Register dialogs) |
| 0x6e8 | mBrightCoinsImage | Board 0x402060 |
| 0x6ec | mFont1 | Board, HelpWidget, HighScoreWidget, 0x408110 |
| 0x6f4 | mFont1OutlineNoShadow | CreditsDialog::Draw |
| 0x6f8 | mFont1Underlined | HelpWidget ctor |
| 0x6fc | mFont1OutlineUnderlined | TitleScreen ctor (play link) |
| 0x700 | mFont3 | Board, CreditsDialog ctor, HighScoreWidget, TitleScreen::Draw, dialogs |
| 0x704 | mFont3Outline | TitleScreen::Draw |
| 0x70c | mFont3OutlineUnderlined | TitleScreen ctor (register link) |
| 0x710 | mScoreFont | Board::Draw |
| 0x714 | mLevelFont | Board::Draw |
| 0x718 | mFloatFont | Board 0x407ea0 (floating points) |
| 0x738 / 0x73c | mCheckboxCheckedImage / mCheckboxUncheckedImage | OptionsDialog ctor |
| 0x740 | mStartArrowsMirrored | StartButton::Draw |
| 0x744..0x770 | mGameTypeButtonImages | GameSelector ctor |
| 0x774..0x7a0 | mSkillButtonImages | GameSelector ctor |
| 0x7a4..0x7c4 | mSelectorButtonImages | GameSelector ctor |
| 0x7c8 (+4n) | mMrMoneyImages | MrMoney::Draw |
| 0x7dc + 4n, n=1..4 | mBillImages[n-1] | Board::Draw |
| 0x7f0 | mCobrandImage | TitleScreen::Draw |
| 0x814 | mSkillLevel | Board (0x401a20, SaveGame, LoadGame, 0x40a110), GameSelector (StartGame w, ctor r) |
| 0x818 | mGameType | Board (~20 functions), GameSelector, HighScoreWidget (Draw, AddHighScore), HelpWidget::ButtonPress |
| 0x81c, 0x81d | mHint81C, mHint81D | Board 0x408a60 (w), 0x409520, 0x40aea0 (r) |
| 0x81e | mHint81E | Board::Update 0x40c9f0 |
| 0x81f | mMovedThisSession | Board 0x40b120 (w), Board::Update (r) |
| 0x821 | mWelcomePlayed | GameSelector::Update |
| 0x822 + type | mDontShowHelp | HelpWidget::ButtonPress |
| 0x825 | mShowInstructor | Board 0x401fe0 |

SexyApp/SexyAppBase fields read by others (not mine, for reference): mWidth 0x88, mHeight 0x8c,
mDDInterface 0x330, mMusicInterface 0x338, mHasFocus 0x498, mLoaded 0x4c3, mVariation 0x604,
mProductVersion 0x35c, mIsRegistered 0x6bc, mBuildUnlocked 0x6bd, mTimesPlayed 0x6c0 (Board increments),
mTimedOut 0x6c8 (TitleScreen writes).

#### 11.4 Other analyses read (reconciliation)

Read at the end: `screens.md` (TitleScreen, StartButton, GameSelector, HelpWidget, CreditsDialog),
`dialogs.md` (HighScoreWidget, HighScoreDialog, OptionsDialog, RecordScoreDialog), `board_draw.md`
(Board drawing, MrMoney, Points, OverlayWidget). Every app offset they use is in the table of 11.3 with
the same name, except:
- board_draw.md calls +0x7c8 `mMrMoneyFrames` in its MrMoney::Draw text (= mMrMoneyImages here).
- dialogs.md calls 0x41cd60 "register flow" (= ShowRegisterDialog) and 0x420c50 "update check"
  (= DoUpdateCheck).
- screens.md reports mSkillLevel as 1..3 (adopted above).
- Their extra framework fields (mWidth/mHeight, mWidgetManager, mDDInterface, mMusicInterface,
  mProductVersion, mLoaded, mVariation, mIsRegistered, mBuildUnlocked, mTimesPlayed, mTimedOut) are
  SexyApp/SexyAppBase members, named as in FRAMEWORK_12.md.
- `board_logic.md` was still being written when this file was finished; its head agrees (game types
  0/1/2 = Action/Strategy/Puzzle, skill 1..3, 0x403300 writes and 0x4075d0 reads savegame.dat). Its final
  names for 0x81c..0x81f and 0x825 should replace the provisional ones here.
No other analysis uses an app offset that is missing from the layout table.

### 12. Open questions

1. Dialog id 9 is not used by any code found (the ids jump from 8 to 10).
2. Song ids 10 and 13 are loaded but never played by any code found (the order list also has a part at
   order 99, after the last `+++`, that no song id starts). Songs 1/4/7 "level-up jingles" are named from
   the Board calls only (Board spec owns the meaning).
3. `mHint81C/81D/81E` and `mMovedThisSession`: names are guesses from the Board's use (instructor hints);
   the Board spec should give the final names.
4. Dialog 4 has a YES handler (ShowRegisterDialog) but nothing creates dialog 4 in the game or the
   framework code seen.
5. Which cobrand file ImageLib would pick (cobrand.jpg vs cobrand.gif) - irrelevant while Cobranded=false.
6. x87 precision: the bill sizes 1344x35 / 1152x30 assume 53-bit precision in the loading thread; at
   64-bit precision they would be 1343x34 / 1151x29. MoneyDialog light positions run on the main thread;
   if a D3D device switched it to 24-bit, `_ftol` results could differ by 1 px at boundaries.
7. The registry key is never closed and HKLM is used; the port should map these values onto its normal
   per-user registry store.

---

## Part 4: Screens (title, selector, help, credits)

Behaviour spec of the game modules TitleScreen.cpp, StartButton.cpp, GameSelector.cpp, HelpWidget.cpp and
CreditsDialog.cpp of the carved game PE (`re/bigmoney/bin/BigMoneyGame.exe`, image base 0x400000), written
from the Ghidra export (`re/bigmoney/ghidra/`) and checked against the disassembly wherever Ghidra hid x87
operands or stack arguments. Framework is SexyAppFramework 1.2 (`docs/bigmoney/FRAMEWORK_12.md`).

Conventions:
- Offsets are absolute from the object start, hex. `app` is the `WinBMApp*` the screen keeps.
- `IMG[n]` is the app image table `app+0x6e4` (an `Image**`, filled by `WinBMApp` from the name table at
  0x5285b8: `images\<name>`); `IMG[n]` is the pointer at `*(app+0x6e4) + 4*n`. Names/sizes of the ones used
  here are in "Images" below. `w(IMG[n])`/`h(IMG[n])` = `Image::mWidth/mHeight` (folded getters
  `get_field_04`/`get_field_08`).
- Fonts are WinBMApp fields (all `ImageFont*`, created in `WinBMApp::Init` 0x41d9a0; names as in app.md):
  `app+0x6ec` mFont1 (data\bigmoneyfont1.txt); `+0x6f0` mFont1NoShadow; `+0x6f4` mFont1OutlineNoShadow;
  `+0x6f8` mFont1Underlined; `+0x6fc` mFont1OutlineUnderlined; `+0x700` mFont3 (data\bigmoneyfont3.txt);
  `+0x704` mFont3Outline; `+0x708` mFont3Underlined; `+0x70c` mFont3OutlineUnderlined; (`+0x710`
  mScoreFont, `+0x714` mLevelFont, `+0x718` mFloatFont, `+0x71c` mScoreFontSmall are not used here).
  "font1"/"font3" below mean mFont1/mFont3 with the tags named. The tags select layers of the
  descriptors (framework ImageFont): bigmoneyfont1.txt = Main (images/fontlefont, ascent 16, lowercase
  mapped to uppercase) + Shadow (same glyphs, offset (2,2), colour x(0,0,0,0.75), dropped by "noshadow") +
  Outline (images/outlinefont1, offset (-2,-1), x(0,0,0,0.5), drawn below, needs "outline") + underline
  layers (images/fontunderline) for "underlined"; bigmoneyfont3.txt = Main (images/bigmoneyfont3, ascent
  13, uppercase mapped to lowercase), no shadow layer (commented out), Outline (images/outlinefont2,
  (-2,-1), x(0,0,0,0.75)) and underline layers. Each layer's colour is `SetColor` colour x its ColorMult
  (so shadow/outline stay black, with the text colour's alpha scaled).
- WinBMApp field and method names follow `app.md` (mBoard 0x6d0, mMrMoney 0x6d4,
  mTitleScreen 0x6d8, mGameSelector 0x6dc, mHelpWidget 0x6e0, mImages 0x6e4, mGameType 0x818,
  mSkillLevel 0x814, StartGame 0x41c170, NewGame 0x41bfb0, ...); see Cross-module references.
- "Update" runs at the framework's 100 updates/s. `mUpdateCnt` (+0x24) is incremented by
  `WidgetContainer::Update` (called first by every Update override here through `Widget::Update` /
  `ButtonWidget::Update` / `Dialog::Update`).
- x87: MSVC 7 runs the FPU at 53-bit precision, so the formulas below are plain `double` arithmetic;
  `_ftol` truncates toward zero. `fsin`/`fcos` are the x87 instructions (the port can use `sin`/`cos`;
  see Open questions for the 1-ulp risk at truncation boundaries).
- Draw modes: 0 = normal, 1 = additive (`Graphics::SetDrawMode`).
- RNG: the only random draw in the five modules is one C-runtime `rand()` in `GameSelector::Update`
  (see there and "RNG" below). No MTRand use.

### Images used by the screens

`IMG[n]` index, table offset, file (in `images/`; `_x.gif` is the alpha mask of `x`), size in pixels.

| n | +off | file | size | used by |
|---|---|---|---|---|
| 4 | 0x10 | titleback.jpg | 640x480 | TitleScreen background |
| 5 | 0x14 | barandlogo.gif (+_barandlogo.gif) | 540x149 | TitleScreen bottom bar |
| 6 | 0x18 | goldbar.jpg (+_goldbar.gif) | 370x76 | TitleScreen loading bar |
| 7 | 0x1c | mrmoneybags.jpg (+_mrmoneybags.gif) | 613x403 | TitleScreen Mr. Moneybags |
| 8 | 0x20 | titlecigartip.gif (+mask) | 35x50 | TitleScreen cigar glow |
| 9 | 0x24 | titleeyeglow.gif | 163x99 | TitleScreen eye glow (additive) |
| 10 | 0x28 | bigmoneylogo.jpg (+mask) | 472x239 | TitleScreen logo |
| 11 | 0x2c | deluxor.gif (+mask) | 155x150 | TitleScreen "Deluxe" badge (hue/lum copy) |
| 12 | 0x30 | dollarstrip.gif (+mask) | 640x20 | TitleScreen scrolling dollar rows |
| 13 | 0x34 | trialversion.jpg (+mask) | 124x126 | TitleScreen trial badge |
| 29 | 0x74 | olddialog.gif (+mask) | 192x192 | CreditsDialog frame (Dialog component image) |
| 33 | 0x84 | dialogbutton.gif | 96x36 | CreditsDialog button component image |
| 68 | 0x110 | _clouds.jpg | 512x512 | CreditsDialog cloud background |

(The GameSelector/StartButton/HelpWidget images are listed in their sections.)

---

### TitleScreen (TitleScreen.cpp 0x419630-0x41b670)

`class TitleScreen : public Widget, public ButtonListener`; vtables 0x4fbf0c (54 slots) and 0x4fbeec
(ButtonListener at +0x80). **sizeof 0x118** (`operator new(0x118)` in `WinBMApp::Init` 0x41d9a0).
Created once by `WinBMApp::Init`: `new TitleScreen(app)`, stored in `app+0x6d8`, then
`Resize(0, 0, app->mWidth, app->mHeight)` (640x480) and `mWidgetManager->AddWidget`.
`WinBMApp::TitleScreenIsFinished` (new slot 96, 0x4215c0) removes it, `SafeDeleteWidget`s it and clears
`app+0x6d8`.

The module also holds three COMDAT std::string helpers used everywhere (not TitleScreen code):
0x419780 `std::string::compare(pos, n, const char*, count)`, 0x419800 `operator+(const string&, const
char*)`, 0x4198b0 `operator+(const string&, const string&)`. The functions from 0x41b670 on (WinMain,
`WinBMApp` Show/Remove helpers) belong to WinBMApp, see Cross-module references.

#### Layout

| Off | Type | Name | Meaning | Init (ctor) | R/W |
|---|---|---|---|---|---|
| 0x00 | | Widget | base | | |
| 0x80 | vfptr | ButtonListener | links' listener | 0x4fbeec | |
| 0x84 | WinBMApp* | mApp | | ctor arg | all |
| 0x88 | double | mLoadProgress | displayed loading-bar fraction 0..1 | 0.0 | Update w, Draw r |
| 0x90 | double | mNagProgress | trial nag timer 0..1 | 0.0 | Update |
| 0x98 | bool | mLoadDone | bar full and play link shown | false | Update |
| 0x9c | int | mShownTimesPlayed | number shown in "YOU HAVE PLAYED %d TIMES", counts up | see ctor | Update w, Draw r |
| 0xa0 | bool | mWantRegister | open the register dialog on the next Update once loaded | false | ButtonDepress, Update |
| 0xa1 | bool | mTrialExpired | plays used up (unregistered) | false | Update w, Draw/ButtonDepress r |
| 0xa4 | int | mNagSeconds | seconds the unregistered bar takes | see ctor | Update r |
| 0xa8 | int | mMaxPlays | trial play limit | see ctor | Update, Draw |
| 0xac | bool | mCountdownTrial | true: "You have N plays remaining" texts; false: "UNREGISTERED" texts | true | Draw r (nothing here writes false) |
| 0xb0 | int | mLogoY | y offset of the logo (negative = above) | -h(IMG[10]) = -239 | Update, Draw |
| 0xb8 | double | mLogoAngle | slide parameter | 0.0 | Update |
| 0xc0 | int | mMoneybagsDY | Mr. Moneybags offset below his rest position | h(IMG[7]) = 403 | Update, Draw |
| 0xc8 | double | mMoneybagsAngle | | 0.0 | Update |
| 0xd0 | int | mBarDY | bottom bar (and texts on it) offset below rest | h(IMG[5]) = 149 | Update, Draw |
| 0xd8 | double | mBarAngle | | 0.0 | Update |
| 0xe0 | int | mStripScroll | dollar-row scroll 64..0 | 0 | Update, Draw |
| 0xe8 | double | mCigarPhase | 0..1 | 0.0 | Update, Draw |
| 0xf0 | double | mEyePhase | 0..1 | 0.0 | Update, Draw |
| 0xf8 | HyperlinkWidget* | mRegisterLink | id 0 | new | |
| 0xfc | HyperlinkWidget* | mPlayLink | id 1 (play, or quit when expired) | new | |
| 0x100 | MemoryImage* | mDeluxeImage | recoloured copy of IMG[11] deluxor | new | Draw |
| 0x104 | ulong* | mDeluxeHSL | IMG[11] pixels as HSL (`new ulong[w*h]`) | | Draw r |
| 0x108 | ulong* | mDeluxeWork | scratch HSL buffer | | Draw |
| 0x10c | int | mDeluxeLum | luminosity added to the badge, 256 -> 0 | 256 | Update, Draw |
| 0x110 | bool | mDeluxeDirty | rebuild mDeluxeImage | **not initialised** (only read when 0x111 set, and Update sets it then) | |
| 0x111 | bool | mDeluxeShown | badge visible | false | Update w, Draw r |

Pads: 0xb4, 0xc4, 0xd4, 0xe4 (doubles 8-aligned), 0x112-0x117.

#### TitleScreen::TitleScreen(WinBMApp* theApp) - 0x41b120 (thiscall, 1 arg)

1. `Widget()`, ButtonListener vfptr, `mApp = theApp`, vtables, `mCountdownTrial = true`.
2. `mMaxPlays`: compares `app->mVariation` (SexyApp +0x604, std::string, whole-string equality) with
   "CntDn30" or "CntUp30" -> 30; "CntDn10" or "CntUp10" -> 10; otherwise
   `app->GetInteger("MaxPlays", 10)`.
3. `mLoadProgress = mNagProgress = 0.0`, `mLoadDone = mWantRegister = mTrialExpired = false`.
4. `mNagSeconds = (app->mTimesPlayed > 10) ? app->mTimesPlayed + 5 : 10`; then
   `if (app->GetBoolean("RealArcade", false)) mNagSeconds = 0`.
5. `mShownTimesPlayed = (t < 2) ? 0 : (t < 10) ? t : 2` with `t = app->mTimesPlayed` (SexyApp +0x6c0).
6. `mRegisterLink = new HyperlinkWidget(0, this+0x80)`; `mUnderlineSize (+0x138) = 0`;
   `SetFont(app+0x70c)` (font3 outline+underlined); `mColor (+0x118) = Color(32,255,32)` (mOverColor stays
   white); `SetVisible(!app->mIsRegistered)` (SexyApp +0x6bc).
7. `mPlayLink = new HyperlinkWidget(1, this+0x80)`; `mUnderlineSize = 0`; `SetFont(app+0x6fc)` (font1
   outline+underlined); `SetVisible(false)`; `mColor = Color(32,255,32)`.
8. `mStripScroll = 0; mLogoAngle = 0; mLogoY = -h(IMG[10]); mMoneybagsAngle = 0; mMoneybagsDY = h(IMG[7]);
   mBarAngle = 0; mBarDY = h(IMG[5]); mCigarPhase = mEyePhase = 0`.
9. Deluxe badge: `src = dynamic_cast<MemoryImage*>(IMG[11])`; `mDeluxeImage = new MemoryImage(app)`
   (`operator new(0x4c)`); `Create(w(src), h(src))`; `SetImageMode(true, true)`; `SetVolatile(true)`;
   `mDeluxeHSL = new ulong[w*h]`; `app->RGBToHSL(src->GetBits(), mDeluxeHSL, w*h)`;
   `mDeluxeWork = new ulong[w*h]`; `mDeluxeLum = 256`; `mDeluxeShown = false`.
   HSL packing (SAF 1.2 `SexyAppBase::RGBToHSL`): hue bits 0-7, saturation 8-15, luminosity 16-23,
   alpha kept in 24-31.
10. Calls `Update()` once (so `mUpdateCnt` is 1 when the screen is added; see Update for what that first
    call does: Mr. Moneybags already moved one step, nothing else visible changes).

#### ~TitleScreen - 0x419630 (vf00 0x419760 is the deleting wrapper)

Deletes `mRegisterLink`, `mPlayLink` (virtual dtor), `free(mDeluxeHSL)`, `free(mDeluxeWork)`, deletes
`mDeluxeImage`, then `~Widget`.

#### AddedToManager(WidgetManager* m) - 0x4196d0, slot 7 / RemovedFromManager - 0x419700, slot 8

`WidgetContainer::AddedToManager(m)`, then `m->AddWidget(mRegisterLink)`, `m->AddWidget(mPlayLink)`
(the links are siblings in the manager, above the title). Removed: base, then `m->RemoveWidget` both.

#### Resize(int x, int y, int w, int h) - slot 25 = 0x412c50

A one-instruction `jmp Widget::Resize` (COMDAT shared with HighScoreWidget): no behaviour of its own.

#### ButtonDepress(int theId) - 0x419730, ButtonListener slot 2 (this = +0x80)

- `theId == 0` (register link): `mWantRegister = true` (acted on in Update).
- otherwise (play/quit link): `if (mTrialExpired) app->Shutdown()` (vtable slot 40, +0xa0)
  `else app->TitleScreenIsFinished()` (WinBMApp new slot 96, +0x180 = 0x4215c0: remove the title, go
  to the game selector).

No sound is played here (the framework ButtonWidget plays none). Input: no mouse/key overrides; only
the two links react (ButtonWidget: ButtonPress on mouse down - default no-op here - and ButtonDepress on
release inside).

#### Update() - 0x41a810, slot 9

1. `Widget::Update()` (mUpdateCnt++). `ra = app->GetBoolean("RealArcade", false)`.
2. Every 8th update (`mUpdateCnt % 8 == 0`):
   - `mCigarPhase += 0.05; if (mCigarPhase > 1.0) mCigarPhase -= 1.0;`
   - `mEyePhase += 0.06; if (mEyePhase > 1.0) mEyePhase -= 1.0;`
   - `mStripScroll = (mStripScroll == 0) ? 64 : mStripScroll - 1;`
   - `if (mUpdateCnt >= 180) mDeluxeShown = true;`
   - `if (mDeluxeShown && mDeluxeLum > 0) { mDeluxeLum -= 16; if (mDeluxeLum < 0) mDeluxeLum = 0;
     mDeluxeDirty = true; }`
   - `MarkDirty()`.
3. Logo slide: `if (mUpdateCnt >= 25 && mLogoY != 0)`: `mLogoAngle += 0.016`;
   `mLogoY = (mLogoAngle < 1.570795) ? -(int)(h(IMG[10]) * (1.0 - sin(mLogoAngle))) : 0`; `MarkDirty()`.
   (The x87 code negates before `_ftol`; truncation toward zero makes that equal to `-(int)(...)`.)
4. Mr. Moneybags slide (no start delay): `if (mMoneybagsDY != 0)`: `mMoneybagsAngle += 0.02`;
   `mMoneybagsDY = (mMoneybagsAngle < 1.570795) ? (int)(h(IMG[7]) * (1.0 - sin(mMoneybagsAngle))) : 0`;
   `MarkDirty()`.
5. Bar slide: `if (mUpdateCnt >= 50 && mBarDY != 0)`: `mBarAngle += 0.025`;
   `mBarDY = (mBarAngle < 1.570795) ? (int)(h(IMG[5]) * (1.0 - sin(mBarAngle))) : 0`; then
   `w = mRegisterLink->mFont->StringWidth(mRegisterLink->mLabel)`;
   `mRegisterLink->Resize(308 - (w + 4) / 2, mBarDY + 374, w + 4, 26)`; `MarkDirty()`.
   (So the register link has a 0x0 rect until update 50, then rides up with the bar.)
   Durations: logo 99 steps (updates 25..123), Moneybags 79 steps (ctor..79), bar 63 steps (50..112).
6. Trial check (only `if (!ra)`): `expired = app->mTimesPlayed >= mMaxPlays && !app->mIsRegistered`;
   if `expired != mTrialExpired`: `mTrialExpired = expired`, and if it became true `mWantRegister = true`.
7. Not expired:
   - `mPlayLink->mLabel = GetString("CLICK_HERE_TO_PLAY")`; `w` = its StringWidth;
     `mPlayLink->Resize(308 - (w + 8) / 2, 435, w + 8, 32)` (every update).
   - `mRegisterLink->mLabel = GetString(ra ? "CLICK_HERE_TO_REGISTER_NOW" :
     "REGISTER_NOW_TO_UNLOCK_THIS_GAME")`.
   - `mNagProgress += 0.01 / (double)mNagSeconds; if (mNagProgress > 1.0) mNagProgress = 1.0`
     (mNagSeconds 0 gives +inf, clamped to 1.0 at once).
   - `n = (int)(app->mTimesPlayed * mNagProgress)`; `if (n <= app->mTimesPlayed && n >
     mShownTimesPlayed) { mShownTimesPlayed = n; MarkDirty(); }`
   - `if (!mLoadDone && mUpdateCnt % 4 == 0)`: `p = app->GetLoadingThreadProgress()` (slot 59, double);
     if not registered `p = min(p, mNagProgress)` (the code calls GetLoadingThreadProgress a second time
     for the min result); `mLoadProgress = p`; if `mLoadProgress == 1.0` (exact) and `app->mLoaded`
     (SexyAppBase +0x4c3): `mPlayLink->SetVisible(true); mLoadDone = true; mLoadProgress = 1.0`;
     `MarkDirty()`.
   So the unregistered bar takes at least `mNagSeconds` seconds (10 s, or plays+5 s after 10 plays);
   registered (Steam) it follows the real loading progress.
8. Expired:
   - only when `mUpdateCnt == 1000` exactly: `mPlayLink->mLabel = GetString("CLICK_HERE_TO_QUIT")`,
     `Resize(308 - (w + 8) / 2, 440, w + 8, 26)`, `SetVisible(true)`.
   - `mRegisterLink->mLabel = GetString("CLICK_HERE_TO_REGISTER_NOW")`.
9. Always: `mRegisterLink->SetVisible(!app->mIsRegistered)`; `if (app->mLoaded && mWantRegister) {
   app->ShowRegisterDialog() (0x41cd60); mWantRegister = false; }`;
   `app+0x6c8 (SexyApp::mTimedOut) = mTrialExpired`.

#### Draw(Graphics* g) - 0x419950, slot 13

`W/H` = this widget's `mWidth/mHeight` (640x480). In order:

1. `app->GetBoolean("RealArcade", false)` (result unused).
2. `DrawImage(IMG[4] titleback, 0, 0)`.
3. Dollar rows: `for (i = 0; i < (H + 31) / 32 + 2; i++) DrawImage(IMG[12] dollarstrip, (i & 1) ? 13 : 0,
   mStripScroll - 64 + 32 * i)` (17 rows for H = 480; scrolls up 1 px per 8 updates, period 64).
4. `my = mMoneybagsDY + H - h(IMG[7])` (= mMoneybagsDY + 77):
   `DrawImage(IMG[7] mrmoneybags, 0, my + 6)`.
5. Cigar tip: `SetColorizeImages(true)`; `SetColor(Color(255,255,255, (int)(fabs(mCigarPhase - 0.5) *
   510.0)))`; `DrawImage(IMG[8] titlecigartip, 26, my + 184)`; `SetColorizeImages(false)`.
6. Eye glow: `SetColorizeImages(true); SetDrawMode(1)`; `SetColor(Color(0,200,0, (int)(fabs(mEyePhase -
   0.5) * 510.0)))`; `DrawImage(IMG[9] titleeyeglow, 122, my + 90)`; `SetDrawMode(0);
   SetColorizeImages(false)`.
7. Bar: `bx = W - w(IMG[5])` (100), `by = H + mBarDY - h(IMG[5])` (331 + mBarDY):
   `DrawImage(IMG[5] barandlogo, bx, by, Rect(0, 0, 540, 149))`.
8. Bar shine: for `i = 0..4` with `a = 1275, 1020, 765, 510, 255` (`a / 6` = 212, 170, 127, 85, 42):
   on a copy of `g`: `SetColorizeImages(true)`, `SetColor(Color(255,255,255, a / 6))`,
   `ClipRect(bx, by + 125 + i, 540, 1)`, `DrawImage(IMG[5], bx, by)` - i.e. rows 125..129 of the bar are
   drawn a second time with those alphas.
9. `DrawImage(IMG[10] bigmoneylogo, 84, mLogoY - 8)`.
10. Badge corner: if `app+0x7f0` (an `Image*`, the cobrand logo; NULL unless cobranded) is set:
    `DrawImage(app+0x7f0, 8, H + mBarDY - h(it))`; else if `!app->mIsRegistered`:
    `DrawImage(IMG[13] trialversion, 0, H + mBarDY - h(IMG[13]))`.
11. Loading bar at `(120, mBarDY + 381)`:
    - `mLoadProgress >= 1.0`: `DrawImage(IMG[6] goldbar, 120, mBarDY + 381)`.
    - `0 < mLoadProgress < 1`: `w = (int)(mLoadProgress * 318.0 + 25.0)`;
      `DrawImage(IMG[6], 120, mBarDY + 381, Rect(0, 0, w, 76))`; then a 16-column fade:
      `for (i = 0; i < 16; i++) { SetColorizeImages(true); SetColor(Color(255,255,255,
      (int)(cos(i * 0.0981746875) * 255.0))); DrawImage(IMG[6], w + i + 120, mBarDY + 381,
      Rect(w + i, 0, 1, 76)); SetColorizeImages(false); }` (0.0981746875 ~ pi/32; alphas 255..24).
    - `<= 0`: nothing.
12. Deluxe badge, `if (mDeluxeShown)`: if `mDeluxeDirty`: for every pixel `k`:
    `v = mDeluxeHSL[k]; L = ((v >> 16) & 0xff) + mDeluxeLum; mDeluxeWork[k] = (L > 255) ? (v | 0xff0000)
    : ((v & 0xff00ffff) | (L << 16))`; `app->HSLToRGB(mDeluxeWork, mDeluxeImage->GetBits(), w*h)`;
    `mDeluxeImage->BitsChanged()`; `mDeluxeDirty = false`. Then `DrawImage(mDeluxeImage, 478, 18)`.
    Effect: the badge appears at update 180 fully white and fades to its own colours over 16 steps of 8
    updates.
13. Status line (font `app+0x704` font3 outline, white): `y0 = 370`;
    - unregistered, `mCountdownTrial`: `r = max(0, mMaxPlays - app->mTimesPlayed)`;
      expired: `GetString("YOU_HAVE_ZERO_PLAYS_REMAINING_YOU_MUST_REGISTER_TO_PLAY_MORE")`;
      else `r > 1`: literal `sprintf("You have %d plays remaining", r)` (not from the string table);
      else (r <= 1, also r = 0 when not expired, e.g. RealArcade) literal "You have 1 play remaining".
    - unregistered, `!mCountdownTrial`: expired: `GetString("YOU_MUST_REGISTER_ONLINE_NOW_TO_CONTINUE_PLAYING")`;
      else `mShownTimesPlayed >= 2`: `sprintf(GetString("YOU_HAVE_PLAYED_X_TIMES_WITHOUT_REGISTERING"),
      mShownTimesPlayed)`; else `GetString("UNREGISTERED_VERSION")`.
    - registered: `!app->mBuildUnlocked` (SexyApp +0x6bd): `GetString("THANKS_FOR_REGISTERING")`, else
      empty; and `y0 = 397`.
    - then, overriding all of that, `if (app->mTimesPlayed == 0 && mLoadProgress < 1.0)` the text is
      `GetString("PLEASE_WAIT")` ("Please wait, unpacking sounds...").
    Strings from the table go through `sprintf(buf, str)` into a stack char buffer (a `%` in them would be
    interpreted). `DrawString(buf, 308 - StringWidth(buf) / 2, mBarDY + y0)` (C division).
    Steam build (`partner.xml` NoReg=true: `SexyApp::InitPropertiesHook` 0x48d990 sets mIsRegistered and
    mBuildUnlocked): empty text, except "PLEASE_WAIT" on the very first run while loading.
14. Version (font `app+0x700` font3, white): `s = GetString("VERSION_") + " " + app->mProductVersion`
    (SexyAppBase +0x35c; "Version " already ends in a space, so two spaces);
    `DrawString(s, W - StringWidth(s) - 16, 211)`.

The two HyperlinkWidgets draw themselves (framework `HyperlinkWidget::Draw`: label centred in the rect,
y = (height + ascent) / 2 - 1, colour mColor, mOverColor (white) under the mouse; no underline drawn since
mUnderlineSize = 0 - the underline comes from the "underlined" font tag).

---

### StartButton (StartButton.cpp 0x419260-0x419630)

`class StartButton : public ButtonWidget`; vtable 0x4fbe04 (57 slots). **sizeof 0x138** (`operator new(0x138)`
in the GameSelector ctor). The "START" bar at the bottom of the game selector.

| Off | Type | Name | Meaning | Init |
|---|---|---|---|---|
| 0x000 | | ButtonWidget | base (`mDoFinger` +0x74 = true) | |
| 0x118 | WinBMApp* | mApp | | ctor arg |
| 0x11c | | pad | | |
| 0x120 | double | mHiliteAlpha | 0..1 cross-fade to the highlighted text cell | 0.0 |
| 0x128 | double | mHiliteSpeed | 0.05; **never read** (Update uses a literal 0.1) | 0.05 |
| 0x130 | int | mUnused130 | zeroed when the highlight starts rising, never read | not set by the ctor |
| 0x134 | int | mArrowFrame | 0..7 chase frame; 5 = all lit | 0 |

Images: `IMG[48]` selectback.jpg 467x480; `IMG[59]` starttext.gif 280x100 (two 280x50 cells: y 0 normal,
y 50 highlighted); `IMG[62]` startarrows.gif 30x60 (two 30x30 cells: y 0 off, y 30 lit); `app+0x740` =
horizontally mirrored copy of IMG[62] (made by `WinBMApp::LoadingThreadProc` 0x41efd0 with `CopyImage` +
`MirrorImage`).

#### StartButton::StartButton(WinBMApp* theApp, int theId, ButtonListener* theListener) - 0x419260

`ButtonWidget(theId, theListener)`; zeroes +0x120/+0x124 (mHiliteAlpha) and +0x134 (mArrowFrame);
`mApp = theApp`; `mDoFinger = true`; `mHiliteSpeed = 0.05`. GameSelector creates it with id 0.
(~StartButton 0x4192c0 is just `~ButtonWidget`; 0x4193d0 is the deleting wrapper.)

#### Update() - 0x4192d0, slot 9

`ButtonWidget::Update()`. Then:
- not hovered and not pressed (`!mIsOver && !mIsDown`, +0x51/+0x50): every 16th update
  (`mUpdateCnt % 16 == 0`) `mArrowFrame = (mArrowFrame + 1) % 8; MarkDirty()`; and
  `if (mHiliteAlpha > 0) { mHiliteAlpha -= 0.1; MarkDirty(); }`
- hovered or pressed: `if (mArrowFrame != 5) { mArrowFrame = 5; MarkDirty(); }`;
  `if (mHiliteAlpha < 1.0) { mUnused130 = 0; mHiliteAlpha += 0.1; MarkDirty(); }`
- clamp: `if (mHiliteAlpha < 0) mHiliteAlpha = 0; else if (mHiliteAlpha > 1) mHiliteAlpha = 1`.

#### Draw(Graphics* g) - 0x4193f0, slot 13

`a = (int)(mHiliteAlpha * 255.0)`.
1. If `IsButtonDown()` (slot 56): `g->Translate(1, 1)`; `DrawImage(IMG[48] selectback, 0, 0, Rect(7, 429,
   455, 38))` (repaints the selector background under the shifted button).
2. `SetColorizeImages(true)`; `SetColor(255,255,255, 255 - a)`; `DrawImage(IMG[59], 87, -7, Rect(0, 0, 280,
   50))`; `SetColor(255,255,255, a)`; `DrawImage(IMG[59], 87, -7, Rect(0, 50, 280, 50))`;
   `SetColorizeImages(false)`.
3. For `i = 0, 1, 2`: `src = (mArrowFrame == i || mArrowFrame == 5) ? Rect(0, 30, 30, 30) : Rect(0, 0, 30,
   30)`; `DrawImage(IMG[62], 16 + 22*i, 4, src)`; `DrawImage(app+0x740, mWidth - 22*i - 46, 4, src)`.
   Idle cycle (16 updates per frame): frames 0/1/2 light arrow 0/1/2 on both sides, 3/4 none, 5 all,
   6/7 none. Hover keeps all lit.
4. If `IsButtonDown()`: `g->Translate(-1, -1)`.

No label is drawn (ButtonWidget::Draw is not called).

---

### GameSelector (GameSelector.cpp 0x40fb90-0x410d80)

`class GameSelector : public Widget, public ButtonListener`; vtables 0x4f5c8c (57 slots: three new virtuals
54-56) and 0x4f5c68 (ButtonListener at +0x80). **sizeof 0xe0** (`operator new(0xe0)` in
`WinBMApp::ShowGameSelector` 0x41b840). The mode/skill chooser panel on the right of the screen
(selectback.jpg at x = 173), with Mr. Moneybags' OverlayWidget, 3 game-type buttons, 3 skill buttons and
the StartButton.

Game types (`app+0x818`, registry `GameType`): **0 = Action, 1 = Strategy, 2 = Puzzle** (voice sounds 42
"action" / 43 "strategy" / 44 "puzzle"; the HelpWidget texts). Skill (`app+0x814`, registry `SkillLevel`):
**1..3**.

#### Layout

| Off | Type | Name | Meaning | Init |
|---|---|---|---|---|
| 0x00 | | Widget | base | |
| 0x80 | vfptr | ButtonListener | | 0x4f5c68 |
| 0x84 | WinBMApp* | mApp | | ctor arg |
| 0x88 | int | mSkill | 1..3, -1 = none | -1, then SetSkill(app+0x814) |
| 0x8c | int | mGameType | 0..2, -1 = none | -1, then SetGameType(app+0x818) |
| 0x90 | int | mPanelY | y of the panel and its buttons; nothing writes it after the ctor | 0 |
| 0x94 | | pad | unused | |
| 0x98 | int[2] | mUnused98 | zeroed, never read | 0 |
| 0xa0 | bool | mSlidingIn | roll-down animation running | false |
| 0xa1 | bool | mSlidingOut | roll-up animation running | false |
| 0xa4 | DDImage* | mSnapshot | screen capture used while sliding | NULL |
| 0xa8 | int | mMidHeight | height of the stretched middle strip, 0..362 | set by the slides |
| 0xac | | pad | unused | |
| 0xb0 | int | mSlideY | y of the top strip while sliding | set by the slides |
| 0xb4 | StartButton* | mStartButton | id 0 | new |
| 0xb8 | ButtonWidget* | mButton0 | id 0, images app+0x7a4/0x7a8/0x7ac; **never resized** (0x0 rect: inert) | new |
| 0xbc | ButtonWidget* | mButton1 | id 1, images app+0x7b0/0x7b4/0x7b8; never resized | new |
| 0xc0 | ButtonWidget* | mButton2 | id 2, images app+0x7bc/0x7c0/0x7c4; never resized | new |
| 0xc4 | LockableButton*[3] | mTypeButtons | ids 20, 21, 22 | new |
| 0xd0 | ButtonWidget*[3] | mSkillButtons | ids 11, 12, 13 | new |
| 0xdc | OverlayWidget* | mOverlay | Mr. Moneybags' side-bar overlay; its `+0x88` double is its alpha | new |

Button images are `Image*` fields of WinBMApp, built in `WinBMApp::LoadingThreadProc` (0x41efd0):
- game type `i`: `app+0x744+16*i` normal, `+0x748+16*i` over, `+0x74c+16*i` down/selected (+0x750+16i
  unused): 154x266 cells cut from IMG[48] selectback at (5+154*i, 58) and composited with IMG[49]
  gametypestext.gif (465x248, 154-px columns) and IMG[50] illustrations.jpg (452x128, plus a grey copy):
  normal = black 50% wash, text tinted (120,120,120), grey illustration tinted (120,120,120); over = 25%
  wash, text (220,220,0), grey illustration (180,180,180); selected = text (120,255,0), colour illustration.
- skill `i`: `app+0x774+16*i` normal, `+0x778+16*i` over, `+0x77c+16*i` down: 113x52 cells cut from
  selectback at ({29,186,339}[i], 359) with IMG[51] skillstext.gif (467x52) tinted (120,120,120) /
  (220,220,0), and IMG[52] skillspressed.jpg for the pressed cell.
- `app+0x7b0` = selectback cut (7, 429, 455, 38) (the StartButton area). No code found writes
  `app+0x7a4..0x7ac` or `0x7b4..0x7c4` (NULL), so mButton0/mButton2 have no images at all.
(The exact compositing belongs to the WinBMApp spec; listed here so the two can be reconciled.)

#### GameSelector::GameSelector(WinBMApp* theApp) - 0x4107c0 (thiscall, 1 arg)

1. `Widget()`, vtables, `mApp = theApp`.
2. For `i = 0..2`: `mSkillButtons[i] = new ButtonWidget(11 + i, this+0x80)`, `mDoFinger = true`,
   `mButtonImage/mOverImage/mDownImage = app+0x774/0x778/0x77c + 16*i`;
   `mTypeButtons[i] = new LockableButton(app, 20 + i, this+0x80)` (0x120 bytes, ctor 0x414d80), finger,
   images `app+0x744/0x748/0x74c + 16*i`.
3. `mStartButton = new StartButton(app, 0, this+0x80)`.
4. `mButton0/1/2 = new ButtonWidget(0/1/2, this+0x80)` with the image triples above, finger on.
5. `mOverlay = new OverlayWidget(app)` (0x90 bytes, ctor 0x417400; alpha starts at 1.0).
6. `mGameType = mSkill = -1`; `SetGameType(app+0x818)`; `SetSkill(app+0x814)` (no sound: mUpdateCnt is 0).
7. `mUnused98 = 0; mPanelY = 0; mSlidingOut = mSlidingIn = false; mSnapshot = NULL`.

#### ~GameSelector - 0x40fb90 (vf00 0x410bd0 is the deleting wrapper)

Deletes mStartButton, mButton0..2, mSkillButtons[0..2], mTypeButtons[0..2], mOverlay, mSnapshot (in that
order), then `~Widget`.

#### SetGameType(int t) - 0x40fcb0 (non-virtual)

`mGameType = t`; for each `i`: if `i == t` all three images = `app+0x74c+16i` (selected look, even under
the mouse) else normal/over/down = `app+0x744/0x748/0x74c + 16i`; `MarkDirty()` on the button. Then, only
if `mUpdateCnt > 0` and Mr. Moneybags is not saying one of the two greetings (`*(int*)(app->mMrMoney
(app+0x6d4) + 0xc0)` is neither 23 nor 9): `app->PlaySample(42 + t)` ("action", "strategy", "puzzle")
and `MrMoney::PlayAnim(t)` (0x415200 on app+0x6d4).

#### SetSkill(int s) - 0x40fdc0 (non-virtual)

`mSkill = s`; for each `i`: if `i == s - 1` all three images = `app+0x77c+16i`, else
`app+0x774/0x778/0x77c + 16i`; `MarkDirty()` each. No sound.

#### Update() - 0x40fe90, slot 9

1. `Widget::Update()`.
2. At `mUpdateCnt == 100` exactly:
   - first selector of the session (`app+0x821` false): `app->PlaySample(37)` ("welcome"),
     `MrMoney::PlayAnim(23)`, `app+0x821 = true`;
   - otherwise **one `rand()` (C runtime, `idiv` so the C `%`)**: `if (rand() % 10 == 0) {
     app->PlaySample(47) ("loveit"); MrMoney::PlayAnim(9); }`.
3. Sliding out (`mSlidingOut`): `mSlideY -= 2`; `if (mMidHeight > 0) mMidHeight -= 8`;
   `if (mMidHeight <= 0) { mMidHeight = 0; mOverlay->mAlpha -= 0.05; if (mOverlay->mAlpha <= 0)
   WinBMApp::RemoveGameSelector(app) (0x41b9c0: RemoveWidget + SafeDeleteWidget, app+0x6dc = NULL); }`;
   `MarkDirty()`; return. (mMidHeight reaches <= 0 on the 46th update, which is also the first of 20 fade
   steps; with the overlay at 1.0 the selector is removed on the 65th update, `1.0 - 20*0.05` being
   -3.2e-16.)
4. Else sliding in (`mSlidingIn`): `mOverlay->mAlpha += 0.05`, clamp to 1.0; `mSlideY += 2`; when
   `mSlideY > 0`: `mSlideY = 0; mSlidingIn = false;` re-add the children to the manager (mButton0, mButton1,
   mButton2, mStartButton, then for i = 0..2 mTypeButtons[i], mSkillButtons[i]); `OrderInManagerChanged()`;
   `WinBMApp::RemoveBoard(app)` (0x41b6a0: removes and SafeDeletes the old Board, `app+0x6d0 = NULL`,
   `KillDialog(3)`). Then `mMidHeight += 8`, clamp to 362; `MarkDirty()`.
   (Starts at mSlideY = -100, so it ends on the 51st update; the middle is full after 46.)

#### AddedToManager(m) - 0x4100b0, slot 7 / RemovedFromManager(m) - 0x410140, slot 8

Base, then `m->AddWidget` (resp. RemoveWidget) of: mButton0, mButton1, mButton2, mStartButton, then for
i = 0..2: mTypeButtons[i], mSkillButtons[i]; then mOverlay.

#### OrderInManagerChanged() - 0x4101d0, slot 15

`PutInfront(w, this)` for mOverlay, then for i = 0..2 mTypeButtons[i], mSkillButtons[i], then
mStartButton, mButton2, mButton1, mButton0. Each call puts `w` directly above `this`, so the final order
above the selector is mButton0, mButton1, mButton2, mStartButton, skill2, type2, skill1, type1, skill0,
type0, mOverlay (topmost).

#### Resize(int x, int y, int w, int h) - 0x410d50, slot 25

`Widget::Resize(x, y, w, h)`, then layout (0x410260, non-virtual):
- `mTypeButtons[i]->Resize(178 + 154*i, mPanelY + 58, 154, 266)` (x 178, 332, 486);
- `mSkillButtons[i]->Resize({29, 186, 339}[i] + 173, mPanelY + 359, 113, 52)` (x 202, 359, 512);
- `mStartButton->Resize(180, mPanelY + 429, 455, 38)`;
- `mOverlay->Resize(0, 0, mWidth, mHeight)`.
mButton0..2 are not positioned (see Open questions).

#### ButtonPress(int theId, int theClickCount) - 0x410330, ButtonListener slot 0

`app->PlaySample(10)` ("buttonclick") for any button (mouse down). The selector itself has no mouse or
key overrides; the app's `ConfirmNewGame` 0x4207e0 (side button "New Game") calls StartGame (Screen flow 7).

#### ButtonDepress(int theId) - 0x410730, ButtonListener slot 2

Jump table on `theId` (0..22):
- 0 (mStartButton, mButton0): `StartGame()`.
- 11..13 (skill): `if (mSkill == id - 10) StartGame(); else SetSkill(id - 10);`
- 20..22 (type): `if (mGameType == id - 20) StartGame(); else SetGameType(id - 20);`
- others (1, 2): nothing.
So a second click on the selected type or skill starts the game.

#### StartGame() - 0x410340, new slot 54

`if (mGameType == -1) mGameType = 0; if (mSkill == -1) mSkill = 1;` `app+0x818 = mGameType;
app+0x814 = mSkill;` `WinBMApp::StartGame(app)` (0x41c170: if neither `app+0x822+type`
(DontShowHelp) nor `app+0x7f4+type` (help shown this session): `app+0x825 = true`, ShowHelp, mark shown;
else `app+0x825 = false`, NewGame 0x41bfb0). Then `SetDisabled(true)` on mSkillButtons[0..2],
mTypeButtons[0..2] and mButton1 (not on mStartButton).

#### SlideIn() - 0x4103e0, new slot 55 (called by WinBMApp::ShowGameSelector when a Board exists)

Only if `!mSlidingIn`: delete the old mSnapshot; `mSnapshot = new DDImage(app->mDDInterface)` (0xcc
bytes); `Create(mWidth, mHeight)`; `SetImageMode(false, false)`; hide mOverlay and the MrMoney widget
(`app+0x6d4`) (`SetVisible(false)`); `Graphics g(mSnapshot)`; `mWidgetManager->DrawWidgetsTo(&g)`
(captures the whole screen including this selector drawn normally); show both again;
`mOverlay->mAlpha = 0.0`; `mHasTransparencies = mHasAlpha = true`; `mSlidingIn = true; mMidHeight = 0;
mSlideY = -100`; remove mButton0..2, mStartButton and the six buttons from the manager.

#### SlideOut() - 0x410590, new slot 56 (called by ShowHelp and NewGame)

Only if `!mSlidingOut`: same snapshot capture (mOverlay alpha untouched); `mHasTransparencies = mHasAlpha
= true; mMouseVisible = false; mSlidingOut = true; mMidHeight = 362; mSlideY = 0`; remove the children.

#### Draw(Graphics* g) - 0x410bf0, slot 13

- Not sliding: `DrawImage(IMG[48] selectback, 173, mPanelY)`.
- Sliding: `SetFastStretch(true)`, then from mSnapshot (`DrawImage(img, destRect, srcRect)`):
  top `Rect(173, mSlideY, 467, 58) <- Rect(173, 0, 467, 58)`;
  middle `Rect(173, mSlideY + 58, 467, mMidHeight) <- Rect(173, 58, 467, 362)` (stretched);
  bottom `Rect(173, mSlideY + 58 + mMidHeight, 467, 57) <- Rect(173, 420, 467, 57)`;
  `SetFastStretch(false)`. A window-shade unroll (in) / roll-up (out) of the captured panel.

#### IsPointVisible - slot 44 = 0x417880

Folded `Widget::IsPointVisible` (returns true) in every vtable of this group; not a real override.

---

### HelpWidget (HelpWidget.cpp 0x410d80-0x412280)

`class HelpWidget : public Widget, public ButtonListener`; vtables 0x4f5d94 (54 slots) and 0x4f5d74.
**sizeof 0xa0** (`operator new(0xa0)` in `WinBMApp::ShowHelp` 0x41b6f0). Full-screen "How to play" page,
shown before the first game of a mode (unless DontShowHelp) and from the Options dialog's Help button.
No Update override (no animation).

| Off | Type | Name | Meaning | Init |
|---|---|---|---|---|
| 0x00 | | Widget | | |
| 0x80 | vfptr | ButtonListener | | 0x4f5d74 |
| 0x84 | WinBMApp* | mApp | | ctor arg |
| 0x88 | | pad | 8 bytes never touched | |
| 0x90 | HyperlinkWidget* | mStartLink | id 0, start / continue | new |
| 0x94 | HyperlinkWidget* | mMoreLink | id 1, "more instructions"; **never resized** (inert) | new |
| 0x98 | HyperlinkWidget* | mDontShowLink | id 2, "don't show this screen again" | new |
| 0x9c | int | mPage | 0 = instructions; nothing in the binary sets it non-zero | 0 |

#### HelpWidget::HelpWidget(WinBMApp* theApp) - 0x410f20

1. `Widget()`, vtables, `mApp = theApp`.
2. `mStartLink = new HyperlinkWidget(0, this+0x80)`; label `GetString(app->mBoard (+0x6d0) == NULL ?
   "CLICK_HERE_TO_START_GAME" : "CLICK_HERE_TO_CONTINUE_GAME")`; `mDoFinger = true`; `SetFont(app+0x6ec)`
   (font1); `mUnderlineSize = 0`; `mColor = (0, 249, 183)`; `w = StringWidth(label)`;
   `Resize(406 - (w + 4) / 2, 411, w + 4, 26)`.
3. `mMoreLink = new HyperlinkWidget(1, this+0x80)`; label `GetString("CLICK_HERE_FOR_MORE_INSTRUCTIONS")`;
   finger; `SetFont(app+0x6f8)` (font1 underlined); `mUnderlineSize = 0`; `mColor = (217, 33, 153)`;
   StringWidth is computed and discarded - no Resize.
4. `mDontShowLink = new HyperlinkWidget(2, this+0x80)`; label `GetString("DONT_SHOW_THIS_SCREEN_AGAIN")`;
   finger; `SetFont(app+0x6ec)`; `mUnderlineSize = 0`; `mColor = (98,98,98)`; `mOverColor = (212,212,212)`;
   `Resize(406 - (w + 4) / 2, 446, w + 4, 26)`; `if (app->mBoard) SetVisible(false)`.
5. `mPage = 0`.

#### ~HelpWidget - 0x410d80 (vf00 0x410f00)

Deletes **only mStartLink**, then `~Widget` (mMoreLink and mDontShowLink leak; they leave the manager
through RemovedFromManager).

#### AddedToManager / RemovedFromManager - 0x410df0 / 0x410e30, slots 7 / 8

Base, then Add/RemoveWidget mStartLink, mMoreLink, mDontShowLink.

#### OrderInManagerChanged() - 0x410e70, slot 15

`PutInfront(mDontShowLink, this); PutInfront(mMoreLink, this); PutInfront(mStartLink, this)` (order above
this: start, more, dontshow).

#### ButtonPress(int theId) - 0x410eb0, ButtonListener slot 1 (acts on mouse **down**)

`app->PlaySample(10)` ("buttonclick"). Then:
- id 2: `app->mDontShowHelp[app->mGameType] = true` (`app+0x822 + app+0x818`; saved by
  `WinBMApp::WriteToRegistry` as DWORD "DontShowHelp0/1/2"), then as id 0;
- id 0: `if (app->mBoard) WinBMApp::RemoveHelp(app)` (0x41b7f0: remove + SafeDelete, `app+0x6e0 = NULL`,
  `mBoard->Pause(false)`) `else WinBMApp::NewGame(app)` (0x41bfb0);
- id 1: nothing else.
(ButtonListener slot 0 is the default that forwards to slot 1, so ButtonWidget's MouseDown triggers it.)

#### Draw(Graphics* g) - 0x411590, slot 13

All text goes through `Widget::WriteString(g, s, x, y, -1, -1, true, 0, -1)` (slot 47, left-justified):
the framework parses `^RRGGBB^` colour changes (`Widget::mWriteColoredString` is the static default
true); every string used here starts with one, the alpha stays 255.

1. `SetColor(0,0,0); FillRect(0, 0, mWidth, mHeight)`; `SetColor(77,77,77); FillRect(0, 404, mWidth, 40)`
   (grey band behind the links).
2. `mPage == 0`:
   - Puzzle (`app+0x818 == 2`): `SetFont(app+0x6f8)`; `"HOW_TO_PLAY_BIG_MONEY_PUZZLE"` at (210, 28);
     `DrawImage(IMG[67] helpcircles, 527, 40, Rect(0, 330, 104, 330))`; `SetFont(app+0x6ec)`;
     `DrawImage(IMG[69] bigarrow, 154, 52)`; `"CLICK_ON_GROUPS_OF_X_OR_MORE"` (194, 93);
     `"X_COINS_TO_COLLECT_THEM"` (194, 113); bigarrow (154, 162); `"X_YOU_MUST_DROP_MONEYBAGS_OFF"`
     (194, 203); `"X_THE_X_BOTTOM_X_TO_COLLECT_THEM"` (194, 223); bigarrow (154, 272);
     `"X_THE_GAME_ENDS_WHEN_YOU_X_CANNOT"` (194, 313); `"X_COLLECT_X_ANY_MORE_COINS"` (194, 333).
   - Action / Strategy: `SetFont(app+0x6f8)`; `"X_HOW_TO_PLAY_BIG_MONEY_ACTION"` (mode 0) or
     `"X_HOW_TO_PLAY_BIG_MONEY_STRATEGY"` (mode 1) at (210, 28); `DrawImage(IMG[67], 527, 40, Rect(0, 0,
     104, 330))`; `SetFont(app+0x6ec)`; bigarrow (154, 52); `"X_CLICK_ON_GROUPS_OF_X_OR_MORE2"` (194, 93);
     `"X_COINS_TO_COLLECT_THEM2"` (194, 113); bigarrow (154, 162); `"X_COLLECTING_COINS_WILL_FILL_UP"`
     (194, 203); `"X_THE_X_MONEY_METER_X_ON_THE_RIGHT"` (194, 223); bigarrow (154, 272);
     `"X_WHEN_METER_IS_FULL_A_X_MONEYBAG"` (194, 313); `"X_WILL_DROP__GRAB_IT_TO_ADVANCE"` (194, 333).
     (Action and Strategy share the text, only the title differs.)
3. `mPage != 0` (unreachable): `GetStringVector("aHelpTexts")` is fetched and destroyed without drawing
   anything (the aHelpTexts page was never finished).
4. Always: `SetColorizeImages(true); SetColor(255,255,255,255); DrawImage(IMG[17] leftbar, 0, 0);
   SetColorizeImages(false); DrawImage(IMG[60] floatlogo, 15, 8)`.

Images: IMG[67] helpcircles.jpg 104x660 (two 104x330 cells: y 0 = Action/Strategy, y 330 = Puzzle);
IMG[69] bigarrow.gif 372x87; IMG[17] leftbar.jpg 640x480 (+_leftbar.gif mask); IMG[60] floatlogo.gif
156x76 (+mask).

---

### CreditsDialog (CreditsDialog.cpp 0x40ee10-0x40fb90)

`class CreditsDialog : public Dialog`; vtables 0x4f5aac (60 slots) and 0x4f5a88 (Dialog's ButtonListener
at +0x80). **sizeof 0x168** (`operator new(0x168)` in `WinBMApp::ShowCreditsDialog` 0x41baf0). Not a scroller:
static credit lines over a drifting, rotating cloud texture.

| Off | Type | Name | Meaning | Init |
|---|---|---|---|---|
| 0x000 | | Dialog | base (`mContentInsets` +0x114 = Insets(36,36,36,36)) | |
| 0x148 | WinBMApp* | mApp | | ctor arg |
| 0x14c | Widget* | mUnused14C | deleted in the dtor if set; nothing sets it | NULL |
| 0x150 | double | mCloudRot | rotation of the big cloud layer (radians) | 0.0 |
| 0x158 | double | mCloudX | horizontal cloud scroll, kept in (-512, 0] | 0.0 |
| 0x160 | double | mCloudY | vertical cloud scroll, (-512, 0] | 0.0 |

#### CreditsDialog::CreditsDialog(WinBMApp* theApp) - 0x40ef30

`Dialog(IMG[29] olddialog, IMG[33] dialogbutton, 14 /*id*/, true /*modal*/, "" /*header*/, "" /*lines*/,
GetString("DONE") /*footer*/, 3 /*BUTTONS_FOOTER*/)`; vtables; `mApp = theApp`;
`mContentInsets = Insets(36, 36, 36, 36)` (built with the Color(r,g,b,a) ctor, COMDAT-folded with
Insets(l,t,r,b)); `SetButtonFont(app+0x700)` (font3); the three doubles and mUnused14C = 0.

WinBMApp::ShowCreditsDialog then: `SetColor(0 HEADER, (255,255,255))`, `SetColor(1 LINES, (0,255,0))`,
`SetColor(3 BUTTON_TEXT, (0,255,0))`, `Resize(196, 16, 388, GetPreferredHeight(388))` = (196,16,388,396),
`app->AddDialog(14, dlg)`. The footer button (DONE) is handled by the app's dialog listener.

#### ~CreditsDialog - 0x40ee10 (vf00 0x40ef10)

Deletes mUnused14C if non-NULL, then `~Dialog`.

#### GetPreferredHeight(int theWidth) - 0x40ee80, slot 57

Returns 396 (0x18c).

#### Update() - 0x40ee90, slot 9

`Dialog::Update()`; `mCloudRot += 0.004`; `mCloudX += 0.2; if (mCloudX > 0) mCloudX -= 512.0`;
`mCloudY += 0.3; if (mCloudY > 0) mCloudY -= 512.0`; `MarkDirty()` (tail call). (After the first update
mCloudX = -511.8.)

#### Draw(Graphics* g) - 0x40f190, slot 13

1. `Dialog::Draw(g)` (frame from IMG[29]; the button is its own widget).
2. Clouds, on a copy `g2` of `g` clipped to `Rect(39, 34, mWidth - 78, mHeight - 110)` (= 310x286):
   `SetDrawMode(1)` (additive): `DrawImage(IMG[68] clouds, (int)mCloudX, 0)`,
   `DrawImage(IMG[68], (int)mCloudX + 512, 0)`, `DrawImage(IMG[68], 0, (int)mCloudY)`,
   `DrawImage(IMG[68], 0, (int)mCloudY + 512)`; `SetDrawMode(0)`;
   `DrawImageRotated(IMG[68], -64, -64, mCloudRot)` (framework: about the image centre; normal blend).
   IMG[68] exists only as `_clouds.jpg` (512x512 alpha mask), so it is a white image with that alpha.
3. Inner shadow, still on `g2`: for `i = 0..5` with `a = 1530 - 255*i`: `SetColor(Color(48,48,48, a / 7))`
   (218, 182, 145, 109, 72, 36); `DrawRect(39 + i, 34 + i, (mWidth - 78) - 2i, (mHeight - 110) - 2i)`.
4. Text on `g`: `SetFont(app+0x6f4)` (font1 outline+noshadow). Each line is `WriteCenteredLine(g, y, s)`
   (slot 46: x = (mWidth - StringWidth(s)) / 2); headings (0,255,0), names (229,255,5):

| y | colour | text |
|---|---|---|
| 62 | (0,255,0) | GetString("GAME_DESIGN") |
| 81 | (229,255,5) | "Jason Kapalka" (literal) |
| 107 | (0,255,0) | GetString("PROGRAMMING") |
| 126 | (229,255,5) | "Brian Fiete" |
| 152 | (0,255,0) | GetString("ART") |
| 171 | (229,255,5) | "Josh Langley, Stephen Notley" |
| 197 | (0,255,0) | GetString("MUSIC") |
| 216 | (229,255,5) | "Skaven" |
| 242 | (0,255,0) | GetString("BUSINESS") |
| 261 | (229,255,5) | "John Vechey" |
| 287 | (0,255,0) | GetString("VOICE_OF_MR_MONEYBAGS") |
| 306 | (229,255,5) | "Roch Shannon Fraser" |

(The (0,255,0) colour is set twice before the first line; harmless.)

---

### Screen flow (who creates what)

All creation/destruction is done by WinBMApp (app agent); listed so the port wires the same order.

1. `WinBMApp::Init` (0x41d9a0): fonts, then `mTitleScreen (app+0x6d8) = new TitleScreen(app)`,
   `Resize(0, 0, 640, 480)`, `mWidgetManager->AddWidget`.
2. Play link (ButtonDepress id 1) -> `WinBMApp::TitleScreenIsFinished` (vf96, 0x4215c0): removes and
   SafeDeletes the title, makes the three side buttons (MakeButton 0x421470), creates MrMoney
   (0x415140, Move(-200, 0)), then `ShowGameSelector` (0x41b840) and `ConfirmContinueGame` (0x420370).
3. `ShowGameSelector`: `KillDialog(3)`, fades out songs 0..13 except 11 (`FadeOut(i, true, 0.01)`),
   deletes an old selector, sets MrMoney `+0x98 = 0, +0x9c = 290`, `mGameSelector (app+0x6dc) = new
   GameSelector(app)`, `Resize(0, 0, 640, 480)`, `AddWidget`, `BringToBack`; if a Board exists:
   `mBoard->0x403b80()` (removes its child widgets, state 8), `BringToBack(board)`,
   `selector->SlideIn()`; if song 11 is not playing `FadeIn(12, 89, 0.005, false)`.
4. Selector StartGame -> `WinBMApp::StartGame` (0x41c170) -> either `ShowHelp` (0x41b6f0:
   `KillDialog(1)`; if no help widget: `selector->SlideOut()`, `mHelpWidget (app+0x6e0) = new
   HelpWidget(app)`, `Resize(0, 0, 640, 480)`, `AddWidget`, then `PutInfront(help, board)` or
   `BringToBack(help)`, and `mBoard->Pause(true)` (0x402930)) or `NewGame` (0x41bfb0:
   `KillDialog(3)`, RemoveHelp, music by mode, `selector->SlideOut()`, RemoveBoard, `new Board(app)`
   (0x810 bytes), add, BringToBack, SetFocus).
5. Help start link -> `RemoveHelp` (game running) or `NewGame`.
6. Options dialog "Credits" -> `WinBMApp::ShowCreditsDialog` (0x41baf0) -> CreditsDialog as dialog 14;
   "Help" -> ShowHelp.
7. `WinBMApp::ConfirmNewGame` 0x4207e0 (side button "New Game") calls `selector->StartGame()` when there
   is no board and the selector is not sliding out (else ShowGameSelector or a confirm dialog).

### Cross-module references

#### WinBMApp / SexyApp fields read or written (names: framework where it is a framework field, else mine)

| app+ | type | name | use here |
|---|---|---|---|
| 0x088/0x08c | int | mWidth/mHeight | creators' Resize |
| 0x2e4 | WidgetManager* | mWidgetManager | creators |
| 0x330 | DDInterface* | mDDInterface | GameSelector snapshot DDImage |
| 0x338 | MusicInterface* | mMusicInterface | creators (fades) |
| 0x35c | std::string | mProductVersion | TitleScreen version text |
| 0x4c3 | bool | mLoaded | TitleScreen (play link, register dialog) |
| 0x604 | std::string | mVariation | TitleScreen mMaxPlays ("CntDn30" ...) |
| 0x6bc | bool | mIsRegistered | TitleScreen |
| 0x6bd | bool | mBuildUnlocked | TitleScreen ("THANKS_FOR_REGISTERING") |
| 0x6c0 | int | mTimesPlayed | TitleScreen |
| 0x6c8 | bool | mTimedOut | **written** by TitleScreen::Update = mTrialExpired |
| 0x6d0 | Board* | mBoard | HelpWidget (start vs continue), creators |
| 0x6d4 | MrMoney* | mMrMoney | GameSelector (PlayAnim, `+0xc0` current anim id, SetVisible) |
| 0x6d8 | TitleScreen* | mTitleScreen | creator |
| 0x6dc | GameSelector* | mGameSelector | creators |
| 0x6e0 | HelpWidget* | mHelpWidget | creators |
| 0x6e4 | Image** | mImages | everywhere (`IMG[n]`) |
| 0x6ec..0x70c | ImageFont* | fonts (see Conventions) | |
| 0x740 | Image* | mStartArrowsMirrored | StartButton right arrows |
| 0x744 + 16i (+0/+4/+8) | Image* | mGameTypeButtonImages[3][4] | GameSelector type buttons |
| 0x774 + 16i (+0/+4/+8) | Image* | mSkillButtonImages[3][4] | GameSelector skill buttons |
| 0x7a4..0x7c4 | Image*[3][3] | mSelectorButtonImages (mButton0..2 x button/over/down; only 0x7b0 = start-button background is set) | GameSelector |
| 0x7f0 | Image* | mCobrandImage (NULL unless "Cobranded") | TitleScreen corner |
| 0x7f4 + mode | bool[3] | mHelpShownThisSession | WinBMApp::StartGame |
| 0x814 | int | mSkillLevel (registry "SkillLevel"); **1..3** as stored by the selector (app.md says 0..2) | GameSelector r/w |
| 0x818 | int | mGameType (registry "GameType") | GameSelector r/w, HelpWidget |
| 0x821 | bool | mWelcomePlayed | GameSelector::Update r/w |
| 0x822 + mode | bool[3] | mDontShowHelp (registry "DontShowHelp0..2") | HelpWidget writes |
| 0x825 | bool | mShowInstructor | WinBMApp::StartGame |

#### App methods called

- Virtual (slot): `PlaySample(int)` 49 (+0xc4); `GetLoadingThreadProgress()` 59 (+0xec, double);
  `Shutdown()` 40 (+0xa0); `TitleScreenIsFinished()` 96 (+0x180, 0x4215c0). Creators use
  `SafeDeleteWidget` 32, `AddDialog(int, Dialog*)` 69, `KillDialog(int)` 71.
- Non-virtual framework: `SexyAppBase::GetString` 0x4822a0, `GetBoolean` 0x47cab0, `GetInteger` 0x47caf0,
  `GetStringVector` 0x4856f0, `RGBToHSL(const ulong*, ulong*, int)` 0x478590, `HSLToRGB(const ulong*, ulong*,
  int)` 0x478520.
- WinBMApp (names from app.md): 0x41cd60 `ShowRegisterDialog` (new RegisterDialog 0x170 as dialog 2, or
  "already registered"), 0x41b9c0 `RemoveGameSelector`, 0x41b6a0 `RemoveBoard`, 0x41c170
  `StartGame` (app), 0x41b7f0 `RemoveHelp`, 0x41bfb0 `NewGame`, 0x41b6f0 `ShowHelp`, 0x41b840
  `ShowGameSelector`, 0x41baf0 `ShowCreditsDialog`, 0x41ba00 `ShowHighScoreDialog`, 0x41bbe0 `ShowOptionsDialog`,
  0x41bd00 `ShowRecordScoreDialog`, 0x41b670 `WinMain` body (Init, Start, Shutdown on the global app at
  0x52ddc8). These eleven functions sit between 0x41b670 and 0x41bd90, i.e. in the address range the
  inventory gives to TitleScreen.cpp, but they are WinBMApp methods (the TitleScreen code ends with its
  ctor at 0x41b120-0x41b66d).
- Other game classes: `MrMoney::PlayAnim(int)` 0x415200 (anims used: 0/1/2 = game type, 9, 23);
  `LockableButton::LockableButton(WinBMApp*, int id, ButtonListener*)` 0x414d80 (sizeof 0x120;
  +0x118 app, +0x11c bool locked = false; Draw = `ButtonWidget::Draw`); `OverlayWidget::OverlayWidget
  (WinBMApp*)` 0x417400 (sizeof 0x90; +0x80 app, +0x88 double alpha = 1.0, mMouseVisible false,
  mHasAlpha true; Draw: colorized IMG[17] leftbar with that alpha, then IMG[60] floatlogo).
- COMDAT bodies shared with other modules: 0x40ee80 (`return 396`, also HighScoreDialog slot 57),
  0x412c50 (`jmp Widget::Resize`, TitleScreen and HighScoreWidget slot 25), 0x417880 (Widget::IsPointVisible,
  slot 44 everywhere), 0x401ae0 (ButtonListener::ButtonPress(id, count) -> ButtonPress(id)).

#### Sounds (`app->PlaySample(id)`, ids index the table at 0x5286d8, `sounds\<name>`)

| id | name | where |
|---|---|---|
| 10 | buttonclick | GameSelector ButtonPress (any button); HelpWidget ButtonPress |
| 37 | welcome | GameSelector, update 100 of the first selector of the session |
| 42/43/44 | action/strategy/puzzle | GameSelector::SetGameType after construction |
| 47 | loveit | GameSelector, update 100, 1 in 10 (`rand() % 10 == 0`) |

All 48 sound files exist in the Steam `sounds/` folder (welcome.ogg, loveit.ogg, action/strategy/puzzle.ogg,
buttonclick.ogg included).

#### RNG

Only `GameSelector::Update` draws: one C-runtime `rand()` at `mUpdateCnt == 100` of every selector
shown after the first one of the session (none for the first). Seed/srand belong to WinBMApp.

### Files, registry, properties

No file I/O in these modules. Read: properties `RealArcade` (bool, default false), `MaxPlays` (int,
default 10; only when mVariation is none of CntDn30/CntUp30/CntDn10/CntUp10), and the SexyApp
fields set from partner.xml (`NoReg=true` -> mIsRegistered = mBuildUnlocked = true). Written state that
WinBMApp persists in the registry (`WinBMApp::WriteToRegistry` 0x41c9f0 / `ReadFromRegistry` 0x41cb60,
all REG_DWORD): `GameType` = app+0x818, `SkillLevel` = app+0x814, `DontShowHelp0/1/2` = app+0x822..0x824
(0/1). The "Don't show this screen again" control is a HyperlinkWidget, not a Checkbox; clicking it sets
the flag for the current mode and starts the game.

### Open questions

1. **x87 transcendental results.** TitleScreen slides use `fsin`, the gold-bar edge uses `fcos`, all
   truncated by `_ftol`. A C++ port with `sin`/`cos` matches except when a product lands within an ulp of
   an integer; not checked frame by frame.
2. **GameSelector mButton0..2** (ids 0/1/2, +0xb8..+0xc0) are created, added to the manager and given
   app images 0x7a4..0x7c4, but nothing resizes them (0x0 rect, so never drawn or clicked) and
   0x7a4..0x7ac/0x7b4..0x7c4 appear never to be written. Leftover buttons? StartGame disables mButton1 only.
3. **GameSelector mPanelY (+0x90)** is 0 forever as far as this module and the WinBMApp code I looked at
   go; a writer elsewhere would move the whole panel.
4. **HelpWidget page 1** (`mPage != 0`, aHelpTexts) and **mMoreLink** are unreachable/inert (no writer of
   +0x9c, no Resize of the link). The aHelpTexts strings in default.xml are therefore never shown.
5. **HelpWidget leaks** mMoreLink and mDontShowLink (dtor deletes only mStartLink). Port: delete all three.
6. **TitleScreen mCountdownTrial (+0xac)** is always true here (no writer found), so the
   `!mCountdownTrial` status texts ("UNREGISTERED VERSION", "YOU HAVE PLAYED %d TIMES...") are dead too.
7. **TitleScreen +0x110** is uninitialised but only read after Update has set it.
8. **MrMoney anim ids**: SetGameType skips the voice while `mMrMoney+0xc0` is 23 or 9 (the two greeting
   animations started at update 100); the meaning of the other MrMoney anim ids is the MrMoney spec's.
9. **Allocation sizes vs FRAMEWORK_12.md**: `operator new(0x4c)` for MemoryImage (doc says sizeof 0x50) and
   `operator new(0xcc)` for DDImage (doc says 0xd0); the framework layout doc may be 4 bytes too large for
   these two classes (irrelevant to the port, which uses its own framework).
10. **StartButton +0x128 (0.05) and +0x130** are dead data; kept in the header for layout only.
11. **Skill range, to reconcile with app.md** (which lists mSkillLevel as 0..2, ctor 1): GameSelector
    stores `id - 10` for skill buttons 11..13, i.e. 1..3, highlights button `mSkill - 1`, and defaults an
    unset skill to 1. So the values written to app+0x814 (and the registry) are 1, 2, 3; the ctor value 1
    is the lowest skill. The Board agent should check how Board reads it.

---

## Part 5: High scores and options

Sources: `re/bigmoney/ghidra/all.c` plus the disassembly of every function below (Ghidra drops stack
arguments in most of these bodies: all coordinates, colours and call arguments here were read from the
disassembly). Framework semantics from `E:\retools\refs\saf12`. The code is a close relative of
Bejeweled Deluxe's HighScoreWidget / HighScoreDialog / OptionsDialog / RecordScoreDialog
(`games/bejeweled/docs/GAME_NOTES.md` sections 6-9, `games/bejeweled/src/*.h`): same file format, same checksum, same
EditWidget colours, same edit-box drawing, same OptionsDialog link mechanism. Differences are called
out as **(vs Bejeweled)**.

### 0. Conventions

- Offsets are absolute from the object start. "app" is `WinBMApp*`. App field names follow the
  framework (FRAMEWORK_12.md) for +0x000..+0x6cf; WinBMApp's own fields (+0x6d0...) are my guesses,
  listed in section 7 for reconciliation with `app.md`.
- `mImages` = `app+0x6e4` (pointer to `Image*[]`, filled from the name table at 0x5285b8,
  `images\<name>`); sound ids index the table at 0x5286d8 (`sounds\<name>`). Resolved entries used here:

| idx | file (images/) | used by |
|---|---|---|
| 29 | olddialog.gif (192x192, DrawImageBox 3x3) | HighScoreWidget background |
| 30 | dialog.jpg (192x192) | component image of every MoneyDialog (HSD, Options, RSD) |
| 33 | dialogbutton.gif (96x36) | every DialogButton here; Dialog mButtonHeight = 36 |
| 35 | slidertrack.gif (209x20) | Options sliders track |
| 36 | sliderthumb.png (36x31) | Options sliders thumb |
| 37 | checkboxes.jpg (44x23) | app+0x73c = CopyImage(Rect(0,0,22,23)) unchecked, app+0x738 = CopyImage(Rect(22,0,22,23)) checked (made in LoadingThreadProc 0x41faec) |
| 44 | editbox.png (96x32) | RecordScoreDialog edit-box frame |

| sound id | file (sounds/) | where |
|---|---|---|
| 10 | buttonclick.ogg | `ButtonPress(int)` of HSW (0x401c70), Options (0x415660); app's own ButtonPress (0x41bfa0) for dialog YES/NO |
| 46 (0x2e) | anewhighscore.ogg | DoCheckForHighScore when a score qualifies |
| 24 (0x18) | gameover.ogg | Board, when it creates the HighScoreWidget (Board code, 0x409520) |

- Fonts (app fields, created in WinBMApp::Init 0x41d9a0; ImageFont tags in brackets):

| app+ | font | used here |
|---|---|---|
| 0x6ec | data/bigmoneyfont1.txt | dialog header font (all dialogs); HSW titles and rows |
| 0x700 | data/bigmoneyfont3.txt | dialog lines and button font; HSD rows; Options labels; RSD label and edit text |
| 0x708 | bigmoneyfont3 [underlined] | HSD per-mode titles |

  Other variants (not used by this group): 0x6f0 font1[noshadow], 0x6f4 font1[outline,noshadow],
  0x6f8 font1[underlined], 0x6fc font1[outline,underlined], 0x704 font3[outline], 0x70c
  font3[outline,underlined], 0x710 scorefont, 0x714 levelfont (image mImages[14]), 0x718 floatfont,
  0x71c scorefont dup with SetPointSize(20).
- Game modes (app+0x818, int): 0 = Action, 1 = Strategy, 2 = Puzzle (order of the HSD titles; Board
  treats 2 as puzzle and WinBMApp::ModalOpen treats 0 as the timed one).
- Dialog ids (SexyAppBase dialog map): 1 Options, 3 RecordScore, 13 HighScores, 14 Credits, 15
  "register to record" YES/NO prompt, 16 "enter a name" notice. YES/OK/footer presses reach
  `WinBMApp::ButtonDepress` (0x420f10) as 2000+id, NO/CANCEL as 3000+id (SexyAppBase::DialogButtonDepress
  0x476b10); app `ButtonPress(2000+id / 3000+id)` (0x41bfa0) plays sound 10.
- Dialog colour indices (SAF 1.2): 0 header, 1 lines, 2 footer, 3 button text, 4 button text hilite,
  5 bkg, 6 outline. DialogButton colour 0 = label.
- `Color_rgba(a,b,c,d)` at 0x42b4e0 is also used (COMDAT-folded) as `Insets(l,t,r,b)`.
- **RNG: no function in this group draws a random number** (no MTRand, no `rand()`), unlike Bejeweled's
  HighScoreWidget ctor (which drew the tip).
- Module boundary: GetDayNumber (0x412b80) and GetRank (0x412bb0) are the first two functions of
  HighScoreWidget.cpp (same order as Bejeweled 0x4ced59/0x4ced82), not HighScoreDialog.cpp as the
  inventory's range suggests.

### 1. HighScoreWidget (`HighScoreWidget.cpp`, 0x412b80-0x414d80)

`class HighScoreWidget : public Widget, public ButtonListener`, **sizeof 0x304** (`operator new(0x304)`
in Board 0x409520 and in HighScoreDialog's ctor). Vtables: primary 0x4f651c (57 slots), ButtonListener
0x4f64f8. It is two things: the game-over high-score panel (created by the Board) and the owner of
the high-score tables / hiscores.dat (HighScoreDialog creates a private one just to read the tables).

#### 1.1 Layout

| off | type | name | meaning / init (ctor 0x414640) | readers / writers |
|---|---|---|---|---|
| 0x000 | Widget | | | |
| 0x080 | vptr | ButtonListener | 0x4f64f8 | listener of both own buttons |
| 0x084 | WinBMApp* | mApp | ctor arg | everywhere |
| 0x088 | int | mUnused88 | 0, never read | |
| 0x08c | DialogButton* | mNewGameButton | id 1, "CLICK_HERE_TO_START_A_NEW_GAME" | Added/RemovedFromManager, dtor |
| 0x090 | DialogButton* | mSubmitButton | id 0, "CLICK_HERE_TO_SUBMIT_TO_INTERNET_HIGH_SCORE_LIST" | vf56, Added/Removed, dtor |
| 0x094 | Widget* | mUnusedWidget94 | NULL; deleted in dtor if set; never created | dtor |
| 0x098 | Widget* | mUnusedWidget98 | NULL; same | dtor |
| 0x09c | std::string | mUnusedString | "" (default ctor), never used except dtor | |
| 0x0b8 | int | mColorCycle | 0; ++ every 10 updates | Update, Draw |
| 0x0bc | int | (unused) | not initialised, never accessed | |
| 0x0c0 | std::string[3] | mTodayNames | "" | Load, Save, Draw, AddHighScore |
| 0x114 | int[3] | mTodayScores | 0 (0 = none: Draw shows N/A) | Load, Save, Draw, DoCheck, AddHighScore |
| 0x120 | std::string[3][5] | mNames | defaults below; mode stride 0x8c, entry 0x1c | Load, Save, Draw, AddHighScore, HSD::Draw |
| 0x2c4 | int[3][5] | mScores | defaults below (descending); mode stride 0x14 | same |
| 0x300 | int | mSubmitState | 0; set to 1000 by vf56, never read | vf56 |

Defaults (ctor, binary literals): Action: Ace 1000, John 900, Chris 800, Brian 700, Ben 600.
Strategy: Bradley 1000, Mike 900, Billy 800, Jason 700, Wim 600. Puzzle: Steve 1000, Terra 900,
Roch 800, Kathy 700, Pam 600. (Bejeweled's two tables are the first two of these.)

#### 1.2 Functions

**`static int GetDayNumber()` (0x412b80).** `_time32`, `localtime`: `tm_year*366 + tm_yday` (local time).

**`int GetRank(int mode, int score)` (0x412bb0).** `rank = 0; for (i = 4; i >= 0; i--) if (mScores[mode][i] < score) rank = i+1;`
1-based place, 0 = not on the table; ties do not place (signed int compares).

**`AddedToManager(WidgetManager* wm)` (0x412bf0, slot 7).** `WidgetContainer::AddedToManager(wm)` (0x4234d0),
then `wm->AddWidget(mNewGameButton); wm->AddWidget(mSubmitButton)`.
**`RemovedFromManager(wm)` (0x412c20, slot 8).** Base (0x423520), then RemoveWidget of the same two.

**`Resize(x,y,w,h)` (0x412c50, slot 25).** A bare `jmp Widget::Resize` (0x424500), COMDAT-shared with
TitleScreen slot 25. Nothing positions the two own buttons, so they stay 0x0 at (0,0): invisible and
unclickable for their whole life (see 1.3).

**`InternetSubmitDone()` (0x412c60, slot 56, name mine).** `mSubmitState = 1000; mSubmitButton->SetDisabled(true);
mSubmitButton->SetColor(0, Color(92,86,68))`. No caller exists in the game code (only the Board and
HighScoreDialog hold HSW pointers and neither calls slot 56): leftover of the internet high-score
feature. port: omit.

**`LoadHighScores()` (0x412eb0).** `fopen("hiscores.dat","rb")` (relative path: the game folder / cwd). If
it fails, the defaults stay. Otherwise for mode m = 0,1,2 (format in section 5):
1. Read int32 day, int16 len, `len` bytes into a 256-byte stack buffer, NUL at [len], int32 score,
   int32 checksum.
2. `ck = score ^ 0x42BEEF; for k < len: ck ^= (int)(signed char)name[k] * 0x11`.
3. If `day == GetDayNumber() && checksum == ck`: mTodayNames[m] = name, mTodayScores[m] = score;
   else today's entry keeps ""/0.
4. For i = 0..4: read len/name/score/checksum the same way; mNames[m][i] = name **always**;
   mScores[m][i] = (checksum ok) ? score : score % 256 (C signed remainder: the tamper penalty).
5. `fclose`. No fread result is checked and `len` is not clamped (len > 255 overflows the buffer;
   a short file leaves the previous iteration's locals in place). port: clamp len to 255 and stop at
   EOF keeping the defaults (as the Bejeweled port does).

**`SaveHighScores()` (0x413100).** `DeleteFileA("hiscores.dat"); fopen("hiscores.dat","wb")`. On failure
`app->Popup("Failed to create high score file!\nPlease report error to support@popcap.com.")` (slot 30)
and return. Otherwise for each mode: int32 `GetDayNumber()` (called once per mode), today's record,
then the 5 all-time records (u16 = low 16 bits of the string length, raw bytes, int32 score, int32
checksum computed as above). The day written is always the current day.

**`DoCheckForHighScore()` (0x4133b0; slot 54 `CheckForHighScore` 0x414520 is `jmp 0x4133b0`).**
1. `score = app->mBoard(+0x6d0)->mScore(+0x490); mode = app+0x818`.
2. `best = score > mTodayScores[mode]` (signed); `rank = GetRank(mode, score)`.
3. If `best || rank > 0`: `app->PlaySample(46)` (slot 49) and `app->mMrMoney(+0x6d4)->PlayAnim(3)`
   (FUN_00415200, MrMoney module).
4. Message (`P` = sprintf("%d", rank); `+` = std::string append; the string ids' values come from
   properties/default.xml and already contain leading/trailing spaces, so some messages have double
   spaces: keep the composition exactly):

| case | registered (app+0x6bc != 0) | unregistered |
|---|---|---|
| best, rank <= 0 | S(YOU_GOT_THE_HIGHEST_SCORE_OF_THE_DAY) | S(YOU_GOT_THE_HIGHEST_SCORE_OF_THE_DAY__WOULD_YOU_LIKE_TO_REGISTER_SO_YOU_CAN_RECORD_YOUR_HIGH_SCORE) |
| best, rank > 0 | S(HSOTD_AND_PLACE1) + " " + P + " " + S(HSOTD_AND_PLACE2) | S(HSOTD_AND_PLACE_X_OAT_REGISTER1) + " " + P + " " + S(HSOTD_AND_PLACE_X_OAT_REGISTER2) |
| !best, rank > 0 | S(YOU_GOT_PLACE_) + " " + P + " " + S(_OF_ALL_TIME) | S(YOU_GOT_PLACE_X_OF_ALL_TIME__REGISTER1) + " " + P + " " + S(YOU_GOT_PLACE_X_OF_ALL_TIME__REGISTER2) |
| otherwise | nothing | nothing |

   Registered: `ShowRecordScoreDialog(msg)` (0x41bd00). Unregistered:
   `ShowRegisterToRecordScoreDialog(msg)` (0x41d6a0). The Steam build has NoReg=true, so
   InitPropertiesHook (0x48d990) sets mIsRegistered = mBuildUnlocked = true and only the registered
   column is reachable. **(vs Bejeweled)**: no ordinal suffixes; messages are property strings.

**`Draw(Graphics* g)` (0x413b10, slot 13).** All drawing goes through a copy `Graphics g2(*g)`.
1. `g2.ClipRect(0, 252 - mY, mWidth, mHeight + (252 - mY))`. With mY absolute (the HSW is a top-level
   widget), this hides everything above screen y 252; the height term only ever makes the clip
   taller than the widget (probably meant `mHeight - (252-mY)`; harmless).
2. `g2.DrawImageBox(Rect(0,0,mWidth,mHeight), mImages[29] olddialog)`.
3. `mode = app+0x818` (read once). SetFont(font3) immediately overridden by SetFont(font1 0x6ec), so
   **every string below uses bigmoneyfont1**. SetColor(255,255,255).
4. `t = S(HIGH_SCORES)`: DrawString(t, (mWidth - StringWidth(t))/2, 48) (int division truncating toward 0).
5. For i = 0..4, y = 70 + 20*i (70,90,110,130,150):
   - `h = abs(((mColorCycle + i) * 10) % 170 - 85)` (int; range 0..85, red..yellow..green);
     `c1 = HSLToRGB(h, 255, 150)`, `c2 = HSLToRGB(h, 255, 96)` (SexyAppBase::HSLToRGB, opaque).
   - c1: DrawString(sprintf("%d.", i+1), 49, y).
   - c2: DrawString(mNames[mode][i], 67, y).
   - c1: `s = sprintf("%d", mScores[mode][i])`: DrawString(s, 349 - StringWidth(s), y).
6. SetFont(font1), white: `t = S(TODAYS_HIGH_SCORES)`: DrawString(t, (mWidth - w)/2, 176).
7. `c = HSLToRGB(abs(((mColorCycle + 8) * 10) % 170 - 85), 255, 150)`; SetColor(c).
   - If `mTodayScores[mode] == 0`: `t = S(NOT_APPLICABLE_SHORTHAND)` ("N/A") centered: DrawString(t, (mWidth - w)/2, 198).
   - Else: `s = "%d"`: DrawString(s, 349 - w, 198); DrawString(mTodayNames[mode], 49, 198).
   (Note: the test is `== 0`, so a negative today score would be shown as a number.)

**`Update()` (0x4144e0, slot 9).** `Widget::Update()` (mUpdateCnt++); `if (mUpdateCnt == 280) DoCheckForHighScore();`
`if (mUpdateCnt % 10 == 0) { mColorCycle++; MarkDirty(); }`.

**`AddHighScore(const std::string& name)` (0x414530, slot 55).** Called by WinBMApp on RecordScoreDialog OK/Enter.
1. If `name.length() == 0` return (no save).
2. `score = app->mBoard->mScore; mode = app+0x818; rank = GetRank(mode, score)`.
3. `if (score > mTodayScores[mode]) { mTodayScores[mode] = score; mTodayNames[mode] = name; }`
4. `if (rank != 0) { if (rank <= 4) for (k = 4; k >= rank; k--) { mNames[mode][k] = mNames[mode][k-1]; mScores[mode][k] = mScores[mode][k-1]; } mNames[mode][rank-1] = name; mScores[mode][rank-1] = score; }`
5. `SaveHighScores()`.

**Ctor `HighScoreWidget(WinBMApp* app)` (0x414640).**
1. Widget(); vptrs; mUnusedString, mTodayNames[3], mNames[15] default-constructed.
2. mApp = app; mUnused88 = 0; mColorCycle = 0; mSubmitState = 0.
3. `mSubmitButton = new DialogButton(mImages[33], 0, this+0x80)`; mLabel = S(CLICK_HERE_TO_SUBMIT_TO_INTERNET_HIGH_SCORE_LIST);
   SetFont(font3 0x700); SetColor(0, Color(224,183,40)).
4. `mNewGameButton = new DialogButton(mImages[33], 1, this+0x80)`; mLabel = S(CLICK_HERE_TO_START_A_NEW_GAME);
   SetFont(font3); SetColor(0, Color(224,183,40)).
5. mUnusedWidget94 = mUnusedWidget98 = NULL; mTodayScores = 0; the default tables; `LoadHighScores()`.

**Dtor (0x414c50; scalar deleting dtor 0x414d60 slot 0).** Delete (virtual, if non-NULL) +0x94, +0x98,
mNewGameButton, mSubmitButton; then `SaveHighScores()` (it always re-saves); string arrays; ~Widget.

**ButtonListener.** Slot 0 is the stock ButtonPress(id,count) → ButtonPress(id). Slot 1
`ButtonPress(int)` = 0x401c70, COMDAT-shared with Board: `mApp->PlaySample(10)`. Slot 2
`ButtonDepress` is the empty stub (0x476c80): neither own button does anything when clicked.

#### 1.3 Who creates it and how it moves (Board code, for reconciliation)

- Board update 0x409520: when the board's flying-piece list is empty, board+0x624 == 0 and the board
  state (+0xa0) == 5: `PlaySample(24)`, `MrMoney->PlayAnim(12)`, state = 6, board+0x7c8 = 180,
  `board->mHighScoreWidget(+0x7dc) = new HighScoreWidget(app)`, `board->mWidgetManager->AddWidget(it)`,
  FUN_004045e0 (creates the board's own "CLICK_HERE_TO_PLAY_AGAIN" DialogButton at board+0x63c, id 0,
  listener = Board, label colour (0,255,0), font3, Resize(197,444,366,27), mMouseInsets -10 each,
  mDoFinger), then that button `Resize(0,0,0,0)`.
- Board update 0x40c9f0, state 6: board+0x5d8 += 5 up to 500; after the 180-tick delay (+0x7c8), each
  tick: `if (-146 < v) v -= 2` (v = board+0x7cc, from 0), `hsw->Resize(board.mX + 182, board.mY + 664 + 3*v, 396, 236)`,
  `playAgain->Resize(212, hsw.mY + 200, 336, 29)`, MarkDirty. The panel slides up from y 664 to 226
  in 73 ticks (and is clipped at screen y 252 by its Draw). The HSW's own two buttons are never
  positioned: **the "start a new game" and "submit to internet" buttons never appear**; the board's
  play-again button is the working one.
- Board::OrderInManagerChanged (0x401460) keeps the HSW in front of the board;
  Board::RemovedFromManager (0x403aa0) removes and deletes it (which saves hiscores.dat).
- `DoCheckForHighScore` runs at the HSW's 280th update (~2.8 s after creation at 100 Hz; the slide
  ends at tick 253).
- WinBMApp::ButtonDepress case 2 (RegisterDialog OK, valid code): `if (mBoard && mBoard->mHighScoreWidget)
  mBoard->mHighScoreWidget->CheckForHighScore()` (slot 54): after registering, the message is shown
  again so the score can be recorded.

**Internet submission branch (not ported).** All that remains is: the submit DialogButton (id 0,
never sized, ButtonDepress empty), `InternetSubmitDone` (slot 56, no caller), mSubmitState (+0x300,
write-only), and the unused members +0x88, +0x94, +0x98, +0x9c (probably the submit widgets, URL or
result text of the removed code). No HTTP/InternetManager call is reachable from this class.
port: drop the two buttons (or create them unsized for fidelity) and slot 56.

### 2. HighScoreDialog (`HighScoreDialog.cpp`, 0x412280-0x412b80)

`class HighScoreDialog : public MoneyDialog`, **sizeof 0x160** (`new(0x160)` in ShowHighScoreDialog
0x41ba00). Vtables 0x4f60fc (60 slots), ButtonListener 0x4f60d8 (Dialog's handlers).

| off | type | name | meaning |
|---|---|---|---|
| 0x000 | MoneyDialog | | Dialog + mApp(+0x148) + light phase (+0x150 double); see app.md |
| 0x158 | HighScoreWidget* | mHighScoreWidget | private instance: loads the tables; never added to a manager |
| 0x15c | WinBMApp* | mApp | |

**Ctor `HighScoreDialog(WinBMApp* app)` (0x412310).**
1. `MoneyDialog(app, mImages[30] dialog, mImages[33] dialogbutton, 13, true, "", "", S(DONE), BUTTONS_FOOTER)`.
2. `mHighScoreWidget = new HighScoreWidget(app)` (loads hiscores.dat; creates its two unused buttons);
   mApp = app.
3. SetButtonFont(font3 0x700), SetHeaderFont(font1 0x6ec), SetLinesFont(font3).
4. mContentInsets = Insets(36,36,36,36); `SetColor(3, Color(224,183,40))` (overridden by the creator).

**`GetPreferredHeight(int)` slot 57 = 0x40ee80: `return 396`** (one body COMDAT-shared with CreditsDialog).

**Dtor (0x412280; deleting 0x4122f0).** Delete mHighScoreWidget (its dtor re-saves hiscores.dat), ~MoneyDialog.

**`Draw(Graphics* g)` (0x4125c0, slot 13).** `MoneyDialog::Draw(g)` (header and lines are empty, so
Dialog::Draw only draws the box), SetFont(font3), then for m = 0,1,2 with `y = 67 + 94*m` (67, 161, 255):
1. SetFont(font3 underlined 0x708); SetColor(255,255,255); `t` = S(ACTION_HIGH_SCORES) /
   S(STRATEGY_HIGH_SCORES) / S(PUZZLE_HIGH_SCORES) for m = 0/1/2 (all three are fetched on every
   iteration); DrawString(t, (mWidth - StringWidth(t))/2, y - 19).
2. SetFont(font3); SetColor(32,255,32).
3. For i = 0..4, `ry = y + 15*i`: DrawString(mNames[m][i], 48, ry); `s = "%d" mScores[m][i]`:
   DrawString(s, mWidth - StringWidth(s) - 48, ry).
No today's entries, no colour cycling. Button: DONE (footer, id 1000) → app 2013 → default case
`KillDialog(13)`.

**Creator `ShowHighScoreDialog()` (WinBMApp, 0x41ba00).** `new HighScoreDialog(app)`; SetColor(0, white),
SetColor(1, (0,255,0)), SetColor(3, (0,255,0)); `Resize(196, 16, 388, GetPreferredHeight(388) = 396)`;
`AddDialog(13, dlg)`. Only caller: OptionsDialog "Scores" (it opens on top of the Options dialog,
which stays open). Footer button rect (Dialog::Resize): (232, 340, 316, 36).

### 3. OptionsDialog (`OptionsDialog.cpp`, 0x415560-0x417400)

`class OptionsDialog : public MoneyDialog, public SliderListener, public CheckboxListener`, **sizeof
0x198** (`new(0x198)` in ShowOptionsDialog 0x41bbe0). Vtables: primary 0x4fb774 (60), ButtonListener
0x4fb754, SliderListener 0x4fb74c (+0x158), CheckboxListener 0x4fb744 (+0x15c).

#### 3.1 Layout

| off | type | name | meaning (ctor 0x416720) |
|---|---|---|---|
| 0x000 | MoneyDialog | | |
| 0x158 | vptr | SliderListener | |
| 0x15c | vptr | CheckboxListener | |
| 0x160 | WinBMApp* | mApp | |
| 0x164 | Slider* | mMusicSlider | id 0, value app->GetMusicVolume() |
| 0x168 | Slider* | mSfxSlider | id 1, value app->GetSfxVolume() |
| 0x16c | Checkbox* | mFullscreenCheckbox | id 0, checked = !app->mIsWindowed; read by app on 2001 |
| 0x170 | Checkbox* | mCustomCursorsCheckbox | id **0** too, checked = app->mCustomCursorsEnabled; read by app on 2001 |
| 0x174 | DialogButton* | mRegisterButton | id 1 S(REGISTER) |
| 0x178 | DialogButton* | mScoresButton | id 2 S(SCORES) |
| 0x17c | DialogButton* | mHelpButton | id 4 S(HELP) |
| 0x180 | DialogButton* | mCheckUpdatesButton | id 5 S(CHECK_UPDATES) |
| 0x184 | DialogButton* | mCreditsButton | id 6 S(CREDITS) |
| 0x188 | std::vector<DialogButton*> | mLinkButtons | ids 100+i (VC7: alloc +0x188, first +0x18c, last +0x190, end +0x194) |

#### 3.2 Functions

**Ctor `OptionsDialog(WinBMApp* app)` (0x416720).**
1. `MoneyDialog(app, mImages[30], mImages[33], 1, true, S(OPTIONS), "", S(DONE), BUTTONS_FOOTER)`.
2. vptrs; mLinkButtons empty; mApp = app.
3. SetButtonFont(font3), SetHeaderFont(font1), SetLinesFont(font3).
4. Buttons, each `new DialogButton(mImages[33], id, this+0x80)` + SetFont(font3 0x700) + mLabel:
   Register (1), Scores (2), Help (4), Check Updates (5), Credits (6), in that order.
5. `mMusicSlider = new Slider(mImages[35], mImages[36], 0, this+0x158)`; `SetValue(app->GetMusicVolume())` (slot 51).
   `mSfxSlider = new Slider(mImages[35], mImages[36], 1, this+0x158)`; `SetValue(app->GetSfxVolume())` (slot 52).
6. `mFullscreenCheckbox = new Checkbox(app+0x73c unchecked, app+0x738 checked, 0, this+0x15c)`;
   `SetChecked(!app->mIsWindowed(+0x307), true)` (notifies CheckboxChecked during construction; the
   member is assigned before the call).
   `mCustomCursorsCheckbox = new Checkbox(same images, 0, this+0x15c)`; `SetChecked(app->mCustomCursorsEnabled(+0x4c8), true)`.
7. Links: `for (i = 0; i < GetInteger("NumOptionsLinks", GetBoolean("NoPopLink", false) ? 0 : 1); i++)`
   (both properties re-read on every test): `b = new DialogButton(mImages[33], 100+i, this+0x80)`;
   SetFont(font3); SetColor(0, Color(187,255,1)); mLabel = `GetString(StrFormat("OptionsLinkText%d", i), "Visit PopCap.com For More Games!")`;
   push_back. Steam partner.xml: 1 link, "More Games".
   **No Diamond Mine / Alchemy / Dynomite / Palm Games links exist in the code**: the string ids
   CLICK_HERE_TO_TRY_DIAMOND_MINE, VISIT_POPCAP_COM_FOR_MORE_GAMES, DIAMOND_MINE, ALCHEMY, DYNOMITE,
   PALM_GAMES in default.xml are not referenced anywhere in the game PE (dead strings of an older dialog).

**`GetPreferredHeight(int)` (0x4156b0, slot 57).** `308 + 34 * mLinkButtons.size()` (342 shipped).

**`AddedToManager(wm)` (0x415770).** Dialog::AddedToManager (adds DONE), then AddWidget: register,
scores, help, music slider, sfx slider, custom-cursors checkbox, fullscreen checkbox, check updates,
credits, links in order. **`RemovedFromManager` (0x415830)**: Dialog's, then RemoveWidget register,
scores, help, music, sfx, fullscreen, custom cursors, check updates, credits, links.

**`Resize(x,y,w,h)` (0x415ad0, slot 25).** cL = mContentInsets.mLeft (+0x114), cR = mContentInsets.mRight (+0x11c).
1. `Dialog::Resize(x,y,w,h)` (positions DONE = mYesButton at (mX+cL+bL, mY+mHeight-cB-bB-36, mWidth-cL-cR-bL-bR, 36)).
2. n = links; link i: `Resize(mX + cL, (i - n)*34 + mYesButton->mY, mWidth - cR - cL, 36)`.
3. `rowY = mYesButton->mY - (n+1)*34`; `half = (mWidth - cL - cR - 3)/2`; `third = (mWidth - cL - cR - 6)/3` (int, truncating).
4. `dontUpdate = GetBoolean("DontUpdate", false)`; unlocked = app->mBuildUnlocked (+0x6bd).
   - `!dontUpdate`: CheckUpdates (mX+cL, rowY, half, 36), Credits (mX+cL+half+3, rowY, half, 36); rowY -= 34;
     - `!unlocked`: Help (mX+cL, rowY, third, 36), Register (mX+cL+third+3, rowY, third, 36), Scores (mX+cL+2*third+6, rowY, third, 36).
     - unlocked: Help (mX+cL, rowY, half, 36), Scores (mX+cL+half+3, rowY, half, 36); Register unsized.
   - `dontUpdate`: Help (mX+cL, rowY, half, 36), Credits (mX+cL+half+3, rowY, half, 36); rowY -= 34;
     CheckUpdates unsized;
     - `!unlocked`: Register (mX+cL, rowY, half, 36), Scores (mX+cL+half+3, rowY, half, 36).
     - unlocked: Scores (mX+cL, rowY, mWidth - cL - cR, 36); Register unsized.
5. Music slider (mX+152, mY+68, mWidth-192, 36); sfx slider (mX+152, mY+100, mWidth-192, 36).
6. Fullscreen checkbox (mX+48, mY+138, 20, 20); custom-cursors checkbox (mX+176, mY+138, 20, 20).

   Shipped (Steam: DontUpdate, NoReg ⇒ unlocked; 1 link; dialog at (186,26,388,342), insets 36):
   DONE (222,296,316,36); "More Games" (222,262,316,36); Help (222,228,156,36); Credits
   (381,228,156,36); Scores (222,194,316,36); sliders (338,94,196,36) and (338,126,196,36);
   checkboxes (234,164,20,20) and (362,164,20,20). Register and Check Updates stay 0x0 (invisible).

**`Draw(Graphics* g)` (0x415eb0, slot 13).** MoneyDialog::Draw (header S(OPTIONS) in font1, colour 0);
SetFont(font3); `SetColorizeImages(true)`; SetColor(mColors[1]) (lines colour: green from the creator);
DrawString(S(OPTDLG_MUSIC), 48, 88); DrawString(S(SOUND_FX), 48, 120);
DrawString(S(FULLSCREEN), fs.mX - mX + 28, fs.mY - mY + 14);
DrawString(S(CUSTOM_CURSORS), cc.mX - mX + 28, cc.mY - mY + 14); `SetColorizeImages(false)`.

**`SetColor(int idx, const Color& c)` (0x415560, slot 19).** Dialog::SetColor(idx, c); if idx == 3, also
`SetColor(0, c)` on register, scores, help, check updates, credits (non-NULL). Link buttons keep their
own (187,255,1).

**`SliderVal(int id, double v)` (0x4155e0, SliderListener slot 0).** id 0: `app->SetMusicVolume(v)` (slot 55);
id 1: `app->SetSfxVolume(v)` (slot 56). Live; nothing is reverted.

**`CheckboxChecked(int id, bool checked)` (0x415620, CheckboxListener slot 0).** Ignores id (both are 0):
`if (!checked && app->mForceFullscreen(+0x30a)) mFullscreenCheckbox->SetChecked(true, true)`.
(Unticking either box while fullscreen is forced re-ticks the fullscreen box.) **(vs Bejeweled)**: no
colour-depth dialog.

**`ButtonPress(int id)` (0x415660, ButtonListener slot 1).** `Dialog::ButtonPress(id)` (0x422b40: ids
1000/1001 → app->DialogButtonPress(1, id) → app ButtonPress(2001/3001) → sound 10); then
`if (id != mId+2000 && id != mId+3000) app->PlaySample(10)`. mId = 1, so every press plays it, and
DONE plays buttonclick twice (same quirk as Bejeweled).

**`ButtonDepress(int id)` (0x416210, ButtonListener slot 2).** `Dialog::ButtonDepress(id)` first (DONE:
mResult = 1000, app → 2001). Then:
- 100 <= id < 200: `app->OpenURL(GetString(StrFormat("OptionsLinkURL%d", id-100), "http://www.popcap.com"), false)`
  (slot 35, WinBMApp override 0x420140). Shipped: `steam://publisher/popcap`. port: platform open.
- 1 Register: FUN_0041cd60 (app: register flow; "ALREADY_REGISTERED" message box when registered).
- 2 Scores: ShowHighScoreDialog (0x41ba00).
- 3: `OpenURL("http://www.popcap.com", false)` (no such button).
- 4 Help: FUN_0041b6f0 (app: KillDialog(1), then shows the HelpWidget, `new(0xa0)` FUN_00410f20).
- 5 Check Updates: FUN_00420c50 (app: update check, `http://www.popcap.com/win32updatecheck.php?prod=`...).
- 6 Credits: FUN_0041baf0 (app: `new CreditsDialog` (0x168), id 14, same colours/size as HSD).

**DONE in the app (WinBMApp::ButtonDepress 2001, case 1).** `d = GetDialog(1)`; `wantWindowed =
!d->mFullscreenCheckbox->IsChecked()`; `if (app->mIsWindowed != wantWindowed) SwitchScreenMode(wantWindowed)` (slot 63);
`EnableCustomCursors(d->mCustomCursorsCheckbox->IsChecked())`; `KillDialog(1)`.

**Dtor (0x415970; deleting 0x416700).** Deletes register, scores, check updates, credits, music, sfx,
fullscreen, custom cursors, help (this order), then each link button, frees the vector, ~MoneyDialog.

**Creator `ShowOptionsDialog()` (WinBMApp, 0x41bbe0).** `new OptionsDialog(app)`; SetColor(0, white),
SetColor(1, (0,255,0)), SetColor(3, (0,255,0)); mContentInsets = Insets(36,36,36,36);
`Resize(186, 26, 388, GetPreferredHeight(388))`; `AddDialog(1, dlg)`. Reached from
WinBMApp::ButtonDepress when a top-level app button with id 1 is released (the in-game menu button;
see app.md).

### 4. RecordScoreDialog (`RecordScoreDialog.cpp`, 0x417730-0x417e80)

`class RecordScoreDialog : public MoneyDialog, public EditListener`, **sizeof 0x168** (`new(0x168)` in
ShowRecordScoreDialog 0x41bd00; 0x164 is alignment padding: MoneyDialog holds a double). Vtables:
primary 0x4fbb7c, ButtonListener 0x4fbb58 (Dialog's), EditListener 0x4fbb44 (+0x158).

| off | type | name | meaning |
|---|---|---|---|
| 0x158 | vptr | EditListener | |
| 0x15c | WinBMApp* | mApp | |
| 0x160 | EditWidget* | mNameWidget | id 0; app reads `->mString` (+0x84) on 2003 |
| 0x164 | | (padding) | |

**Ctor `RecordScoreDialog(WinBMApp* app, const std::string& theLines)` (0x417a30).**
1. `MoneyDialog(app, mImages[30], mImages[33], 3, false /*modeless*/, S(RSDLG_HIGH_SCORE), theLines, "", BUTTONS_OK_CANCEL)`.
   **(vs Bejeweled)**: modeless (Bejeweled's was modal), id 3.
2. mApp = app; SetButtonFont(font3), SetHeaderFont(font1), SetLinesFont(font3).
3. mContentInsets = Insets(38,38,38,38); SetColor(1, Color(255,173,17)); SetColor(3, Color(0,255,0)).
   Header colour stays Dialog's default white; button hilite white.
4. `mNameWidget = new EditWidget(0, this+0x158)`; `SetFont(font3, NULL)`; `SetColors(gRecordScoreEditColors, 5)`
   (int[5][4] at 0x528448: bkg (0,0,0,0), outline (0,0,0,0), text (240,240,255,255), hilite
   (255,255,255,255), hilite text (0,0,0,255): identical to Bejeweled's); `mMaxPixels = 140`;
   `SetText(app->mLastScoreName(+0x7f8), true)`.

**`GetPreferredHeight(int w)` (0x4177a0).** `Dialog::GetPreferredHeight(w) + 56`.
**`AddedToManager` (0x4177b0).** Dialog's; AddWidget(mNameWidget); `wm->SetFocus(mNameWidget)`.
**`RemovedFromManager` (0x4177e0).** Dialog's; RemoveWidget(mNameWidget).
**`Resize(x,y,w,h)` (0x417810).** Dialog::Resize; `mNameWidget->Resize(mX + 48, mY + mHeight - 110, mWidth - 96, 24)`.
**`EditWidgetText(int, const string&)` (0x417860, EditListener slot 0, Enter).** `app->ButtonDepress(mId + 2000)` = 2003
(no Dialog::ButtonDepress, no sound). AllowKey/AllowChar/AllowText: shared `return true` stub 0x417880.

**`DrawEditBox(Graphics* g, const Rect& r)` (0x4178b0).** With editbox = mImages[44]:
`DrawImage(editbox, r.x, r.y, Rect(0,0,32,32)); DrawImage(editbox, r.x + r.w - 32, r.y, Rect(64,0,32,32));`
`Graphics g2(*g); g2.ClipRect(r.x + 32, r.y, r.w - 32, 32);`
`for (i = 0; i < (r.w - 33)/32; i++) g2.DrawImage(editbox, r.x + 32 + 32*i, r.y, Rect(32,0,32,32));`
(division truncates toward 0). The middle tiles are drawn after the right cap and can overdraw it: keep
the order (identical to Bejeweled 0x4d2857).

**`Draw(Graphics* g)` (0x417d00).** MoneyDialog::Draw; `r = mNameWidget->GetRect()`;
`DrawEditBox(g, Rect(r.x - mX - 4, r.y - mY - 4, r.w + 8, r.h + 8))`; SetFont(font3);
SetColor(255,173,17); DrawString(S(ENTER_YOUR_NAME), name.mX - mX, name.mY - mY - 8).

**Dtor (0x417730; deleting 0x417890).** Delete mNameWidget; ~MoneyDialog.

**Creator `ShowRecordScoreDialog(const std::string& msg)` (WinBMApp, 0x41bd00).** `new RecordScoreDialog(app, msg)`;
`Resize(250, 32, 316, GetPreferredHeight(316))`; `AddDialog(3, dlg)` (no SetColor calls). Edit widget
rect: (298, 32 + h - 110, 220, 24).

**OK / CANCEL (WinBMApp::ButtonDepress).**
- 2003 (OK or Enter), case 3: `name = GetDialog(3)->mNameWidget->mString`;
  - empty: `DoDialog(16, true, S(ENTER_NAME), S(IF_YOU_WANT_TO_RECORD_YOUR_SCORE_ENTER_YOUR_NAME_IN_THE_HIGH_SCORE_BOX), S(OK), BUTTONS_FOOTER)`;
    the record dialog stays open; dialog 16's button → 2016 → KillDialog(16).
  - else: `mBoard->mHighScoreWidget->AddHighScore(name)` (slot 55, no NULL check),
    `app->mLastScoreName = name` (registry value "LastScoreName", ReadFromRegistry 0x41cb60), `KillDialog(3)`.
- 3003 (CANCEL): `KillDialog(3)`; nothing recorded.

**`ShowRegisterToRecordScoreDialog(msg)` (WinBMApp, 0x41d6a0).** `DoDialog(15, true, S(HIGH_SCORE), msg, "", BUTTONS_YES_NO)`.
YES → 2015: KillDialog(15) + FUN_0041cd60 (register flow). NO → 3015: KillDialog(15). Unreachable in
the Steam build (NoReg).

### 5. hiscores.dat (byte-exact)

- Path: `"hiscores.dat"` relative to the current directory (the install folder; a real one exists
  there). No directory creation. Written by `SaveHighScores` (0x413100), read by `LoadHighScores`
  (0x412eb0); both in HighScoreWidget. Little-endian, no header, no padding.
- Three tables in mode order Action (0), Strategy (1), Puzzle (2). Each table:

```
int32  day                       // GetDayNumber() at save time = tm_year*366 + tm_yday (local time)
record today                     // best score of 'day' in this mode ("" / 0 if none)
record alltime[5]                // descending
record := { u16 len; char name[len] /* no NUL */; int32 score; int32 checksum; }
checksum = score ^ 0x42BEEF ^ XOR over k of ((int)(signed char)name[k] * 17)
```

- File size = 3 * (4 + 6*10) + total name bytes = 192 + Σ len.
- Load rules: today's record accepted only if `day == today && checksum ok`; all-time names always
  taken, all-time scores replaced by `score % 256` when the checksum fails. Save always stamps today.
  ("Today" is therefore per calendar day per mode; there is no separate today file.)
- Verified against a real file: it parses exactly as above with
  three tables, 18 records, every checksum valid, day fields consistent with the file's date; one
  table has a non-empty today record, the other two have len 0 / score 0. (No names or scores copied.)
- Same format and key as Bejeweled's hiscores.dat (which has two tables).

### 6. Quirks worth keeping (or deciding about) in the port

1. HSW's own buttons never appear (unsized); the board's play-again button does the work.
2. Every HighScoreWidget destruction rewrites hiscores.dat, including HighScoreDialog's private copy.
   If the Scores dialog is opened while the modeless RecordScoreDialog is up and closed after OK,
   its stale copy overwrites the just-saved score; the board's widget writes it back when the board
   is destroyed later (order-dependent). Same in the original; keep or fix knowingly.
3. Options DONE plays buttonclick twice.
4. Both Options checkboxes have id 0.
5. HSW Draw clips everything above screen y 252.
6. Messages have double spaces where property strings already carry spaces.

### 7. Cross-module references

WinBMApp fields read (names are my guesses; reconcile with app.md):

| app+ | type | guess | used by |
|---|---|---|---|
| 0x307 | bool | mIsWindowed (SAF) | Options ctor, app case 1 |
| 0x30a | bool | mForceFullscreen (SAF) | Options CheckboxChecked |
| 0x498 | bool | mHasFocus (SAF) | MoneyDialog::Update |
| 0x4c8 | bool | mCustomCursorsEnabled (SAF) | Options ctor |
| 0x6bc | bool | mIsRegistered (SAF) | DoCheckForHighScore |
| 0x6bd | bool | mBuildUnlocked (SAF) | Options Resize |
| 0x6d0 | Board* | mBoard | DoCheck, AddHighScore (board+0x490 mScore, board+0x7dc mHighScoreWidget) |
| 0x6d4 | MrMoney* | mMrMoney | DoCheck (FUN_00415200 = MrMoney::PlayAnim(int) (board_draw.h; screens.md calls it Talk), sets +0xc0 anim, +0xc4/+0xc8 = 0, +0xcc from table 0x4f6818 stride 400, MarkDirty) |
| 0x6e4 | Image** | mImages | everywhere |
| 0x6ec/0x700/0x708 | ImageFont* | fonts (section 0) | everywhere |
| 0x738/0x73c | Image* | mCheckboxCheckedImage / mCheckboxUncheckedImage | Options ctor |
| 0x7f8 | std::string | mLastScoreName | RSD ctor, app case 3 |
| 0x818 | int | mGameType | HSW Draw, DoCheck, AddHighScore |

App virtuals called (slot = offset/4): PlaySample(int) 49 (0xc4); Popup(string) 30 (0x78);
OpenURL(string,bool) 35 (0x8c); GetMusicVolume 51 (0xcc); GetSfxVolume 52 (0xd0); SetMusicVolume 55
(0xdc); SetSfxVolume 56 (0xe0); ButtonDepress(int) 2 (0x08, from RSD EditWidgetText); in the creators
GetPreferredHeight/Resize on the dialog and AddDialog(int, Dialog*) 69 (0x114); DoDialog 66 (0x108);
GetDialog 67 (0x10c); KillDialog(int) 71 (0x11c); SwitchScreenMode(bool) 63 (0xfc).
Non-virtual framework: SexyAppBase::GetString (0x4822a0), GetString(id, default) (0x47cb30),
GetBoolean (0x47cab0), GetInteger (0x47caf0), HSLToRGB (0x478280), StrFormat (0x465070),
EnableCustomCursors.

App functions (WinBMApp, app agent): ShowHighScoreDialog 0x41ba00, ShowCreditsDialog 0x41baf0,
ShowOptionsDialog 0x41bbe0, ShowRecordScoreDialog 0x41bd00, ShowRegisterToRecordScoreDialog
0x41d6a0, register flow 0x41cd60, ShowHelp 0x41b6f0, update check 0x420c50, ButtonDepress 0x420f10
(cases 1, 2, 3, 0xf above; default `KillDialog(id)`), ReadFromRegistry 0x41cb60 (LastScoreName).

MoneyDialog (app agent): ctor 0x414e00 `(app, componentImage, buttonImage, id, isModal, header, lines,
footer, buttonMode)`, sets +0x148 = app, +0x150 (double) = 0; Update 0x414e80 (phase += 0.0005 while
app has focus, wraps at 1.0, MarkDirty); Draw 0x414f00 (black FillRect(8,8,w-16,h-16), 40
dialoglight (mImages[31]) around the border from the phase, Dialog::Draw, then additive dialogglow
(mImages[32]) at the same points outside the y bands 28..54 and h-56..h-30).

Board (board agent): +0x490 mScore, +0x7dc mHighScoreWidget, +0x63c mPlayAgainButton, +0x7c8
slide delay (180), +0x7cc slide counter (0 → -146 step 2), state +0xa0 (5 → 6), FUN_00409520 creates
the HSW, FUN_004045e0 creates the play-again button.

Shared COMDAT bodies outside my ranges: 0x401c70 (Board.cpp: `mApp->PlaySample(10)`, ButtonPress(int)
of Board and HSW), 0x40ee80 (CreditsDialog.cpp: `return 396`, GetPreferredHeight of CreditsDialog and
HighScoreDialog), 0x412c50 (`jmp Widget::Resize`, HSW and TitleScreen slot 25).

std helpers in my ranges: 0x412cb0 `std::string::append(const string&, pos, n)`, 0x412da0
`std::string::append(const char*, n)`, 0x4156e0 uninitialized_copy<T*>, 0x415710 copy_backward<T*>,
0x415740 uninitialized_fill_n<T*>, 0x4158f0 vector::_Xlen (throws length_error "vector<T> too long"),
0x416460 vector<DialogButton*>::_Insert_n (+ its Catch blocks 0x4165f3/0x4165ff).

### 8. Open questions

1. Slot 56 / +0x300 / +0x88 / +0x94 / +0x98 / +0x9c of HighScoreWidget: the names are guesses for the
   removed internet-submission code; nothing reads them.
2. Whether DIALOG_OK_STRING / DIALOG_CANCEL_STRING (RSD button labels) and DIALOG_YES/NO are replaced
   from DIALOG_BUTTON_* properties by WinBMApp (app.md).
3. HSW Draw's clip height formula looks like a typo (`mHeight + (252-mY)`); it is harmless at every
   reachable position, but confirm the board's mX/mY are 0 so the "screen y 252" reading holds.
4. The Board's game-over condition fields (+0x518 list size, +0x624) are only paraphrased here (board.md).

### 9. Translation (games/bigmoney/src)

`HighScoreWidget.h/.cpp`, `HighScoreDialog.h/.cpp`, `RecordScoreDialog.h/.cpp`, `OptionsDialog.h/.cpp`.
- hiscores.dat: same byte format, read with `ReadBufferFromFile("hiscores.dat")` (user dir first, then the
  game folder, so the original install's file is picked up) and written with `EraseFile` +
  `WriteBufferToFile` (user dir). port: loading stops at the end of the data or at a name length
  outside 0..255 (the original reads past both unchecked).
- Not ported (one-line notes in the code): the internet submit button, slot 56 and the unused internet
  members; the unregistered DoCheckForHighScore branch (Steam is NoReg); Options "Check Updates"
  (DoUpdateCheck).
- Kept: the never-sized "start a new game" button, both checkboxes with id 0, the double buttonclick
  on Done, the stale-copy overwrite when HighScoreDialog closes (commented in its dtor).

---


---

## Part 6: The port's test commands, fixes, removals and drivers

### The standard test commands (`src/testhooks/TestCommands.cpp`)

The commands of `docs/TEST_COMMANDS.md`, reached only from a test (`-script`, `-commands`, the web test page's `command`) through
`WinBMApp::HandleScriptCommand`. A command that is never given changes nothing, draws no random number and moves no state. Each
answers `STATE {...}`, `MOVE {...}` or `MOVE null`, `OK <command>` or `ERROR <command>: <reason>`. They act through the game's own
code: the screens' own `Show` functions, `StartGame`, and the board's own `LevelComplete`, `GameOver` and `StartNextLevel`. While
the title screen is up every command but `set mode` and `set skill` answers an error (click its play link first, which `move`
does).

| Command | Does |
|---|---|
| `state` | one JSON line. Always: `screen` (title, help, selector, board, none), `dialog` (the id of the open dialog, or -1), `mode` (action, strategy, puzzle), `skill` (1 to 3), `updates`, `over`. On the title `loaded`. With a dialog open `yes` and `no`: the screen points of its Yes/OK and No buttons. With a board `level` (1-based), `score`, `boardState` (playing, level done, level complete, sweep, clear fall, game over, game over scores, bomb, frozen), `bags` and `bagsNeeded`, `paused`, `settled` (every coin rests in its cell), `go` (the get-ready counter), `colors` (the colours in play), `moves` (Strategy: the moves left) and `board`: 15 strings of 14 characters, `.` empty, a hex digit for a coin's colour, `B` a money bag, `X` any other special. `over` is true once the game-over table has slid in, the name (if one is asked) is entered and "play again" is shown |
| `goto <screen>` | `menu` or `selector` (the game selector), `options`, `help`, `highscores` (or `scores`), `credits`; `<mode> [skill]` (action, strategy, puzzle) starts that game as the selector's start button does. `title` is refused: the title is shown once, at launch |
| `level <n>` | on a board, the board as `StartNextLevel` makes it at level n (1-based; the game's own level setup) |
| `unlock all` | answers `OK` (nothing in Big Money is locked; the command exists so a driver need not know) |
| `set mode action\|strategy\|puzzle`, `set skill 1..3` | what `move` picks on the selector (works from the title on) |
| `set bot win\|lose` | `win` (the default) plays well; `lose` plays to lose so that a game ends by itself (a bot that never errs would play an Easy game for hours): Action stops clicking and lets the stack rise, Puzzle takes the largest group in place of the solver's plan, Strategy plays on until its moves run out |
| `set score <n>`, `set moves <n>` | the board's score and Strategy's moves left (a board is needed) |
| `win`, `lose` | `Board::LevelComplete`, or `Board::GameOver` |
| `seed <n>` | seeds the C library generator (the one that picks the bag columns); the board's own generator is seeded by `-seed` |
| `move` | the bot's next move as `MOVE {"kind", "input": [{"click":[x,y]}, {"key":"B"}, ...], "updates": n}`: the real input in game coordinates and in order, and the updates to wait after it; `MOVE null` with no board and nothing to press |

`move` plays whatever the game shows: the title's play link once loaded, the selector's type, skill and start (from `set mode` and
`set skill`), the help screen's link, the name of a high score (`Bot`, after clearing what is there), a dialog's No (or its only
button), a click that resumes a paused board, "next level" and "play again", and on a board that is playing, in Puzzle the
solver's next click (`Board::GetSolverModel`, the solver of `mods/puzzle-solver`, once the coins rest), in Action and Strategy the
largest group (the one the autoplay bot picks as its best). It waits (`kind` starting "wait", no input) while the board is busy,
the get-ready counter runs, or there is no group to click. Every group click lands 14 pixels inside the coin's cell. The bot has
no dice, so a game is played the same way for the same coins.

### Fix candidates

What the original does that the fixes mod (below) mends, and what it left alone.

- **Made: the shake offsets** (`shakeSettles`). In Action, when the stack reaches the top the board shakes under the countdown
  (`Update`, the overflow branch; quirk 3 of section 11). The original never clears the coins' `mShakeX` and `mShakeY`, so a board that
  survived the countdown keeps its coins displaced by the last offsets for the rest of the level (a few pixels; it does not change
  what can be clicked). With the fix the offsets are zeroed when the overflow ends. Seen in Action skill 3 games of the autoplay bot
  at a click pace of about 110 updates (seeds 12, 14, 15 and 16 logged it); the frozen baseline never shakes.
- **Made: the colours of a continued game** (`continueColours`). The save does not hold `mNumColors`, so a game resumed from "continue"
  deals its new coins with the colour count of level 0 (one fewer at skill 1) until the next level (quirk 2 of section 11). With the
  fix `Board::LoadGame` gives it the count `SetupLevelParams` gives the saved level. Seen: a continued Action skill 1 game at level 2
  dealt 3 colours, now 4. The random generator's index after a load is the other half of the same quirk; the save holds no `mti`
  (the original's own format), so a continued game's coins are not the ones the saved game would have drawn. Kept, and listed below.
- **Made: Done's double click** (`doneClicksOnce`). `OptionsDialog::ButtonPress` meant to skip the dialog's own two buttons but
  tests ids 2000 and 3000 above the dialog's, while those buttons are 1000 and 1001, so Done plays the click here and once more in
  `WinBMApp::ButtonPress`. The fix returns before the dialog's play. Seen: 2 click sounds, now 1.
- **Made in the port itself: the Puzzle generator's hang** (section 6.5). The original never ends when `GenerateBoard` picks a
  column of 13 for the first coin of a group and no candidate column is left; the port breaks out of the loop in that case, which
  changes no run that does not hang. It is a bug that breaks play, so it is not in the mod.
- **Kept: the stale copy in the scores dialog** (quirk 2 of Part 5, section 6). Every `HighScoreWidget` destruction rewrites
  `hiscores.dat`, including the Scores dialog's private copy, so a score saved while the modeless name dialog was up and the
  Scores dialog closed afterwards is overwritten by the dialog's older table; the board's own widget writes it back when the board
  is destroyed. It needs a sequence the game does not make easy to reach, loses nothing in the end, and fixing it would change what
  the original writes to the file: left as it is.

Oddities (not fixed, not chased): the follower's column slide takes `mGrid[r + 1]` twice (quirk 4 of section 11); leftover flood
marks make an early click in the first two seconds of a board fail (quirk 7); both Options checkboxes have the id 0 (so unticking
either re-ticks full screen when it is forced); the high-score widget clips everything above y 252 (a typo of the original); the
messages carry double spaces; the random generator's index is not restored by a load (above). Each one can be seen as described in
its section.

### The fixes mod (`mods/fixes`)

`mod.json` with the feature `fixes` and one bool setting for each fix (`shakeSettles`, `continueColours`, `doneClicksOnce`), on
by default except `continueColours`, which starts off: a continued game's fewer colours looks intended and is not a
known bug (docs/MOD_ROADMAP.md, "The fixes mod"). The game reads them where it needs them (`Platform::FixOn("<id>")`), so without the mod every fix is
off and the game is the original; a toggle takes effect at its next use. The mod has features and no rules or tunables, so it does
not split the high scores (it does not touch scoring). The site ships it beside the page as `bigmoney-fixes/` and starts the game
with it on (`cmake/ByopopWeb.cmake`; Big Money is not in `BYOPOP_WEB_NO_MODS`); natively it is a folder in `mods-bigmoney`, or in a
`-modsdir`. Options shows a MODS button when an installed mod has settings, which opens the mod menu over the dialog.

### Registration and the buttons that lead out of the game (taken out 2026-11-08)

The port was the registered Steam game already (the shareware branches were never translated, and `mBuildUnlocked` and
`mIsRegistered` are always true in the framework); what was still in it is gone now. The frozen baseline (a whole game of
Action, score 66646) is the same before and after, and no screen of the registered game changed.

- The Options dialog's Register button (id 1) and "check for updates" button (id 3), their layouts without `DontUpdate` and
  `mBuildUnlocked` (a row of three, a row of two) and `ShowRegisterDialog`. The dialog has only the Steam layout of the registered
  original (Help and Credits, then Scores; with a mod that has settings, MODS and Scores share the second row). No visible change.
- The title screen's register link (hidden when registered), the nag timer and the trial and play-count members behind it
  (`mNagProgress`, `mShownTimesPlayed`, `mWantRegister`, `mTrialExpired`, `mNagSeconds`, `mMaxPlays`, `mCountdownTrial`), the
  copy of the trial flag to `mTimedOut`, and the status line's THANKS_FOR_REGISTERING branch (which only ran for a locked build:
  the line is empty, but for PLEASE_WAIT on the first run, as before). No visible change.
- The dialogs and pictures only they used: `DIALOG_REGISTER`, `DIALOG_UNUSED4` (never created), the update, URL and
  opening-the-browser dialogs and their ids; the images `trialversion`, `waitbar`, `register` and `register_overlay`
  (`NUM_IMAGES` 71 to 68; every image id after them moved up, and `gImageNames` with them).
- `WinBMApp::OpenURL`, `URLOpenFailed` and `URLOpenSucceeded` (the "opening your browser" dialog and its failure notice): the game
  opens no address of its own. The framework's `OpenURL` stays for what the shared code needs.
- Kept: the Scores, Help and Credits buttons, and the board's "quit game" (which leaves the game, not to a page).
- Kept: the play count the framework keeps (`mTimesPlayed`, one for each game's first click): the title's PLEASE_WAIT line reads it
  (it shows only while the very first launch loads), so removing it would show that line at every launch.

### The driver that plays by `move`

`tools/realplay/drive.py` plays a game natively with real input only: it starts the exe under rundesk with `-commands` and an empty
`-realinput` file, asks `move` for each step and sends it as real SDL events (`click game X Y`; the name's characters as `type` and
`key`), and reads `state` to know when a game is over. `--plan action:1,strategy:2,puzzle:1` plays that list of games, going back
to the selector with `goto selector` when the next game is of another mode or skill and otherwise "play again"; `--at N:<lines>`
sends focus, full screen and resize lines in the middle of a game; also `--pre`, `--steps`, and `--shots` (a screenshot of every
new screen; the state's `windowed`, `focus` and `paused` are logged when they change). It prints one JSON line and exits 0 when the
plan was played to its end.

On the web `tools/web/playgame.js` is the same loop as a session driver of the player-only page: `?player=1&silent=1&assets=1&fast=1&framems=2&driver=games/bigmoney/tools/web/playgame.js&plan=action:1,strategy:1,puzzle:1&cap=12000&blurat=30,200,260`, then `await
byopop.test.driver` in the pane (`blurat` loses and regains the focus after those moves; `steps`, `maxseconds`). The built-in
`?driver=move` plays one game of whatever mode the selector's last choice was. Three whole games take 15 s with
`fast=1&framems=2`; a real-time page in the hidden pane stopped answering after about 350 s, so a long web run uses `fast`.

Natively the game runs in real time: one command takes about 16 ms (the framework's read of the file and the log's flush), which in
`-fast` is about 350 updates, and Action's coins rise by themselves, so a driver that slow lets the board fill between two clicks
(a whole Action game of 8 clicks in `-fast`). Real time keeps one command to under two updates.


### Performance (measured 2026-11-08, original and port back to back)

`tools/profile/perf.py` takes the numbers (its docstring has the commands): it samples the game's process from outside every 0.1 s
(working set, private bytes, kernel plus user time) and takes the load time from the process's creation to the title's play link, which
the port reports as the `move` kind "title: play" and, for the original, the title's `mLoadDone` byte read out of the copy's memory. The
original ran under origrun on its real clock from a fresh copy of `orig.sh --prepare`'s, with the seed patch (it draws the port's boards),
the port natively under rundesk without `-fast`. No speed-ups were made to this port, so these are the port as merged. The scripted
minute is the last 6000 updates of a 9000-update Action game, skill 1, that the autoplay bot played with seed 21 (`-autoplaylog`);
the original got the same input through `fromport.py` (its title click at update 1000), the port its own log. The port marks the minute's
first update with a screenshot's file time, the original's is found from its update counter.

**The machine was not quiet.** Other sessions' game processes were running throughout (3 to 7 at the start and end of each run) and the
whole machine was 46 to 52 % busy (75 % during one port run), so every figure is slower and noisier than on a quiet one. The performance
pass re-measures later.

| First screen that takes input | Original under origrun | Port native | Port in the web page |
|---|---|---|---|
| Seconds from launch, every run | 1.77, 1.37, 1.32, 1.39, 1.38 | 0.75, 0.73, 0.73, 0.74, 0.72 | 1.15, 1.56, 1.86, 1.98, 1.88 |
| Median | 1.38 s | 0.73 s | 1.88 s |

The web column is the player-only test page in the Browser pane (`?player=1&silent=1&assets=1`, `tools/web/loadtime.js`), from the start of
the navigation to the title's play link, with the game's files served from this machine by `tools/webtest/serve.py` (they were in after
0.67 to 1.18 s), so a visitor on the site adds the network's time. At the title the port holds 83 MB of working set and 83 MB of private
bytes, the original 50 to 51 MB and 37 MB; the processor time up to then is 0.67 to 0.72 s against 0.42 to 0.59 s. Loading was already
as fast as the machine allows: the loading thread runs in one go after the first frame, and the title polls its progress every fourth
update, so the play link appears once the files are in, with no minimum time. (The title bar's slide, which starts at update 50, is an
animation that does not hold the player.)

| The scripted minute (6000 updates) | Original A | Original B | Port A | Port B |
|---|---|---|---|---|
| Peak working set | 56.2 MB | 56.0 MB | 124.2 MB | 124.1 MB |
| Peak private bytes | 42.1 MB | 41.8 MB | 124.1 MB | 124.1 MB |
| Processor time | 7.14 s | 7.16 s | 4.03 s | 4.66 s |
| Processor time per update | 1.19 ms | 1.19 ms | 0.67 ms | 0.78 ms |
| Wall time (updates a second) | 60.0 s (100) | 60.1 s (100) | 59.9 s (100) | 60.9 s (98.5) |

Game processes of other sessions meanwhile (start and end of the run): 7 and 4, 4 and 3 for the originals' minutes, 3 and 3, 3 and 4
for the port's. The port starts about twice as fast as the original and uses about 40 % less processor time in play, and its memory
is the other way round: the working set is twice the original's and its private bytes three times (124 MB against 42 MB). Why was not
looked into; it is a figure for the performance pass.


## Coverage check (2026-11-08)

The coverage check of `docs/COVERAGE.md`: origrun's `--coverage` recorded which of the original's functions ran, in 17 runs on the host
(`tools/coverage/run.sh`, `all.sh` and `extra.sh`; the hits in `build/coverage/bigmoney/hits/`, git-ignored).

- **The nine games of the identical-play pass** (`origcmp/game.sh`'s way: the port's bot plays, the original gets its input through
  `fromport.py`): Action skills 1 to 3 (seeds 1, 7 and 2), Strategy skills 1 to 3 (seeds 3, 8 and 4, the last of two games) and Puzzle
  skills 1 to 3 (seeds 5, 9 and 6, two games for skill 2), each about 1,550 to 1,650 functions. They reach everything a game plays: the
  three modes' boards and selectors, levels won and lost, bags, bombs, the overflow countdown, game overs, the name of a high score.
- **The menus tour** (`tools/coverage/menus.txt`, `origcmp/tours/menus.txt` without its shot lines, which origrun refuses on the host):
  the Options dialog with its music slider, the scores, the credits, the help screen with the focus lost and regained, the new-game
  and quit questions, and the selector sliding in over a board.
- **Two saved-game runs** (`cont-yes.txt`, `cont-no.txt`, with the `savegame.dat` that the first Action run left when its window was
  closed in the middle of a game): the "continue your last game?" question answered Yes (the load, then clicks across the board) and No.
- **Quits** (`quit-title.txt`, `quit-selector.txt`; a board's quit is the first Action run's end), and **an Action game nobody plays**
  (`idle-action.txt`: the stack rises to the top and the game is over).
- **Two runs on the real clock** with origrun's silent stand-in (`ORIG_CLOCK=real`): the menus tour and the first Action game. They
  reached no function the virtual-clock runs had not, but for one in the first Action game that the others reach too
  (`Board::ShowHintAt`); the stand-in serves the DirectSound the game asks for in every run, so the sound code ran in all of them.

Every run ended with the game exiting normally, by its own `quit`. `games/bigmoney/coverage.txt` sets the game's own code as
0x401000-0x421820, which ends at the last function before the framework (`Sexy::WidgetContainer::MarkDirty`, 0x421830); the rest of
the exe is SexyAppFramework 1.2, the C runtime and the libraries. That code has 269 functions (the C++ library's templates and the
compiler's deleting destructors left aside), and the 17 runs reached 223 of them. The 46 that no run reaches are the left-out ones
below: 67 are left out on purpose, 21 of them reached.

**Reached in the original with no counterpart in the port: 19 at first (nine runs), 0 at the end.** The Options dialog's vector insert,
which the menus tour reached, made it 20. All were the compiler's and MSVC 7.0's library code, instantiated among the game's own
functions because the linker put them there, and are `none` lines with their reasons: the framework's `ButtonListener::ButtonPress`
forwarder (`framework/ButtonListener.h` has the same one), a rectangle constructor, an operator delete thunk, `std::string` appends,
`==`, `substr` and `operator+`, `std::list`'s size check in `WinBMApp::MakeButton`, vector pointer copies and fills and `_Insert_n`, and
the `std::map<string, string>` code of the framework's string maps (rotations, node and pair copies, erase). No source file was
changed and no missing game logic was found; the frozen baseline is the same as before (`tools/check.sh bigmoney`: game 1 over,
score 66646).

**Defined in the port, never reached: 42 at first, 2 at the end.** The first nine runs missed what they never ask for, and each gap
was closed by a run:

| Gap | Closed by |
|---|---|
| `Board::LoadGame`, `ReadPiece` | the saved game answered Yes |
| `DeleteSaveGame` (called for both answers) | the saved game answered Yes and No |
| the Options, credits and scores dialogs (constructors, `Draw`, `Resize`, `ButtonDepress`, sliders and checkbox, destructors), `ShowOptionsDialog`, `ShowCreditsDialog`, `ShowHighScoreDialog`, `ConfirmQuit`, `ConfirmNewGame`, `GotFocus`, `LostFocus`, `NewDialog`, `ModalOpen`, `ModalClose`, `WinBMApp::ButtonPress` (34 in all) | the menus tour (the last four also by the saved-game runs' dialogs) |
| `Board::Freeze`, `RemovePointsWidgets`, `GameSelector::SlideIn` | the menus tour's "new game": the selector slides over the board |

The two left are the compiler's deleting destructors of `MoneyDialog`, which is only the base of the Options, scores, credits and
record-score dialogs and is never created itself, and of `WinBMApp`, a global object that is never deleted: no play of the original
reaches them and the port has them only as a comment on the destructor. They are `none` lines (the report still lists them, since the
port names their addresses). Nothing in the port is a candidate for taking out because of this check.

**Never reached and not defined in the port: 45 at first, 0 at the end.** The registration, update-check and URL code that the
port left out on 2026-11-08 (the register dialog and its helpers, the shareware dialogs for one level and a locked next level, the
"opening your browser" dialogs, `OpenURL`, the update check, and the online score submission), the map code behind the register page's
parameters, and twelve exception funclets (catch blocks of the compiler's tables) are `none` lines. No run should have reached any of
them. Part 6 ("Registration and the buttons that lead out of the game") lists what was taken out.

**Open:** nothing. The Options dialog's checkboxes (full screen, 3D) were not clicked in these runs, since the original would change
the display mode, and the port's version of full screen is a listed deviation of its own; `OptionsDialog::CheckboxChecked` and
`ButtonDepress` were reached by the menus tour. The framework's sound code lies outside the game code and is not compared.

**What it cost.** The 17 runs took roughly 30 minutes of machine time (estimated from the end times), one game process at a time; the first
nine waited for room for most of an hour. No agent was used: the 19 functions were identified from their decompilation by this session. A spec for the tool
goes to the coordinator: the report lists a function the port defines under "Defined in the port, never reached" even when a `none`
line leaves it out.
