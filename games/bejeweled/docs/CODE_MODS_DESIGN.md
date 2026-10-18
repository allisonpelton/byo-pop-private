# Code mods: design

Status: design only, nothing implemented. This document combines three candidate designs and two
reviews of them. The base is the "fidelity-first" design (both reviewers chose it). Ideas are taken
from the "determinism-first" and "rules-engine" designs, and every factual error the reviewers
found has been corrected (Appendix A). Section 1.3 records each point where the designs disagreed
and the decision taken.

The facts below were checked against the current tree: `games/bejeweled/src/Board.h`, `Board.cpp`,
`BoardRules.cpp`, `Piece.h`, `WinDMApp.*`, `GameConfig.*`, `HighScoreWidget.cpp`,
`platform/ModManager.*`, `WebPersist.cpp`, `Json.*`, `SexyAppBase.cpp` (test hooks, backup),
`MTRand.*`, `Common.cpp`, `web/shell.html`, `games/bejeweled/docs/GAME_NOTES.md` and
`build/native/playtest/*`.

---

## 1. Overview and principles

### 1.1 Scope

All 12 features in `docs/MOD_ROADMAP.md` become mods that the user turns on in the existing mods
panel:

1. allow invalid moves
2. more gem colours
3. board size and shapes
4. Chuzzle-style slides
5. Twist-style rotations
6. custom match patterns
7. flame and lightning gems
8. Jewel Quest gold board
9. set seed
10. save and watch replays
11. undo
12. online leaderboards

Added later: dirt tiles (4.14), auto LiveSplit support (4.15) and Twilight gravity (4.16).

Where it makes sense the features combine. For example, a diamond board with Twist moves, flame
gems, a gold layer and undo is a single valid set of rules, built from four mods or from one.

### 1.2 Principles

1. **The no-mod game is the translated code.**
   - `Board` stays `class Board : public Widget`.
   - Every addressed function keeps its body and its `// 0xADDR` comment.
   - Only three kinds of edit are allowed inside a translated function:
     - **(a) A constant becomes a member holding the same value.** Loop bound `8` becomes
       `mRows`/`mCols`. Origin `203`/`16` becomes `mLayout.mOriginX/Y`. `mHeight` used as the
       fall limit becomes `mLayout.mScreenBottom`.
     - **(b) An inert hook.** Either `if (mVariant.X) { XVariant(...); return; }`, or a NULL guard
       on a cell that can never be NULL on a full 8x8 board.
     - **(c) An observer hook.** `if (mRecorder) mRecorder->...;`, which only reads state.
   - Every edited line carries a `// port(variant):` tag. `games/bejeweled/tools/list_variant_hooks.py` prints
     the tagged lines grouped by function, so the whole intrusion can be reviewed against the
     decompilation.
2. **Variant behaviour lives in new files.** It is written as `Board` members in
   `games/bejeweled/src/BoardVariant*.cpp`, plus framework-free rule code (`MatchFinder`, `MoveSet`,
   `StateCodec`). The game, the hint search and the headless verifier all share that code.
3. **Mods remain data.** `mod.json` gains three keys:
   - `rules`: gameplay. Rules affect scores.
   - `features`: replays, seed entry, leaderboard and LiveSplit. These never change gameplay.
   - `imageOps`: generated art.

   The engine implements every rule; a mod only switches it on and sets its parameters. A
   sandboxed scripting layer could later plug into the same hooks. That is listed as a possible
   extension, not part of this plan.
4. **There are two tiers of gameplay change.**
   - *Tunables* are today's keys (`scoring.*`, `timing.*`, `board.gemColors`). Tunables-only mods
     such as `example-relaxed` stay on the original code paths, the original global RNG stream and
     the v1 save format.
   - *Rules* are everything new. Any active rule turns the board into a *variant board*, which
     has:
     - its own per-game RNG,
     - the v2 save format,
     - extra trace lines.
5. **Geometry is simulated in logical 51-px space.**
   - Every board, whatever its size, is simulated with the original pitch (51), gem size (49) and
     physics constants. That covers falls, the spawn offsets 65..304 and 69, the vortex radii,
     the click zones and the drag threshold.
   - Only drawing and hit-testing scale the board into the original 406x406 window.
   - So no gameplay constant changes inside translated code, and every board size feels the same
     (section 5).
6. **Everything that affects play is deterministic across native and web.**
   - Input is recorded as commands stamped with the board tick.
   - The RNG source of every board is either captured or owned by the board.
   - Floating-point math is restricted to operations that give identical results on both
     platforms.
7. **Without mods nothing changes.** There are no new files in the user dir, no new log lines, no
   new registry keys and no new UI.
8. **Identity is proved, not assumed.** The proofs are:
   - a frozen baseline executable,
   - the `-forcevariant` run,
   - the `BEJ_NO_VARIANTS` build,
   - a lockstep difftest,
   - trace equality between recording, in-game playback and the headless verifier.

### 1.3 Decisions where the source designs disagreed

| Topic | Decision | Why |
|---|---|---|
| Board structure | `Board` stays a Widget. There is no `BoardSim`/`BoardWidget` split. The headless verifier compiles `Board.cpp` unchanged behind header shims. | A split touches every translated function, which is the most-verified code. Shim drift is a smaller risk, and CI trace diffs catch it (6.9). |
| Geometry | Logical 51-px simulation with a view-only scale (from rules-engine). This replaces "constants scaled by pitch" (fidelity). | Removes edits to `24.0`, `49`, `22/88.2/26`, `16`, `69`, `240/65` and the sparkle offsets inside translated functions. Physics and click feel are identical at every board size. |
| RNG | A `Board::Rand()` member hides `Sexy::Rand` at the 9 draw expressions. Original-rules unseeded games keep the global stream. Seeded, rule, seed-feature and ranked games own an `MTRand` (from determinism). The global `gMTRand` is never reseeded. | Draws outside the board (the `HighScoreWidget` tip roll) cannot shift a board's stream, and ranked players cannot burn draws. |
| External draws | A board-own draw counter is compared with a global counter at `Board::Update` entry. Draws are not sampled at frame boundaries. | `-script` input runs inside `SexyAppBase::UpdateFrames` (`RunScript` before `WidgetManager::UpdateFrame`), and the board's own draws also happen inside the frame. |
| Resume | A resume is a new segment: a fresh constructor (with its `FillBoard` draws), then `LoadGame` at board tick 0. | `LoadGame` does not restore `mUpdateCnt` (it restarts at 0), `mTimeDrainDelay` (300 from the ctor), timers or popups, and tick 150 runs again. A resumed game cannot continue the uninterrupted trace. |
| Undo | Undo uses `StateCodec` snapshots, not the `LoadGame` body. | `LoadGame` calls `mApp->SetTimedMode(..., false)` and sets `mApp->mShowInstructions = false`, and it misses unsaved fields. |
| Move types | A set of move types. swap+twist, swap+slide, twist+slide and all three are allowed (from determinism). | The combinations the user asked for. Input resolution is defined in 4.4. |
| Patterns | Straight runs, optional diagonals, optional merging of crossing runs into L/T/plus, and keyed cell patterns with rotations and mirrors (from rules-engine). | Covers "L of three", squares and arbitrary shapes. |
| Rules hash | SHA-256 over a binary canonical encoding. Doubles are hashed as IEEE bit patterns (from determinism). | No `%.17g` or `strtod` text formatting goes into a hash. |
| Local score key | Unchanged FNV-1a of `id@version` (fidelity). Rule mods count as score-affecting, and so does a mod that overrides `data/diff.txt` (from the other two designs). | `example-relaxed` keeps `savegame-58ebd9d7.dat`. Closes today's `diff.txt` gap. |
| Trig | libm stays. There is no DetMath. New animations use integer tables. | DetMath would change native no-mod output (the raw `mHoleAngle` bytes in vortex saves, and the swap positions that `GetPieceAt` sees). Trig-derived values never feed a gameplay decision, and the LOGIC hash excludes them (6.6). |
| Replay storage | Native: `<userdir>/replays/`. Web: `/persist/replays`, a sibling of `/persist/userdata`. | `webpersist_backup` copies the whole user dir into one localStorage item and deletes it on a quota error. `webpersist_restore` also deletes user-dir files missing from the backup, so excluding a user-dir folder from the backup would delete it. |
| Invalid rule sets | A mod that fails its own checks has its `rules` ignored. A merged set that fails drops the lowest-priority mod contributing to the failing group, then merges again. Groups are never applied partly. | Rule-engine's "everything falls back to classic" punished unrelated mods. |
| Kept invalid move | Clears `mIsNewGame` (`invalidCountsAsMove`, default true) and runs through `STATE_FALLING`'s settle path. | With `mIsNewGame` left true, `IsGameInProgress()` is false and the game would not be saved on exit. The settle path reuses the original no-moves check. |
| Watching a replay | Watching replaces the current board the way a mode switch does: a game in progress is saved by `~Board`. When the watch ends, `NewGame(); ConfirmLoadGame();` offers to resume it. | Only one board exists at a time, so `gGameConfig` can be switched to the replay's rules safely. Reuses existing flows. |
| Verifier wasm | `-sSTANDALONE_WASM`, no JS glue (from rules-engine and determinism). | Workers instantiate imported modules. Minimal imports. |

---

## 2. Architecture

### 2.1 Layers

```
 web/shell.html (mods panel + status, ?seed= ?replay=, Replays + Leaderboard panels)      Cloudflare Worker
 WinDMApp  -- NewGame seeds/attaches -->  Board : Widget                                   (TypeScript, D1)
   | ExtrasDialog, ReplayBrowserDialog,     translated: Board.cpp, BoardRules.cpp            |
   | WatchController, ReplayRecorder        port(variant): BoardVariant*.cpp, BoardSave.cpp   |
   |                                        uses: GameRules, BoardLayout, MatchFinder,        |
   |                                              MoveSet, StateCodec, Replay                 |
   +--------------------------------------------------------------------------------------+  |
   bejeweled_verify (native exe + standalone wasm): the same Board sources + src/headless   <-+
   shims, BoardTrace, GameRules, StateCodec, Replay, MTRand, Buffer, Json, sha256
```

### 2.2 Variant flags and test switches

The Board constructor computes `VariantFlags` once from its rules snapshot. Every flag is false
for the original rules and for tunables-only mods.

```cpp
struct VariantFlags
{
	bool mAny;        // any rule differs from default: v2 save, variant trace lines
	bool mOwnRng;     // board owns an MTRand (seeded / rules / seeds feature / ranked game)
	bool mShape;      // not a full 8x8: size, mask or per-level shapes
	bool mScaled;     // logical extent D > 406, so the view scales (section 5)
	bool mDraw;       // DrawVariant: mShape || specials || gold || non-swap moves || mNoTutorial
	bool mMatch;      // ProcessMatchesVariant / HasMatchVariant: patterns, minRun != 3, specials, gold
	bool mSearch;     // FindMoveVariant / DoHintVariant: patterns, minRun != 3, non-swap move types
	bool mMoves;      // twist / slide in the move set: the variant input layer
	bool mInvalid;    // moves.allowInvalid
	bool mNoMoves;    // a non-default noMoves rule
	bool mNoTutorial; // the tutorial move (rows 4..6, cols 1..6, horizontal 3-run) cannot apply
	bool mGravity;    // board.gravity.direction != down: the flip, fall-out and game-over variants (4.16)
	bool mFall;       // FallPiecesVariant, RefillMasked, FillBoardVariant: mShape, mGravity or fallAccel != 180
	bool mChecker;    // DrawVariantChecker in SetupLevel: mShape || look.backdrop == "twilight"
	bool mSpecials, mGold, mUndo;
};
```

- **`-forcevariant`** is a test hook parsed in `main.cpp`, like `-nomods`.
  - It sets `mShape`, `mMatch`, `mSearch` and `mDraw` while the rules stay the default. From stage 7c
    it also sets `mGravity`, `mFall` and `mChecker` (4.16).
  - It leaves `mAny`, `mOwnRng` and `mNoTutorial` alone, so the RNG source, the save format and
    the tutorial stay original.
  - Every generalised path then runs at default rules, and the oracle must still match: traces,
    files, and screenshots pixel for pixel.
- **`-DBEJ_NO_VARIANTS`** turns `mVariant` into a `static constexpr VariantFlags` with every flag
  false, so the compiler removes every hook. This build must also match the oracle, which shows
  the hooks are inert by construction.

### 2.3 Files

| File | Kind | Contents |
|---|---|---|
| `games/bejeweled/src/GameRules.h/.cpp` | new | The `GameRules` value type (board, moves, noMoves, match, specials, gold, undo, plus the legacy tunables and the 12 diff values). Covers the schema table, defaults, `LoadFromMerged()`, `Validate()`, `Canonical()` (binary), `RulesId()` (SHA-256) and `FromCanonical()`, used by replays. SDL-free. |
| `games/bejeweled/src/GameConfig.*` | changed | Split in two. `GameConfig.cpp` stays SDL-free and holds `SetValue` and `LoadFromRules`. The new `GameConfigMods.cpp` holds `Load()` from ModManager and `ScopedFileName`. Adds `GameRules mRules`, `GameFeatures mFeatures` and `mNextBoardSeed`. The gem-colour clamp becomes 3..12. |
| `games/bejeweled/src/BoardLayout.h` | new, header only | Columns, rows, playable mask, per-column playable rows, logical origin, extent `D`, scale, `mScreenBottom`, `ToScreen`/`ToLogical`, contiguous line segments, and from stage 7c the clip's bottom edge `mClipBottom` in logical space (4.16). On 8x8 the default layout is exactly the original constants. |
| `games/bejeweled/src/MatchFinder.h/.cpp` | new, pure | `ColorGrid` (int8 colour, -1 empty, -2 hole, plus a kind byte), `FindGroups(grid, MatchRules, out)` in the original scan order, `AnyGroup`, `GroupsThrough(cells)`, and pattern compilation (rotations, mirrors). No Sexy headers. |
| `games/bejeweled/src/MoveSet.h/.cpp` | new, pure | `Move {type, a, b, c, d}`, `EnumerateMoves(grid, layout, rules, visitor)` in a fixed order, `ApplyMove(grid&, move)`, `MovedCells(move)`. |
| `games/bejeweled/src/StateCodec.h/.cpp` | new | One field table that drives the v2 save chunks, undo snapshots, and the FULL and LOGIC hashes (6.6). |
| `games/bejeweled/src/BoardVariant.cpp` | new, Board members | `InitVariant`, `SetupLevelVariant`, `ApplyPendingLayout`, `UpdateGoldGoal`, `OnNoMovesVariant`, `KeepInvalidMove`, the undo stack. |
| `games/bejeweled/src/BoardMatch.cpp` | new, Board members | `ProcessMatchesVariant`, `HasMatchVariant`, `FindMoveVariant`, `DoHintVariant`, `ScoreGroup`, special gems, gilding. |
| `games/bejeweled/src/BoardMoves.cpp` | new, Board members | Variant input (`MouseDownVariant/DragVariant/UpVariant`), `StartTwist/UpdateTwist`, `StartSlide/UpdateSlide`, the integer twist table. |
| `games/bejeweled/src/BoardShape.cpp` | new, Board members | `FillBoardVariant`, `RefillMasked`, `FillBoardWithMove`, `DrawVariantChecker`. |
| `games/bejeweled/src/BoardDrawVariant.cpp` | new, Board members | `DrawVariant`: a superset of `Draw` 0x4cbc70 with scaling, holes, gold layer, special overlays, effects, twist cursor, slide preview and the gold HUD. |
| `games/bejeweled/src/BoardSave.cpp` | new, Board members | The v2 header and chunk writer/reader. |
| `games/bejeweled/src/Replay.h/.cpp`, `ReplayRecorder.*`, `WatchController.*` | new | The `.bjr` format, recording, the playback driver and gate checks, segments and the sidecar. |
| `games/bejeweled/src/BoardTrace.cpp` | new (moved) | `TraceBoard`, moved out of `WinDMApp.cpp` so the verifier prints the same trace. |
| `games/bejeweled/src/ExtrasDialog.*`, `ReplayBrowserDialog.*` | new | UI, reachable only with feature mods. |
| `games/bejeweled/src/LiveSplitBlock.h`, `LiveSplitObserver.*` | new, desktop only | The LiveSplit block (layout 1, `static_assert`s on every offset) and the read-only observer that fills it after each app update (4.15). |
| `framework/MTRand.*` | small addition | `GetState(uint32_t mt[624], int* mti)` / `SetState`. `Serialize()` drops `mti`, and the serialising constructor forces `mti = N+1`. |
| `framework/Common.*` | small addition | `Sexy::GetRandCount()`: a counter incremented in the three `Rand` overloads. |
| `framework/SexyAppBase.*` | small addition | `virtual bool HandleScriptCommand(name, args)`, so the game can add `save` and `autoplay` script commands. `GetShutdownSave` becomes `GetShutdownSaves(std::vector<BackupFile>*)`. |
| `framework/SoundManager.h`, `SDLSoundManager.*` | small addition | `SetDefaultPitch(steps)`: `GetSoundInstance` applies `AdjustPitch(steps)` to every instance it hands out (4.16). |
| `platform/ModManager.*` | changed | Reads `rules`, `features` and `imageOps`. Merges with provenance, atomic groups, keyed objects, `null` resets and a union operator. Marks rule mods and `diff.txt` overriders as score-affecting. Writes `mods-status.json`. |
| `platform/ImagePatch.cpp` | addition | `imageOps` `extendRows`: copies or recolours a base row on both `mBits` and `mHDBits` of a `LoadedImage`. |
| `games/bejeweled/src/headless/` | new | Header shims, `WinDMAppHeadless.cpp`, `verify_main.cpp`, `sha256.c`. Target `bejeweled_verify`. |
| `server/leaderboard/` | new | Worker (TypeScript), `schema.sql`, `wrangler.toml`. |
| `web/shell.html` | changed | `?seed=`, `?replay=`, `-replaydir`, gameplay/feature tags, rules status, Replays and Leaderboard panels. |
| `games/bejeweled/tools/oracle/` | new | `compare.sh`, `compare.py` (masks, PNG diff), `list_variant_hooks.py`, new playtest scripts. |
| `games/bejeweled/tools/livesplit/` | new | `BejeweledDeluxe-byopop.asl` with its README and licence, `lsprobe`, `lsdecide.py`, `asltest`, `check_lslog.py`, `check_asl_offsets.py` (4.15). |

### 2.4 Data additions

**Board.** These members are appended after `mOrphanedPieces`. The original offset comments stay
as they are: they describe the binary's layout, not the port's.

```cpp
	enum { MAX_COLS = 16, MAX_ROWS = 16, MAX_GEM_COLORS = 12, SAVEGAME_VERSION_VARIANT = 0x101 };
	// port: mBoard is enlarged from [8][8] to [MAX_ROWS][MAX_COLS]; the original grid is [0..7][0..7]
	// ---- port(variant): code mods; all inert for the original rules ----
	GameRules        mRules;          // snapshot of gGameConfig.mRules taken in InitVariant
	VariantFlags     mVariant;
	BoardLayout      mLayout;         // current level's layout (8x8: origin 203/16, D 406, bottom 480)
	BoardLayout      mPendingLayout;  // next level's layout; SetupLevel draws it, UpdateBlackHole applies it
	int              mCols, mRows;    // == mLayout dims; 8, 8
	int              mLevelShape;     // index into rules.board.levels (0 without levels)
	MTRand*          mOwnRng;         // NULL: Sexy::Rand (original stream)
	uint32_t         mGameSeed;  bool mUserSeeded;  bool mRanked;
	uint32_t         mBoardDraws;     // Board::Rand() calls (external-draw detection)
	uint8_t          mGold[MAX_ROWS][MAX_COLS];
	int              mGoldLeft, mGoldTicksLeft, mGoldTickLimit;
	int              mMoveCount, mInvalidMoves, mUndosUsed;
	TwistState       mTwist;          // anchor col/row, dir, angle step 0..18, back
	SlideState       mSlide;          // axis, line, segment start/len, k, release offset, tick, back
	Piece*           mMovePieces[MAX_COLS];
	std::list<BoardEffect> mEffects;  // blast / beam / gild visuals: cosmetic, unsaved (like mScorePopups)
	UndoStack        mUndo;
	ReplayRecorder*  mRecorder;       // owned by WinDMApp; NULL unless recording
	bool             mPlaybackOnly;   // driven by a WatchController / the verifier
	const Buffer*    mPlaybackLoad;   // LoadGame reads this instead of the file (playback LOAD)
	Buffer           mPlaybackSave;   // SaveGame writes here instead of the file (playback SAVE)
	RngStart         mRngStart;       // seed, or MT state captured at ctor entry (only when recording)
	uint32_t         mBoardSerial;  bool mResumed;      // LiveSplit observer bookkeeping, unhashed (4.15)
	int              mGravityFlips; bool mGravityNoFlip; // Twilight gravity (4.16, stage 7c)
```

**Piece.** `Board::SaveGame` writes the first 0x78 bytes raw. The new fields go after them:

```cpp
	enum { SAVE_SIZE = 0x78 };   // the raw savegame.dat record; the layout up to here is unchanged
	// port(variant): not part of the raw save record
	int mKind;        // +0x78 PIECE_NORMAL 0 / PIECE_FLAME 1 / PIECE_LIGHTNING 2
	int mKindFrame;   // +0x7c overlay animation frame (cosmetic)
};
static_assert(offsetof(Piece, mKind) == Piece::SAVE_SIZE, "variant tail starts after the save record");
```

- `sizeof(Piece) == 0x78` becomes the assert above. The other layout asserts stay.
- `Piece::Piece` (0x4c9aec) already runs `memset(this, 0, sizeof(Piece))`, so it zeroes the tail
  without any change.
- `WriteSaveGame` and `LoadGame` change `sizeof(Piece)` to `Piece::SAVE_SIZE`, so v1 bytes are
  identical. Without that change `LoadGame` would read 0x80 bytes per piece and break existing
  saves.
- GAME_NOTES section 12 says "keep Piece.h exactly as is". Read that as "keep the 0x78-byte record
  as is", and update the note.

**Board::Rand.** It is declared in the class. Inside member functions, class-scope lookup finds it
before `Sexy::Rand`, so the 9 translated `Rand()` expressions do not change:

```cpp
	int Rand() { mBoardDraws++; return mOwnRng ? (int)mOwnRng->Next() : Sexy::Rand(); }   // port(variant)
```

The 9 translated `Rand()` expressions are:
- `UpdateClearing` refill
- `UpdateGameOver`: 4 draws
- `FillBoard`: 2 draws
- `UpdateLevelWipe`
- `UpdateFallOut`

GAME_NOTES section 3.16 groups them into 7 items. `Sexy::Rand()` and `MTRand::Next()` return the
same value, so the original stream is unchanged.

### 2.5 Function map: what changes inside each translated Board function

Kinds of change:
- **B**: loop bound `8`/`7` becomes `mRows`/`mCols`.
- **L**: a layout member with the original value.
- **H**: an inert hook.
- **N**: a NULL guard that cannot fire on a full board.
- **R**: a recorder hook that only observes.

| Function | Addr | Change | Detail (default path identical) |
|---|---|---|---|
| `Piece::Piece` | 0x4c9aec | none | The memset covers the new tail. |
| `GetColX`, `GetRowY` | 0x4c9b25, 0x4c9b37 | L | Static becomes member (`this` is unused in the binary anyway): `theCol * CELL_SIZE + mLayout.mOriginX`. |
| `GameOver` | 0x4c9b47 | H, R | `UsePendingSeed();` (stage 2: a game over uses up a seed the player chose, 4.10). `mRecorder->OnGameOver()` (result snapshot). |
| `RequestNewGame` | 0x4c9bab | H, R | First line: `mTakesPendingSeed = false;` (stage 2: New Game is no game over for the seed, 4.10). `NEW_GAME` event. |
| `IsGameInProgress` | 0x4c9bbe | H | `if (mPlaybackOnly) return false;` Covers both checks in `~Board`, `Shutdown`, `GetShutdownSaves` and `ConfirmQuit`. |
| `Pause`, `Unpause` | 0x4c9bda, 0x4c9c2d | R | `PAUSE u` / `UNPAUSE u`. Calls nested inside `SaveGame` are suppressed, because the `SAVE` event replays them. |
| `GetPieceAt` | 0x4c9c7c | B, H | Logical coordinates, so `GEM_SIZE` stays. Stage 7c: `if (mVariant.mGravity && GravityUp() && theY >= mLayout.mClipBottom) return NULL;` (a rising gem hidden under the clip; `GravityUp()` keeps it off under `-forcevariant`, 4.16). |
| `HasMatch` | 0x4c9cf5 | B, H | `if (mVariant.mMatch) return HasMatchVariant();` |
| `GetPanForX` | 0x4c9d94 | B, L | Static becomes member. |
| `FindMove` | 0x4c9dda | B, N, H | `if (mVariant.mSearch) return FindMoveVariant(...)`. Before the swap: `if (mBoard[aRow2][aCol2] == NULL) continue;` (hole target). |
| `MouseMove`, `MouseLeave` | 0x4c9fd0, 0x4c9fe9 | H, R | `if (mPlaybackOnly) return;`, `if (mVariant.mScaled) mLayout.ToLogical(&x, &y);` and `HOVER` (optional channel). Every input handler starts with the same playback guard, because playback applies events directly, never through the handlers. |
| `StartSwap` | 0x4c9ff8 | H | After the adjacency check: `mMoveCount++` (variant boards) and `if (mVariant.mUndo) TakeUndoSnapshot();` |
| `GetBoardSize`, `GetBoardCenter` | 0x4ca13e, 0x4ca179 | B, L | Members; 406x406 and (406,219) on 8x8. |
| `ShowText`, `ColorizeText` | 0x4ca1c8, 0x4ca36e | none | |
| `StartBlackHole` | 0x4ca58d | B, H | `if (mVariant.mUndo) mUndo.Clear();` The `24.0` stays (logical). |
| `MouseDown` | 0x4ca6b0 | H, R | Top: `if (mPlaybackOnly) return;` and `ToLogical`. After the state gate: `if (mVariant.mMoves) { MouseDownVariant(x, y, theClickCount); return; }` Events: `SELECT` after the selection branch, `TARGET` at the tail, `DESELECT` on a right click. |
| `MouseUp` | 0x4ca8b7 | H | Playback guard. `if (mVariant.mMoves) MouseUpVariant(x, y);` |
| `MouseDrag` | 0x4ca8c1 | H, N, R | Playback guard, `ToLogical`. `if (mVariant.mMoves) { MouseDragVariant(x, y); return; }` The four `StartSwap` calls get NULL-neighbour guards (holes). `TARGET` event. |
| `KeyChar` | 0x4ca9d3 | H | Playback guard. `if (mVariant.mUndo && IsUndoKey(theChar)) { RequestUndo(); return; }` Other keys still toggle pause. |
| `UpdateClearing` | 0x4ca9ed | B, H | The shrink pass stays. Refill: `if (mVariant.mShape) RefillMasked(); else { original do/while }` (stage 7c: `mFall`). |
| `UpdateGameOver` | 0x4cab82 | B, L, H | `mHeight + GEM_SIZE` becomes `mLayout.mScreenBottom + GEM_SIZE` (480 unless scaled). Stage 7c, fall phase: `if (mVariant.mGravity) allGone = GameOverFallVariant(); else { original loop }`. |
| `DoHint` | 0x4cad2f | H | After the penalty lines: `if (mVariant.mSearch) { DoHintVariant(); return; }` (which also sets `mIdleTicks = 0`). |
| `SetupLevel` | 0x4cae29 | H | Tile loop: `if (mVariant.mShape) DrawVariantChecker(&g, anImageA, anImageB); else { original 10x10 loop }` (stage 7c: `mChecker`, which adds the Twilight backdrop). At the end: `if (mVariant.mAny) SetupLevelVariant();` (next level's layout into `mPendingLayout`, gold reset, gold timer). |
| `FillBoard` | 0x4cb182 | H | First line: `if (mVariant.mShape) { FillBoardVariant(); return; }` (stage 7c: `mFall`). |
| `UpdateLevelWipe` | 0x4cb284 | none | Screen-space background; `FillBoard` dispatches itself. |
| `UpdateFallOut` | 0x4cb3bf | B, L, H | One `Rand()%15` per column per tick (8 by default). `mHeight + 32` becomes `mLayout.mScreenBottom + 32`. Stage 7c, first line: `if (mVariant.mGravity) { UpdateFallOutVariant(); return; }` |
| `ShowHint` | 0x4cb475 | H, R | `if (mPlaybackOnly) return;` and `HINT` inside its `if`. |
| `Board::Board` | 0x4cb5bc | H | After the existing `memset(mBoard...)`: `InitVariant();` (rules, flags, layout, RNG, `mRngStart`). Tutorial loop: `if (mVariant.mNoTutorial) FillBoardWithMove(); else { original do/while }` |
| `UpdateBlackHole` | 0x4cb931 | B, H | After the deletion loop in the done-block: `if (mVariant.mShape) ApplyPendingLayout();`, so the loop deletes with the old bounds. |
| `Draw` | 0x4cbc70 | H | First line: `if (mVariant.mDraw) { DrawVariant(g); return; }` Nothing else changes. |
| `LoadGame` | 0x4cca98 | B, H, R | The file read becomes `mPlaybackLoad ? copy : file`. Version: `if (mVariant.mAny && aVersion == SAVEGAME_VERSION_VARIANT) { if (!ReadVariantHeader(aBuffer)) return false; } else if (aVersion != SAVEGAME_VERSION) return false;` `SAVE_SIZE` replaces `sizeof(Piece)`. States 10..13 map to 1 like 2/3. At the end: `if (mVariant.mAny) ReadVariantChunks(aBuffer);` and `LOAD` (R). |
| `SaveGame` | 0x4ccdfe | H, R | Before the file write: `if (mPlaybackOnly) { mPlaybackSave = aBuffer; return true; }` `SAVE` marker and sidecar write (R). |
| `WriteSaveGame` (port) | body of 0x4ccdfe | B, H | `SAVE_SIZE`. Variant boards write `SAVEGAME_VERSION_VARIANT` and the header after the version, then chunks at the end. |
| `SaveGameSnapshot` (port) | | H | States 10..13 map to 1. |
| `DeleteSaveGame` | 0x4cd130 | H | Also deletes the sidecar when one exists. |
| `~Board` | 0x4cd1ba | B, R | `mRecorder->OnBoardDestroyed()` (finalises the replay). |
| `ProcessMatches` | 0x4cd287 | B, H | First line: `if (mVariant.mMatch) return ProcessMatchesVariant();` Bounds: `aLines = aPass ? mRows : mCols`, `aLen = aPass ? mCols : mRows`, `j != aLen - 1`. |
| `UpdateFalling` | 0x4cd687 | B, N, H | `if (aPiece == NULL) continue;` (holes). No-moves branch: `if (mVariant.mNoMoves) OnNoMovesVariant(); else if (mTimedMode) ...` Stage 7c: the per-piece loop becomes `if (mVariant.mFall) aSettled = FallPiecesVariant(&aLandedCount, &aLandedXSum); else { original loop }`, which then hosts the hole guard, 7b's x settling, the direction and `fallAccel`; at the end of the no-match branch `if (mVariant.mGravity) OnGravitySettle();` |
| `UpdateSwap` | 0x4cd84f | H | After `mIsNewGame = false;`: `UsePendingSeed();` (stage 2, 4.10). Swap back only `if ((mState == STATE_SWAPPING) && !mRules.mMoves.mAllowInvalid)`. New `else if (mState == STATE_SWAPPING) KeepInvalidMove();` In the final `else` (swap-back done): `if (mVariant.mUndo) mUndo.PopLast();` |
| `Update` | 0x4cdabc | H, R | Top: `if (mRecorder) mRecorder->SyncExternalRand();` Drain predicate adds `\|\| IsVariantMoveState(mState)`. After the bar clamp: `if (mVariant.mGold) aTargetWidth = GoldBarTarget();` Level block: `if (mVariant.mGold) UpdateGoldGoal(); else if (mState == STATE_PLAYING) { original }` New `case`s 10..13. After the switch: `if (mRecorder) mRecorder->AfterUpdate();` (CHECK at settle). The tick-150 block is untouched. |

New states, which never occur under the original rules:
- `STATE_TWISTING = 10`
- `STATE_TWISTING_BACK = 11`
- `STATE_SLIDING = 12`
- `STATE_SLIDING_BACK = 13`

**Tick 150 and the tutorial on variant boards.**
- A board with `mNoTutorial` leaves `mTutorialCol = -1`. Its tick-150 block runs as in the
  original: the instructions dialog for the first 5 games, otherwise the GO banner.
- `mShowTutorial` becomes true, but `DrawVariant` (always used when `mNoTutorial`, since that sets
  `mDraw`) draws no arrows when `mTutorialCol < 0`.
- On a swap board, the first scoring swap dismisses the dialog through the existing `UpdateSwap`
  branch. `UpdateTwist` and `UpdateSlide` contain the same three dismissal lines.
- No edit to the tick-150 block is needed. An `mTutorialCol >= 0` guard there would have
  suppressed the GO banner, because the original shows GO only when the instructions are not
  shown.

### 2.6 Changes outside Board

| Where | Change |
|---|---|
| `WinDMApp::NewGame` (0x4d4455) | After `gGameConfig.Load()`, chooses the next board's RNG source (6.2) and stores it in `gGameConfig.mNextBoardSeed`. After `new Board`, attaches the recorder when recording. In watch mode the replay's config is used instead of `Load()`. |
| `WinDMApp::LoadingThreadProc` (0x4d82d2) | `mGemShrinkImages` has `max(7, gemColors)` rows. `EnsureGemSheetRows(n)` runs only when n exceeds the sheet's rows. `NUM_LOADING_TASKS` stays 60: the extra shrink rows are counted in the last task, so the loading bar is unchanged. Optional art (`images/flamegem`, `lightninggem`, `goldtile`, `twistcursor`, `voidtile`) is loaded without the failure `Popup`. |
| `~WinDMApp` (0x4d4b08) | Frees the stored shrink-row count instead of `NUM_GEM_COLORS`. Its port section makes the last LiveSplit block write: `active = 0`, `boardSerial = 0` (4.15). |
| `WinDMApp::UpdateFrames` (0x4d4921) | Before `SexyApp::UpdateFrames()`: `if (mWatch) mWatch->BeforeFrame();` (due events, extra updates for speed). The `TraceBoard` call moves to `BoardTrace.cpp`. After it and the test hooks (stage 2b, desktop): `if (LiveSplitObserver::Wanted()) LiveSplitObserver::AfterUpdate(this);` (4.15). |
| `WinDMApp::HandleCmdLineParam` (port) | `-livesplitlog=<file>` (4.15). |
| `WinDMApp::ShowInstructionsDialog` (0x4d59cc) | The text id comes from `mBoard->InstructionsStringId()`. That is `"INSTRUCTIONDIALOG"` unless twist or slide is active, in which case it is `_TWIST` or `_SLIDE`, falling back to the default text. With a gravity rule it is `_TWILIGHT` or `_GRAVITY_UP` (4.16). |
| `SoundManager::GetSoundInstance` (framework) | Applies the default pitch that `Board::InitVariant` sets from `look.soundPitch` (4.16). |
| `WinDMApp::ButtonDepress` (0x4d6a40) | `BUTTON_UNDO = 16`. Extras ids. While watching, side-bar presses go to the watch controller. |
| `WinDMApp::TitleScreenIsFinished` (0x4d6f28) | Creates the undo hotspot only when `undo.enabled`. With `-replay=` it enters watch mode instead of the two `NewGame` calls. |
| `WinDMApp::GetShutdownSave` (port) | Becomes `GetShutdownSaves(std::vector<BackupFile>*)`: the save snapshot plus the sidecar ending in a `SAVE(snapshot)` marker, taken at the same tick. `SexyAppBase::UpdateUserDataBackup` stages each file. |
| `OptionsDialog` ctor/Resize/ButtonDepress (0x4d1fda / 0x4d15ac / 0x4d1c58) | Only when a feature mod is on: a new `DialogButton` id 5, "Extras", in the register slot of the original locked-build three-button row (`bw = (mWidth-60)/3`): help at `mX+24`, extras at `mX+30+bw`, high scores at `mX+36+2bw`, all at `mY+166`. |
| `HighScoreWidget` ctor/Draw (0x4d07cd / 0x4cf50a) | Only for owned-RNG games or with features: a "SEED n" line and the "WATCH REPLAY" / "SUBMIT SCORE" links. A user-seeded game loads the `-seeded` table. The default screen is unchanged. |
| `main.cpp` | `-forcevariant`, `-gameseed=N`, `-replay=<file>`, `-replaydir=<dir>`, `-autoplay=<seed>`, `-tracehash=<file>`, `-dumprules`. |

---

## 3. mod.json rules schema

### 3.1 New top-level keys

| Key | Affects the score key | Purpose |
|---|---|---|
| `"rules"` | yes | Gameplay. Flattened into the same dotted-key map as `"tunables"` (a superset of it; inside one mod `rules` wins over `tunables`). Its presence makes the mod score-affecting. |
| `"features"` | no | Port features with no gameplay effect: `seeds`, `replays`, `leaderboard`, `livesplit`. |
| `"imageOps"` | no | Asset generation, such as extra gem rows. |

Optional art uses fixed VFS paths, like any mod image:
- `images/flamegem`, `images/lightninggem`: additive overlay strips
- `images/goldtile`
- `images/twistcursor`
- `images/voidtile`
- `sounds/flame.ogg`, `sounds/lightning.ogg`

Every one has a generated fallback.

### 3.2 Key reference

Defaults are the original behaviour.

| Key | Type | Default | Range and meaning |
|---|---|---|---|
| `board.cols`, `board.rows` | int | 8 | 3..16. Atomic group *shape*. |
| `board.mask` | string[] | full | `rows` strings of `cols` characters: `#` is a cell, `.` a hole, `d` dirt with one layer and `2`..`9` dirt with that many layers (4.14). Without `cols`/`rows`, they come from the mask. *shape* |
| `board.levels` | object[] | none | `[{cols, rows, mask}]`. Level L uses `levels[L % n]`. Excludes `mask` in the same mod. *shape* |
| `board.gemColors` | int | 7 | 3..12. Also a legacy tunable; the clamp rises from 7 to 12. |
| `board.gravity.direction` | `"down"` / `"up"` / `"alternate"` | `"down"` | `"up"`: every fall, refill and fill rises. `"alternate"`: Bejeweled 2's Twilight (4.16). Atomic group *gravity*. |
| `board.gravity.fallAccel` | int | 180 | 50..1000: the fall acceleration in `UpdateFalling`, in thousandths of a logical pixel per tick² (the original 0.18). *gravity* |
| `moves.type` | string or string[] | `"swap"` | Any of `"swap"`, `"twist"`, `"slide"`. Atomic group *moves*. |
| `moves.addTypes` | string[] | none | Union operator: added to the lower-priority mods' `type` (4.4). |
| `moves.twist.direction` | `"cw"` / `"both"` | `"cw"` | `"both"`: a right click rotates counter-clockwise. *moves* |
| `moves.slide.snapTicks` | int | 12 | 4..60. Fixed snap and slide-back length. *moves* |
| `moves.allowInvalid` | bool | false | Keep a move that makes no match (leaf, not part of *moves*). |
| `moves.invalidPenalty` | int | 0 | Points times the multiplier, like the hint penalty. |
| `moves.invalidCountsAsMove` | bool | true | A kept invalid move clears `mIsNewGame`. |
| `noMoves.normal`, `noMoves.timed` | `"gameOver"` / `"fallOut"` / `"continue"` | `"gameOver"` / `"fallOut"` | `"continue"` requires `allowInvalid`. With gold enabled, `normal` defaults to `"fallOut"`. |
| `match.minRun` | int | 3 | 2..5 (straight runs). |
| `match.diagonal`, `match.diagonalMinRun` | bool, int | false, = minRun | Both diagonals. |
| `match.crossings` | `"separate"` / `"merge"` | `"separate"` | `"merge"`: same-colour crossing runs form one group, classified `L`, `T` or `plus`. |
| `match.shapes.<id>` | object or null | none | Keyed: `{"preset": "square"\|"L3"\|"T4"}` or `{"cells": [[x,y],...], "rotations": bool, "mirror": bool}`. |
| `match.bonus.<pattern>` | int | 0 | Base points added before the multiplier. Patterns: `diagonal`, `L`, `T`, `plus` or a shape id. |
| `specials.flame` | object or null | null | `{"createdBy": [patterns], "radius": 1..3, "points": int}`. Radius 1 is 3x3. Keyed. |
| `specials.lightning` | object or null | null | `{"createdBy": [patterns], "points": int}`: clears the row and the column. Keyed. |
| `specials.chain`, `specials.createIn` | bool, `"any"` / `"move"` | true, `"any"` | Blasts detonate caught specials. With `"move"`, specials form only in the move's own resolve. |
| `gold.enabled` | bool | false | Atomic group *gold*. |
| `gold.gild` | `"matches"` / `"matchesAndBlasts"` | `"matchesAndBlasts"` | |
| `gold.timeLimit.normal`, `.timed` | null or `{base, perLevel, min}` in seconds | null / `{180, -10, 60}` | Per mode. |
| `gold.timeBonus` | int | 10 | Points per second left, times the multiplier. |
| `undo.enabled`, `.depth`, `.max`, `.penalty`, `.inTimed`, `.restoreTime`, `.restoreRng` | | false, 1, -1, 0, false, false, true | Atomic group *undo*. depth 1..100; max per game, -1 = unlimited. |
| `look.backdrop` | `"normal"` / `"twilight"` | `"normal"` | `"twilight"`: the level backdrop mirrored left to right at half brightness (4.16). Presentation: never makes a variant board, but as a `rules` key it is in the RulesId and the score key (Q27). |
| `look.soundPitch` | int | 0 | -12..12 semitones for every sound effect (4.16). Presentation. |
| `scoring.*`, `timing.*` | | as in `GameConfig.h` | Legacy tunables. They are also accepted under `rules`. |
| `features.seeds` | `true` or `{entry, perGame}` | off | `entry`: seed field in Extras. `perGame`: every game gets a shown, replayable seed. `true` turns on both; the object form only the members it sets to `true`. |
| `features.replays` | `true` or `{keep, hover}` | off | `keep` (default 20) recent replays are kept automatically. `hover`: record the hover channel. |
| `features.leaderboard` | `true` or `{ranked: "ask"\|"always"}` | off | Implies `replays` and `seeds.perGame`. The endpoint is always same-origin `/api`, never set by a mod. |
| `features.livesplit` | `true` | off | Desktop builds fill the LiveSplit block that the published auto splitter reads (4.15). No options yet; the web build warns and ignores it. |

**Pattern names** (used by `bonus` and `createdBy`):
- `runN`: a straight run of length N or more. Only `run4` and longer are meaningful as triggers.
- `diagonal`
- `L`, `T`, `plus` (from `crossings: "merge"`)
- any shape id

When a group qualifies for both kinds of special, lightning is checked first.

### 3.3 One example per feature

```jsonc
// 1. Allow invalid moves
{ "id": "free-swaps", "name": "Free Swaps", "version": "1.0",
  "rules": { "moves": { "allowInvalid": true, "invalidPenalty": 0 }, "noMoves": { "normal": "continue" } } }

// 2. More gem colours (ship images/gemsheet6 with 9 rows of 104 px, or let imageOps generate them)
{ "id": "nine-gems", "version": "1.0",
  "rules": { "board": { "gemColors": 9 } },
  "imageOps": [ { "target": "images/gemsheet6", "op": "extendRows", "rowHeight": 104, "rows": 9,
                  "generate": [ { "from": 1, "hue": 35 }, { "from": 6, "tint": [255, 120, 200] } ] } ] }

// 3. Board size and shape
{ "id": "wide-board", "version": "1.0", "rules": { "board": { "cols": 10, "rows": 7 } } }
{ "id": "diamond", "version": "1.0", "rules": { "board": { "mask": [
    "...###...", "..#####..", ".#######.", "#########", "####.####",
    "#########", ".#######.", "..#####..", "...###..." ] } } }

// 4. Chuzzle-style slides
{ "id": "chuzzle-slides", "version": "1.0", "rules": { "moves": { "type": "slide" } } }

// 5. Twist-style rotation
{ "id": "twist", "version": "1.0", "rules": { "moves": { "type": "twist", "twist": { "direction": "both" } } } }

// 6. Custom match patterns
{ "id": "shapes", "version": "1.0",
  "rules": { "match": { "diagonal": true, "crossings": "merge",
      "shapes": { "square": { "preset": "square" },
                  "L3":     { "cells": [[0,0],[0,1],[1,1]], "rotations": true, "mirror": true } },
      "bonus": { "L": 50, "T": 80, "plus": 120, "square": 30, "L3": 10, "diagonal": 20 } } } }

// 7. Flame and lightning gems (optional art: images/flamegem.png, images/lightninggem.png)
{ "id": "power-gems", "version": "1.0",
  "rules": { "specials": { "flame":     { "createdBy": ["run4"], "radius": 1, "points": 20 },
                           "lightning": { "createdBy": ["run5", "T", "L"], "points": 30 } } } }

// 8. Gold board (optional art: images/goldtile.png)
{ "id": "gold-quest", "version": "1.0",
  "rules": { "gold": { "enabled": true,
                       "timeLimit": { "normal": null, "timed": { "base": 180, "perLevel": -10, "min": 60 } } } } }

// 9. Set seed (-gameseed=N and ?seed=N work without this mod; it adds the Extras seed field
//    and a shown seed for every game)
{ "id": "seeds", "version": "1.0", "features": { "seeds": { "entry": true, "perGame": true } } }

// 10. Replays
{ "id": "replays", "version": "1.0", "features": { "replays": { "keep": 20, "hover": true } } }

// 11. Undo
{ "id": "undo", "version": "1.0", "rules": { "undo": { "enabled": true, "depth": 3, "penalty": 25 } } }

// 12. Online leaderboard (web; implies replays and per-game seeds)
{ "id": "online", "version": "1.0", "features": { "leaderboard": { "ranked": "ask" } } }
```

### 3.4 Combined example

One mod, or several mods with the same effect:

```jsonc
{ "id": "twisted-quest", "name": "Twisted Quest", "version": "1.0",
  "rules": {
    "board":    { "gemColors": 8, "levels": [
                  { "cols": 8, "rows": 8 },
                  { "mask": ["..#####..", ".#######.", "#########", "#########", "####.####",
                             "#########", "#########", ".#######.", "..#####.."] } ] },
    "moves":    { "type": ["swap", "twist"] },
    "match":    { "crossings": "merge", "bonus": { "L": 40, "T": 60 } },
    "specials": { "flame": { "createdBy": ["run4"] }, "lightning": { "createdBy": ["T", "run5"] } },
    "gold":     { "enabled": true },
    "undo":     { "enabled": true, "depth": 1, "penalty": 50 } },
  "imageOps": [ { "target": "images/gemsheet6", "op": "extendRows", "rowHeight": 104, "rows": 8,
                  "generate": [ { "from": 4, "hue": 60 } ] } ],
  "features": { "replays": true, "seeds": true } }
```

As separate mods (`diamond`, `twist`, `power-gems`, `gold-quest`, `undo`), the rules land in
different groups and combine without warnings.

### 3.5 Merge rules

`ModManager::Apply` already visits enabled mods lowest priority first and flattens `tunables` into
dotted leaves. `rules` goes into the same map, with these additions:

1. **Leaves.** Higher priority wins. Arrays are replaced as a whole.
2. **Atomic groups**: *shape* (`board.cols/rows/mask/levels`), *moves* (`type`, `twist.*`,
   `slide.*`), *gold*, *undo*, *gravity* (`board.gravity.*`, 4.16). The highest-priority mod that
   sets any key of a group supplies the whole group. Keys it leaves out take their defaults, never
   another mod's values. So one mod's mask never meets another mod's row count.
3. **Keyed objects**: `match.shapes.<id>`, `match.bonus.<id>`, `specials.flame`,
   `specials.lightning`. These merge per id.
4. **`null` resets.** `"specials": { "lightning": null }` or `"gold": null` returns a key or
   group to its default, which removes another mod's rule.
5. **Union.** `moves.addTypes` is added, in the fixed order swap, twist, slide, to the `type` the
   *moves* group resolved to. A "slides too" mod therefore combines with a Twist mod.
6. **Provenance.** A `key → mod id` map records who set what. A conflict (two mods, same leaf,
   different values) gives a warning such as
   `"twist: moves.type overrides chuzzle-slides"`.
7. **Validation** (3.6):
   - **Per mod.** A mod that fails type or range checks has its whole `rules` ignored, with an
     error.
   - **Merged set.** If a cross-field check fails, the lowest-priority mod contributing to the
     failing group is dropped and the merge runs again. This is deterministic, with at most one
     iteration per mod.
8. **Status.** `<userdir>/mods-status.json` gets:

   ```
   { rulesId, canonical, warnings[], errors[], dropped[] }
   ```

   It is written only when an enabled mod has `rules` or `features`, so no-mod and legacy user
   dirs stay byte-identical. `web/shell.html` reads it and shows the warnings "as of the last
   start", because mods only apply after Apply and restart. `scanMods` computes the gameplay tag
   as `affectsScores || tunables || rules` and adds a "Features" tag for `features`.

| Combination | Result |
|---|---|
| diamond (shape) + twist (moves) + power-gems (specials) | Different groups, so they combine. |
| twist (higher) + chuzzle-slides (lower) | twist supplies *moves*, with a warning. Add `"addTypes": ["slide"]` to combine them. |
| free-swaps + chuzzle-slides | `allowInvalid` is a leaf outside *moves*: slides that make no match are kept. |
| `minRun: 2` (A, higher) + `diagonal` (B, lower), `gemColors: 4` | The leaf merge fails the colour minimum (5). B, the lower contributor to *match*, is dropped and the merge runs again. |

### 3.6 Validation (`GameRules::Validate`)

| Check | On failure |
|---|---|
| `cols`/`rows` 3..16. Each mask has `rows` strings of `cols` characters from `#.d23456789`. At least 9 cells. The same for every `levels[i]`. `mask` and `levels` not both in one mod. | Reject the mod's rules. |
| Some line has at least `minRun` consecutive cells. `twist` needs at least one full 2x2 of cells; `slide` at least one segment of length 2 or more. | Reject. |
| `gemColors` is at least the fill minimum. Brute force in fill order (rows bottom-up, columns left to right, cells only): for each cell, count the pattern instances, runs included, whose other cells are already filled. The minimum is 1 + the maximum count. Examples: orthogonal runs give 3; `minRun: 2` with diagonals gives 5; squares give 4. With a gravity rule, the larger result over the fill orders in use (4.16). | Reject (in the merged set: drop the lowest contributor). The message names the minimum. |
| `gemColors` exceeds the rows in the gem sheet. | Not an error: rows are generated (4.2). |
| `createdBy` names a pattern that cannot occur under the merged `match` rules. Flame and lightning triggers identical. | Reject. |
| `noMoves: "continue"` without `allowInvalid`. | Reject. |
| `gold.timeLimit` values ≥ 10 s; `undo.depth` 1..100; `slide.snapTicks` 4..60; `board.gravity.fallAccel` 50..1000; `look.soundPitch` -12..12. | Clamp and warn. |
| Unknown keys under `rules` or `features`. | Warn ("did you mean ..."), then ignore. |

As a backstop for combinations the static check cannot predict, `FillBoardVariant` caps rerolls:
after 1000 rerolls the board restarts, still drawing from the board RNG, so it stays
deterministic.

### 3.7 Score keys, RulesId

- **Local file key.** It is unchanged: FNV-1a over the sorted `id@version;` of the score-affecting
  mods.
  - Score-affecting now means `affectsScores || tunables || rules`.
  - A mod is also score-affecting if `data/diff.txt` resolves into it (a file in the mod root, or
    a `"files"` remap). Today such a mod changes gameplay without changing the key.
  - `example-relaxed` keeps `savegame-58ebd9d7.dat`, so `playtest_mods` stays byte-identical.
- **User-seeded games** (a seed the player chose) use `hiscores-<key>-seeded.dat`, or
  `hiscores-seeded.dat` without a key, so practice on a known board does not mix with the normal
  table (open question Q2). Per-game random seeds keep the normal table.
- **RulesId** = SHA-256 of `GameRules::Canonical()`:
  - a u16 `rulesVersion`;
  - then, in schema-table order, each field whose value differs from the default, as
    `(u16 fieldId, u8 type, value)`;
  - ints as little-endian i32, doubles as their 8 IEEE bytes, bools as u8, strings and masks
    length-prefixed;
  - keyed objects in sorted key order;
  - finally the 12 `diff.txt` values as loaded, always included.

  Consequences:
  - Adding a new field with a default does not change existing ids.
  - Two mods with the same effective rules share a leaderboard board.
  - No text formatting of numbers takes part.

  `-dumprules` prints the canonical form as text, and the RulesId.

---

## 4. Per-feature design

### 4.1 Allow invalid moves

- **Rule:** `moves.allowInvalid`, `invalidPenalty`, `invalidCountsAsMove`, `noMoves.*`.
- **Code:** `UpdateSwap` (0x4cd84f) gets the condition and branch from 2.5.
  `KeepInvalidMove()` does the following:
  - sets `mSwapPiece1/2 = NULL`;
  - applies the penalty with `DoHint`'s formula
    (`mScore = (int)((double)mScore - mPointMultiplier * penalty)`, floored at 0);
  - if `invalidCountsAsMove`, sets `mIsNewGame = false`, so the board change makes the game "in
    progress" and it is saved on exit;
  - increments `mInvalidMoves`;
  - plays `SOUND_BAD` as feedback;
  - sets `mState = STATE_FALLING`.

  On the next tick `UpdateFalling` settles at once (all pieces are at rest). `ProcessMatches`
  finds nothing, the stats update is harmless, then `FindMove` gives `STATE_PLAYING` or the
  no-moves branch. The original no-moves logic is therefore reused unchanged.
- **Other move types:** `UpdateTwist` and `UpdateSlide` call the same function instead of
  animating back.
- **No moves:** with `noMoves.<mode> = "continue"`, `OnNoMovesVariant` shows the banner
  `NO_MATCHES_LEFT` (string id with a default) and returns to `STATE_PLAYING`. The player can
  still make invalid moves. `"fallOut"` reuses `STATE_FALL_OUT`.
- **Save:** v2 with a `MOVE` chunk.
- **Replay:** no new events.

### 4.2 More gem colours

- **Rule:** `board.gemColors` up to 12. `FillBoard` and the refill already draw
  `Rand() % gGameConfig.mGemColors`, so the logic stays on the original paths: this is a tunable,
  not a variant.
- **Behaviour change:** an existing mod that sets 8..12 was clamped to 7 before and now gets its
  value. No shipped mod does this.
- **Art:**
  - `Board::Draw` selects the gem with `mColor * 104`, so a mod's taller `images/gemsheet6` works
    as it is.
  - Otherwise `imageOps.extendRows` grows the canvas of the `LoadedImage`, both `mBits` and
    `mHDBits` at the HD scale, so `example-crystal-gems` stays sharp. It fills new rows by
    copying a base row's normal and glow sub-rows with a hue rotation, saturation, lightness or
    tint.
  - With no op given, `WinDMApp::EnsureGemSheetRows(n)` registers a default op: hue steps from the
    white gem (row 1), chosen to stay distinct.
  - Generated rows get a small corner glyph (circle, square, star, ...) by default
    (`"glyph": false` turns it off), so colour-blind players can tell the extra colours apart.
- **Shrink images and tints:**
  - `mGemShrinkImages` is built for `max(7, n)` colours.
  - `gGemTints[MAX_GEM_COLORS]` is computed at load as the average opaque colour of each row. It
    is used by effects that tint by gem.
  - Watching a replay whose rules need more colours than are loaded calls
    `EnsureGemColors(n)` lazily.
- **Save, UI:** no change. The colour is already in the Piece record.

### 4.3 Board size and shapes

Section 5 has the details.

- **Rule:** `board.cols/rows/mask/levels`.
- **Code:**
  - bounds edits;
  - `FillBoardVariant`, `RefillMasked`;
  - NULL guards in `FindMove`, `MouseDrag` and `UpdateFalling`;
  - `DrawVariant` and `DrawVariantChecker`;
  - the per-level shape switch in `SetupLevelVariant` (pending) and `UpdateBlackHole`
    (applied after deletion).
- **Unchanged:** `ProcessMatches` and `HasMatch` already handle NULL cells, since a NULL resets the
  run. With standard matching they need only bounds edits.
- **Save:** v2 header with dims and level-shape index *before* the cell records (7.2).

### 4.4 Chuzzle-style slides and move-type combinations

**Slides (`STATE_SLIDING`/`_BACK`).**
- **Input** (`BoardMoves.cpp`, logical coordinates):
  1. `MouseDownVariant` remembers the pressed piece's cell and the press point.
  2. `MouseDragVariant` locks the axis once the drag passes 16 px, with ties going vertical as in
     `MouseDrag`. It then offsets the line for drawing only; this is not simulation state.
  3. `MouseUpVariant` computes `k = round(offset / 51) mod len`. A non-zero `k` issues
     `SLIDE(axis, line, segStart, k, releaseOffset)`. Zero snaps back with no event.
- **The line** is the maximal contiguous segment of playable cells containing the pressed cell.
  Wrap-around stays inside the segment, and holes split a row.
- **Simulation:**
  1. `StartSlide` animates from the release offset to `k*51` over exactly `snapTicks` ticks,
     with integer interpolation. The duration depends only on the rules, never on the offset, so
     the offset is cosmetic.
  2. Then it rotates the pointers along the segment, updates `mCol/mRow/mX/mY`, and runs
     `mCascadeCount = 1; mMoveMatchCount = 0; ProcessMatches()`.
  3. With no match, it either goes to `STATE_SLIDING_BACK` (another `snapTicks`) with
     `SOUND_BAD`, or calls `KeepInvalidMove()`.
  4. On a match: `mIsNewGame = false`, and the tutorial dismissal lines run as in `UpdateSwap`.
- **Draw:** the segment's pieces at their offsets, plus the wrapped copy at the other end, clipped
  to the segment's rectangle.
- **Save:** the grid is committed only at the end of the forward animation, so a save during a
  slide stores the grid before the move with state 1.

**Input when several types are active** (`moves.type` is a set):

| Types | Click | Drag (16 px or more) | Right click |
|---|---|---|---|
| swap | select / target (original) | drag-swap (original) | deselect |
| swap + twist | within 13 px (P/4) of an intersection: twist; elsewhere select/target | drag-swap | CCW twist near an intersection with `"both"`, else deselect |
| swap + slide | select / target | slide (drag-swap off) | deselect |
| twist + slide | twist (nearest intersection) | slide | CCW twist with `"both"` |
| all three | corner: twist; centre: select/target | slide | as for swap + twist |

The search order is fixed: swaps (original order), then twists, then slides (4.7).

### 4.5 Twist-style rotation (`STATE_TWISTING`/`_BACK`)

- **Input:**
  - The intersection index is `ic = clamp((lx - mOriginX + 26) / 51, 1, cols-1)`, and the same
    for `ir`. The anchor (top-left cell) is `(ic-1, ir-1)`.
  - The twist is valid when all four cells are playable and non-NULL.
  - It issues `TWIST(c, r, dir)`.
- **Animation:**
  - 18 ticks at 5° per tick, the swap's pace.
  - Positions are `centre + R(θ)·offset`, using a baked integer table of `cos/sin × 1024` with C
    truncation. Positions are therefore bit-exact on every platform and can enter even the
    cross-platform hash.
- **On completion:**
  1. Rotate clockwise on screen (TL→TR→BR→BL) and snap.
  2. Run `mCascadeCount = 1; mMoveMatchCount = 0; ProcessMatches()`.
  3. On a match: the same bookkeeping as `UpdateSwap`'s success branch (state 4,
     `mIsNewGame = false`, tutorial dismissal).
  4. Otherwise: rotate back with `SOUND_BAD`, or call `KeepInvalidMove()`.
- **Draw:** a ring at the hovered intersection: `images/twistcursor`, or the four corner quarters
  of `target1`, scaled.
- **Hint:** sparkle the rotated gem that lands in a group.
- **Save:** state 1, with the grid from before the move.

### 4.6 Custom match patterns

- **Group order.** `MatchFinder::FindGroups(ColorGrid, MatchRules)` emits groups in this order:
  1. vertical runs, columns 0..C-1, top to bottom (the original pass 0);
  2. horizontal runs, rows 0..R-1 (pass 1);
  3. diagonals `\` then `/`;
  4. pattern instances: shape ids in sorted order, anchors row-major, orientations in a fixed
     order. An instance is skipped when its cells are a subset of one group already found.
- **`crossings: "merge"`** unions same-colour runs that share a cell into one group. The group
  sits at the first constituent's position and is classified `L`, `T` or `plus` by geometry.
- **Default equivalence.** With the default rules, steps 1 and 2 are exactly the original run
  order. The difftest and `-forcevariant` prove it.
- **`ProcessMatchesVariant`** builds the grid from `mBoard` and runs the finder. For each group,
  `ScoreGroup()` is a faithful copy of 0x4cd287's scoring block:
  - `SetValue(++mMoveMatchCount)`, the length bonuses for runs, then `+ bonus[pattern]` before
    the multiplier;
  - the popup at the average position `+24`/`+16` (logical);
  - `mGemsCleared`, EXCELLENT, the move-score timer clamp.

  It then marks pieces dying (except special creations), and plays the original
  gotset/combo/multishot sounds with the original pan rule.
- **`HasMatchVariant`** is `AnyGroup`, used by `FillBoardVariant`. For `minRun` alone, the same
  variant path is used: no edit to the translated `== 3` lines.
- **Search:** see 4.7.
- **UI:** optional banners `PATTERN_<name>` through `ShowText`, only when the string is defined.
- **Save:** none beyond v2.

### 4.7 Hint and no-moves search (`FindMoveVariant`)

- It keeps `FindMove`'s contract `{col,row,col2,row2}` and adds the move type.
- **Swaps:** candidates in the original order (row, col, then directions (1,0), (-1,0), (0,1),
  (0,-1)), skipping holes and NULL targets. Each candidate is applied to a scratch `ColorGrid`.
  It counts when some group contains the moved-in cell. That generalises "only the gem swapped
  into (aCol, aRow) is measured", keeps the original's double finding of each swap, and returns
  the same n-th move at default rules.
- **Twists:** anchors row-major, CW then CCW (with `"both"`), counting groups through any of the
  four cells.
- **Slides:** rows then columns, segments in order, `k = 1..len-1`.
- **Cost:** at most about 1,500 candidates per settle on 16x16, each a local scan. That is
  negligible, and it runs only at settle and on hints.
- **Callers:**
  - the no-moves check in `UpdateFalling` (unchanged call, which dispatches);
  - `DoHintVariant`: the sparkle and spin go to the moved gem that lands in a group;
  - `TraceBoard` (variant move lines with click/drag coordinates, for writing `-script`
    playtests).

### 4.8 Flame and lightning gems

- **Data:** `Piece::mKind` and `mKindFrame` (2.4).
- **Creation** (in `ProcessMatchesVariant`):
  - Triggers are checked lightning first, then flame. For a group matching a trigger, one piece
    stays alive (`mDying = false`) and gets `mKind`.
  - That piece is the gem the player moved, if it is in the group (`mSwapPiece1/2`, the slid
    segment, the four twisted cells). Otherwise it is the group's middle cell in scan order.
  - Creation wins over dying when a cell is in two groups.
  - `createIn: "move"` limits creation to the resolve right after the move.
  - The group still scores normally.
- **Detonation:**
  - A special piece marked dying is pushed onto a FIFO.
  - A flame marks every playable cell within Chebyshev radius r. A lightning gem marks its whole
    row and column, playable cells only.
  - Specials newly hit are queued when `chain` is set. Specials created in the same pass are
    exempt.
  - Each blast is one scoring group: `points × destroyed × mPointMultiplier`. It counts toward
    `mMoveScore` and EXCELLENT, and adds to `mGemsCleared`, but does not advance
    `mMoveMatchCount`. Its popup sits at the blast centre.
  - Everything resolves within the tick, so `UpdateClearing` just shrinks everything dying. No new
    state is needed.
- **Art:**
  - Overlays come from `images/flamegem` / `images/lightninggem` (additive strips).
  - Flame fallback: the `sparkle` sheet colourised with `gGemTints[color]`, pulsing.
  - Lightning fallback: the `elect` strip (level-wipe art) clipped inside the gem, and drawn along
    the row and column as the beam.
  - Blasts are `BoardEffect`s in `mEffects`, which are cosmetic and unsaved, like popups.
  - The view never calls `Rand()`. Drawing already runs under `MTAutoDisallowRand` in
    screenshots, and effects use their own deterministic counters.
- **Sounds:** `sounds/flame`, `sounds/lightning` if present, else `SOUND_EXPLODE` /
  `SOUND_FIRECRACKLE`.
- **Save:** a `KIND` chunk with one byte per present cell.

### 4.9 Jewel Quest gold board

- **Data:** `mGold[r][c]` (per cell, so it does not move with gems), `mGoldLeft`,
  `mGoldTicksLeft`, `mGoldTickLimit`. Gold forces `mVariant.mMatch`.
- **Gilding:** cells of scored groups turn gold in `ProcessMatchesVariant`. With
  `"matchesAndBlasts"`, blast cells do too. Each gilding adds a shine `BoardEffect`.
- **Goal:** `Update`'s level block calls `UpdateGoldGoal()`, which:
  - counts down `mGoldTicksLeft` in the same states as the timed drain (PLAYING, SWAPPING, and the
    variant move states), only once `mTimeDrainDelay` is 0, so the GO period is free;
  - in `STATE_PLAYING` with `mGoldLeft == 0`: adds `timeBonus × secondsLeft × multiplier`, then
    calls `StartBlackHole()` (vortex → `SetupLevel` → wipe → `FillBoard`, all original);
  - when time runs out: `ShowText(GetString("TIME_UP"), 400)` + `GameOver(200)`, as the original
    time-up does.

  The original score-based level end and time-up checks are skipped in gold mode, and the
  timed-mode score drain has no effect there. Points still accumulate for high scores.
- **Modes:** the original Normal/Timed buttons map to `gold.timeLimit.normal` (null by default:
  untimed) and `gold.timeLimit.timed`.
- **Bar:** `GoldBarTarget()` shows gold progress when untimed, and time left when timed. The
  original timed colouring and warning code keep working on that width.
- **Level setup:** `SetupLevelVariant()` resets gold, sets the limit to
  `max(min, base + perLevel × level)` seconds times 100, and picks the next level's shape into
  `mPendingLayout`.
- **No moves:** `noMoves.normal` defaults to `"fallOut"` under gold, so the board reshuffles
  through `STATE_FALL_OUT` and `FillBoard`. The gold layer persists.
- **Art:**
  - The gold tile is `images/goldtile`, or the level's checker tile through
    `CreateColorizedImage(..., Color(235,185,45))`, built once per level. It is drawn between the
    background and the gems.
  - HUD: `GOLD 23/64` in mainfont, centred over the bar.
- **Save:** `GOLD` (bitmap) and `GTIM` (ticks left, limit). `LoadGame` never calls `SetupLevel`,
  so gold state comes only from these chunks.

### 4.10 Set seed

The RNG model is in 6.2.

**Seed sources:**
- `-gameseed=N` on native (main.cpp sets `GameConfig::mPendingSeed`);
- `?seed=N` on the web (the shell passes `-gameseed=N`; digits only);
- the Extras dialog field (`features.seeds.entry`).

**When the pending seed is used** (the user's decision of 2026-10-18: a chosen seed waits until the
player actually starts playing, so switching to Timed straight away still plays it):
- `NewGame` gives the pending seed to every board it creates (`mTakesPendingSeed`).
- The seed is used up (`Board::UsePendingSeed`) when a game on it is played:
  - at the board's first swap that makes a match: `UpdateSwap`, where `mIsNewGame` is cleared, the
    same moment that decides whether quitting saves the game;
  - or at its game over: `GameOver`, the one entry to `STATE_GAME_OVER` (no moves in Normal mode,
    time up). A timed game can run out without any move.
- New Game is no game over: `RequestNewGame` clears `mTakesPendingSeed` before its `GameOver(50)`.
- Until then every board that replaces the untouched one gets the seed again: the second board of
  the double `NewGame` in `TitleScreenIsFinished`, a mode switch (which needs no confirmation while
  `mIsNewGame` is set), and New Game. A swap that makes no match does not count.
- A board that resumes a save never uses the seed up (it plays unseeded until stage 3, GAME_NOTES
  12.3), so the next new game gets it.
- A timed fall-out is not a game over: the game goes on with a refilled board. It also cannot come
  before the first match, because the constructor's board always has a move, and only a match's
  refill can leave none.
- Later paths that clear `mIsNewGame` (a kept invalid move, 4.1; twist and slide matches, 4.4,
  4.5) call `UsePendingSeed()` there too.
- Once the seed is used up, later games get per-game seeds (with `perGame`), or the original
  stream (without it).
- Oracle cases: `seed_timed`, `seed_match_timed`, `seed_nomatch_timed`, `seed_newgame` (with
  `seed_start` and `seed_resume`).

**Display:**
- For every owned-RNG game, `HighScoreWidget::Draw` shows "SEED n" (mainfont, about y = 120,
  between the stats at 105 and the tip at 136; to be checked on a screenshot), and the replay
  list shows it too.
- Unseeded original-rules games show nothing, so the default screen stays pixel-identical.
- `MTRand::SRand(0)` maps to 4357, so seed entry rejects 0.

**Save:**
- Variant boards keep the seed and the full RNG state in the v2 `RNGS` chunk.
- Default-rules owned-RNG games keep them in the sidecar (7.3).
- A resumed seeded game continues its stream, which also stops "quit and resume to reroll".

### 4.11 Save and watch replays

Format, recording and playback are in section 6.

- **Watching:** from the replay browser, the game-over screen link, `-replay=<file>`, or
  `?replay=<id>`.
- **The watch session:**
  1. Asks to leave a game in progress. The game is saved by `~Board` as on a mode switch.
  2. Saves `mTimedMode`, `mShowInstructions`, `mTimesPlayed`, `gGameConfig` and the 12 diff
     fields.
  3. Sets them from the replay header and creates a playback board (`mPlaybackOnly`, owned RNG
     from the header).
  4. At the end, shows "REPLAY END" (Watch again / Back) instead of the high-score screen.
  5. Restores everything, then calls `NewGame(); ConfirmLoadGame();`, which offers to resume the
     saved game.

  Because `IsGameInProgress()` is false for a playback board, it never saves, counts or deletes
  anything.
- **Speeds:** 1x, 2x, 4x, 8x (extra `Update()` calls per frame), pause, step to the next move, exit.
- **Storage:** native `<userdir>/replays/`; web `/persist/replays` via `-replaydir`.
  - Finished replays are written at game end (no-move games are dropped) as
    `<date>-<mode>-<score>.bjr`.
  - The newest `keep` are kept, plus every replay that made a local high-score table or that the
    user marked Keep.

### 4.12 Undo

- **Rule:** the `undo.*` group. It is a rule, so the game gets its own score table and an owned
  RNG.
- **Snapshot:** `TakeUndoSnapshot()` runs in `StartSwap` after the adjacency check, and in
  `StartTwist`/`StartSlide`, only in `STATE_PLAYING`. It writes `StateCodec`'s UNDO profile
  (6.7).
  - It is popped when the move is taken back by the game: swap-back completion in `UpdateSwap`'s
    final `else`, and `_BACK` completion.
  - The stack is cleared in `StartBlackHole`, so undo never crosses a level.
- **`UNDO`:**
  - **Allowed when:** `STATE_PLAYING`, `mPauseCount == 0`, the stack is non-empty, `max` is not
    reached, and (in timed mode) `inTimed`.
  - **What it restores:** the snapshot: grid (colour, kind, at rest), gold layer, score fields,
    counters. Also the RNG with `restoreRng` (default), so repeating the move gives the same
    refill and undo cannot be used to reroll. The drain and gold timer come back only with
    `restoreTime`.
  - **What it clears:** the selection, the popups and the effects.
  - **What it keeps:** `mUpdateCnt`, the pause counts, the play time and `mUndosUsed`.
  - **Penalty:** `penalty × multiplier`.
- **UI:** `KeyChar` handles `z`, `u` and Backspace before "any key pauses". The decorative red orb
  left of the timer bar (about x 193..226, y 440..472) becomes a hotspot with a glow when undo is
  available, created in `TitleScreenIsFinished`. To be checked on a screenshot.
- **Save:** the stack is not saved; it is empty after a load.

### 4.13 Online leaderboards

The Worker side is in 6.10.

- **Game side:**
  1. `features.leaderboard` implies replays and per-game seeds.
  2. The shell prefetches a ranked session (seed + token) for the current `(RulesId, mode)` at
     start and after each use.
  3. With `ranked: "always"`, or `"ask"` answered yes, `NewGame` takes the prefetched seed if one
     is ready (ranked game). Otherwise the game is unranked. Network delay never blocks
     `NewGame`.
  4. At game over, an eligible game shows "SUBMIT SCORE" in `HighScoreWidget`. The name defaults
     to `mLastScoreName`. `EM_JS bj_submit(path)` hands the replay to the shell, which POSTs it.
- **Native:** no HTTP client. Exported `.bjr` files can be uploaded from the web page (Q10).

### 4.14 Dirt tiles and diagonal falling (Jewel Quest)

Added from the user's answer to Q6 (2026-10-18): gems fall through holes, and boards can also have
**dirt tiles** that block falling gems, while gems still slide past them diagonally.

- **Rule:** the mask legend gains dirt: `d` is a dirt tile with one layer, `2`..`9` one with that many
  layers. Dirt counts as a playable cell for the "at least 9 cells" check. It works in `board.mask`
  and in every `board.levels[i].mask`. Dirt is part of the *shape* group (3.5), so the score key
  follows.
- **A dirt cell** holds no gem. It cannot be selected, swapped, slid, rotated or matched, and the
  hint search (`FindMoveVariant`) skips it. It stops vertical falls: a gem above it rests on it.
- **Clearing:** each scored match group removes one layer from every dirt cell orthogonally next to
  any of its gems. A dirt cell next to two groups in the same resolution step loses two layers. With
  special gems, cells in a blast lose one layer too. When the last layer goes, the cell becomes an
  empty playable cell that gravity fills, with a dust `BoardEffect`. Dirt removal scores
  `dirtPoints × multiplier` (default 0).
- **Gravity (`RefillDiagonal`, replaces `RefillMasked` when the board has dirt):**
  1. *Vertical pass:* `RefillMasked`, with dirt cells as floors. Each column splits into segments
     between dirt cells. Only the top segment is refilled from the spawn above row 0; lower segments
     are filled only by diagonal slides. A column whose top cell is dirt spawns nothing.
  2. *Diagonal pass:* scan rows from the bottom up, columns left to right. For each empty playable
     cell E that no vertical fall can reach (the cell straight above is dirt or a hole with nothing
     falling through it), the candidates are the resting gems at up-left and up-right. A gem is
     resting when the cell below it is occupied or dirt. If both sides have one, the board-level
     toggle `mDiagonalLeftFirst` picks the side. It starts true, flips after every diagonal slide,
     and is saved and hashed, so replays stay exact. The chosen gem moves into E with a diagonal
     animation (`mVelX`, variant-only). Then go back to step 1, because the slide opened a cell
     above.
  3. Repeat until nothing moves. `UpdateFalling` settles x and y independently.

  On a board without dirt, `RefillDiagonal` is never used, so the vertical pass stays the original
  refill, line for line.
- **Fill (`FillBoardVariant`):** dirt cells are skipped like holes. The reroll-until-no-match rule
  treats dirt as a run breaker.
- **Gold mode (4.9):** a dirt cell cannot be gilded until its last layer is gone. By default
  `gold.countDirt: true`, so dirt cells count toward the cells left. Revealing a cell does not gild
  it; a later match there does.
- **Save:** `DIRT` chunk (layers per cell, u8) plus `mDiagonalLeftFirst` in the v2 header.
- **Art:** `images/dirttile` from a mod if present. Otherwise a generated overlay: the level's
  checker tile darkened and tinted brown, with one crack line per layer, built once per level. It is
  drawn above the checker and below the gems.
- **Tests:** fixture boards with dirt mid-column (the diagonal fill alternates left and right),
  dirt in the top row, multi-layer dirt, two groups next to one dirt cell, dirt + holes + gold, a
  save mid-cascade, and replay verification.

### 4.15 Auto LiveSplit (`features.livesplit`)

Added from the user's decision (2026-10-18): "Use a published autosplitter script (ASL) reading a tagged
memory block, so it works through LiveSplit's normal Activate flow." Desktop builds expose a read-only
block; a published ASL script finds it and starts, splits and resets LiveSplit's timer. Publishing, the
licence, contacting HyperNervie and the registry PR are steps for the user to approve later.

**Boards** ([bejeweled1](https://www.speedrun.com/bejeweled1), as of 2026-10-18). It is the only
speedrun.com entry this port can run; the other five Bejeweled entries are other games.
- Real time only (no game time, no load removal), video required, emulators not allowed, platforms PC,
  Xbox and Mac, no individual levels, no game-level rules text, no autosplitter. 118 verified runs: 116 PC,
  1 Xbox, 1 without a platform.
- Moderators retime from video: from the first frame GO shows to the gems starting to move (or the first
  frame of the vortex). One run can serve several boards: a 20-Levels run also gave a 100K time.

| Category | Timing | Subcategories |
|---|---|---|
| Normal | From GO until the wormhole animation starts | 5 Levels, 10 Levels |
| Timetrial | The same, in Timetrial | 5, 10, 15, 20 Levels |
| Timetrial - Score Attack | From GO until the target score or more is on screen | 10K, 25K, 50K, 100K, 150K, 200K Points |

**Events and readings.**

| Event | Code | Reading |
|---|---|---|
| Start | Tick 150 of a board that did not resume a save: `Board::Update` plays `SOUND_GO`, then shows the GO banner or, in a profile's first 5 games, the instructions dialog | A1: tick 150, although the banner's first non-zero alpha comes at tick 151 (Q21). It is about 0.1–0.2 s before a "GO visible" retime (derived from the fade, not measured), so times run slightly long. A2: the same tick when the dialog replaces the banner. A3: never a resumed board (`LoadGame` does not restore `mUpdateCnt`, so it shows GO again) |
| Level split | `StartBlackHole` (entry into state 7) | A4: "N Levels" is N wormholes; the run ends while the HUD still shows level N. A5: the `StartBlackHole` tick; the gems visibly move one tick later |
| Score split | The first tick with the HUD score at or above the threshold | A6: "on screen" is the rolling `mDisplayScore`, not `mScore`. A7: no split after `GameOver`, although the HUD keeps rolling during the shake (ask the moderators). A8: hints roll the HUD down; a split is never undone |
| Reset | Game over (no moves in Normal, time up, New Game), board replaced (mode switch), quit | A Timetrial fall-out is not a reset |
| None | Pauses, dialogs, focus loss | A9: real time runs on; the splitter never pauses the timer. A10: no rule covers ports; whether runs on this port count is for the moderators |

**Timing.**
- LiveSplit's real-time timer only. The script defines no `gameTime` or `isLoading`: board ticks stop on
  pauses, modal dialogs, focus loss and minimising, so they would be game time, which speedrun.com does not
  accept.
- The game sees each event on its exact tick; LiveSplit learns of it by polling. ASL runs on a
  `System.Windows.Forms.Timer` with `Interval = round(1000 / refreshRate)`, stopped during each tick, with
  no `timeBeginPeriod`. With `refreshRate = 100` (as in HyperNervie's Twist script; Q24) the period should
  be about 16 ms and sometimes about 31 ms (read from the source, not measured).
- So each event reaches LiveSplit between 0 and one period (about 15–30 ms) late, and a duration is off by
  the difference of two independent lags: up to about ±30 ms, zero on average. Moderators retime from
  video anyway. `qpc` lets `lsprobe` and the `log` setting measure the lag; v1 does not correct for it.

**The block (layout 1).** `games/bejeweled/src/LiveSplitBlock.h`, shared by the observer and `lsprobe`;
little-endian, naturally aligned, 0xB0 bytes:

```
0x00 u8  magic[20]      "BYOPOP-LIVESPLIT" F0 F1 F2 F3, initialised as a brace list of bytes
0x14 u32 gameId         0x58444A42u: the bytes "BJDX" in memory (clang's 'BJDX' would be 0x424A4458)
0x18 u16 layoutVersion  1;   0x1A u16 size 0xB0
0x1C u32 self           the block's own address once active, else 0
0x20 u32 seq            odd while the game writes
0x24 u32 active         1 while features.livesplit is on;   0x28 u32 updates (heartbeat)
0x2C u32 flags          01 timed, 02 resumed, 04 vanilla (no score key, no chosen seed), 08 chosen seed,
                        10 own RNG, 20 replay (stage 3), 40 game over (latched per board), 80 paused
0x30 u32 boardSerial    the current board, 0 = none
0x34 u32 goCount, goSerial;   0x3C levelClears, levelClearSerial;   0x44 gameOvers, gameOverSerial
0x4C u32 deathsNoMoves, deathsTimeUp, deathsNewGame, deathsOther    (they add up to gameOvers)
0x5C i32 tick, state, level, displayLevel, displayScore, score, boardLevelClears, goTick (-1 before GO)
0x7C i32 displayScoreMax  the board's highest displayScore up to its game-over tick, then frozen
0x80 u8  rulesId[32];   0xA0 u64 qpc (SDL_GetPerformanceCounter at this write);   0xA8 u64 qpcFrequency
```

- **Counters, not pulses.** The event counters only ever increase, each with its board's serial, so a late
  poll never loses an event (SAR's block uses a 50 ms pulse instead). `displayScoreMax` does the same for
  score splits: the HUD score can roll back down, and a crossing shown for 1–2 ticks would otherwise fall
  between polls.
- **Append-only.** Fields are never moved, resized or given a new meaning. An append bumps `layoutVersion`
  and `size`; a script for layout N accepts `layoutVersion >= N` and `size >= size_N` and reads `size_N`
  bytes. An incompatible change gets a new magic.
- **Definition** (`games/bejeweled/src/LiveSplitObserver.cpp`):
  `extern "C" __attribute__((used, aligned(64))) volatile LiveSplitBlockV1 gLiveSplitBlock = {...};`
  It is constant-initialised (C++17, no constructor), so the magic exists only in `.data`'s initialised
  bytes. A build check asserts that it occurs exactly once in the exe, inside `.data`'s raw range. `used`
  matters only if gc-sections or LTO is ever added (lld's MinGW driver defaults to `-opt:noref`, and the
  build adds no LTO).
- **In every desktop build, written only with the feature** (Q19). While the feature is off nothing writes
  it (`active` and `self` stay 0), so the script can tell a runner to turn the mod on. A static `.data`
  object adds no file, log line, key or UI (principle 7).

**Observer.**
- **Hook:** `WinDMApp::UpdateFrames` (0x4d4921), after `SexyApp::UpdateFrames()` and the test hooks:
  `if (LiveSplitObserver::Wanted()) LiveSplitObserver::AfterUpdate(this);` (`port(variant)`).
  `Board::Update` runs at most once per app update, so the observer sees every board tick once, before
  its frame is drawn. Each write is `seq++` (odd), release fence, the fields, release fence, `seq++`.
- **Board data**, outside translated functions: `InitVariant` sets `mBoardSerial = ++sBoardSerial;
  mResumed = false;` and `GameLoadedVariant` sets `mResumed = true`, both outside their
  `#ifndef BEJ_NO_VARIANTS` parts, or the `BEJ_NO_VARIANTS` oracle would count GO on resumed boards.

| Event | Condition (against the previous update's serial, tick and state) | Writes |
|---|---|---|
| New board, or none | The serial differs (0 without a board) | `boardSerial`, `boardLevelClears = 0`, `goTick = -1`, `displayScoreMax = 0`, flags; RulesId once per board |
| GO | `prevTick < 150 <= mUpdateCnt`, same board, not resumed, not a replay | `goCount++`, `goSerial`, `goTick` |
| Level clear | `mState == 7 && prevState != 7`, and the tick advanced (a `LoadGame` that restores state 7 does not) | `levelClears++`, its serial, `boardLevelClears++` |
| Game over | `mState == 5 && prevState != 5` (`GameOver` is the only setter) | `gameOvers++`, its serial, flag 0x40, one cause: `mNewGamePending` → newGame; else previous state PLAYING → timeUp (only state 1 checks the time); else FALLING → noMoves; else other |
| Every update | Always; `displayScoreMax` only up to and including the game-over update, then frozen | `displayScoreMax = max(displayScoreMax, mDisplayScore)`, then the snapshot fields |

- **Activation:** the first update with the feature on writes `self`, `qpcFrequency` and `active = 1`,
  logs the address and copies the templates. **Deactivation:** `NewGame`'s `gGameConfig.Load()` with the
  feature off writes `active = 0`. `~WinDMApp` writes `active = 0` and `boardSerial = 0`, so the script
  sees the quit and `lsprobe` can tell a clean quit from a crash.
- **It only reads** `mBoard`'s fields, `ModManager::GetScoreKey()`, `mRules.RulesId()` and
  `gGameConfig.mFeatures`. It never calls `Rand()` (`games/bejeweled/tools/check_rng_use.py` covers it),
  writes game state, marks anything dirty, draws, or touches saves, `hiscores`, the registry or replays.
  QPC comes from `SDL_GetPerformanceCounter`, so there is no `<windows.h>` (the clash noted in
  `platform/SteamPath.cpp`).
- **Status lines** (SDL_Log, only with the feature on): the block address, the templates copied, a resumed
  board whose GO is not counted, each GO, level clear and game over with its tick, and "off".
  `-livesplitlog=<file>` appends one deterministic line per write (`self` and QPC masked) for the oracle.

**Finding and reading it** (the script and `lsprobe` alike).
- The port's exe has ASLR (DllCharacteristics 0x8140), so there is no fixed address. The reader takes the
  main module's PE header from the game's memory (checking `MZ`, `PE\0\0` and positive sizes, since
  `SignatureScanner`'s constructor throws) and scans only the `IMAGE_SCN_MEM_WRITE` sections (`.data`,
  `.tls`), one read each, at 4-byte alignment. (`MemoryPages()` skips image sections, and one scan
  over the whole module fails on a single unreadable page.) A hit counts only with
  `gameId == 0x58444A42`, `layoutVersion >= 1`, `size >= 0xB0`, and `self` 0 or equal to the hit.
- Reading: `seq` → s1, the 0xB0 bytes, `seq` → s2, in three `ReadProcessMemory` calls. The copy counts when
  s1 == s2, s1 is even and the copy's `seq` equals s1; otherwise retry up to 3 times, then skip the cycle.
  This is sound on x86 for a 32-bit game read by 64-bit LiveSplit (stores and loads keep their order); the
  C++ memory model does not cover it.

**The script** (`games/bejeweled/tools/livesplit/BejeweledDeluxe-byopop.asl`, about 200 lines).
- **C# 5 only.** LiveSplit 1.8.37, the latest release, bundles ScriptableAutoSplit 9c1e207. It compiles
  scripts with `CSharpCodeProvider` (`CompilerVersion v4.0`, the .NET Framework compiler), and its generated
  usings lack `System.Text.RegularExpressions`; Roslyn is only on master. So the script writes
  `System.Text.RegularExpressions.Regex` in full or, simpler, parses split names by hand.
- **Processes:** `state("bejeweled") {}` (the target in `games/bejeweled/CMakeLists.txt`) and
  `state("popcapgame1")` for the original. ASL attaches to the newest live process of the first listed name
  that has one, at connect time only. 64-bit LiveSplit reads the 32-bit game through `ModulesWow64Safe()`.

| Action | Rule |
|---|---|
| `startup` | `refreshRate = 100`; the settings; `vars.Mode = "none"` and empty `vars.Target`/`vars.IsScore`, so no action meets an unset var; helpers `FindBlock`, `ReadBlock`, `ParseSplits`, `UpdateOriginal`; a `TimerModel` for `exit` |
| `init` | Sets an explicit list of custom fields to 0 every time (ASL reuses the state object across reconnects). `popcapgame1`: `Mode = "probe"` when the `original` setting is on and `ModuleMemorySize == 0x192000`, else ignored. Port: `Mode = "port"`, `FindBlock`. Never throws: an exception reruns `init` every cycle and logs each time |
| `update` | Port: until the block is found, rescan once a second. A rejected read returns false; the counters catch up next cycle. `active == 0`: one `MessageBox` per attach ("turn on the game's livesplit mod"; `print` reaches only DebugView), return false. Otherwise copy the fields into `current`; the first copy after attaching or activation is a baseline only (returns false), so a game in progress is never a run. `probe`: the vftable check below |
| `start` | `goCount` changed and `goSerial == boardSerial`; **mode guard** (setting): a category name containing "Timetrial" needs a timed board, one containing "Normal" an untimed one; not a replay (0x20); with `vanillaOnly`, flag 0x04 |
| `onStart` | Always parses the segment names. Sets `vars.RunSerial = current.boardSerial` only when `current != null` and `Mode` is "port" or "original": a manual start with no game attached must not throw |
| `split` | Only while `boardSerial == RunSerial`. Segment `i = timer.CurrentSplitIndex`: a score split needs `displayScoreMax >= threshold`, the k-th level split `boardLevelClears >= k`. It depends only on the index, the names and the snapshot, so skips and undos cannot desynchronise it (an undone level split splits again on the next poll; the README says so) |
| `reset` | With `resets` and `RunSerial != 0`: `gameOvers` changed with `gameOverSerial == RunSerial` → by cause; the run's board replaced while its flag 0x40 is clear → `resetQuit` (new serial 0) or `resetReplaced` |
| `onReset`, `exit` | `RunSerial = 0`. `exit`: while Running or Paused, with `settings.ResetEnabled`, `resets` and `resetQuit` on, `vars.TimerModel.Reset()` |

- ASL evaluates `start` only while NotRunning, and `reset`, then `split` if there was no reset, only while
  Running or Paused. Nothing runs while Ended, so a finished run stays until the runner resets it.
- **Split names:** a subsplit `-` or `{Section}` prefix is stripped; an integer of at least 1000 (also
  written `25,000` or `25K`) is a score split, anything else a level split. One split per cycle is enough:
  the HUD rolls at most 3 × multiplier points per tick (36 at level 20).
- **Settings** (ASL settings are booleans): `modeGuard` on; `vanillaOnly` off (practice; the mod signs and
  the SEED line show on video anyway); `resets` on, with `resetNoMoves`, `resetTimeUp`, `resetNewGame`,
  `resetReplaced` and `resetQuit` under it (the registry README calls automatic reset dangerous, so each is
  switchable, and an Ended run is never reset; Q20); `original` on; `log` off (appends each event and its
  lag to a text file).
- **Cases:** a resumed save never starts by itself, but a manual start uses that board and splits
  (practice). Seeded and modded games start unless `vanillaOnly` is on. Replays never start. An unrelated
  `popcapgame1` (another Steam PopCap game) that attaches first holds the script until it exits.

**The original 1.87 in the same script** (optional, recommended).
- Steam's `WinBej.exe` is only PopCap's 2007 wrapper, so the game should run as `popcapgame1`. That is
  inferred from other games on the same wrapper (the registry's Feeding Frenzy 2 script uses the name), not
  observed.
- The carved image has no ASLR (ImageBase 0x400000, no `.reloc`, SizeOfImage 0x192000), and `gApp` is a
  static object, so one dereference reaches the board. The offsets come from
  `games/bejeweled/src/Board.h`, `WinDMApp.h` and `docs/FRAMEWORK_NOTES.md`:

  ```csharp
  state("popcapgame1")                          // offsets relative to the main module
  {
      uint appVtbl : 0x13B570;                  // gApp 0x53B570; its vftable is 0x507CCC
      uint board   : 0x13BCF8;                  // gApp.mBoard (+0x788)
      int  tick    : 0x13BCF8, 0x28;            // mUpdateCnt
      int  level   : 0x13BCF8, 0x8C;     int state : 0x13BCF8, 0x194;     bool timed : 0x13BCF8, 0x198;
      int  score   : 0x13BCF8, 0x1EC;    int displayScore : 0x13BCF8, 0x1F8;
      bool newGame : 0x13BCF8, 0x240;           // mNewGamePending
  }
  ```

- **Identity:** `ModuleMemorySize == 0x192000` (the file the wrapper writes may differ, so the test checks
  it) and `appVtbl == base + 0x107CCC`. A static constructor writes the vftable during CRT start-up, which
  can come after `init` when LiveSplit was open first, so `update` re-checks it each cycle for about 15 s
  before giving up. Other PopCap games that run as `popcapgame1` stay inert.
- **`UpdateOriginal`** builds the block's fields by polling. The serial comes from `board` pointer changes
  (`NewGame` defers deleting the old board, so a new board never reuses its address), the counters from
  polled transitions (state 7 lasts about 250 ticks, state 5 at least 50). A board is resumed when its
  score or level is non-zero before tick 50 (a new board's gems take about 70 ticks to land); one first
  seen after tick 50 is never a run. The death cause comes from `newGame`, then timed or previous state 1
  → time up, else no moves. `RequestNewGame` sets state 5 (in `GameOver`) before `mNewGamePending`, so a
  poll between the two stores takes a New Game for another cause, about once in 10⁴; that matters only
  when the runner turned one cause off. `displayScoreMax` comes from polled values, so on the original a
  threshold shown for less than one poll period can be missed; only the port's block is lossless.
- **Testing it is the user's call.** It needs Steam; it plays sound through BASS and DirectSound, which
  rundesk cannot silence; DirectDraw on rundesk's hidden desktop is untested. With the user's OK, the user
  plays a few short games (Normal to the first wormhole, then no moves or New Game; Timetrial with a time
  up; quit and resume; a mode switch) while `lsprobe -original` logs, and `lsdecide.py` checks the
  decisions. The session also confirms the process name and module size.
- **Include it only after that test, and only if the script is registered as "Bejeweled Deluxe" (Q15):**
  all 118 runs are of the original, and a port-only entry would take the name without serving them.
  Otherwise the script stays port-only and unregistered, loaded by hand.

**Registry and publishing** (every step needs the user's approval).
- **One entry per game name.** Activate looks the splits' game name up in the registry's
  `LiveSplit.AutoSplitters.xml`, which LiveSplit lower-cases into a `ToDictionary`. A duplicate name
  throws; from 1.8.37's source the exception probably escapes on every splits load, for every user. The
  registry CI runs LiveSplit's tests on each PR, so a second "Bejeweled Deluxe" entry cannot be merged.
- **HyperNervie** (the Bejeweled 3 and Twist splitters; Unlicense; last push 2024-11-03) lists a
  Bejeweled 1 splitter as coming soon and invites pull requests. **Recommendation (Q14):** ask first (an
  issue on their repo, or the Bejeweled Discord) and propose **one combined script** for the port and the
  original 1.87 under "Bejeweled Deluxe", with them as co-maintainer. If they would rather own the entry,
  contribute the port path to their script. A separate name such as "Bejeweled Deluxe (byo-pop)" would
  split Activate from LiveSplit's speedrun.com features; not recommended.
- **Hosting (Q17):** GitHub raw from byo-pop's default branch once it is pushed and public (it has an
  `origin` but was never pushed), or a small separate public repo. LiveSplit downloads a Script entry again
  on every activation, so a push reaches every user at their next start: the file changes only through
  commits that pass the tests below. LiveSplit saves the file under the URL's last path segment in its
  Components folder, and the name `BejeweledDeluxe-byopop.asl` avoids other entries' files there.
- **Entry:** `<Game>Bejeweled Deluxe</Game>`, the raw URL (not a `/blob/` link), `<Type>Script</Type>`
  without `ScriptType` (that is for ASR), a Description of at most 120 characters, a Website. The PR
  template asks the author to confirm no malicious code, no replacement of another author's splitter
  without permission, and an open-source licence. byo-pop has none, so the script needs its own (Q16).
- **Steps, in order:** choose the licence; push byo-pop or create the script repo; ask the speedrun.com
  moderators whether port runs count (A10), how to read A7 and whether they would list the splitter (Q22);
  contact HyperNervie; the manual test (and the original's); open the registry PR (editing the XML on
  GitHub creates the fork, branch and PR); check Activate once.

**Templates and the mod.**
- `games/bejeweled/tools/make_example_mods.py` generates `games/bejeweled/mods/livesplit/`: `mod.json`
  (`{"id": "livesplit", "name": "LiveSplit", "version": "1.0", "features": {"livesplit": true}}`),
  `livesplit/*.lss`, a copy of the script to load by hand, and `README.txt`.
- Twelve `.lss` files in LiveSplit's 1.7.0 format: game name "Bejeweled Deluxe", the category name, the
  subcategory as a metadata variable with speedrun.com's names (`Normal` = "5 Levels",
  `Timetrial - Score rush` = "100K Points"), platform PC, and an empty `AutoSplitterSettings` that
  LiveSplit fills after Activate. LiveSplit writes those settings only while the splitter is active, so
  splits saved with it inactive lose them; the tools README says so.

| Templates | Segments |
|---|---|
| Normal 5 / 10 Levels; Timetrial 5 / 10 / 15 / 20 Levels | `Level 1` … `Level N` (a 20-Levels file also gives the 5, 10 and 15 times) |
| Score Attack 10K / 25K / 50K / 100K / 150K / 200K Points | `10000`, `25000`, `50000`, `100000`, `150000`, `200000`, up to the target |

- Score Attack names are plain integers, which HyperNervie's Bejeweled 3 grammar (ascending integers
  only) also accepts.
- On activation the game copies missing files to `<userdir>/livesplit/` and never overwrites one, since
  LiveSplit saves PBs into the file it opened. No oracle step compares those copies.
- **Plumbing:** `FEATURE_LIVESPLIT` in `gFeatureNames`, `ParseFeature` and `MergeFeatures`;
  `GameFeatures::mLiveSplit`, shown as "livesplit". It is not in the RulesId or the score key, and
  `ComputeVariantFlags` never reads it, so the RNG source and the save format do not change.
- **Mod signs:** any enabled mod adds the crossbar on GO's G and a fourth hint sparkle, which would show
  in run videos. Q18 proposes leaving a mod whose only key is `features.livesplit` out of `mAnyModEnabled`.
- **Desktop only:** the block and the observer sit under `#if defined(_WIN32) && !defined(__EMSCRIPTEN__)`
  around the whole translation unit (the source glob also feeds the web build). The feature is parsed
  everywhere, so `mods-status.json` is the same; the web build warns "features.livesplit: desktop only,
  ignored".

**Fidelity.** No translated Board function changes and the observer only reads, so the default oracle is
unchanged: the block is never written. `mBoardSerial` and `mResumed` are observer bookkeeping. They go on
the "Left out" list of the FULL hash in `games/bejeweled/src/testhooks/StateHash.cpp` and get no
StateCodec row, so FULL hashes stay equal to the stage-0 baselines.

**Tests** (LiveSplit itself is not needed):
1. **Oracle case "livesplit":** the candidate with the `livesplit` mod against the candidate with an inert
   `{"features": {"livesplit": false}}` mod (the same mod signs), over the `playtest*.txt` scripts, the
   stage-0 cases and `-autoplay` in both modes. TRACE, saves, `hiscores`, `registry.ini`, `-tracehash` and
   screenshots are identical; the logs are equal without the `livesplit:` and `mods:` lines. Repeated
   under `-forcevariant` and on the `BEJ_NO_VARIANTS` build.
2. **Event log:** `-livesplitlog` against the `-tracehash` state column: GO at tick 150 of each fresh board
   and none on the resumed one (`playtest_load`), one level clear per entry into state 7, the right death
   cause, monotonic counters, a new serial at each `NewGame`.
3. **`lsprobe`** (Win32 C++, about 250 lines, built x86_64 with llvm-mingw so that it reads the 32-bit game
   as LiveSplit does): `rundesk lsprobe -log=<f> -- bejeweled.exe -script=... -livesplitlog=<g>` finds and
   reads the block exactly as the script does. `check_lslog.py` checks that the address equals the logged
   one and that every accepted snapshot equals the game-log line with its `seq`. Also a `-fast` stress run,
   `-dumplayout` (for `check_asl_offsets.py`) and `-original` (the 1.87 adapter).
4. **`lsdecide.py`:** an independent re-implementation of the script's decisions and ASL's phase rules,
   with expected events for Normal 5 Levels, Score Attack 10K, the mode guard, a resumed save (no start; a
   manual start), each reset, the feature off, and attaching mid-game. It runs on game logs (exact ticks)
   and on probe logs (real polling).
5. **`asltest`** (about 200 lines of C#): compiles the published `.asl` with the in-box .NET Framework
   `csc.exe` (C# 5, as 1.8.37 does) in a wrapper with 9c1e207's using list (without `System.Text.Json`),
   stubs only the members the script uses, and replays test 4's logs. Its events must equal
   `lsdecide.py`'s.
6. **`check_asl_offsets.py`** (the script's magic and offsets against `lsprobe -dumplayout`) and the
   magic-once build check.
7. **Manual, once, after the user approves the download (Q23):** LiveSplit 1.8.37 (released 2026-02-15)
   with the script loaded from a local file. Normal 5 Levels and Score Attack 10K are recorded and
   compared with a frame retime; the `log` setting turns the lag estimate into a measurement. Activate is
   checked after the registry PR.

**Size and stage.** About 1,550 lines, about 340 of them game code: block header ~80, observer ~210,
feature plumbing ~35, hooks ~15, script ~200, mod and templates ~150, `lsprobe` ~250, `lsdecide.py` ~200,
`asltest` ~200, checks ~120, docs ~80. Stage **2b** comes after stage 2 and before stage 3; it needs only
stage 1's `features` plumbing. The stage commit holds the game side, the mod, the script and tests 1–6.
Publishing, test 7 and the original's test are separate steps the user approves.

**Sources:**
- speedrun.com: [Bejeweled Deluxe categories](https://www.speedrun.com/api/v1/games/46w3v591/categories?embed=variables),
  [verified runs](https://www.speedrun.com/api/v1/runs?game=46w3v591&status=verified&max=200)
- LiveSplit 1.8.37: [AutoSplitterFactory.cs](https://raw.githubusercontent.com/LiveSplit/LiveSplit/1.8.37/src/LiveSplit.Core/Model/AutoSplitterFactory.cs),
  [SignatureScanner.cs](https://raw.githubusercontent.com/LiveSplit/LiveSplit/1.8.37/src/LiveSplit.Core/ComponentUtil/SignatureScanner.cs),
  [XMLRunSaver.cs](https://raw.githubusercontent.com/LiveSplit/LiveSplit/1.8.37/src/LiveSplit.Core/Model/RunSavers/XMLRunSaver.cs)
- ScriptableAutoSplit 9c1e207, bundled with 1.8.37: [ASL/](https://github.com/LiveSplit/LiveSplit.ScriptableAutoSplit/tree/9c1e207059d80615408750f709309dba87a929ea/src/LiveSplit.ScriptableAutoSplit/ASL)
  (`ASLScript.cs`, `ASLMethod.cs`), [Component.cs](https://raw.githubusercontent.com/LiveSplit/LiveSplit.ScriptableAutoSplit/9c1e207059d80615408750f709309dba87a929ea/src/LiveSplit.ScriptableAutoSplit/Component.cs)
- The registry: [README](https://github.com/LiveSplit/LiveSplit.AutoSplitters),
  [PR template](https://raw.githubusercontent.com/LiveSplit/LiveSplit.AutoSplitters/master/.github/pull_request_template.md),
  [LiveSplit's registry test](https://raw.githubusercontent.com/LiveSplit/LiveSplit/master/test/LiveSplit.Tests/AutoSplitterTests/AutoSplitterXML.cs)
- Precedents: [HyperNervie/Bejeweled-Autosplitters](https://github.com/HyperNervie/Bejeweled-Autosplitters),
  [FeedingFrenzy2.asl](https://raw.githubusercontent.com/Xh3o1vn/FeedingFrenzy2_Autosplitter/main/FeedingFrenzy2.asl);
  [GCC: multi-character constants](https://gcc.gnu.org/onlinedocs/cpp/Implementation-defined-behavior.html)

### 4.16 Twilight gravity (Bejeweled 2)

Added from the user's decisions (2026-10-18): Bejeweled 2's secret Twilight mode, matched exactly, with all
three of its cosmetic touches, in Normal and Timed, and with no direction indicator.

**What Bejeweled 2 does.** These facts were read from a static disassembly of the game embedded in the
installed `WinBej2.exe` (file offset 0x1841a2, image base 0x400000, built April 2007). Nothing was run or
observed in play. The web descriptions are loose and disagree in places (wiki.gg's Bejeweled 2 page has
gravity change with every match); where they differ, the code wins.

| Fact | Address |
|---|---|
| Twilight is mode 4, the secret counterpart of Classic, and unlocks after Classic level 17. Otherwise it plays as Classic (untimed, level bar, game over when no move is left; wiki.gg). | `IsModeUnlocked(4)` 0x5cf3e3 |
| One gravity byte on the board (+0x1721), which the constructor sets in mode 4: Twilight starts **up**. | 0x5adb76..0x5adb9c |
| Up: rows count from the bottom (`GetRowY` 620 − 84·row, else 32 + 84·row); refills spawn below the screen; `FillBoard` starts its gems below. | 0x594bd5, 0x5a7255, 0x597efd |
| **The only flip** is in `UpdateFalling`: everything has settled, `ProcessMatches` found nothing, and the state has just become idle. The same step swaps grid rows r and 7 − r, so row 0 is always the entry side. | 0x5a9edb..0x5a9f5d |
| So a move's cascades share one direction, and a swap that makes no match never flips: the swap-back goes straight to idle. | 0x5a8b99..0x5a8ba5 |
| The opening fill runs with the flag set, so **the opening board rises**; its settle flips, so the first move's gems fall from the top and the second's rise. | 0x5aded3 |
| Each level's fill also flips at its settle, so the last move of a level and the first of the next share a direction. There is no reset per level; the flag is saved. | 0x597f15, 0x5a51c2, 0x59c993 |
| Slower falls: acceleration 0.1 per tick in Twilight, in both directions, against 0.24 in the other modes (in 84-px cells). | 0x5a9de0..0x5a9e07 |
| Every sound plays with `AdjustPitch(-2.0)`, 2 semitones lower. | 0x5dbec8..0x5dbefe |
| The backdrop has each pixel row mirrored left to right and each colour channel halved. | 0x59bfbd..0x59c066 |
| Power Gems and Hypercubes are still created. | 0x5a8119..0x5a81a3 |
| Mode 4 saves to `s_normal.sav`. `twilight.sav` belongs to mode 8, probably the hidden Original mode (an inference). | 0x59c512..0x59c54a |

**Rule keys** (3.2).
- `board.gravity.direction`: `"down"` (default), `"up"` or `"alternate"` (Twilight).
- `board.gravity.fallAccel`: the fall acceleration in `UpdateFalling`, in thousandths of a logical pixel
  per tick², default 180. `180 / 1000.0` is the same double as the original literal `0.18`, so the default
  is bit-identical. It is an integer, as 6.1 asks of new rules.
- `look.backdrop` (`"normal"` / `"twilight"`) and `look.soundPitch` (semitones, default 0) are
  presentation. They sit in `rules` so that a replay's RULE record carries them and a watched game looks
  the same. Like tunables, they never make a variant board, but they do enter the RulesId and the score
  key, so a look-only mod gets its own tables (Q27).
- Not included: a choice of starting direction, other flip triggers (Astraware's flips on illegal moves, a
  per-refill zig-zag) and a direction indicator. Each can come later as a key with a default, which leaves
  existing RulesIds unchanged.

```jsonc
{ "id": "twilight", "name": "Twilight", "version": "1.0",
  "rules": { "board": { "gravity": { "direction": "alternate", "fallAccel": 75 } },
             "look":  { "backdrop": "twilight", "soundPitch": -2 } } }
```

- `fallAccel` 75 applies Bejeweled 2's slowdown (0.24 → 0.1, ×0.42) to BJ1's 0.18. 100 would copy
  Bejeweled 2's number, which is in its 84-px cells (Q25).
- The preset is BJ1 Normal or Timed plus gravity. Bejeweled 2's Twilight also has Power Gems and
  Hypercubes, which BJ1 does not; `power-gems` (4.8) combines with it.

**Merge, validation, RulesId.**
- **Group:** a new atomic group *gravity* (`board.gravity.*`), appended before `NUM_GROUPS`; group numbers
  are not part of the canonical form. `"board": {"gravity": null}` resets it. `look.*` are leaves.
- **Schema:** ids 7 and 8 in the `board` block (1..6 are taken), 144 and 145 in a new `look` block;
  `board.gravity` and `look` join `gNamespaces`.
- **Validation:** enum values (reject the mod's rules); `fallAccel` 50..1000 and `soundPitch` -12..12
  (clamp and warn). The fill colour minimum (3.6) is computed for each fill order in use (`down` bottom-up,
  `up` top-down, `alternate` both) and the larger is taken. The orders differ for cell patterns that are
  not closed under a vertical mirror, and for vertically asymmetric masks or levels.
  `board.gravity.direction` joins that check's cause keys, so the merged-set drop finds the right
  contributor.
- **RulesId:** the new fields are written only when they differ from their defaults (3.7), so every
  existing id stays. A Twilight mod has `rules`, so it gets its own score tables, and a non-default
  `direction` or `fallAccel` makes a variant board (own RNG, v2 save, variant trace).

**Semantics.**
- **State:** `int mGravityFlips` (settles that flipped) and `bool mGravityNoFlip`. `GravityUp()` is true
  for `up`, false for `down`, and `(mGravityFlips & 1) == 0` for `alternate`, which therefore starts up.
- **The flip** (`alternate` only): `OnGravitySettle()` runs at the end of `UpdateFalling`'s no-match
  branch. If the state is now `STATE_PLAYING` and `mGravityNoFlip` is clear, `mGravityFlips++`; it always
  clears `mGravityNoFlip`. That gives exactly Bejeweled 2's sequence:
  - the opening fill rises and its settle flips, so the first move's gems fall from the top and the
    second's rise;
  - all of a move's cascades use one direction;
  - a swap that makes no match never flips. A swap-back returns to `STATE_PLAYING` from `UpdateSwap`
    without a settle, and `KeepInvalidMove` (4.1) sets `mGravityNoFlip`. Twist and slide (4.4, 4.5) behave
    the same;
  - every fill flips once at its settle (level, timed fall-out), so across a level-up the last move and
    the first move of the next level share a direction.
- **No moves** is the port's choice, since Bejeweled 2's no-moves path was not read and it has no Timed
  Twilight: a settle that ends with no moves does not flip. Normal ends, and Timed falls out in the
  current direction, then its refill's settle flips.
- Gravity never changes while a piece moves. Every fall (refill, cascade, fill, fall-out, game-over fall)
  reads `GravityUp()` when it starts.
- Bejeweled 2 swaps the grid rows at each flip so that row 0 is the entry side. The port keeps the grid in
  screen order and orders each column from the entry side instead: the same motion, with saves, hints and
  traces keeping their meaning.

**Falling, refill and fill.**
- **`FallPiecesVariant`** (the `UpdateFalling` loop when `mFall`): speeds stay positive (1.0 for a moved
  gem, 0.3 for a spawned one), and the side of its cell gives the direction: a piece above its rest Y
  falls, one below it rises. `mVelY += fallAccel / 1000.0`. Only `+ - /` and comparisons are used, so
  native and wasm agree (6.1). Fall-out and the game-over fall keep their own constants (0.18, 0.15): the
  slowdown read in Bejeweled 2 is in its `UpdateFalling`.
- **`RefillMasked`** (5.4): each column's playable rows `V` are ordered entry side first (reversed under
  up), and spawns start at `edgeY ± 69`. The RNG order is unchanged (columns ascending, one draw per column
  with a gap in each pass), and the down branch is the original pass line for line. An upward refill draws
  the same colours as the downward refill of the vertically mirrored grid, since `(int)` truncates toward
  the board on both sides. On 8x8, upward spawns start at y ≥ 442, under the clip's bottom edge 428.
- **`FillBoardVariant`** (5.5): under up, rows `0..rows-1`; the reference Y is the nearest filled cell
  above, else M = `GetRowY(0) + GetRowY(rows-1)` (389 on 8x8); gems start at `ref + Rand()%240 + 65`. For
  one RNG state the upward fill is the vertical mirror of the downward one, with the same draws.
- **Seeds (4.10):** the tutorial loop keeps the first fill with the tutorial move in rows 4..6, and a
  rising fill puts its mirror in rows 1..3, so a seed may keep a different fill. A seed's Twilight board is
  therefore not simply the mirror of its Classic board.
- **Fall-out and game over follow gravity** (Q26). `UpdateFallOutVariant` keeps one `Rand()%15` per
  column per tick, scans from the top under up, moves by `-mVelY` and drops a piece at
  y ≤ M − (`mScreenBottom` + 32), −123 on 8x8. `GameOverFallVariant` leaves the shake and explosion draws
  in translated code and mirrors only the fall (gone at M − (`mScreenBottom` + 49), −140).
- Hint, the no-moves search and the tutorial search test only the grid, so they do not change.

**What the player sees.**
- No direction indicator, as in Bejeweled 2.
- **Backdrop** (`look.backdrop: "twilight"`). BJ1 has no backdrop picture, only the level's tinted checker
  tiles, so this approximates Bejeweled 2's effect. `SetupLevel`'s tile-loop hook (`mChecker`) runs
  `DrawVariantChecker`, which draws the tiles, then mirrors that layer left to right about the board
  centre x 406 and halves each colour channel. The translated `IMAGE_ALPHABACK` draw (frame and HUD)
  follows unchanged, so the HUD is not mirrored. The constructor's copy and the level wipe show the
  result.
- **Pitch** (`look.soundPitch: -2`). `SoundManager` gains a default pitch that `GetSoundInstance` applies
  to every instance it hands out (`SDLSoundInstance::AdjustPitch` already exists). `InitVariant` sets it
  from the rules (0 otherwise). That covers `PlaySample` and the whirlpool instance that `StartBlackHole`
  takes directly, with no edit in translated code. Music is unchanged.
- **Clip** (amends stage 7's `DrawVariant`). The gem layer is clipped to the board's own screen
  rectangle, `ToScreen(origin) − (9, 4)` to `ToScreen(origin + (w, h)) + (13, 6)`. On 8x8 that is
  (194, 12)–(622, 428), the original `ClipRect(194, 12, 428, 416)`. On short boards such as 10x7 the fixed
  8x8 clip would show spawns in the void, in either direction.
- **Hidden gems.** `GetPieceAt` gets
  `if (mVariant.mGravity && GravityUp() && theY >= mLayout.mClipBottom) return NULL;`, where `mClipBottom`
  is the clip's bottom edge in logical space (428 on 8x8), since `GetPieceAt` receives logical
  coordinates. Otherwise a rising gem hidden in the bar row could be selected, and its cursor (drawn
  without the clip) would show over the HUD. The guard needs `GravityUp()`, not only the flag: `MouseDown`
  blocks only states 5–8, so the original can select a gem falling out at y 428..480, and
  `-forcevariant`, which sets the flag at default rules, must not change that.
- **Help.** `InstructionsStringId()` (2.6) returns `INSTRUCTIONDIALOG_TWILIGHT` for `alternate` and
  `_GRAVITY_UP` for `up`, unless the twist or slide text applies. Help uses a replacement for
  `FULL_INSTRUCTIONS2`, whose description of gems dropping in from above would be wrong. The engine ships
  English defaults; mods override them through `strings`.

**Interactions.**

| Feature | Interaction |
|---|---|
| Holes and masks (5.4) | `V` and M come from the current layout; gems rise through holes as they fall through them. |
| Dirt (4.14, 7b) | Mirrored: dirt is a floor for down and a ceiling for up, only the entry-side segment is refilled, and diagonal slides come from the entry side. `mDiagonalLeftFirst` is shared. 7b adds a warning per direction in use: dirt at a column's entry-side end blocks its refill. |
| Shapes and per-level shapes (5.3) | No restriction: the layout switches in `UpdateBlackHole` before `FillBoard` uses the new M. |
| Scaled boards (5.1) | Logical space throughout; the limits are `mScreenBottom` and its mirror; the clip above. |
| Twist, slide (4.4, 4.5) | A matching move flips at its settle; `_BACK` returns to PLAYING without a settle, so it never flips. |
| Allow invalid moves (4.1) | A kept invalid move never flips (`mGravityNoFlip`). With `noMoves: "continue"`, a move's settle still reaches PLAYING and flips. |
| Flame and lightning (4.8) | Blasts resolve within the move; the refill takes the move's direction. |
| Gold (4.9) | The gold layer is per cell and does not move. Gold's fall-out and time-up game over follow gravity. |
| Undo (4.12, 6.7) | The `StartSwap` snapshot, taken after the previous settle's flip, holds `mGravityFlips` and `mGravityNoFlip`, so undo restores the direction, and with `restoreRng` the repeated move refills the same way. |
| Replays (6.3) | No new events: gravity follows from the moves. |
| StateCodec, hashes, trace (6.6) | Rows for `mGravityFlips` and `mGravityNoFlip` (V2, UNDO, LOGIC, FULL) and the trace field `gravity up\|down flips n` exist only when the rules set a gravity rule. The condition reads `mRules`, not the flags `-forcevariant` sets, so default-rules hashes and the stage-0 `-tracehash` baselines stay frozen. |
| v2 save (7.2) | Chunk `GRAV {i32 flips, u8 noFlip}` under the same condition; a missing chunk reads as 0. Saves mid-cascade (states 0 and 4) resume in the saved direction, since `LoadGame` snaps positions. A save in `STATE_SWAPPING_BACK` already holds the swapped grid as state 1, as in the original; no flip is involved. |
| Set seed (4.10) | `UsePendingSeed` is unchanged; see the fill note above. |
| Leaderboards (4.13) | The RulesId covers gravity and look, so Twilight gets its own boards. |
| Patterns, gem colours (4.6, 4.2) | Matching does not depend on gravity; only the fill minimum needs both orders. |
| Timed mode | Timed Twilight is new (Bejeweled 2's is untimed). The drain runs only in states 1 and 2, so slower falls cost no time. |
| Auto LiveSplit (4.15) | A Twilight game is not vanilla (flag 0x04 clear); `vanillaOnly` refuses it. |

**Code plan.**
- **`VariantFlags`** (2.2): `mGravity` (direction not down), `mFall` (`mShape || mGravity || fallAccel !=
  180`), `mChecker` (`mShape || look.backdrop == "twilight"`). `-forcevariant` also sets `mGravity`, `mFall`
  and `mChecker`; the rules stay down, so the flip and the `GetPieceAt` guard stay off.
- **Translated hooks** (2.5): `UpdateFalling` (the loop goes to `FallPiecesVariant` when `mFall`, and
  `OnGravitySettle` ends the no-match branch), `UpdateClearing` and `FillBoard` (condition `mFall`),
  `UpdateGameOver` (fall phase), `UpdateFallOut` (first line), `GetPieceAt` (guard), `SetupLevel`
  (condition `mChecker`). About 6 tagged lines, none in `Draw`. `UpdateSwap` is not touched: the flip sits
  at the settle, where Bejeweled 2 has it.
- **Variant code** (`BoardShape.cpp`): `FallPiecesVariant`, the direction branches of `RefillMasked` and
  `FillBoardVariant`, `UpdateFallOutVariant`, `GameOverFallVariant`, `OnGravitySettle`, the backdrop pass,
  the `GRAV` chunk, the StateCodec rows and the trace field. `KeepInvalidMove` sets `mGravityNoFlip`.
- **Outside Board:** `SoundManager::SetDefaultPitch` (framework), `InstructionsStringId`, the strings.

**Tests.**
- **Identity at down.** `-forcevariant` runs every new path at default rules (the guard and the flip stay
  off): traces, files and screenshots pixel-exact. The stage-0 no-moves game-over and timed fall-out
  scripts cover the end animations. Default-rules `-tracehash` baselines are unchanged. `BEJ_NO_VARIANTS`
  must match too, and the difftest now covers the gravity paths at down.
- **Mirror unit tests** (verifier): for one RNG state, `FillBoardVariant(up)` and `RefillMasked(up)` on a
  grid with holes equal the mirrors of the down versions, in colours, rows and draw counts.
- **`bejeweled_verify mirrortest --seeds 1..200`**: a mirrored copy of a down board is restored into an
  `up` board with the same RNG state, and the bot plays mirrored moves. Grid, score and stats are compared
  at every settle until the first level-up, fall-out or game over, whose draws depend on tick alignment.
  Settle-tick differences are warnings (float rounding at another magnitude).
- **Scripts with screenshots:** the opening rise, the first move falling and the second rising; a level-up
  (last and first move share a direction); a swap-back and a kept invalid move (`allowInvalid`) that do not
  flip; a Timed fall-out; a game over going up; a save in state 4 going up, then load and finish; undo;
  diamond + Twilight; the backdrop; landing ticks with `fallAccel`.
- **Pitch:** a `fwtest` check that `GetSoundInstance` applies the default pitch.
- **Replay:** record → in-game → native → wasm verifier, with equal `CHECK` and `END`.
- **`rulestest`:** group merge, `null` reset, canonical bytes of fields 7, 8, 144 and 145, existing
  RulesIds unchanged, the fill minimum for an asymmetric mask under up and alternate.

**Size and stage.** About 690 lines: rules and `rulestest` ~150, flags, helpers and hooks ~50, falling,
refill and fill ~90, fall-out and game over ~70, clip and guard ~30, chunk, StateCodec, trace and strings
~60, backdrop and pitch ~60, mirror tests and scripts ~180. Stage **7c** comes after stage 7 and before 7b.
It needs stages 3 (StateCodec), 5 (v2 chunks), 6 (the undo row) and 7 (`RefillMasked`, `FillBoardVariant`,
`DrawVariant`), and coming before 7b lets 7b write its diagonal pass and x settling direction-aware once.

**Sources:**
- The installed Bejeweled 2 Deluxe (Steam): the game embedded in `WinBej2.exe`, disassembled statically.
- [wiki.gg: Twilight](https://bejeweled.wiki.gg/wiki/Twilight),
  [wiki.gg: Bejeweled 2](https://bejeweled.wiki.gg/wiki/Bejeweled_2),
  [Wikipedia: Bejeweled 2](https://en.wikipedia.org/wiki/Bejeweled_2),
  [kosgames: unlockable modes](https://kosgames.com/bejeweled-2-deluxe-all-the-unlockable-modes-2585/),
  [Astraware Palm manual](https://tapwave.net/files/bejeweled-2/dl/Astraware%20Bejeweled%202%20for%20Palm.htm)

---

## 5. Board geometry and rendering

### 5.1 Logical space

Every board is simulated with the original constants: pitch 51, gem 49, gravity +0.18 per tick
(`board.gravity.fallAccel` can change it in `UpdateFalling`, 4.16), spawn `-Rand()%240-65` and
`minY-69`, the vortex thresholds 64/80/96, the click zone 22..88.2 / 26, and the drag threshold 16.
Only `BoardLayout` differs:

```
w  = cols*51 - 2;   h = rows*51 - 2;   D = max(406, w, h)          // 8x8: w = h = D = 406
mOriginX = 203 + (D - w)/2;   mOriginY = 16 + (D - h)/2            // logical; GetColX(c) = c*51 + mOriginX
s  = 406.0 / D                                                     // view scale; 1 unless D > 406
ToScreen(lx, ly)  = (203 + (lx - 203)*s, 16 + (ly - 16)*s)
ToLogical(sx, sy) = (203 + floordiv((sx - 203)*D, 406), 16 + floordiv((sy - 16)*D, 406))   // integers
mScreenBottom     = 16 + (480 - 16)*D/406                          // 480 when D == 406
```

| Board | w × h | D | scale | logical origin | gem on screen |
|---|---|---|---|---|---|
| 6x6 | 304 × 304 | 406 | 1 | (254, 67) | 49 |
| 8x8 | 406 × 406 | 406 | 1 | (203, 16) | 49 |
| 9x9 | 457 × 457 | 457 | 0.888 | (203, 16) | 43.5 |
| 10x7 | 508 × 355 | 508 | 0.799 | (203, 92) | 39.2 |
| 10x10 | 508 × 508 | 508 | 0.799 | (203, 16) | 39.2 |
| 12x12 | 610 × 610 | 610 | 0.666 | (203, 16) | 32.6 |
| 16x16 | 814 × 814 | 814 | 0.499 | (203, 16) | 24.4 |

Every board is centred in the original 406x406 window. Its screen centre is always (406, 219). The
HUD is never scaled. Boards up to 8x8 in both directions are unscaled, so for them only the origin
moves.

### 5.2 Hit testing

- `MouseDown`, `MouseDrag` and `MouseMove` convert to logical coordinates when `mScaled` (2.5).
  `GetPieceAt`, the click zone and the drag threshold then run unchanged.
- Twist intersections and slide axes are computed in logical space as well.
- Replays record cells, never pixels, so hit testing (including `GetPieceAt` on swapping pieces,
  whose positions come from `cos`) can never affect a replay.

### 5.3 Shapes and level changes

- **Storage:** `Piece* mBoard[16][16]`, with `mCols/mRows` as loop bounds and `mLayout.mPlayable`
  as the mask.
- **Holes:** permanent NULLs. Every translated loop already skips NULL, except three places that
  assume a full board and get guards: `UpdateFalling`, `FindMove`'s swap target, and
  `MouseDrag`'s neighbour.
- **Level changes with `board.levels`:**
  1. `SetupLevel` runs inside `UpdateBlackHole`'s done-block, *before* that block's deletion
     loop. At that point `SetupLevelVariant()` only computes `mPendingLayout = levels[(mLevel) % n]`,
     and `DrawVariantChecker` draws the next background from the pending layout.
  2. The deletion loop runs with the old bounds.
  3. Then `ApplyPendingLayout()` switches `mLayout`, `mCols`, `mRows` and `mLevelShape`.
  4. In the constructor, `InitVariant` sets both layouts to level 0 before `SetupLevel`.
- **Loading:** the v2 save stores `mLevelShape` and the dims in its header, ahead of the cell
  records. `ReadVariantHeader` applies that layout before the cells are read, and then rebuilds
  the background for the loaded level and shape.
  - This is a deliberate difference from the original on variant boards only. The original
    "background stays level 0 after loading" bug would otherwise show a wrong mask.
  - Q12 asks whether to keep the level-0 texture instead.

### 5.4 Gravity and refill with holes (`RefillMasked`)

This is the original `UpdateClearing` refill pass, run over each column's playable rows `V`, top to
bottom. Gems fall through holes, as in Jewel Quest (Q6 offers blocking holes instead):

```
do { changed = false;
  for c in 0..cols-1:
    V = playable rows of c; if V empty: continue
    minY = GetRowY(V[0]); gap = false
    for k = |V|-1 .. 0:  r = V[k]; p = mBoard[r][c]
      if p == NULL: { if gap: mBoard[V[k+1]][c] = NULL;  gap = true }
      else: { if p->mY < minY: minY = (int)p->mY
              if gap: { p->mRow = V[k+1]; p->mVelY = 1.0; mBoard[V[k+1]][c] = p } }
    if gap: { changed = true; spawn at V[0]: mVelY 0.3, mX GetColX(c), mY minY - 69, mColor Rand() % gemColors }
} while (changed)
```

On a full column `V = 0..rows-1`, which is line for line the original pass, with the same `Rand`
order. `UpdateFalling` already settles each piece to `GetRowY(mRow)`. Stage 7c orders `V` from the entry
side, so the same pass also refills upward (4.16).

### 5.5 Fill with holes (`FillBoardVariant`)

- It parks the old pieces exactly as `FillBoard` does (all `MAX` cells, except a stale
  `mSelectedPiece`).
- Fill order: rows `rows-1..0`, columns `0..cols-1`, skipping holes. `aBelowY` is the `(int)mY` of
  the nearest playable cell below, which is already filled since filling runs bottom-up; else 0.
- Each cell draws `Rand()%240`, then rerolls `Rand() % gemColors` while `HasMatch()` (the variant
  when `mMatch`), with the cap from 3.6.
- On a full 8x8 this is the original algorithm. Stage 7c adds the mirrored upward fill (4.16).
- **`FillBoardWithMove()`** replaces the tutorial loop when `mNoTutorial` is set:
  `do FillBoardVariant(); while (!AnyMove() && ++n < 100);`. It leaves `mTutorialCol = -1`.

### 5.6 Drawing (`DrawVariant`, `DrawVariantChecker`)

**`DrawVariant`** is a copy of `Draw` (0x4cbc70) with these additions:
- **Board layer:** a `Graphics` copy with `SetScale(s, s, 203, 16)`, which `Graphics::DrawImage`
  honours (`Graphics.cpp` scale path). Gems, glow crossfades, shrink frames, the `target1` cursor
  and sparkles go through it.
  - `SetScale` affects only `DrawImage`, so popups are drawn with `DrawString` at `ToScreen`
    positions in the unscaled font.
  - At scale 1 the scale branch is never taken, so `DrawVariant` at 8x8 is pixel-identical to
    `Draw`. `-forcevariant` screenshots check this.
- **Layering:** gold tiles under the gems, special overlays over them, then effects, the twist
  ring and the slide preview.
- **Centre-based drawing:** the banner, the vortex image and the hole mask use the screen centre
  (406, 219), not the logical centre.
- **Unchanged:** the clip `(194, 12, 428, 416)`, the level wipe (screen space) and the HUD. The
  gold HUD is added on top. Stage 7c replaces the fixed clip with the board's own screen rectangle,
  which is the same on 8x8 (4.16).
- **Tutorial arrows:** only when `mTutorialCol >= 0`.

**Checkerboard (`DrawVariantChecker`, into `mNextBackgroundImage`).** The original draws 10x10
tiles from (151, -35): one ring cell outside the board on each side, with x 1 px left of the gems.
The ring shows inside the frame window (about x 192..202 and 610..624, y 9..15 and 424..430). The
generalisation:
- Fill the window with the void tile: `images/voidtile`, or the level texture colourised
  `(40, 32, 36)`.
- For `r = -1..rows`, `c = -1..cols`, draw the cell at logical `(GetColX(c) - 1, GetRowY(r))`, size
  51, transformed to the screen. Playable and ring cells get tile A or B by `(r + c) % 2 == 0`,
  with C `%` on negative values as in the original. Holes get the void tile.
- At non-integer scales, tile rectangles are computed from rounded screen edges so that
  neighbouring tiles do not leave seams.
- On 8x8 the 10x10 tiles cover the whole transparent window, so the result is pixel-identical to
  the original loop.

**Effects that assumed 8x8:**

| Effect | Handling |
|---|---|
| Vortex (`StartBlackHole`/`UpdateBlackHole`) | `GetBoardCenter`/`GetBoardSize` are logical, so `aMaxRadius = (int)((int)sqrt(w²+h²)/2.2)` grows with the board (260 at 8x8). Pieces spiral in logical space. The 256 px vortex image is drawn unscaled at (406, 219). |
| Game-over shake and fall | Per piece; the fall limit is `mScreenBottom + GEM_SIZE` (mirrored under up gravity, 4.16). |
| Fall-out | `mCols` draws per tick; the limit is `mScreenBottom + 32` (mirrored under up gravity, 4.16). |
| Level wipe | Unchanged (full screen); `FillBoard` dispatches. |
| `HighScoreWidget::Resize` (0x4cedf5) | Draws `mApp->mBoard->Draw` into its snapshot, so it picks up `DrawVariant` by itself. |

---

## 6. Determinism, replays, undo and the headless verifier

### 6.1 Rules for code that affects play

1. **Randomness.** Only `Board::Rand()` inside the board. Outside it, only `Sexy::Rand`; the only
   such draw today is the `HighScoreWidget` constructor's `Rand()%9`. A CI grep forbids `rand(`,
   `<random>` and `std::shuffle` in `games/bejeweled/src`. The autoplay bot's `std::mt19937` lives in test
   code. `srand(GetTickCount())` in `Init` stays unused by game code.
2. **Clock.** Only the board tick, `mUpdateCnt`: `WidgetContainer::mUpdateCnt`, incremented by
   `Widget::Update()` in step 3 of `Board::Update`, after the pause checks. There is no wall time
   in play; `GetDayNumber` is used for high scores only.
3. **Floating point.**
   - Gameplay doubles use only `+ - * /`, `sqrt` and `(int)` casts, which give identical results
     with SSE2 (the existing `-msse2 -mfpmath=sse`) and in wasm.
   - Add `-ffp-contract=off` to the game sources on every target. It is a no-op today, and
     insurance for the planned ARM/3DS build.
   - Never `-ffast-math`.
   - New code clamps doubles before `(int)` casts: out-of-range conversion differs between x86 and
     wasm.
4. **Trig stays libm.** `cos` (`UpdateSwap`), `cos/sin` (`UpdateBlackHole`) and `atan2`
   (`StartBlackHole`) only set positions that no gameplay decision reads:
   - `MouseDown` returns in state 7;
   - swap positions are snapped at 180°;
   - commands carry cells;
   - `aDone` in the vortex comes from the radius, which is pure arithmetic.

   Those fields (positions in states 2, 3 and 7, and `mHole*`) are left out of the LOGIC hash. The
   new twist animation uses an integer table.
5. **Memory.** The `Piece` memset covers the tail. Every new Board member is initialised in
   `InitVariant`. The v2 save and `StateCodec` write fields, not structs, except the zeroed 0x78
   Piece record.
6. **Order.** No `unordered_*` containers and no pointer-keyed ordering in play code. Keyed rule
   objects are iterated in sorted key order. `JsonValue` keeps insertion order, so canonical forms
   sort explicitly.
7. **Rule numbers.** New rules are integers (points, seconds, ticks). Doubles exist only in the
   legacy tunables. `Json.cpp` parses with `strtod`, which is correctly rounded on both libcs. The
   replay carries the parsed bits, and the server never re-parses.

### 6.2 RNG model

| Game | Board RNG | Replay start record |
|---|---|---|
| Original rules (incl. tunables-only mods), no seed source, `seeds.perGame` off | the global stream (`mOwnRng == NULL`), exactly as today | MT state (624 words + `mti`, 2.5 KB) captured at constructor entry, plus `XRAND` events |
| Explicit seed (`-gameseed`, `?seed=`, Extras) | owned `MTRand(seed)`, `mUserSeeded` | seed |
| Any rule active, or `seeds.perGame`, with no explicit seed | `seed = (uint32)Sexy::Rand()` drawn in `NewGame` (deterministic under `-seed`), owned `MTRand(seed)` | seed |
| Ranked | owned `MTRand(serverSeed)` | seed + session token |

- **Playback** boards always own an `MTRand` initialised from the start record. Playback never
  touches the global stream.
- **`-forcevariant`** never changes the RNG source.

**External draws** (session mode only):
- `Sexy::GetRandCount()` counts every global draw.
- The board records `mGlobalAtStart` in `InitVariant`, and counts its own draws in `mBoardDraws`.
- At `Board::Update` entry (before the pause checks), `SyncExternalRand` computes
  `external = GetRandCount() - mGlobalAtStart - mBoardDraws - alreadyLogged`. A positive value is
  logged as `XRAND n` at the current board tick.
- Commands never draw, so this placement stamps every outside draw before the board's next draw.
  That covers Options → High Scores during a game (its hidden `HighScoreWidget` rolls `Rand()%9`
  while the board is paused), whether it was triggered by real input or by `-script`.
- The game-over `HighScoreWidget` draws after the board's last draw, so it is outside the replay.

### 6.3 Commands, recording points and gates

**Tick stamping.** Live input arrives between board updates: in `ProcessEvents`, or in `RunScript`
before `WidgetManager::UpdateFrame`. So an event recorded at board tick T happened after Update #T
and before #T+1. Playback applies all events with `tick == mUpdateCnt` before calling `Update()`.

This is equivalent to live play, including during pauses:
- a paused board does not tick;
- events keep their order within a tick;
- `Pause` commits `(mUpdateCnt - mPlayStartTick)/100` in board ticks, so recording `PAUSE`/`UNPAUSE`
  reproduces `mPlaySeconds`, `mGameDuration` and `mInputDelay` exactly.

| Op | Args | Recorded in | Playback `Apply` | Gate re-checked by playback and verifier |
|---|---|---|---|---|
| `SELECT` | c r | `MouseDown` selection branch (the cell where `GetPieceAt` found the piece) | `mCursorFrame = 0; mSelectedPiece = mBoard[r][c]; MarkDirty(); PlaySample(SOUND_SELECT, ...)` | `mUserPauseCount == 0`, `mInputDelay == 0`, state not 5..8, no selection, cell non-NULL |
| `DESELECT` | | `MouseDown` right button | `mSelectedPiece = NULL; MarkDirty()` | as above, without the selection test |
| `TARGET` | c r, or -1 | `MouseDown` tail (click zone or clicked piece), `MouseDrag` | `t = cell or NULL; if (t && t != sel) StartSwap(sel, t); sel = NULL; MarkDirty()` | `mState == STATE_PLAYING`, selection set (and the `MouseDown` gates for clicks) |
| `TWIST` | c r dir | variant input | `StartTwist` | PLAYING, no user pause, `mInputDelay == 0`, four cells present |
| `SLIDE` | axis line start k off | variant input | `StartSlide` | PLAYING, the same, valid segment, `1 <= k < len` |
| `HINT` | | inside `ShowHint`'s `if` | `DoHint()` | `mState == STATE_PLAYING && mPauseCount == 0` |
| `PAUSE` / `UNPAUSE` | u | `Pause`/`Unpause` (KeyChar, click-to-unpause, ModalOpen/Close, focus) | `Pause(u)` / `Unpause(u)` | none |
| `UNDO` | | `RequestUndo` | `Undo()` | 4.12 conditions |
| `NEW_GAME` | | `RequestNewGame` | `RequestNewGame()` | none |
| `XRAND` | n | `SyncExternalRand` | n × `mOwnRng->Next()` | session-mode replays only |
| `HOVER` | x y (-1 = leave) | `MouseMove`/`MouseLeave`, last value per tick (optional channel) | `mMouseX/Y = ...` | none |
| `SAVE` | kind, u32 hash | `SaveGame` (kind 1), `GetShutdownSaves` snapshot (kind 2) | the same call into `mPlaybackSave` | none |
| `LOAD` | | `LoadGame` after `ConfirmLoadGame` → YES | `LoadGame` from the bytes the previous segment's last `SAVE` produced (`mPlaybackLoad`) | first board tick only |
| `CHECK` | u64 | `AfterUpdate` when the state enters PLAYING | compare with the LOGIC hash | |

- **Stale selections.** Commands carry the grid cell and the selection is a recorded event, so the
  stale-selection quirk (GAME_NOTES 3.6, a piece orphaned by `FillBoard` swapped back in by
  `StartSwap`) replays exactly.
- **Corrupt replays.** The drive loop flags a replay as corrupt when:
  - an event fails its gate;
  - the board is paused and no further event at that tick unpauses it;
  - a `CHECK` hash differs.

### 6.4 Replay file (`.bjr`, little-endian)

```
"BJRP"  u16 format = 1  u16 flags (bit0 hover, bit1 ranked, bit2 multi-segment)  u32 engineAbi
records { u32 tag, u32 len, payload }:
  RULE  GameRules::Canonical() bytes (incl. tunables and the 12 diff values)
  RLID  32-byte SHA-256 of RULE
  META  UTF-8 JSON, not verified: name, date, mods [id@version], build, platform, title
  SESS  ranked session token (ranked only)
  SEGM  one per board lifetime:
          u8 mode, RNG start (u8 kind: 0 = u32 seed | 1 = 624 x u32 + i32 mti), u8 userSeeded,
          ENV (u8 showInstructions, i32 timesPlayed, i32 pointsAscent: for full-fidelity watching),
          u32 count + events { varint tickDelta, u8 op, zigzag-varint args }
  RNGS  (resume of a default-rules owned-RNG game) RNG state the LOAD restores
  END   u8 reason (0 abandoned, 1 no moves, 2 time up, 3 new game, 4 gold time up, 5 saved),
        u32 endTick, i32 score, i32 level, i32 gems, u64 LOGIC hash, u32 board draws
```

- **Size:** a move costs about 8 bytes (`SELECT` + `TARGET`), so 1,000 moves are about 8 KB, plus
  2.5 KB of MT state in session mode. Hover adds a few KB and is stripped for ranked submissions.
- **`engineAbi`** is bumped whenever variant simulation output changes. The original rules are
  frozen. Old verifier builds are kept per ABI.

### 6.5 Segments: save, resume and the web backup

**Saving.**
- A game that ends by saving (Shutdown, mode switch, watching a replay) leaves `SAVE` markers in
  its last segment.
- Shutdown saves twice: `SaveGame` in `WinDMApp::Shutdown`, then the deferred `~Board`. Both are
  recorded, and the verifier repeats both, because the file holds the second save.

**The sidecar** (in-progress replay, 7.3) is written next to the save, with the save's hash.

**Resuming.** On the next launch:
1. `TitleScreenIsFinished` creates a fresh board. Its constructor runs, with `FillBoard` and
   tutorial draws.
2. `ConfirmLoadGame` opens a modal before the first tick (`PAUSE false`). YES runs `KillDialog`,
   then `ModalClose` (`UNPAUSE false`), then `LoadGame` (`LOAD`), all at board tick 0.
3. The recorder reads the sidecar and checks that its hash matches the loaded save. It appends this
   new segment to the old replay. `DeleteSaveGame` also deletes the sidecar.
4. Session mode: the segment's RNG start is the MT state at the new constructor.
   Owned-RNG variant boards: the v2 chunk restores the RNG. Owned-RNG default-rules boards: the
   sidecar's `RNGS` restores it.
5. A missing sidecar or a hash mismatch starts a fresh replay. Its first segment embeds the loaded
   save bytes, so it can be watched but not ranked.

**Verifying a resume.** The verifier never trusts the client's save bytes.
1. It simulates the previous segment up to its last `SAVE`.
2. It calls the same `SaveGame`/`SaveGameSnapshot` code to produce the bytes.
3. It runs the new segment's constructor and `LOAD` from those bytes.
4. For owned-RNG games it continues with *its own* RNG state at the save point.

A client that restored a different RNG state diverges, and the next `CHECK` fails.

**Web backup.**
- `GetShutdownSaves` returns the `SaveGameSnapshot` bytes and the sidecar ending in
  `SAVE(snapshot)`, both taken at the same tick, so a backup restored after a crash is always
  consistent.
- Finished replays live in `/persist/replays`. IDBFS syncs the whole `/persist` mount, so they
  persist. They are outside the localStorage backup, which copies only the user dir as one item,
  so a browser crash can lose the most recent finished replay.

### 6.6 StateCodec and hashes

`StateCodec.cpp` holds one table with one row per Board field and per Piece field: name, accessor,
type and flags `V2`, `UNDO`, `LOGIC` and `FULL`. The table drives:
- the v2 save chunks,
- undo snapshots,
- **FULL**: everything, raw Piece bytes included. Same platform only. Used by `-tracehash` and the
  oracle.
- **LOGIC**:
  - Board fields: state, level fields, every score and stat field, multiplier and drain doubles as
    bits, bar width and colour, the timers that affect play (`mTimeDrainDelay`, `mIdleTicks`,
    `mMoveScoreTimer`, `mGameOverCountdown`, `mWipeY`, `mElectOffset`), board tick, pause counts,
    `mInputDelay`, play seconds, the selection and swap pieces as cells, the RNG state, and the
    variant state.
  - Per cell: present, colour, kind, col, row, dying, dying frame, `mVelY`, and positions outside
    states 2, 3 and 7.
  - Excluded: glow fields (they depend on hover), `mHole*`, popups, the banner, the hint-glow and
    move-score alphas, the mouse fields, and the tutorial bob.
  - Used for `CHECK`, `END`, record-vs-playback, native-vs-wasm, and the difftest.

A standing test, from stage 3 on, runs in variant playtests with `-trace=2`: at every settle,
StateCodec write → restore into a scratch board → write must give identical bytes, and the FULL hash
must be identical. A variant field missing from the table fails this test.

### 6.7 Undo snapshots

The UNDO profile is the grid (present, colour, kind; in `STATE_PLAYING` every piece is at rest, not
dying, `mVelY == 0`, which is asserted), the gold layer, the UNDO-flagged Board fields and the RNG
state. That is at most 1.5 KB at 16x16, so depth 100 costs at most 150 KB.

- **Restore** creates fresh pieces at their rest positions. Cosmetic fields start at zero.
- **Determinism:** undo is an event, so replays with undo reproduce exactly.

### 6.8 Playback driver

```
for (;;) {
  while (ev < end && ev->tick == board.mUpdateCnt) if (!Apply(*ev++)) return CORRUPT;   // gates
  if (board ended || (ev == end && board.mState == STATE_GAME_OVER_DONE)) break;
  if ((board.mPauseCount > 0 || board.mUserPauseCount > 0) && (ev == end || ev->tick != board.mUpdateCnt))
      return CORRUPT;
  board.Update();                       // CHECKs are compared in AfterUpdate
}
```

- **In the game:** `WatchController::BeforeFrame()` runs one or more iterations per app frame,
  depending on the speed.
- **Native CLI:** `-replay=<file> -fast -trace` prints the same TRACE lines as the recording.
  Everything is compared except the app-level `update N` field. The `board tick` field matches.

### 6.9 Headless verifier (`bejeweled_verify`)

- **Goal:** compile `Board.cpp`, `BoardRules.cpp` and the variant files unchanged, with no SDL,
  fonts, images or user assets, since the server has none.
- **Sources:**
  - the Board files, `BoardTrace`, `GameRules`, `GameConfig.cpp` (the SDL-free half),
    `MatchFinder`, `MoveSet`, `StateCodec`, `Replay`;
  - `MTRand.cpp`, `Buffer.cpp`, `Json.cpp`, `sha256.c`;
  - `games/bejeweled/src/headless/`.
- **Include order:** `games/bejeweled/src/headless/shim` comes before `framework/`.
  - `Board.h`'s `#include "Widget.h"` and `WinDMApp.h`'s `#include "SexyApp.h"` resolve to the
    shims.
  - The quoted `#include "WinDMApp.h"` from `games/bejeweled/src` still finds the real header.
  - So the game sources need no `#ifdef`.
- **Shims** (about 450 lines):
  - `Widget`/`WidgetContainer`: `mUpdateCnt` incremented exactly as `WidgetContainer::Update`;
    `Resize` sets `mWidth/mHeight`.
  - `Graphics`, `Image`, `MemoryImage`, `DDImage`, `ImageFont`: no-op drawing; metric getters
    return the real fonts' constants, taken from the replay's ENV where they matter.
  - `SoundInstance`/`SoundManager`: no-ops.
  - A `Common` replacement: `Rand`, `SRand`, `GetRandCount`, `StrFormat`, `vformat`. The real
    `Common.cpp` includes SDL.
- **`WinDMAppHeadless.cpp`:**
  - defines every `WinDMApp` virtual and member that Board calls;
  - turns `NewGame`/`ShowHighScoreWidget` into end flags;
  - makes `SetTimedMode` and `KillDialog` no-ops;
  - serves `ReadBufferFromFile` and `WriteBufferToFile` from memory.
- **Targets:**
  - **Native i686 exe:** `bejeweled_verify replay.bjr [-trace]` → result JSON.
    `bejeweled_verify difftest --seeds 1..500 [--ticks N]`: two boards in lockstep, one plain and
    one with the `-forcevariant` flags, each driven by the same autoplay bot (own `std::mt19937`),
    with the LOGIC hash compared every tick.
  - **Wasm:** `-O2 -sSTANDALONE_WASM --no-entry -sFILESYSTEM=0 -sALLOW_MEMORY_GROWTH=1`, exporting
    `bv_alloc`, `bv_free`, `bv_verify(ptr, len)` and `bv_rules_id`. Imports are stubbed (WASI
    `fd_write`, `proc_exit`, clock).
  - `MTRand` asserts a 32-bit `unsigned long`, so CI runs the wasm build under Node, not an x86_64
    native build.
- **Shim drift guard (CI):** every replay in the corpus is played in the real game
  (`-replay -fast -trace`), in the native verifier and in the wasm verifier, and the TRACE, `CHECK`
  and `END` values must agree.

### 6.10 Leaderboard service (Cloudflare Worker + D1)

**Hosting.** Workers static assets serve the engine and the shell, with `/api/*` on the same
origin. The user still imports their own game files in the browser. The Worker, the verifier and
replays contain no PopCap assets.

**API:**
- `POST /api/session {rulesId, mode, engineAbi}` → `{token, seed, expires}`, where
  `seed = HMAC(secret, id)` truncated to u32 (not 0) and
  `token = HMAC(secret, id|seed|rulesId|mode|issued)`.
- `GET /api/daily?rulesId&mode` → the day's seed, `HMAC(secret, date|rulesId|mode)`.
- `POST /api/submit {replay, token, name, playerSecret}`.
- `GET /api/scores?rulesId&mode&period=all|day`.
- `GET /api/replay/:id`.

**Submit checks, in order:**
1. size ≤ 256 KB;
2. `engineAbi` supported;
3. token signature valid, unexpired (48 h) and unused, or the daily seed for that date;
4. replay seed, mode and `RLID` match the token, and `RLID == SHA-256(RULE)`;
5. tick cap (6 h = 2.16 M ticks);
6. `bv_verify` reaches a natural end (reason 1, 2 or 4), and score, level, gems, final LOGIC hash
   and every `CHECK` match;
7. plausibility: total ticks / 100 ≤ now − issued + 60 s;
8. insert the score and mark the session used;
9. rate limit per IP hash and per player.

Session-mode (unseeded original) replays are never ranked.

**D1 schema:**

```sql
CREATE TABLE rulesets (id TEXT PRIMARY KEY, canonical BLOB NOT NULL, title TEXT, first_seen INTEGER);
CREATE TABLE sessions (id TEXT PRIMARY KEY, seed INTEGER NOT NULL, rules_id TEXT NOT NULL, mode INTEGER NOT NULL,
                       engine_abi INTEGER, daily TEXT, issued INTEGER NOT NULL, used INTEGER DEFAULT 0, ip_hash TEXT);
CREATE TABLE players  (id TEXT PRIMARY KEY, name TEXT UNIQUE NOT NULL, secret_hash TEXT NOT NULL, created INTEGER);
CREATE TABLE scores   (id INTEGER PRIMARY KEY, rules_id TEXT NOT NULL, mode INTEGER NOT NULL, daily TEXT,
                       player_id TEXT, name TEXT NOT NULL, score INTEGER NOT NULL, level INTEGER, gems INTEGER,
                       ticks INTEGER, seed INTEGER, engine_abi INTEGER, session_id TEXT UNIQUE,
                       replay BLOB, created INTEGER NOT NULL);
CREATE INDEX scores_board ON scores(rules_id, mode, daily, score DESC);
```

- **Identity:** a random player secret in localStorage claims a name (Q10).
- **CPU:** a 10-minute game is about 60k ticks. The estimate is 0.1–0.3 s in wasm, to be measured
  in stage 14. That needs the Workers Paid CPU limit; the free plan's 10 ms is too little.

---

## 7. Save format

### 7.1 Default rules: v1, unchanged

`savegame.dat` (or `savegame-<key>.dat` for tunables-only mods) is byte-identical to today: version
1, the original field order, 0x78-byte Piece records. This holds also when only feature mods are
enabled.

### 7.2 Variant boards: v2

The file name is scoped, since rule mods are score-affecting. `LoadGame`'s original
`aVersion != SAVEGAME_VERSION` check rejects v2 files on non-variant boards, and the variant check
rejects v1 files on variant boards.

```
u32 version = 0x101
u8  rulesId[32]                  // refuse when it differs from the board's rules (mod edited without a version bump)
u16 cols, u16 rows, i32 levelShape, u8 mask[(cols*rows+7)/8]     // applied before the cells are read
--- then the v1 field sequence from mTimedMode on, with the cells over rows x cols (holes written as absent) ---
--- then chunks { u32 tag, u32 len, data }, unknown tags skipped:
RNGS  u32 seed, u8 userSeeded, u8 ranked, 624 x u32 mt, i32 mti
KIND  u8 per present cell, row-major
GOLD  bitmap rows x cols;  GTIM  i32 ticksLeft, i32 limit
MOVE  i32 moves, i32 invalidMoves, i32 undosUsed
END
```

- States 10..13 are mapped to 1 on load, like 2/3. The grid is always the grid before the move.
- Resume semantics are those of v1 plus the variant state: `mUpdateCnt`, `mTimeDrainDelay`, timers
  and popups are not restored (6.5).
- The header is checked before anything is changed. Chunk errors make `LoadGame` return false, as a
  truncated v1 file would.

### 7.3 Sidecar (in-progress replay)

The file is `<save name>.bjr`, scoped like the save (for example `userdata/savegame.bjr`). It
contains:
- the segments so far, ending in a `SAVE` marker;
- the save's hash;
- for default-rules owned-RNG games, an `RNGS` record.

It exists only when recording, or when a default-rules game owns its RNG. It is written by
`SaveGame`, staged into the web backup by `GetShutdownSaves`, and deleted by `DeleteSaveGame`.

### 7.4 Not saved

These are lost on save, as in the original or by design:
- the undo stack;
- effects and popups;
- `mUpdateCnt` and the timers listed in 6.5;
- the hover state.

---

## 8. UI

All new UI depends on mods. Without them the screens are pixel-identical.

| Surface | Change | Shown when |
|---|---|---|
| Web mods panel | Tags "Changes gameplay · separate high scores" (`affectsScores`, `tunables`, `rules`) and "Adds features". Rule warnings, errors and dropped mods from `mods-status.json`. | mods installed |
| Options dialog | "Extras" button in the register slot of the three-button row (2.6). | any feature mod |
| Extras dialog (new, 348 wide like Options) | "Seed for the next game" `EditWidget` (digits) with "Play this seed" (confirms like New Game); "Last game: SEED n"; "Replays..."; "Online scores..." (web: opens the shell panel through `EM_JS`). | `seeds.entry` / `replays` / `leaderboard` |
| Game-over screen | "SEED n" line at about y 120; "WATCH REPLAY" and "SUBMIT SCORE" `HyperlinkWidget`s next to it. Positions checked on a screenshot. | owned-RNG game / replays / eligible ranked game |
| Undo | Red-orb hotspot with a hover glow and a remaining-count digit when `max ≥ 0`; keys z, u, Backspace; a short "UNDO" banner. | `undo.enabled` |
| Gold HUD | "GOLD n/m" over the bar; the bar shows progress (untimed) or time (timed); gold tiles under the gems. | `gold.enabled` |
| Twist / slide | Ring cursor at the hovered intersection; the dragged segment follows the pointer, with its wrap copy. | move types |
| Instructions | `INSTRUCTIONDIALOG_TWIST`, `_SLIDE`, and `FULL_INSTRUCTIONS_TWIST/_SLIDE` paragraphs appended by Help. `INSTRUCTIONDIALOG_TWILIGHT`, `_GRAVITY_UP` and a replacement `FULL_INSTRUCTIONS2` text for gravity rules (4.16). Strings have defaults and can be overridden by mods. | move types, gravity |
| Twilight look | The level backdrop mirrored left to right at half brightness; sounds pitched down. No gravity indicator. | `look.*` |
| Specials | Overlays, blast and beam effects, optional `PATTERN_<name>` banners. | specials / patterns |
| Replay browser (new) | `ListWidget` + `ScrollbarWidget`, rows "date · mode · rules title · score · seed". Buttons Watch / Keep / Delete / Close. | `replays` |
| Watch mode | Banner "REPLAY 2x". Space or click pauses, 1–4 set the speed, N steps to the next move, Esc exits. End screen "REPLAY END" (Watch again / Back). A transparent overlay widget above the board takes the clicks and keys. Side-bar buttons are routed to the controller. | watching |
| Web shell | Replays panel (list of `/persist/replays`, import/export `.bjr`, links `?replay=<id>`), Leaderboard panel (board picker, top 50, Watch), `?seed=`. | features |

---

## 9. Staged implementation plan

### 9.1 The regression oracle and the identity proofs

1. **Baseline.** The project is not under version control yet. Stage 0 starts by committing the
   current tree (Q1) and copying today's `build/native/bejeweled.exe` to
   `build/baseline/bejeweled.exe`.
2. **`games/bejeweled/tools/oracle/compare.sh`** runs baseline and candidate the same day, because
   `hiscores.dat` stores day numbers; `compare.py` can also mask the two day fields. It uses the existing
   `run.sh`/`runm.sh` order:
   - `playtest.txt` → `user_main`;
   - `playtest_load.txt` in a copy of that dir;
   - `playtest_level.txt`;
   - `playtest_mods.txt`;
   - the new scripts.

   It compares:
   - the logs, after dropping only `Seconds =` and `Resource Loading Time` lines;
   - `savegame*.dat`, `hiscores*.dat` and `registry.ini` with `cmp`, plus each user dir's file
     list;
   - `shots/*.png`, pixel-exact.
3. **`-forcevariant`:** all oracle runs give identical results with the generalised paths.
4. **`BEJ_NO_VARIANTS` build:** identical results.
5. **Recording is inert:** the oracle with a `replays` + `seeds` feature mod (and `perGame` off)
   gives identical TRACE lines, saves and registry. The user dir gains only `replays/`, the
   sidecar and `mods-status.json`. The only on-screen difference is the crossbar and fourth
   sparkle that every mod already shows.
6. **Replay round trip:** every recording from 5 is played in the game, in the native verifier and
   in the wasm verifier under Node. All must give the recorded TRACE, `CHECK` and `END` values.
7. **Difftest:** 500 seeds × both modes, translated paths against variant paths, LOGIC hash every
   tick.
8. **Reviewability:** the `list_variant_hooks.py` count is tracked per stage. GAME_NOTES gets a
   "variant hooks" section.

### 9.2 Stages

Every stage builds native and web, passes 9.1 (the items that exist so far), and adds its own
tests. Sizes are new or changed lines.

| # | Stage | Contents | Tests | Size |
|---|---|---|---|---|
| 0 | Oracle hardening | Commit/baseline. `compare.sh`/`compare.py`. Global RNG counter. `HandleScriptCommand` with script `save <file>` (snapshot during swap, clearing, vortex and wipe states). `-tracehash=<file>` (per-tick FULL hash from a hand-written field list). `-autoplay=<seed>` (bot with its own `std::mt19937`, input through the same mouse path, to game over or a tick cap). New scripts: normal no-moves game over, timed fall-out, drag swaps in 4 directions and at the edge, selection during falling (stale selection), save mid-swap/clearing/vortex then load, timed level 2+, Options → High Scores mid-game, pause/unpause, mode-switch save. | Plain `-trace` byte-identical to the baseline; new hash, snapshot and autoplay baselines recorded with both exes | ~500 |
| 1 | Rules plumbing, no behaviour | `GameRules` (schema, merge, validation, canonical, RulesId), ModManager `rules`/`features`/`imageOps` with provenance, groups, keyed, `null`, union, `mods-status.json`, `diff.txt` detection, shell tags and status. `GameConfig` split. `mBoard[16][16]`, bounds, `BoardLayout`, `GetColX/GetRowY` members, `Piece::SAVE_SIZE` + tail, `VariantFlags`, `-forcevariant` (inert so far), `-dumprules`, `BEJ_NO_VARIANTS`, hook lister. | Oracle; `rulestest` unit fixtures (merge, groups, keyed, null, union, drop-lowest, hash vectors stable across native/wasm); `playtest_mods` keeps `savegame-58ebd9d7.dat` | ~1,000 |
| 2 | RNG core + set seed (9) | `MTRand` Get/SetState, `Board::Rand`, owned RNG, `NewGame` seed choice, pending-seed rule, `-gameseed`, `?seed=`, game-over SEED line, `-seeded` table | Oracle (global stream untouched); two `-gameseed` runs with different `-seed` give identical board traces; double-NewGame startup plays the seed; SEED screenshot only for seeded games | ~350 |
| 2b | Auto LiveSplit (4.15) | `features.livesplit`; `LiveSplitBlock.h` (layout 1) and `LiveSplitObserver` (detection, counters, `displayScoreMax`, sequence counter, status lines, template copy) under the desktop `#if`; Board serial and `mResumed`; `UpdateFrames` and `~WinDMApp` hooks; `-livesplitlog`; the `livesplit` mod with 12 templates; `BejeweledDeluxe-byopop.asl` (the original 1.87 only after its test) | Oracle case "livesplit" (also `-forcevariant` and `BEJ_NO_VARIANTS`); `-livesplitlog` against `-tracehash` states; `lsprobe` with `check_lslog.py`, incl. a `-fast` stress run; `lsdecide.py` scenarios; `asltest` (Framework csc, 1.8.37's usings) equals `lsdecide.py`; `check_asl_offsets.py`; magic once in `.data`. Publishing and the manual LiveSplit test wait for the user | ~1,550 (≈340 game) |
| 3 | Replay core (10) + StateCodec | StateCodec table and hashes; recorder hooks; `SyncExternalRand`; `HOVER`; `CHECK`; `.bjr` writer and reader; playback driver with gates; `-replay=`; `mPlaybackOnly`, `mPlaybackLoad/Save`; segments and sidecar; `GetShutdownSaves`; `-replaydir`, `/persist/replays`; pruning | 9.1 items 5 and 6 (in-game); scripted resume (`playtest_load`) as a two-segment replay; external-draw script replays; StateCodec round trip at every settle | ~1,300 |
| 4 | Headless verifier | Shims, `WinDMAppHeadless`, `verify_main`, `BoardTrace` move, standalone wasm, Node runner, difftest harness, CI job | Verifier = game for all corpus replays; native-recorded → wasm-verified; difftest runs (trivially equal until stage 7); the Node runner takes the stage-0 switches (`-script`, `-tracehash`, `-autoplay`), which the web shell does not pass, and the oracle scripts' LOGIC hashes match native | ~900 |
| 5 | Allow invalid moves (1) + noMoves + v2 save | `UpdateSwap` branch, `KeepInvalidMove`, `OnNoMovesVariant`, v2 header and chunks (`RNGS`, `MOVE`), variant trace lines | Kept-swap script; `"continue"` script; v2 save → quit → load round trip; replay verify; v1 untouched (oracle) | ~450 |
| 6 | Undo (11) | UNDO profile, `UndoStack`, `RequestUndo`/`Undo`, pop/clear points, keys and orb hotspot, `UNDO` event | Undo script (repeat the same move → identical refill); timed `inTimed`/`restoreTime`; replay with undos verifies; snapshot round trip | ~450 |
| 7 | Geometry and shapes (3) | `BoardLayout` scaling, `ToLogical`, `FillBoardVariant`, `RefillMasked`, `FillBoardWithMove`, NULL guards, `DrawVariant`, `DrawVariantChecker`, per-level shapes (pending → apply), v2 dims | `-forcevariant` oracle incl. pixel-exact screenshots; difftest now meaningful; scripts and screenshots for 6x6, 10x7, 16x16, diamond and a two-level shape mod (board, vortex, wipe, game-over fall); save/load on level 2; replay verify | ~1,400 |
| 7c | Twilight gravity (4.16) | `board.gravity.*` and `look.*` (group, validation, fill minimum per order); `mGravity`, `mFall`, `mChecker`; `FallPiecesVariant` (direction, `fallAccel`); direction-aware `RefillMasked` and `FillBoardVariant`; fall-out and game-over variants; `OnGravitySettle`; `GetPieceAt` guard; per-board clip; `GRAV` chunk, StateCodec rows and trace field (gravity rule sets only); backdrop pass; default sound pitch; help strings | `-forcevariant` oracle pixel-exact, default-rules hashes unchanged; difftest; mirror unit tests and `mirrortest`; Twilight scripts and screenshots (opening rise, alternation, flip per fill, no flip on swap-back or kept invalid move, Timed fall-out, game over, save mid-cascade, undo, diamond); replay verify; `rulestest` | ~690 |
| 7b | Dirt tiles and diagonal falling (4.14) | Mask legend `d`/`2`..`9`, dirt cells in input/match/hint code, layer clearing next to groups (and blasts), `RefillDiagonal` with the alternating toggle, `mVelX` in `UpdateFalling` (inside 7c's `FallPiecesVariant`), dirt art and dust effect, `DIRT` chunk | Fixture boards (alternation, top-row dirt, layers, two adjacent groups); screenshots; save mid-cascade; replay verify; boards without dirt unchanged (oracle, difftest) | ~700 |
| 8 | More gem colours (2) | Clamp 12, `imageOps.extendRows` (1x + HD), default op, glyphs, N shrink images, tints, `EnsureGemColors` | PNG of the generated sheet (1x and `example-crystal-gems` HD); 9- and 12-colour screenshots; loading bar unchanged | ~450 |
| 9 | Patterns (6) | `MatchFinder` (runs, diagonals, crossings, cell patterns), `ProcessMatchesVariant`, `ScoreGroup`, `HasMatchVariant`, `FindMoveVariant` (swap), `DoHintVariant`, fill cap, colour-minimum validation | `-forcevariant` oracle (exact run order and scoring); finder fixtures (L3, square, diagonal, minRun 2, merge L/T/plus, subset skip); hint equals brute force; scripts | ~1,200 |
| 10 | Twist + slide (5, 4) + combinations | States 10–13, variant input, integer twist table, segments, `MoveSet` twist and slide, multi-type input, instruction strings, trace lines | Rotate and drag scripts (coordinates from the trace); kept vs reverted with `allowInvalid`; diamond + twist; swap+twist+slide input matrix; save mid-move; replay verify; generic search equals brute force | ~1,500 |
| 11 | Special gems (7) | Creation, detonation FIFO, chains, scoring, effects, art fallbacks, sounds, `KIND` chunk | Fixture boards with exact expected scores and creation cells; chain order; screenshots; save with specials; undo round trip; replay verify | ~1,000 |
| 12 | Gold board (8) | Gold layer, `UpdateGoldGoal`, bar and HUD, gold tile, `noMoves` defaults, `GOLD`/`GTIM`, levels + gold | Small gold mod: a script completes level 1 → vortex → level-2 mask; timed time-up; save/load mid-level; the Twisted Quest combination script | ~750 |
| 13 | UI | Extras dialog, Options button, replay browser, watch controller and session save/restore, game-over links, shell Replays panel and badges | Screenshots with and without feature mods (default Options and game-over pixel-identical); watch-to-end matches `END` at every speed; resume offered after watching; browser smoke test | ~1,000 |
| 14 | Leaderboards (12) | Worker, D1 schema, sessions, daily seeds, submit checks, rate limits, shell panel, `bj_submit`, docs (README Mods section, MOD_ROADMAP, GAME_NOTES) | `wrangler dev` with the corpus: accepted; tampered score, event, rules, reused or expired token, and a wrong RNG on resume all rejected; CPU timing of a long timed replay | ~1,400 (≈700 TS) |

The total is about 16.6k lines. Stage 6 comes early on purpose: its round-trip test guards every later
change to a chunk. Stages 7–12 can be reordered after stage 6. Stage 2b needs only stage 1. Stage 7c
needs stages 3, 5, 6 and 7 and comes before 7b, so 7b writes its diagonal pass direction-aware once.
Stage 14 needs stages 3, 4 and 2's owned RNG.

---

## 10. Risks

| Risk | Mitigation |
|---|---|
| A hidden 8x8 assumption in translated code | Grep audit (`8`, `7`, `BOARD_`, `mHeight`). `-forcevariant`. Per-shape screenshots of the vortex, wipe and game over. Fuzz invariants in the difftest: no NULL in a playable cell at settle, nothing in holes, `mRow/mCol` matches the grid index. |
| Replay divergence from an unrecorded input or RNG source | Board-own draw counter plus `XRAND` (session mode), owned RNGs elsewhere, `CHECK` at every settle, the verifier-vs-game trace diff in CI. Any future mid-game UI that calls `Rand()` must get a test. |
| Shim drift in the verifier | The CI trace diff; minimal shims; a test of `mUpdateCnt`/`Resize` semantics. |
| Floating point across platforms | 6.1: SSE2/wasm IEEE, `-ffp-contract=off`, no fast-math, clamped casts, libm trig only for cosmetic positions excluded from LOGIC. |
| localStorage quota (one backup item for the whole user dir) | Replays in `/persist/replays`; only the small sidecar sits in the user dir. |
| Restrictive rule combinations hang `FillBoard` | Static colour minimum plus a deterministic reroll cap. |
| Combination explosion | Pairwise playtests (shape × move set × specials × gold × undo), the difftest, fuzzed rule sets with an autoplay bot. Exhaustive coverage is impossible. |
| Engine changes invalidate old replays | `engineAbi`; corpus re-verified in CI; old verifier builds kept per ABI. |
| Cheating | Server seeds, single use, time plausibility, gate checks. Bots and tool-assisted play remain possible. |
| Worker CPU and plan | Workers Paid; tick cap; queue verification if needed. |
| UI space on the fixed 640x480 art | Feature-gated controls on existing art (register slot, red orb); screenshot review. |
| Generated colours too similar | Curated hue steps, glyph overlay; mods can ship art. |
| Score key from `id@version` splits tables on a version bump | Existing behaviour, documented. Leaderboards key on the RulesId instead. |

---

## 11. Open questions for the user

**Answers (2026-10-18):**
- **Q1, version control:** yes. The baseline is commit `9dcba7d` on `master`; the stages are
  committed on the `code-mods` branch. The frozen baseline exe is `build/baseline/bejeweled.exe`.
- **Q3, feature gating:** feature mods.
- **Q6, holes:** gems fall through holes. Also add dirt tiles that need a match next to them, block
  falling gems, and let gems fall diagonally, alternating left and right (Jewel Quest). See 4.14
  and stage 7b.
- **Q8, special gems:** Bejeweled 2 rules, including cascades.
- **Seed timing** (asked after stage 2: should a chosen seed wait until you actually start
  playing?): yes. A chosen seed stays pending until the first move or game over, so switching to
  Timed straight away still plays the seed. See 4.10.
- **Auto LiveSplit:** "Use a published autosplitter script (ASL) reading a tagged memory block, so it works
  through LiveSplit's normal Activate flow." See 4.15 and stage 2b. Questions 14–24 remain.
- **Twilight gravity:** match Bejeweled 2 exactly. Gravity flips each time the board comes to rest after a
  move with no further match (one direction for a move's whole cascade; a swap that makes no match never
  flips), and once per board fill: the opening board rises, so the first move's new gems fall from the
  top, and each level's fill flips too. All three Bejeweled 2 touches are part of the mod: the backdrop
  mirrored left to right with each colour channel halved, slower falls (fall acceleration 0.1 per tick in
  both directions instead of 0.24, a rule value, so it enters the RulesId) and every sound 2 semitones
  lower. Normal and Timed (Timed Twilight is new). No indicator, as in Bejeweled 2; a key can add one
  later. See 4.16 and stage 7c. Questions 25–27 remain.
- **All other questions:** the recommendations in brackets below apply unless the user says
  otherwise. Q14–Q17, Q22 and Q23 are the exception: contacting HyperNervie or the moderators, running
  the original, the licence, publishing and the download each wait for the user's explicit approval.

Recommendations are in brackets.

1. **Version control.** The project is not a git repository. May stage 0 run `git init` and commit
   the current tree as the baseline? [yes]
2. **Seeded games.** Should a seed you choose yourself get its own local high-score table? Should
   unseeded original games show any seed on game over? [own table; no seed line, so the default
   screen stays identical]
3. **Feature gating.** Should replays, seeds and the leaderboard need feature mods, or be always
   on? Always on would change the default UI and user dir. [feature mods]
4. **UI entry.** Is an "Extras" button in Options (register slot, only with feature mods)
   acceptable? Is the undo hotspot on the red orb plus keys acceptable? [yes]
5. **Undo.** Allowed in timed mode? Should it rewind time? Should the stack be saved with the
   game? [no; no; no]
6. **Holes.** Gems fall through holes (Jewel Quest), or holes block columns with a spawner per
   segment? [fall through]
7. **Twist and slide.** Revert a move that makes no match (as swaps do), or keep it? Should slides
   show a live match preview while dragging, as Chuzzle does? [revert unless `allowInvalid`; no
   preview in v1]
8. **Special gems.** Bejeweled 2 semantics (4 → flame 3x3, 5 → lightning row and column), formed in
   cascades too? Add a hypercube later? [yes; yes; later]
9. **Gold mode.** Map Normal/Timed to untimed/timed gold? Do blasts gild cells? Is time-up a game
   over, or a retry of the board? [as proposed; yes; game over]
10. **Leaderboards.** Ranked and daily boards only, or also an "open" board for unseeded replays?
    Anonymous names with a per-browser secret, or accounts, and name moderation? Is Workers Paid
    acceptable? Replays in D1 BLOBs or R2? Is web-only submission enough, with native exporting
    `.bjr`? [ranked + daily; anonymous + rate limits + a name filter; Paid; D1 at first; web only]
11. **Watching a replay mid-game.** It saves the current game and offers to resume it afterwards.
    Is that acceptable? [yes]
12. **Background after loading a variant save.** Rebuild it for the loaded level (fixes the
    original's level-0 background, on variant boards only), or keep the original bug? [rebuild]
13. **Maximum gem colours** 12, or more (16)? [12, for legibility]
14. **The "Bejeweled Deluxe" registry entry.** There can be only one, and HyperNervie lists a Bejeweled 1
    splitter as coming soon. Ask them first, proposing one combined script (port and original) with them
    as co-maintainer, or contributing the port path to their script? [ask first; the combined script]
15. **The original 1.87 in the script.** It needs one session of the original with its sound on (Steam,
    BASS): you play while `lsprobe` logs, or you allow a session to run it. [yes, if the script is
    registered under the name; otherwise port-only and unregistered]
16. **Licence for the script.** The registry requires an open-source one, and byo-pop has none. [the
    Unlicense, as HyperNervie's, so either way of coordinating works]
17. **Hosting.** Make byo-pop public when it is pushed and serve the raw URL, or put the script in a small
    separate public repo? [byo-pop, default branch]
18. **Mod signs.** Should a mod whose only key is `features.livesplit` skip the crossbar on GO's G and the
    fourth hint sparkle, which would otherwise show in every run video? [skip them; that mod changes
    nothing on screen or in play]
19. **The block in every desktop build**, inert until the mod is on, so the script can say "turn on the
    mod"? Or written even without the mod? [in every build; written only with the mod, as decided in Q3]
20. **Resets** on by default (game over, New Game, mode switch, quit), each switchable, and a finished run
    never reset? [yes]
21. **Start tick:** 150 (go1 sound, banner created) or 151 (first visible banner alpha)? [150]
22. **speedrun.com moderators.** Ask before the registry PR whether runs on the port are accepted (A10),
    how to read A7 (a score shown after TIME UP), and whether they would list the splitter? [yes; the site
    does not say]
23. **Manual test.** A one-time download of LiveSplit 1.8.37 for the manual test (4.15 test 7)? [yes]
24. **Poll rate.** `refreshRate` 100, revisited after the manual test's lag numbers? [yes]
25. **Twilight fall acceleration.** Bejeweled 2 slows its falls from 0.24 to 0.1 in its 84-px cells, and
    BJ1 falls at 0.18 in 51-px cells. Apply the same ×0.42 to BJ1's 0.18 (75), copy the number (100), or
    keep Bejeweled 2's speed in cells (0.1 × 51/84, 61, if its tick is also 10 ms; not checked)? [75]
26. **Twilight fall-out and game over.** Bejeweled 2 has no timed Twilight, and its game-over fall was not
    read. Should the timed fall-out and the game-over fall follow gravity, or always fall down? [follow
    gravity]
27. **Twilight look keys.** `look.backdrop` and `look.soundPitch` sit under `rules` so that replays carry
    them, which also puts them in the RulesId and the score key: a mod that only lowers the pitch gets its
    own high-score tables and leaderboard boards. Keep them there, or make them a non-scoring key that
    replays do not carry? [keep them in `rules`; the Twilight preset has its own tables anyway]

---

## Appendix A: corrections to the source designs

**Fidelity-first:**
- `FindMove` is at 0x4c9dda, not 0x4cd9dda.
- "Black fill + playable cells" is not pixel-exact at 8x8. The original 10x10 tile loop also draws
  a visible ring outside the board, so the variant checker draws the ring too (5.6).
- A seeded save → resume cannot continue one segment. `mUpdateCnt` restarts at 0, the fresh
  constructor draws first, `mTimeDrainDelay` is reset to 300, timers and popups are not restored,
  and tick 150 runs again. Resume is a new segment (6.5).
- External draws cannot be sampled at frame boundaries. `RunScript` runs inside
  `SexyAppBase::UpdateFrames`, and `Board::Update`'s own draws are inside the frame. A board-own
  counter is used instead (6.2).
- Undo through the `LoadGame` body would call `mApp->SetTimedMode(..., false)` and set
  `mApp->mShowInstructions = false`. Undo uses StateCodec instead (6.7).
- Per-level shapes must switch after `UpdateBlackHole`'s deletion loop, not in `SetupLevel`. The
  save's dims and shape index must come before the cell records (5.3, 7.2).
- "Following games take splitmix32(seed)" contradicted the double `NewGame` at startup. It is
  replaced by the pending-seed rule (4.10).
- An `mTutorialCol >= 0` guard at tick 150 would hide the GO banner. The tutorial is suppressed in
  drawing instead (2.5).

**Rules-engine:**
- Live boards drew from the global stream with no record of outside draws, so the
  `HighScoreWidget` tip roll (`HighScoreWidget.cpp:380`) desynced replays.
- Pause and unpause were not logged, so `mPlaySeconds` and the saved bytes could not be reproduced.
- GAME_NOTES 3.16 lists 7 items. Board*.cpp has 9 `Rand()` expressions.
- `MouseDown`, `MouseDrag` and `KeyChar` do call `MarkDirty`.
- "Undo only when pointer fields are NULL" conflicted with `mSelectedPiece` being set during play.
  Undo now clears the selection.

**Determinism-first:**
- Finished replays in the user dir would be swept into the single localStorage backup item, and the
  whole backup is deleted on a quota error.
- DetMath would change native no-mod output, so it is dropped.
- There are 13 `gGameConfig.` reads in Board*.cpp, 12 of them outside `Draw`. (This design keeps
  them on the global `gGameConfig`, which a watch session sets from the replay.)
- A kept invalid move must clear `mIsNewGame`, or the game is not saved on exit.
- The mask legend is now one convention everywhere: `#` is a cell, `.` is a hole. All examples are
  valid JSON.
- The tick-150 GO banner shows only when the instructions are not shown.
