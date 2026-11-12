# Bejeweled Deluxe 1.87: game-code behaviour spec

This is the behavioural specification of the game code (0x4c9a94..0x4d87bc of
`re/bejeweled/bin/BejeweledGame.exe`), consolidated from six module analyses plus spot re-checks against
the disassembly. Translators use it together with the decompilation in `re/bejeweled/game/decomp/` and the
headers in `games/bejeweled/src/`. Framework facts (vtable slots, Widget/Dialog/SexyAppBase layouts, RNG,
float rules) are in `docs/FRAMEWORK_NOTES.md`.

**Symbols.** Function, struct-field and data names are in `re/bejeweled/symbols/20_game.jsonl` (for
`ApplySymbols.java`). Header names match those symbols.

**What was re-verified in the disassembly during synthesis:**
- Board::Update score roll (section 3.7, step 16).
- UpdateClearing grid shifting (3.8.4).
- FindSets run scan, averages and the 300..1200 clamp (3.9).
- ColorizeText pixel formula (3.12).
- UpdateWhirlpool x87 expressions (3.8.7).
- FindMove scan (3.6).
- MouseDown click zone (3.10).
- Piece hover glow order (3.7, step 18).
- Gem-shrink sizes (4.2).
- Every constant and name table read from the PE.

---

## 0. Conventions and global rules

**Offsets.** All offsets are absolute from the object start (hex). Ghidra's RecoverClasses data
structs use a different base per class:

| Ghidra struct | data offset = absolute - |
|---|---|
| `Board_data` | 0x88 |
| `WinDMApp_data` | 0x724 |
| `TitleScreen_data` | 0x8c |
| `HighScoreWidget_data` | 0x8c |
| `HighScoreDialog_data` | 0x100 (sic) |
| `OptionsDialog_data` | 0x158 |
| `RecordScoreDialog_data` | 0x154 |
| `RegisterDialog_data` | 0x154 |

**Ticks.**
- Update() runs at a fixed 100 Hz.
- `mUpdateCnt` (WidgetContainer +0x28) is incremented by `Widget::Update()`.
- Board calls Widget::Update only when not paused, so Board's `mUpdateCnt` freezes while paused. All `% N` cadences and the tick-149/150 events use it.
- Time values are in ticks (1/100 s) unless stated.

**Arithmetic.**
- `(int)double` truncates toward zero (`_ftol2_sse`).
- Integer `/` and `%` are C signed operations (truncate toward zero).
- The game uses x87 doubles with 53-bit precision control. `atan2`, `sqrt`, `sin` and `cos` are real CRT calls.
- The three helpers 0x4c9a94 / 0x4c9ab7 / 0x4c9ad1 are `atan2f`, `sqrtf` and `atan2(float,float)`. Each computes in double and rounds the result to float, which callers then store into doubles.
- Ghidra hides the x87 operand of `_ftol2_sse` calls; every formula below was recovered from the disassembly.

**RNG.**
- `Sexy::Rand()` is MT19937 (31-bit results, never negative), used as `Rand() % n`.
- The 11 game draw sites are the only consumers during play. Section 3.16 lists them in call order.

**Strings.**
- `GetString(id)` reads `properties/default.xml`. The `&cr;` entity is a newline.
- Strings marked "literal" are hard-coded English in the exe.

**Colors.** `Color(r,g,b)` has alpha 255. `Color(int argb)` maps alpha 0 to 255.
`HSLToRGB(h,s,l)` is the framework's 1.3 integer algorithm.

**Original quirks are preserved unless marked "port:".**
- Leaks and uninitialised fields are listed where they occur.
- Recommended port handling is given next to each.

---

## 1. Resources (`games/bejeweled/src/Res.h`)

**Images.**
- `WinDMApp::mImages` (app+0x790) is a `new Image*[36]` indexed by `ImageId`.
- `DoLoadImage(i)` sets `mImages[i] = GetImage("images\\" + gImageNames[i], true)` (app slot 65), then calls `((DDImage*)mImages[i])->GenerateDDSurface()`.
- If GetImage fails: `Popup("Failed to load images\\" + name)` (slot 32), `mLoadingFailed = true`, return false.
- Init loads images 0..6. LoadingThreadProc loads 7..35.

**Image facts used by drawing code.**

| Image | Size / layout |
|---|---|
| gemsheet6 | 780x728: 7 colors x 104 px. Each color has two 52 px rows: the normal/spin row at y = color*104 and the glow row at y+52. 15 frames of 52 px. A gem is the 49x49 sub-rect starting at x = frame*52 + 2. |
| target1 | 6 frames of 52x52 |
| sparkle | 14 frames of 40x40 |
| blackhole | 9 frames of 256x256 (2304x256) |
| checkertextures2 | 5 textures. Texture t occupies x = t*102..t*102+101, two 51x51 cells. |
| checkbox1 | 62x32. Left half is checked, right half is unchecked. |
| editbox | 96x32, 3-slice: caps 0..32 and 64..96, middle 32..64 |
| titlegreenbar2 | 340x24 |
| titlescreen6 | 640x480 |
| trialcircle2 | 105x107 |

**Sounds.**
- Loaded as `SoundManager->LoadSound(i, "sounds\\" + gSoundNames[i])` for i = 0..20 (only if `mSoundManager->Initialized()`).
- Id 6 loads `combo52` a second time (combo62 ships but is never loaded).
- Played with app `PlaySample(id)` (slot 54) or `PlaySample(id, pan)` (slot 53).
- Shipped but never loaded: combo62, combo72, goodbye1, sidebarclick2, welcome1.

**Music.**
- In Init: `LoadMusic(0,"music\\1984-network.s3m")`, `LoadMusic(1,"music\\1981-datajack.s3m")`, `LoadMusic(2,"music\\1980-autonomus.s3m")` (results ignored).
- Then `PlayMusic(2, 0, false)`.
- On-disk names are capitalised differently, so lookup is case-insensitive.

**Fonts (owned by WinDMApp).**

| Offset | Font | Constructor | Created in |
|---|---|---|---|
| +0x7c4 | alphadigits4 | `ImageFont(mImages[21], "data\\alphadigits4.txt")` | loading thread |
| +0x7c8 | scorefont1 | `ImageFont(mImages[23], "data\\scorefont1.txt")` | loading thread (never deleted) |
| +0x7cc | afont1 | `ImageFont(this, "data\\afont1.txt")` | Init |
| +0x7d0 | mainfont1 | `ImageFont(this, "data\\mainfont1.txt")` | Init |
| +0x7d4 | overfont2 | `ImageFont(mImages[29], "data\\overfont2.txt")` | loading thread |

---

## 2. Piece (`games/bejeweled/src/Piece.h`)

- 0x78-byte POD; the field table is in the header.
- The ctor (0x4c9aec) zeroes mColor, mX, mY, mCol, mRow, mVelX, mVelY, mSwapOffX/Y, mGlowing, mGlowAlpha, mGlowFrame, mDying, mDyingFrame, mSpinFrame, mSparkleCount and mSparkleFrame.
- It leaves the padding (+0x04, +0x24, +0x39..+0x3f, +0x4d..+0x4f) and the vortex fields (+0x60, +0x68, +0x70) uninitialised.
- The struct is written raw to savegame.dat (0x78 bytes per present cell).
- port: zero everything. When loading, accept arbitrary padding bytes.
- port: code-mod fields follow the record at +0x78 (`Piece::SAVE_SIZE`); the save still writes and reads 0x78 bytes per piece (12.1).

---

## 3. Board (`games/bejeweled/src/Board.h`)

### 3.1 Geometry

| Function | Result |
|---|---|
| `GetColX(c)` | `c*51 + 203` |
| `GetRowY(r)` | `r*51 + 16` |
| `GetBoardSize` | w = (49 - ColX(0)) + ColX(7) = 406; h = (49 - RowY(0)) + RowY(7) = 406 |
| `GetBoardCenter` | x = (ColX(7)+49-ColX(0))/2 + ColX(0) = 406; y = (RowY(7)+49-RowY(0))/2 + RowY(0) = 219 |

- Gems are 49x49, drawn at `((int)mX, (int)mY)`.
- `GetPanPosition(x)` = `(int)(((double)(x - 203 - 203) / 406.0) * 2.0 * 800.0)`, range about -800..800. It is used as the PlaySample pan.
- `GetPieceAt(x,y)` scans rows 0..7 (outer) and cols 0..7. It returns the first piece with `p->mX <= x < p->mX + 49.0` and `p->mY <= y < p->mY + 49.0` (double compares), or NULL.

### 3.2 Construction: `Board(WinDMApp* theApp, bool timedMode)` (0x4cb5bc)

Steps, in order:
1. Widget(); construct the list and +0x2c8 string.
2. Basic fields:
   - mMoveScoreAlpha = mHintGlowAlpha = 0.0
   - mTimedMode = timedMode
   - mTimeDrain = mTimeDrainRate = 0.0
   - mIsNewGame = true
   - mApp = theApp
3. Zero these:
   - mCursorFrame, mSelectedPiece, mLastLandSoundTick, mGameOverCountdown, mNewGamePending, mShowTutorial
   - mGameDuration, mPlaySeconds, mGemsCleared, mLongestCascade, mBestMoveScore, mScore, mDisplayScore
4. mBarColor = 23; mLastWarningTick = 0.
5. Zero these:
   - mMoveScore, mClearedAtMoveStart, mMoveScoreTimer, mHintGlowTimer, mHintGlowDuration
   - mInputDelay, mMouseDown, mPlayStartTick, mPauseCount, mUserPauseCount
   - mStatsDisabled, mLevelStartScore, mSavedZero, mSwapPiece1, mSwapPiece2
6. Fonts and level: mPointsFont = app->mAlphaDigitsFont; mScoreFont = app->mScoreFont; mLevel = 0; mDisplayLevel = 1.
7. Background images:
   - `mBackgroundImage = new DDImage(app->mDDInterface)`, then `SetImageMode(false,false)` (MemoryImage slot 32), then `Create(640,480)`.
   - Same for `mNextBackgroundImage`.
8. `SetupLevelData()` (3.5).
9. mLevelTargetScore = mLevelPoints.
10. `Graphics g(mBackgroundImage); g.DrawImage(mNextBackgroundImage, 0, 0)`.
11. Remaining fields:
    - mTutorialCol = mTutorialRow = mTutorialCol2 = -1
    - mBlackHoleLevel = 0.0; mBackgroundChanged = false; mBarWidth = 0
    - mTimeDrainDelay = 300; mTextImage = mTextColorImage = NULL; mShowingText = false
    - mElectOffset = 0; mWhirlpoolSound = NULL; mMoveMatchCount = 0; mCascadeCount = 0
    - mTutorialBob = 0; mTutorialBobDir = 1
12. Tutorial board loop:
    ```
    do {
      NewBoard();                       // RNG, sets mState = 0, mIdleTicks = 0
      for (i = 0; FindMove(m, i, true, false); i++)   // m = {col,row,col2,row2}
        if (m[2] == m[0]+1 && (unsigned)(m[1]-4) <= 2 && m[0] >= 1 && m[0] < 7)
          { mTutorialCol = m[0]; mTutorialRow = m[1]; mTutorialCol2 = m[2]; break; }
    } while (mTutorialCol == -1);
    ```
    - The loop accepts the first horizontal-run move (in FindMove order) that swaps to the right, in rows 4..6, with col 1..6.
    - Rejected boards consume RNG and their pieces leak; NewBoard does not delete them.

Not initialised by the ctor: mState (NewBoard sets it), mSwapAngle, mSwapCenterX/Y, mMouseX/Y,
mMouseDownX/Y, mTextPhase, mTextDuration, mTextTimeLeft, mWipeY, mBlackHoleFrame,
mLevelTextShown. port: zero them, and set mMouseX/Y to -1 (as after MouseLeave). Before the first
mouse event, the original's hover test reads heap garbage.

### 3.3 Destruction: `~Board` (0x4cd1ba)

1. If `IsInProgress()`, call `SaveGame()`.
2. Delete (virtual) mBackgroundImage, mNextBackgroundImage, mTextImage and mTextColorImage.
3. `mWhirlpoolSound->Release()` if set.
4. `app->KillDialog(9)`.
5. If `IsInProgress()` is still true, `++app->mTimesPlayed`. An abandoned game counts as played.
6. Destroy the string and the list; Widget::~Widget.

Pieces in the grid are not deleted (leak). port: delete them here.

`IsInProgress()` (0x4c9bbe) = `!mIsNewGame && mState != 5 && mState != 6`.

### 3.4 Pause / GameOver / hint helpers

**`Pause(bool theUserPause)` (0x4c9bda).**
1. If mPauseCount == 0:
   - `mPlaySeconds += (unsigned)(mUpdateCnt - mPlayStartTick) / 100` (the remainder is dropped)
   - MarkDirty
2. If theUserPause: MarkDirty if mUserPauseCount == 0, then ++mUserPauseCount.
3. Always ++mPauseCount.

**`Unpause(bool theUserPause)` (0x4c9c2d).**
1. mInputDelay = 10.
2. If theUserPause && mUserPauseCount > 0: --mUserPauseCount; MarkDirty if it is now <= 0.
3. If mPauseCount > 0: --mPauseCount; if it is now 0, mPlayStartTick = mUpdateCnt.

Pause sources:

| Source | Call |
|---|---|
| KeyChar | `Pause(true)` / `Unpause(true)` toggle |
| SaveGame | `Pause(false)`, then `Unpause(false)` |
| WinDMApp ModalOpen / LostFocus | `Pause(app->mTimedMode)` |
| WinDMApp ModalClose / GotFocus | `Unpause(app->mTimedMode)` |

So in normal mode a dialog or focus loss freezes the board silently. In timed mode it also shows the pause screen.

**`GameOver(int theShakeTicks)` (0x4c9b47).** In order:
1. mSelectedPiece = NULL; mShowTutorial = false
2. `app->KillDialog(9)`
3. mState = 5
4. `mGameDuration = (unsigned)(mUpdateCnt - mPlayStartTick)/100 + mPlaySeconds` (mPlaySeconds is not modified)
5. mGameOverCountdown = theShakeTicks
6. ++app->mTimesPlayed

Callers:
- No moves in normal mode: 200.
- Time up: 200.
- `RequestNewGame` (0x4c9bab): `GameOver(50); mNewGamePending = true`. Called by WinDMApp on YES to dialog 0.

**`ShowHint()` (0x4cb475).** If `mState == 1 && mPauseCount == 0`, call `TryShowHint()`. Called from WinDMApp button 15.

**`TryShowHint()` (0x4cad2f).**
1. `mScore = (int)((double)mScore - mPointMultiplier*50.0)`, clamped to >= 0.
2. If `FindMove(m, 0, true, true)`:
   - q = mBoard[m.row2][m.col2], the gem the move would bring in
   - q->mSparkleCount = 58; q->mSparkleFrame = 0; q->mSpinFrame = 1 (one spin)
3. mIdleTicks = 0.

No sound, no RNG.

### 3.5 `SetupLevelData()` (0x4cae29): background and level parameters

Let `L = (double)mLevel`, `ci = mLevel % 4` and `ti = mLevel % 5`.

1. `Graphics g(mNextBackgroundImage)`; mBackgroundChanged = true (dead flag).
2. Tiles:
   - tileA = MemoryImage(app), `Create(51,51)`. Draw `mImages[28]` at (0,0) with src `Rect(ti*102, 0, 51, 51)`.
   - tileB = MemoryImage(app), `Create(102,51)`. Draw src `Rect(ti*102+51, 0, 51, 51)` at (0,0); only the left 51 px are used.
3. Colorize: `imgA = app->CreateColorizedImage(&tileA, Color(gBoardTileColors[ci][0]))`, `imgB` likewise with `[ci][1]`. The two colors in each pair are identical.
4. Checkerboard:
   ```
   for (r = -1, y = -35; y < 425; y += 51, r++)     // 10 rows: y = -35..424
     for (c = -1, x = 151; x < 611; x += 51, c++)   // 10 cols: x = 151..610
       g.DrawImage(((r + c) % 2 == 0) ? imgA : imgB, x, y);
   ```
   Uses C `%` on r+c, which can be -2..; the test is `== 0`.
5. Delete imgA and imgB; the tile images are stack objects. `g.DrawImage(mImages[7] alphaback1, 0, 0)`.
6. Level parameters:
   - mTimeDrain = 0.0.
   - Timed mode: `M = app[0x760]*L + app[0x758]` and `T = app[0x750]*L + (double)app[0x748]`.
   - Normal mode: `M = app[0x740]*L + app[0x738]` and `T = app[0x730]*L + (double)app[0x728]`.
   - Then:
     ```
     mPointMultiplier = M;
     mLevelPoints     = (int)(T * M);
     mTimeDrainRate   = (app[0x770]*L + app[0x768]) * M;
     mTimeDrainAccel  = (app[0x780]*L + app[0x778]) * M;
     ```

With the shipped diff.txt:

| Mode | M | points | rate | accel |
|---|---|---|---|---|
| normal | 1.0+0.5L | (int)((500+150L)*M) | (1+2L)*M | (0.01+0.005L)*M |
| timed | 2.0+0.5L | (int)((250+10L)*M) | (1+2L)*M | (0.01+0.005L)*M |

No RNG.

### 3.6 Board generation and move search

**`NewBoard()` (0x4cb182).**
1. mState = 0; mIdleTicks = 0.
2. Set all 64 cells to NULL without deleting the old pieces.
3. Fill:
   ```
   for (row = 7; row >= 0; row--)
     for (col = 0; col < 8; col++) {
       p = new Piece; p->mRow = row; p->mCol = col; p->mX = (double)GetColX(col);
       base = (row < 7) ? (int)mBoard[row+1][col]->mY : 0;
       p->mY = (double)(base - Rand()%240 - 65);           // RNG draw 1
       mBoard[row][col] = p;                               // placed before the color loop
       do { p->mColor = Rand()%7; } while (HasSet());    // RNG draws 2..n
     }
   ```
The result has no 3-in-a-row. Pieces start above the screen with mVelY = 0 and fall in state 0.

**port: do not free the orphaned pieces in NewBoard.**
- Original: the pieces from state 9 (and from rejected ctor boards) leak but stay valid. `mSelectedPiece` can still point at one, because a gem can be selected during states 0/2/3/4/9.
- A later click in state 1 then calls `SwapPieces(stalePiece, target)`. If adjacent by the stale col/row, the stale piece is swapped back into the grid. This is an original bug.
- To stay faithful and memory-safe, park orphans in a list owned by the Board and free them in ~Board.

**`HasSet()` (0x4c9cf5).**
- Pass 1 scans rows 0..7 left to right; pass 2 scans cols 0..7 top to bottom.
- Track the run count and previous color (start -1). NULL sets prev = -1.
- Equal color: ++count, and return true when it reaches 3. Different color: count = 1, prev = color.
- Return false otherwise.

**`FindMove(int* m, int n, bool horz, bool vert)` (0x4c9dda).** Enumerates candidate swaps and returns the n-th (0-based) one that creates a match.
- Order: row 0..7, col 0..7, skipping NULL cells; then directions (dx,dy) = (1,0), (-1,0), (0,1), (0,-1).
- Targets outside 0..7 are skipped.
- For each candidate:
  1. Swap the two grid pointers. Measure only the piece now at (row,col), color C.
  2. `left`: walk left while col>0 and the cell is non-NULL with color C.
  3. `right` (only if col<7): walk right while right<7 and the next cell has color C.
  4. `up` / `down`: the same vertically.
  5. Swap back.
  6. It is a match if `(right-left >= 2 && horz) || (down-up >= 2 && vert)`.
- On a match:
  - If count == n, write `{col, row, col+dx, row+dy}` (if m != NULL) and return true.
  - Otherwise ++count.
- Return false when the enumeration is exhausted.
- Original quirk: the NULL test in the right/down loops checks the fixed cell [row][left+1] / [up+1][col] instead of the scanned cell.
  - Every caller passes a full board, so per-cell NULL checks are equivalent (port: use them).
  - The run lengths must match exactly.
- Because only the moved-in piece is measured, each geometric swap appears up to twice (once from each side).

Callers:

| Caller | Call |
|---|---|
| ctor (tutorial) | `FindMove(m, i, true, false)` |
| UpdateFalling (no-moves check) | `FindMove(NULL, 0, true, true)` |
| TryShowHint | `FindMove(m, 0, true, true)` |

### 3.7 `Update()` (0x4cdabc): per-tick order

1. `if (mPauseCount > 0) return; if (mUserPauseCount > 0) return;`. mUpdateCnt does not advance.
2. `if (mInputDelay > 0) --mInputDelay;`
3. `Widget::Update()` (++mUpdateCnt). Below, `cnt` is the new value.
4. Banner: if mShowingText:
   - mTextPhase += 0.07; MarkDirty
   - if (--mTextTimeLeft == 0) mShowingText = false
5. `if (mTimeDrainDelay > 0) --mTimeDrainDelay;`
6. Timed drain: if `mTimedMode && mTimeDrainDelay == 0 && (mState == 1 || mState == 2)`:
   - `mTimeDrain += mTimeDrainRate/100.0`, then `mTimeDrainRate += mTimeDrainAccel/100.0`.
7. Hint-button glow: if mHintGlowTimer > 0:
   ```
   t = --mHintGlowTimer; P = mHintGlowDuration/3;        // 133 for 400
   v = abs(t % P - P/2);
   mHintGlowAlpha = 1.0 - (double)v / ((double)P * 0.5);
   if (mHintGlowTimer == 0) mHintGlowAlpha = 0.0;
   MarkDirty;
   ```
8. If `mBlackHoleLevel > 0.0 && cnt % 2 == 0`: `if (--mBlackHoleFrame < 0) mBlackHoleFrame = 8;` MarkDirty.
9. If `mShowTutorial && cnt % 4 == 0`: `mTutorialBob += mTutorialBobDir; if (abs(mTutorialBob) > 2) mTutorialBobDir = -mTutorialBobDir;` MarkDirty.
10. Move-score text: if mMoveScoreTimer > 0:
    - t = --mMoveScoreTimer.
    - If t < 50: mMoveScoreAlpha = t/50.0; MarkDirty.
    - Else if mMoveScoreAlpha < 1.0: alpha += 0.05, clamped to 1.0; MarkDirty.
    - Else: nothing.
11. Bar target:
    ```
    prog  = (mDisplayScore - (int)mTimeDrain - mLevelStartScore) * 340;
    range = mLevelTargetScore - mLevelStartScore;
    T = mTimedMode ? (prog/2)/range + 170 : prog/range;    // int division
    clamp T to 0..340;
    moved = false;
    repeat 2 times: if (T > mBarWidth) { moved = true; ++mBarWidth; MarkDirty; }
                    else if (T < mBarWidth) { --mBarWidth; MarkDirty; }
    ```
    In timed mode the bar starts half full (170 = 0 progress).
12. Bar color:
    - Normal mode: `idx = 23`.
    - Timed mode:
      ```
      frac = mBarWidth/340.0;  idx = (mBarWidth*24 - 1)/340;
      if (!moved && mState != 5 && mState != 6) {
        iv = max(80, (int)(frac*1600.0));
        if (iv <= 240 && (unsigned)(cnt - mLastWarningTick) >= (unsigned)iv)
          { PlaySample(18 warning1); mLastWarningTick = cnt; }
        if (frac < 0.15 && (cnt/8) % 2 == 0) idx = 24;      // white flash
      }
      ```
    - Then, in both modes: `if (mState == 7 && (cnt/8) % 2 == 0) idx = 24;`
    - `if (idx != mBarColor) { mBarColor = idx; MarkDirty; }`
    - Effect: the warning beeps every 80..240 ticks while the bar is below 15% (frac < 0.15 gives iv <= 240).
13. `if (cnt == 149 && !app->mIsRegistered && app->mTimesPlayed >= 20) app->ShowRegisterNagDialog();`
14. At `cnt == 150`:
    - `PlaySample(14 go1)`.
    - If `!app->mShowInstructions || app->mTimesPlayed >= 5`: `DoBouncyText(GetString("GO"), 250)`.
    - If `app->mShowInstructions && app->mTimesPlayed < 5`: `app->DoInstructionDialogette(); mShowTutorial = true;`
15. `if (++mIdleTicks > 1500 && mState == 1) { mIdleTicks = 0; mHintGlowDuration = 400; mHintGlowTimer = 400; }`
16. Score roll (verified in disassembly). m = mPointMultiplier.

    Pass 1 reads `d = mScore - mDisplayScore`:
    ```
    if      (d >= (int)(m*150.0)) mDisplayScore -= (int)(m*-3.0);           // += trunc(3m)
    else if (d >= (int)(m*100.0)) mDisplayScore -= (int)(m*-2.0);
    else if (d >= (int)(m*50.0) || (mDisplayScore < mScore && cnt%2 == 0))
         { mDisplayScore += (int)m; if (mDisplayScore > mScore) mDisplayScore = mScore; }
    else goto pass2;                       // no MarkDirty
    MarkDirty;
    ```
    Pass 2 re-reads both values:
    ```
    d = mScore - mDisplayScore;
    if      (d <= (int)(m*-150.0)) mDisplayScore -= (int)(m*3.0);
    else if (d <= (int)(m*-100.0)) mDisplayScore -= (int)(m+m);
    else if (d <= (int)(m*-50.0) || (mDisplayScore > mScore && cnt%2 == 0))
         { mDisplayScore -= (int)m; if (mDisplayScore < mScore) mDisplayScore = mScore; }
    else goto done;
    MarkDirty;
    ```
    Pass 2 runs after hints, which lower mScore.
17. `if (mSelectedPiece && cnt % 8 == 0) { MarkDirty; ++mCursorFrame; }`
18. Pieces, row-major, each non-NULL p:
    1. If `cnt%5 == 0 && p->mSparkleCount > 0`: ++mSparkleFrame; --mSparkleCount; MarkDirty.
    2. If `cnt%3 == 0 && (p->mSpinFrame != 0 || p == mSelectedPiece)`: `mSpinFrame = (mSpinFrame+1) % 15`; MarkDirty. Skip step 3 for this piece.
    3. Otherwise:
       - If `p->mGlowing`: mGlowAlpha -= 0.012; if it is now < 0: mGlowing = false; MarkDirty. The alpha is left negative.
       - If `mState == 1 && p != mSelectedPiece` and the mouse is inside the gem (`p->mX <= mx && p->mY <= my && mx < p->mX+49.0 && my < p->mY+49.0`, with mx = (double)mMouseX):
         - mGlowAlpha += 0.045, clamped to 1.0.
         - If !mGlowing: mGlowFrame = 0; mGlowing = true.
         - The decay above also ran this tick, so the net fade-in is +0.033/tick.
       - If `cnt%3 == 0 && p->mGlowing`: `mGlowFrame = (mGlowFrame+1) % 15`; MarkDirty.
19. Score popups, list order:
    - `if (cnt%3 == 0) { MarkDirty; if (v.mY > mPointsFont->GetAscent()/2 + 2) --v.mY; }`
    - MarkDirty; `if (--v.mLife < 1) erase (MarkDirty)`.
20. Level end / time up, only if mState == 1:
    - `if (mDisplayScore - (int)mTimeDrain - mLevelStartScore >= mLevelTargetScore - mLevelStartScore) DoWhirlpool();` (sets state 7)
    - Then, if still state 1: `if (mDisplayScore - (int)mTimeDrain - mLevelStartScore <= mLevelStartScore - mLevelTargetScore) { DoBouncyText(GetString("TIME_UP"), 400); GameOver(200); }`
    - The time-up test is not restricted to timed mode. In normal mode it can only fire when hints (TryShowHint) push mScore far enough below mLevelStartScore (one level's worth); see open question 3.
21. Dispatch on mState:

    | State | Handler |
    |---|---|
    | 0 | UpdateFalling |
    | 2, 3 | UpdateSwapping |
    | 4 | UpdateClearing |
    | 5 | UpdateGameOverAnim |
    | 7 | UpdateWhirlpool |
    | 8 | UpdateLevelTransition |
    | 9 | UpdateBoardDropping |
    | 1, 6 | nothing |

### 3.8 State handlers

#### 3.8.1 State 0: `UpdateFalling()` (0x4cd687)

```
landed = 0; sumX = 0; settled = true;
for row 0..7 { ty = (double)GetRowY(row);
  for col 0..7 { p = mBoard[row][col];           // no NULL check: the board is full here
    if (p->mY == ty) p->mVelY = 0.0;
    else { settled = false; p->mY = p->mVelY + p->mY;
           if (p->mY >= ty) { landed++; sumX = (int)(p->mX + 24.0 + (double)sumX); p->mY = ty; }
           p->mVelY = p->mVelY + 0.18; } } }     // velocity zeroed only on the next tick
if (landed > 0 && abs(mLastLandSoundTick - cnt) >= 10)
  { mLastLandSoundTick = cnt; PlaySample(8 gemongem2, GetPanPosition(sumX/landed)); }
if (settled) {
  if (FindSets()) mState = 4;
  else {
    if (!mStatsDisabled) { mLongestCascade = max(mLongestCascade, mCascadeCount);
                           mBestMoveScore  = max(mBestMoveScore,  mMoveScore); }
    if (FindMove(NULL, 0, true, true)) mState = 1;
    else if (mTimedMode) mState = 9;
    else { DoBouncyText(GetString("NO_MOVES"), 400); GameOver(200); }
  }
}
MarkDirty;
```

#### 3.8.2 State 1

Idle. Input (3.10) and Update steps 18 and 20 apply.

#### 3.8.3 States 2/3: swap animation

**`SwapPieces(Piece* a, Piece* b)` (0x4c9ff8).**
1. If `abs(a->mCol - b->mCol) + abs(a->mRow - b->mRow) != 1`: PlaySample(7 bad2) and return.
2. Tutorial check: if `mShowTutorial && a->mCol == mTutorialCol && a->mRow == mTutorialRow && b->mCol == mTutorialCol2 && b->mRow == mTutorialRow`:
   - app->mShowInstructions = false; mShowTutorial = false; app->KillDialog(9).
   - Only the exact demonstrated move (first gem dragged right) dismisses it here.
3. mSwapAngle = 0; mSwapPiece1 = a; mSwapPiece2 = b.
4. `mSwapCenterX = (int)((b->mX + a->mX)*0.5); mSwapCenterY = (int)((a->mY + b->mY)*0.5)`.
5. `a->mSwapOffX = (int)(a->mX - mSwapCenterX)`, and likewise for a's Y and for b.
6. mState = 2. There is no NULL check on the pieces.

**`UpdateSwapping()` (0x4cd84f).** MarkDirty; mSwapAngle += 5.

If mSwapAngle == 180 (A = mSwapPiece1, B = mSwapPiece2):
1. Exchange the pieces:
   - (c,r) = (A.col, A.row); A.col = B.col; A.row = B.row
   - A.x = ColX(A.col); A.y = RowY(A.row); mBoard[A.row][A.col] = A
   - B.col = c; B.row = r; B snaps likewise; mBoard[r][c] = B
2. mCascadeCount = 1; mMoveMatchCount = 0.
3. If `FindSets()`:
   - mSwapPiece1 = mSwapPiece2 = NULL; mState = 4.
   - If mShowTutorial: mShowTutorial = false; app->KillDialog(9); app->mShowInstructions = false.
   - mIsNewGame = false.
4. Else if mState == 2 (illegal swap):
   - `A.mSwapOffX = -A.mSwapOffX; A.mSwapOffY = -A.mSwapOffY; B.mSwapOffX = -A.mSwapOffX; B.mSwapOffY = -A.mSwapOffY` (B uses A's new values)
   - mSwapAngle = 0; PlaySample(7 bad2); mState = 3.
5. Else (state 3 finished): mSwapPiece1 = mSwapPiece2 = NULL; mState = 1. FindSets already ran on the restored board and found nothing.

Otherwise (mid-animation):
```
a = cos((double)mSwapAngle * 3.14159 / 180.0);     // multiply by PI first, then divide
A.mX = (double)(int)((double)A.mSwapOffX * a + mSwapCenterX); A.mY likewise with Y; same for B.
```
A swap takes 36 ticks; the swap back takes another 36.

#### 3.8.4 State 4: `UpdateClearing()` (0x4ca9ed), verified

Phase 1, shrink:
```
allDone = true;
for each cell row-major with p && p->mDying {
  if (cnt % 3 == 0) {
    if (++p->mDyingFrame > 5) { if (p == mSelectedPiece) mSelectedPiece = NULL;
                                delete p; cell = NULL; }
    MarkDirty;
  }
  allDone = false;
}
if (!allDone) return;
```

Phase 2, refill (only on a tick on which no dying piece existed):
```
top = GetRowY(0);                       // 16
do { changed = false;
  for col 0..7 { minY = top; gap = false;
    for row 7..0 { p = mBoard[row][col];
      if (!p) { if (gap) mBoard[row+1][col] = NULL; gap = true; }
      else { if (p->mY < (double)minY) minY = (int)p->mY;
             if (gap) { p->mRow++; p->mVelY = 1.0; mBoard[row+1][col] = p; } } }
    if (gap) { changed = true;
      q = new Piece; q->mVelY = 0.3; q->mRow = 0; q->mCol = col;
      q->mX = (double)GetColX(col); q->mY = (double)(minY - 69);
      q->mColor = Rand() % 7;                       // one draw per gapped column per pass
      mBoard[0][col] = q; } }
} while (changed);
MarkDirty; mState = 0; ++mCascadeCount;
```
- Each pass moves every piece above the lowest gap down by one row. The source cell is overwritten by the next iteration or by the spawn.
- Spawned gems stack above the highest gem of their column (minY - 69 per pass).
- Colors are not re-rolled, so cascades occur.

#### 3.8.5 State 5: `UpdateGameOverAnim()` (0x4cab82)

MarkDirty first.

If `mGameOverCountdown >= 1`:
1. `if (--mGameOverCountdown == 0) PlaySample(9 explode2);`
2. If `cnt % 6 == 0 || mGameOverCountdown == 0`, for each piece row-major:
   ```
   p->mX = (double)(GetColX(p->mCol) + Rand()%9 - 4);
   p->mY = (double)(GetRowY(p->mRow) + Rand()%9 - 4);
   if (mGameOverCountdown == 0) { p->mVelY = (double)(Rand()%8 - 6); p->mVelX = Rand()%7 - 3; }
   ```
   RNG order per piece: x, y, then [vy, vx].

Else (countdown already 0):
```
allGone = true;
for each piece: p->mX = (double)p->mVelX + p->mX; p->mY += p->mVelY; p->mVelY += 0.15;
                if (p->mY < (double)(mHeight + 49)) allGone = false;
if (allGone) {
  if (mNewGamePending) app->NewGame();
  else { PlaySample(16 gameover1); mState = 6; app->ShowHighScoreWidget(); }
}
```

#### 3.8.6 State 6

Nothing. The HighScoreWidget is shown and the Board stays alive but is removed from the
WidgetManager (it remains `app->mBoard`).

#### 3.8.7 State 7: vortex

**`DoWhirlpool()` (0x4ca58d).** Called from Update step 20.
1. If mWhirlpoolSound: Release it.
2. `mWhirlpoolSound = app->mSoundManager->GetSoundInstance(19)`; if non-NULL, `Play(true, false)` (looping).
3. mBlackHoleLevel = 0.0; mLevelTextShown = false; mBlackHoleFrame = 0; mSelectedPiece = NULL.
4. (cx,cy) = (406,219). For each piece row-major:
   ```
   dx = (int)(p->mX + 24.0 - cx);  dy = (int)(24.0 + p->mY - cy);
   p->mHoleAngle  = (double)atan2f((float)dy, (float)dx);
   p->mHoleRadius = (double)sqrtf((float)((double)((float)dx*(float)dx) + (double)(dy*dy)));
   p->mHoleAngVel = 0.0;
   ```
   The x87 sum is rounded to float before sqrtf; `dy*dy` is an int multiply.
5. mState = 7.

**`UpdateWhirlpool()` (0x4cb931).** Verified.
```
mBlackHoleLevel += 0.02; if (mBlackHoleLevel > 1.0) mBlackHoleLevel = 1.0;
if (mWhirlpoolSound) mWhirlpoolSound->SetVolume(mBlackHoleLevel);
(cx,cy) = (406,219); (w,h) = (406,406);
maxR = (int)((double)(int)sqrtf((float)((double)w*w + (double)(h*h))) / 2.2);   // 574/2.2 -> 260
max1 = 0.0; max2 = 0.0; done = true;
for each piece row-major {
  p->mX = cos(p->mHoleAngle)*p->mHoleRadius + cx - 24.0;
  p->mY = sin(p->mHoleAngle)*p->mHoleRadius + cy - 24.0;
  if (!(p->mHoleRadius < max1)) max1 = p->mHoleRadius;
  p->mHoleRadius -= 1.0;
  if (p->mHoleRadius < 0.0) p->mHoleRadius = 0.0; else done = false;   // exactly 0 keeps done = false for one tick
  if (!(p->mHoleRadius < max2)) max2 = p->mHoleRadius;
  p->mHoleAngVel += ((double)maxR - p->mHoleRadius) / 110000.0;
  p->mHoleAngle  += p->mHoleAngVel;
  if (cnt % 8 == 0) {
    f = (int)((6.0 - p->mHoleRadius*6.0/96.0) - 1.0);
    if (f >= 0) { if (!p->mDying) { p->mDyingFrame = 0; p->mDying = true; }
                  else if (f > p->mDyingFrame) p->mDyingFrame++; }
  }
}
if (max1 >= 80.0 && max2 < 80.0) PlaySample(15 levelup1);
if (max1 >= 64.0 && max2 < 64.0) { mLevelTextShown = true;
   DoBouncyText(StrFormat(GetString("LEVEL").c_str(), mLevel + 2), 400); }   // "LEVEL %d", the next level's number
if (done) {
  ++mLevel; SetupLevelData();               // new background into mNextBackgroundImage, new multiplier/points/drain
  delete all 64 pieces, cells = NULL;
  mLevelTargetScore = mLevelPoints + mScore; mLevelStartScore = mScore;
  PlaySample(17 firecrackle); mWipeY = 0; mState = 8;
}
MarkDirty;
```
No RNG.

#### 3.8.8 State 8: `UpdateLevelTransition()` (0x4cb284)

1. `Graphics g(mBackgroundImage); g.ClipRect(0, mWipeY, mWidth, 3); g.DrawImage(mNextBackgroundImage, 0, 0);`
2. Whirlpool fade:
   - `mBlackHoleLevel -= 0.015`.
   - If it is still >= 0: `mWhirlpoolSound->SetVolume(level)` (if set).
   - Else: `mBlackHoleLevel = 0.0`; Release the sound and set it to NULL.
3. `if (cnt % 4 == 0) mElectOffset = Rand() % 128;`
4. MarkDirty; mWipeY += 3.
5. If `mWipeY > mHeight`:
   - mDisplayLevel = mLevel + 1; mTimeDrainDelay = 200; NewBoard().
   - Then `if (!app->mIsRegistered && app->mTimesPlayed >= 15 && Rand()%5 == 0) app->ShowRegisterNagDialog();`
   - The Rand draw happens only when the first two conditions hold, after NewBoard's draws.

#### 3.8.9 State 9: `UpdateBoardDropping()` (0x4cb3bf)

Timed mode with no moves left:
```
allGone = true; MarkDirty;
for col 0..7 { start = (Rand() % 15 == 0);          // exactly 8 draws per tick, even for empty columns
  for row 7..0 { p = mBoard[row][col]; if (!p) continue;
    if (p->mVelY == 0.0) { if (start) { p->mVelY = 1.0; start = false; } }   // no move this tick
    else { p->mY += p->mVelY; p->mVelY += 0.18; }
    if (p->mY < (double)(mHeight + 32)) allGone = false; } }
if (allGone) { mTimeDrainDelay = 200; NewBoard(); }
```
- Pieces start in UpdateFalling with mVelY = 0, so each column is released bottom-up, one gem per lucky tick.
- The drain keeps running only in states 1 and 2, so it pauses during the fall-out.

### 3.9 `FindSets()` (0x4cd287): scoring (verified)

```
numMatches = 0; sumAvgX = 0; maxLen = 0;
for dir 0..1 {              // 0: vertical runs (line i = column i, j = row); 1: horizontal (line i = row i, j = col)
  for i 0..7 { color = -1; count = 0; start = (0,0);
    for j 0..7 { (col,row) = dir==0 ? (i,j) : (j,i); p = mBoard[row][col];
      if (p && p->mColor == color) { count++; if (j != 7) continue; }
      if (count >= 3) {
        numMatches++; sx = 0; sy = 0;
        for r = start.row..row, c = start.col..col: if (q = mBoard[r][c]) && q->mColor == color
             { sx = (int)((double)sx + q->mX); sy = (int)((double)sy + q->mY);
               q->mDyingFrame = 0; q->mDying = true; }
        ax = sx / count; ay = sy / count; sumAvgX += ax;
        ++mMoveMatchCount;
        base = gMatchBaseScores[min(mMoveMatchCount, 9) - 1];   // 10,20,30,40,60,80,110,160, then 210
        if (count > 3) base += (count-3)*10;
        if (count > 5) base += (count-5)*10;
        pts = (int)((double)base * mPointMultiplier);
        mScorePopups.push_back({ax+24, ay+16, 110, -1, pts});
        mGemsCleared += count; mScore += pts;
        if (mMoveMatchCount == 1) { mHintGlowAlpha = 0.0; mHintGlowTimer = 0; mMoveScore = 0; mClearedAtMoveStart = 0; }
        thr = (int)(mPointMultiplier * 120.0);
        if (mMoveScore < thr && mMoveScore + pts >= thr)
          { DoBouncyText(GetString("EXCELLENT"), 200); PlaySample(20 excellent1); }
        mMoveScore += pts;
        if (mMoveScoreTimer != -1)
          mMoveScoreTimer = (int)clamp((double)mMoveScore / mPointMultiplier * 3.0, 300.0, 1200.0);
        maxLen = max(maxLen, count);
      }
      color = p ? p->mColor : -1; count = 1; start = (col,row);
    } } }
if (numMatches > 0) {
  pan = GetPanPosition(sumAvgX / numMatches + 24); mIdleTicks = 0;
  PlaySample(maxLen == 3 ? 0 : 1, pan);                    // gotset2 / gotsetbig2
  c = mCascadeCount - 1; if (c >= 5) c = 4; PlaySample(c + 2, pan);  // combo22..combo52 (id 6 = combo52 again)
  if (numMatches > 1) PlaySample(13 multishot, pan);
}
return numMatches > 0;
```

Notes:
- A gem in both a vertical and a horizontal run is scored in both.
- Vertical runs are scanned before horizontal ones.
- The run counter continues through the cascades of one move: mMoveMatchCount resets only at swap completion.
- The run base is per run index, not per cascade level.
- Combo sound: mCascadeCount = 1 for the swap's own matches, 2 after the first refill, and so on (combo22 for the first pass).

### 3.10 Input

**Mouse position.**
- `MouseMove(x,y)` (0x4c9fd0): `mMouseX = x; mMouseY = y`.
- `MouseLeave()` (0x4c9fe9): both = -1.

**`MouseDown(x,y,cc)` (0x4ca6b0).** Verified.
1. `if (mUserPauseCount != 0) { Unpause(true); return; }`. Any click resumes a user pause.
2. `if (mInputDelay > 0) return;`
3. mMouseDown = true; mMouseDownX = x; mMouseDownY = y.
4. If mState is 5, 6, 7 or 8, return.
5. If cc <= 0 (right button): mSelectedPiece = NULL; MarkDirty; return.
6. p = GetPieceAt(x,y).
7. If there is no selection:
   - If p is NULL, return (no MarkDirty).
   - Otherwise: mCursorFrame = 0; mSelectedPiece = p; MarkDirty; `PlaySample(11 select, GetPanPosition((int)(p->mX + 24.0)))`; return.
   - Selection works in every state except 5..8.
8. If mState != 1, return. The selection is kept.
9. Click zone:
   ```
   sel = mSelectedPiece;
   dx = (int)(x - (sel->mX + 24.0)); dy = (int)(y - (sel->mY + 24.0));
   big = |dx| >= |dy| ? |dx| : |dy|  (max);  small = min(|dx|,|dy|);
   target = p;
   if (big >= 22 && (double)big <= 88.2 && small <= 26) {
     c = sel->mCol; r = sel->mRow;
     if (|dy| < |dx|) c += (dx < 0 ? -1 : 1); else r += (dy < 0 ? -1 : 1);   // ties go vertical
     if ((unsigned)c < 8 && (unsigned)r < 8) target = mBoard[r][c];
   }
   if (target && target != sel) SwapPieces(sel, target);   // a non-adjacent target plays bad2
   mSelectedPiece = NULL; MarkDirty;
   ```
   Clicking the selected gem again deselects it.

**`MouseUp(x,y,cc)` (0x4ca8b7):** mMouseDown = false.

**`MouseDrag(x,y)` (0x4ca8c1).**
1. Return unless `mMouseDown && mSelectedPiece && mState == 1`.
2. dx = x - mMouseDownX; dy = y - mMouseDownY. Return if `|dx| < 16 && |dy| < 16`.
3. If |dx| > |dy|:
   - dx > 0 && col < 7: neighbour is [row][col+1].
   - dx < 0 && col > 0: neighbour is [row][col-1].
4. Else (ties go vertical):
   - dy > 0 && row < 7: neighbour is [row+1][col].
   - dy < 0 && row > 0: neighbour is [row-1][col].
5. `SwapPieces(sel, neighbour)` (no NULL check).
6. In every case past the threshold: mSelectedPiece = NULL; MarkDirty.

**`KeyChar(c)` (0x4ca9d3):** any key toggles the user pause (`Unpause(true)` if mUserPauseCount > 0, else `Pause(true)`).

### 3.11 Banner text

**`DoBouncyText(text, duration)` (0x4ca1c8).**
1. Delete mTextImage and mTextColorImage (virtual; not nulled first).
2. `font = app->mOverFont`.
3. mTextImage:
   - `new MemoryImage(app)`, `Create(font->StringWidth(text), font->GetHeight())`, `SetImageMode(true,true)`.
4. mTextColorImage:
   - `new MemoryImage(app)`, `Create(w, h + 48)`, `SetImageMode(true,true)`.
5. Draw the text: `Graphics g(mTextImage); g.SetFont(font); g.SetColor(Color(255,255,255)); g.DrawString(text, 0, font->GetAscent())`.
6. mTextPhase = 0.0; mTextDuration = mTextTimeLeft = duration; mShowingText = true.

| Banner | Duration |
|---|---|
| GO | 250 |
| NO_MOVES | 400 |
| LEVEL %d | 400 |
| TIME_UP | 400 |
| EXCELLENT | 200 |

### 3.12 `ColorizeText()` (0x4ca36e), verified

Called from Draw each frame while mShowingText.

```
src = mTextImage bits, dst = mTextColorImage bits; w = width(src); h1 = height(src); h2 = height(dst) (= h1+48)
memset(dst, 0, h2*w*4);
elapsed = mTextDuration - mTextTimeLeft; fade = 255;
if (elapsed < 75) fade = elapsed*255/75; else if (mTextTimeLeft < 75) fade = mTextTimeLeft*255/75;
pal = new ulong[h2]; for i: pal[i] = app->HSLToRGB(i & 255, 255, 148);
for x 0..w-1 { off = (int)((sin(x*0.024 + mTextPhase) + 1.0) * 24.0);      // 0..48
  for y 0..h1-1 { s = src[y*w+x]; c = pal[off+y];
    dst[(off+y)*w + x] = (((sa*fade)>>8 & 0xff) << 24) | (((cr*sr)>>8) << 16)
                       | (((cg*sg)>>8) << 8) | ((cb*sb)>>8); } }        // channels of s and c
delete[] pal; mTextColorImage->BitsChanged();
```
- The rainbow is vertical (hue = output row & 255). Each column is shifted down by a sine wave.
- Text alpha fades in over 75 ticks and out over the last 75.

### 3.13 `Draw(Graphics* g)` (0x4cbc70)

(cx,cy) = (406,219). DrawMode NORMAL unless stated.

1. `g->DrawImage(mBackgroundImage, 0, 0)`.
2. If mBlackHoleLevel > 0:
   - `SetColorizeImages(true); SetColor(Color(255,255,255,(int)(mBlackHoleLevel*255.0))); SetDrawMode(ADDITIVE);`
   - `DrawImage(mImages[30], cx-128, cy-128, Rect(mBlackHoleFrame*256, 0, 256, 256))`
   - Restore NORMAL and colorize off.
3. If `mUserPauseCount == 0`:
   1. `Graphics g2(*g); g2.ClipRect(194, 12, 428, 416);`
   2. Gems, row-major. `sx = spin ? spin*52+2 : 2; sy = color*104;`
      - If `glowing && spin == 0 && !dying`:
        - `tmp = app->CreateCrossfadeImage(mImages[8], Rect(sx,sy,49,49), mImages[8], Rect(glowFrame*52+sx, sy+52, 49,49), glowAlpha)`
        - `g2.DrawImage(tmp, (int)x, (int)y)`; delete tmp.
      - Else if dying: `g2.DrawImage(app->mGemShrinkImages[color][dyingFrame], (int)x, (int)y)`.
      - Else: `g2.DrawImage(mImages[8], (int)x, (int)y, Rect(sx, sy, 49, 49))`.
   3. Sparkles, on g (unclipped), row-major, for pieces with mSparkleCount > 0. Each sparkle is drawn ADDITIVE with `DrawImage(mImages[22], X, Y, Rect(f*40, 0, 40, 40))`:
      - count >= 16: f = frame % 14 at ((int)(x+13.0), (int)(y-7.0)).
      - count >= 8: f = (frame-8) % 14, drawn if f >= 0, at ((int)(x-10.0), (int)(y+3.0)).
      - Always: f = (frame-16) % 14, drawn if f >= 0, at ((int)(x+5.0), (int)(y+19.0)).
   4. Target cursor for each non-NULL of {mSelectedPiece, mSwapPiece1, mSwapPiece2}: `DrawImage(mImages[9], (int)(x-2.0), (int)(y-2.0), Rect((mCursorFrame%6)*52, 0, 52, 52))`.
   5. Popups:
      - SetColor(White) once.
      - Per popup: `SetFont(app->mAlphaDigitsFont); SetColor(Color(255,255,255,min(255, life*255/18)));`
      - `s = "%d"(points)`; `DrawString(s, v.mX - mPointsFont->StringWidth(s)/2, mPointsFont->GetAscent()/2 + v.mY)`.
   6. If mShowTutorial:
      - `DrawImage(mImages[34], ColX(mTutorialCol)+3, RowY(mTutorialRow)+38)`.
      - c = (sel && sel->col == mTutorialCol && sel->row == mTutorialRow) ? mTutorialCol2 : mTutorialCol.
      - `DrawImage(mImages[35], ColX(c)-5, RowY(mTutorialRow)-42+mTutorialBob)`.
   7. If mState == 8:
      - `Graphics g3(*g); g3.ClipRect(0, mWipeY-8, mWidth, 16); g3.SetDrawMode(ADDITIVE);`
      - `for (i = 0; i < (mWidth + mElectOffset + 127)/128; i++) g3.DrawImage(mImages[32], i*128 - mElectOffset, mWipeY-8);`
      - `g3.SetDrawMode(NORMAL); g3.DrawImage(mImages[7], 0, 0)` (the frame redrawn over the strip, inside the clip).
   8. If mShowingText: `ColorizeText(); DrawImage(mTextColorImage, cx - W/2, cy - H/2 - mHeight/6)` (W,H = its size).
   9. If mBlackHoleLevel > 0: `SetColorizeImages(true); SetColor(255,255,255,(int)(level*255.0)); DrawImage(mImages[31] holemask, cx - w/2, cy - h/2); SetColorizeImages(false)`.
4. Else, if `(mHasFocus || !app->mHasFocus)`, the pause screen (gems are not drawn):
   - `SetFont(app->mHeaderFont); SetColor(White)`.
   - `s1 = GetString("GAME_PAUSED")` at (cx - SW(s1)/2, cy - font->GetHeight()/2).
   - `s2 = GetString("CLICK_HERE_TO_CONTINUE")` at (cx - SW(s2)/2, cy + font->GetHeight()).
5. HUD, always:
   1. `SetFont(app->mScoreFont); SetColor(173,231,255);` "%d" mDisplayScore at (93 - mScoreFont->StringWidth(s)/2, 106).
   2. `DrawImage(app->mTimerBarImages[mBarColor], 237, 444, Rect(0, 0, mBarWidth, 25))`.
   3. `SetFont(app->mHeaderFont); SetColor(Black);` "%d" mDisplayLevel at (606 - SW/2, 465).
   4. If mMoveScoreAlpha > 0: s = "+%d" mMoveScore; `SetFont(afont1); SetColor(255,255,255,(int)(alpha*255.0))`; `DrawString(s, 91 - SW(s)/2 - font->CharWidth('+')/3, 350)`.
   5. If mHintGlowAlpha > 0: `SetColorizeImages(true); SetColor(255,255,255,(int)(alpha*255.0)); DrawImage(mImages[24], 64, 303); SetColorizeImages(false)`.

All `/2`, `/3`, `/6` are int divisions. blackcover.png is never used.

### 3.14 Savegame (`<AppData>userdata\savegame.dat`)

The original format, version 1. port(variant): a board of code-mod rules writes the v2 save instead: a header
before these fields and chunks after them (12.7).

**`HasSavedGame()` (0x4cca37):** `app->FileExists(GetAppDataFolder() + "userdata\\savegame.dat")`.

**`SaveGame()` (0x4ccdfe).**
1. `Pause(false); Unpause(false)`. This commits play seconds, sets mPlayStartTick = cnt and mInputDelay = 10.
2. Build a Buffer (little-endian, no padding):

| Offset | Size | Value |
|---|---|---|
| 0 | i32 | version = 1 |
| 4 | u8 | mTimedMode |
| 5 | i32 | mLevel |
| 9 | i32 | mDisplayLevel |
| 13 | 64 x (u8 present [+ 0x78 raw Piece bytes]) | cells row-major |
| ... | i32 | mState (raw; afterwards the in-memory state 2/3 becomes 1) |
| | u8 | mTimedMode (again) |
| | u8 | mIsNewGame |
| | i32 x 6 | mBarWidth, mPlaySeconds, mIdleTicks, mScore |
| | f64 | mPointMultiplier |
| | i32 x 6 | mDisplayScore, mGemsCleared, mMoveMatchCount, mCascadeCount, mLongestCascade, mBestMoveScore |
| | u8 | mStatsDisabled |
| | i32 x 4 | mSavedZero, mLevelStartScore, mLevelTargetScore, mLevelPoints |
| | f64 x 3 | mTimeDrain, mTimeDrainRate, mTimeDrainAccel |
| | i32 x 3 | mBarColor, mWipeY, mBlackHoleFrame |
| | f64 | mBlackHoleLevel |
| | u8 | mLevelTextShown |

The tail after the cells is 116 bytes. Exact order of the tail:
`state, timed, isNewGame, barWidth, playSeconds, idleTicks, score, mult, displayScore,
gemsCleared, moveMatchCount, cascadeCount, longestCascade, bestMoveScore, statsDisabled,
unused264, levelStartScore, levelTargetScore, levelPoints, timeDrain, timeDrainRate,
timeDrainAccel, barColor, wipeY, blackHoleFrame, blackHoleLevel, levelTextShown`.

3. `MkDir(GetAppDataFolder()+"userdata")`; return `app->WriteBufferToFile(path, &b)`.

**`LoadGame()` (0x4cca98).**
1. `if (!app->ReadBufferFromFile(path, &b, false)) return false;`
2. Read the version; return false unless it is 1.
3. byte m: `app->SetTimedMode(m, false)` (sets the mode buttons; no new game).
4. Read mLevel and mDisplayLevel.
5. 64 cells: delete the old piece (raw delete), read a flag; if set, `p = new Piece`, read 0x78 bytes into it, then snap `p->mX = ColX(p->mCol), p->mY = RowY(p->mRow)`.
6. Read state; if it is 2 or 3, set mState = 1.
7. Read the remaining tail fields in the save order.
8. app->mShowInstructions = false; return true.

Not restored:
- the swap pointers/center, selection and tutorial
- the whirlpool sound (a state-7 save resumes silently)
- the popups and mPlayStartTick
- port(variant): a board's own RNG is not in the v1 save; the sidecar next to it (`savegame.bjr`, its
  `RNGS` record) restores it: seed, stream, SEED line and the `-seeded` table. Without a matching sidecar
  the resumed game goes on on the global stream (12.3)

`SetupLevelData()` is NOT called, so the background stays the level-0 look (faithful bug). A state-4 save resumes with dying gems.

**`DeleteSavedGame()` (0x4cd130):** `app->EraseFile(path)`.

Callers:
- WinDMApp ButtonDepress 2016: LoadGame + DeleteSavedGame.
- 3016: DeleteSavedGame.
- Shutdown and ~Board: SaveGame when IsInProgress.

### 3.15 Sounds played by Board

| Id | Sound | When |
|---|---|---|
| 0 / 1 | gotset / gotsetbig | per scoring pass (pan) |
| 2..6 | combo | per scoring pass (pan) |
| 7 | bad2 | illegal or non-adjacent swap |
| 8 | gemongem2 | landing (pan, throttled) |
| 9 | explode2 | end of shake |
| 11 | select | gem selected (pan) |
| 13 | multishot | 2+ runs in a pass |
| 14 | go1 | tick 150 |
| 15 | levelup1 | vortex radius 80 crossed |
| 16 | gameover1 | after the fall |
| 17 | firecrackle | vortex done |
| 18 | warning1 | timed bar low |
| 19 | whirlpool1 | looping instance |
| 20 | excellent1 | move score crosses 120*mult |

### 3.16 RNG call order (all of Board)

1. NewBoard: per cell (rows 7..0, cols 0..7): `Rand()%240`, then `Rand()%7` until no match.
2. UpdateClearing refill: `Rand()%7` per gapped column per pass, cols 0..7, passes repeated.
3. UpdateGameOverAnim: every 6th tick and the final tick, per piece row-major: `%9` (x), `%9` (y); on the final tick also `%8` (vy) and `%7` (vx).
4. UpdateLevelTransition: `%128` every 4th tick; at the end, NewBoard, then optionally `%5` (unregistered, 15+ plays).
5. UpdateBoardDropping: 8 x `%15` per tick.
6. Outside Board: `HighScoreWidget()` ctor: 1 x `%9` (game over and every High Scores dialog open).
7. Startup consumes RNG in two Board ctors (TitleScreenIsFinished calls NewGame twice).

port(variant): items 1-5 draw through `Board::Rand()`, which is this stream unless a seed or a code mod
gives the board its own (12.3); then NewGame draws the board's seed (one `Rand()`) instead.

---

## 4. WinDMApp (`games/bejeweled/src/WinDMApp.h`)

### 4.1 Construction, Init, loading

**`WinDMApp()` (0x4d4d82).**
1. SexyApp(); construct mLastScoreName and mButtonList.
2. mShowInstructions = true; mImages = mGemShrinkImages = NULL; mTitleScreen = mBoard = NULL; mNoPopLink = false.
3. Strings:
   - mProdName = "Bejeweled"
   - mTitle = "Bejeweled Deluxe " + mProductVersion
   - mRegKey = "PopCap\\Bejeweled"
4. Fonts and mHighScoreWidget = NULL; mUnusedImagesA/B and mTimerBarImages zeroed.
5. mCustomCursorsEnabled = false; mIsDiamondMine = false.
6. Not set: the diff values, the mode buttons and mTimedMode. They rely on global zero-init (port: zero them).

**`ReadFromRegistry()` (0x4d51bc).**
1. SexyApp::ReadFromRegistry.
2. `mRegisterLink = "http://www.popcap.com/register.php?theGame=bejeweled&referid=" + mReferId`.
3. `RegistryReadString("LastScoreName", &mLastScoreName)`.
4. `RegistryReadBoolean("NoPopLink", &mNoPopLink)`.

It runs inside SexyAppBase::Init.

**`WriteToRegistry()` (0x4d5174):** SexyApp::WriteToRegistry, then `RegistryWriteString("LastScoreName", mLastScoreName)`.

**`PreDisplayHook()` (0x4d6399).**
- mIsDiamondMine = GetBoolean("IsDiamondMine", false).
- If set: `mTitle = GetString("Title", "Diamond Mine") + " " + mProductVersion`.
- It does not call the base.

**`Init()` (0x4d649e).**
1. SexyApp::Init(). This runs SexyAppBase::Init (properties hook partner.xml, ReadFromRegistry, window, PreDisplayHook) and ++mTimesExecuted.
2. If CheckForVista(): MkDir and AllowAllAccess on GetAppDataFolder(), then UserDataMigrator: AddPath("userdata"), MigrateAll. port: skip.
3. LoadProperties() (properties\default.xml).
4. `DIALOG_YES/NO/OK/CANCEL_STRING = GetString("DIALOG_BUTTON_YES"/"_NO"/"_OK"/"_CANCEL")`.
5. mImages = new Image*[36] (all NULL). For i = 0..6: `if (!DoLoadImage(i)) { Shutdown(); return; }`.
6. mHeaderFont = new ImageFont(this, "data\\afont1.txt"); mMainFont = new ImageFont(this, "data\\mainfont1.txt").
7. `mTitleScreen = new TitleScreen(this); Resize(0,0,mWidth,mHeight); mWidgetManager->AddWidget(mTitleScreen)`.
8. Music: LoadMusic 0/1/2 (section 1); PlayMusic(2, 0, false).
9. Cursors: `SetCursorImage(0, mImages[3]); (1, mImages[4]); (2, mImages[5]); (3, mImages[6])`.

The loading thread starts later, in Start().

**`LoadingThreadProc()` (0x4d82d2).**
1. mNumLoadingThreadTasks = 60.
2. diff.txt:
   - `std::fstream f("data\\diff.txt", ios::in)` (plain file, not the pak). port: VFS.
   - `f >> int 0x728 >> d 0x730 >> d 0x738 >> d 0x740 >> int 0x748 >> d 0x750 >> d 0x758 >> d 0x760 >> d 0x768 >> d 0x770 >> d 0x778 >> d 0x780`.
   - No error check; values are whitespace-separated.
3. Images 7..35: `DoLoadImage(i)`; on failure go to end; ++mCompletedLoadingThreadTasks per image.
4. Sounds, if `mSoundManager->Initialized()`: for i = 0..20, `LoadSound(i, "sounds\\" + gSoundNames[i])`.
   - On failure: `Popup("Failed to load sounds\\" + name)`, mLoadingFailed = true, go to end.
   - ++completed per sound. If the manager is not initialised, completed += 21.
5. Fonts: mAlphaDigitsFont, mScoreFont, mOverFont (section 1); completed += 2.
6. Timer bar images:
   - For k = 0..23: `mTimerBarImages[k] = CreateColorizedImage(mImages[20], Color(HSLToRGB(85*k/23, 255, 128)))`. Color(int) maps alpha 0 to 255. Hues are 0,3,7,11,...,85 (red to green).
   - `mTimerBarImages[24] = CreateColorizedImage(mImages[20], Color(255,255,255))`.
7. Gem shrink frames: `mGemShrinkImages = new MemoryImage**[7]`. For c = 0..6:
   - Row = new MemoryImage*[6]. For j = 0..5:
     ```
     m = new MemoryImage(this); m->Create(49,49);
     s = (int)((1.0 - (j+1)/7.0) * 49.0) | 1;     // 43,35,29,21,15,7 (verified; 24-bit FPU would give 43,35,27,21,13,7)
     o = (49 - s)/2;                              // 3,7,10,14,17,21
     Graphics g(m); g.DrawImage(mImages[8], Rect(o,o,s,s), Rect(2, c*104, 49, 49));   // stretch, mFastStretch=false
     row[j] = m;
     ```
   - ++completed per color.
8. `PrecacheAdditive(mImages[30]); PrecacheAlpha(mImages[31]);` ++completed.

No RNG.

`LoadingThreadCompleted()` (0x4d4591) is empty. `CreateMusicInterface()` (0x4d45b1) always returns a new BassMusicInterface (port: the SDL music interface).

### 4.2 Title screen to game

**`TitleScreenIsFinished()` (0x4d6f28).** Called by TitleScreen::ButtonDepress.
1. `mMusicInterface->FadeOut(2, true, 0.004)`.
2. `mWidgetManager->RemoveWidget(mTitleScreen); SafeDeleteWidget(mTitleScreen); mTitleScreen = NULL;`
3. Side-bar buttons, `CreateControlButton(id, Rect, overImg, downImg)`:

| Id | Button | Rect (x,y,w,h) | over | down |
|---|---|---|---|---|
| 10 | New Game | (18,142,156,41) | -1 | 13 |
| 11 | Normal (mNormalModeButton) | (18,185,78,31) | -1 | 14 |
| 12 | Timed (mTimedModeButton) | (96,185,78,31) | -1 | 16 |
| 13 | Options | (18,241,156,41) | -1 | 18 |
| 14 | Quit | (18,405,156,40) | -1 | 19 |
| 15 | Hint | (64,303,53,73) | 24 | 24 |

   The hint button then gets `mMouseInsets = Insets(-16,-8,-16,-8)`.
4. `SetTimedMode(false, true)`. This already calls NewGame().
5. `NewGame()` again. The first board is SafeDeleted; both ctors consume RNG; the music fades repeat.
6. `TryConfirmLoad()`.

**`CreateControlButton` (0x4d6e71).**
1. `b = new ButtonWidget(id, this)`; `b->Resize(rect)`; `b->mDoFinger = true`.
2. `if (over != -1) b->mOverImage = mImages[over]`; `b->mDownImage = mImages[down]`.
3. `b->mMouseInsets = Insets(1,1,3,3)`.
4. mButtonImage stays NULL: the buttons are invisible until hovered or pressed, and the board background shows the art.
5. `mWidgetManager->AddWidget(b); mButtonList.push_back(b)`.

**`SetTimedMode(bool timed, bool newGame)` (0x4d4730).**
1. mTimedMode = timed.
2. Timed: `mNormalModeButton->mButtonImage = NULL; mTimedModeButton->mButtonImage = mImages[17]`. Normal: `mNormalModeButton->mButtonImage = mImages[15]; mTimedModeButton->mButtonImage = NULL`.
3. MarkDirty both buttons. If newGame, call NewGame().

**`NewGame()` (0x4d4455).**
1. If mHighScoreWidget: RemoveWidget, SafeDeleteWidget, NULL. Its dtor re-saves hiscores.dat.
2. If mBoard: RemoveWidget and SafeDeleteWidget. The Board dtor may save the game and ++mTimesPlayed.
3. `mBoard = new Board(this, mTimedMode); mBoard->Resize(0,0,640,480);`
4. `AddWidget(mBoard); mWidgetManager->BringToBack(mBoard); mWidgetManager->SetFocus(mBoard);`
5. Music:
   - Timed: `FadeOut(0, true, 0.004); FadeIn(1, -1, 0.002, false)`.
   - Normal: `FadeOut(1, true, 0.004); FadeIn(0, -1, 0.002, false)`.

**`ShowHighScoreWidget()` (0x4d47b1).** Called by Board state 5.
1. `mWidgetManager->RemoveWidget(mBoard)`. The Board is not deleted.
2. `mHighScoreWidget = new HighScoreWidget(this); Resize(0,0,mWidth,mHeight); AddWidget; BringToBack`.

**`Shutdown()` (0x4d43f9).**
1. If not mShutdown, and mBoard && `mBoard->IsInProgress()`:
   - `mBoard->SaveGame(); RemoveWidget(mBoard); SafeDeleteWidget(mBoard); mBoard = NULL`.
   - When the deferred delete runs, ~Board sees the game still in progress: it saves again and ++mTimesPlayed.
2. Then SexyAppBase::Shutdown.

**`~WinDMApp` (0x4d4b08).**
1. Delete the 36 images and delete[] the array.
2. Delete the 42 shrink images (the arrays leak).
3. Remove and delete each button.
4. Delete mUnusedImagesA/B and the 25 timer bars.
5. Remove and delete mHighScoreWidget.
6. Delete fonts 0x7c4, 0x7cc, 0x7d0 and 0x7d4 (not 0x7c8).
7. Remove and delete mBoard and mTitleScreen.
8. SexyApp dtor.

### 4.3 Dialogs

**`NewDialog(id, modal, header, lines, footer, mode)` (0x4d4653).**
- `new Dialog(mImages[10], mImages[11], id, modal, header, lines, footer, mode)`.
- `SetButtonFont(mMainFont); SetHeaderFont(mHeaderFont); SetLinesFont(mMainFont); Resize(250, 50, 316, GetPreferredHeight(316))`.
- It is not added. SexyAppBase::DoDialog = KillDialog(id) + NewDialog + AddDialog, so every DoDialog gets this default layout.

Button modes: 0 none, 1 YES_NO, 2 OK_CANCEL, 3 FOOTER.

| Function | Dialog | Content / layout |
|---|---|---|
| ConfirmNewGame (0x4d5610) | 0 | If `mBoard->mState == 6`, NewGame() directly. Else DoDialog(0, modal, NEWGAMEDIALOG1, NEWGAMEDIALOG2, "", YES_NO). |
| ConfirmQuit (0x4d58af) | 1 | lines = CONFIRMQUIT1, + CONFIRMQUIT2 if `mBoard && mBoard->IsInProgress()`. DoDialog(1, modal, GetString("QUIT"), lines, "", YES_NO). |
| DoOptions (0x4d483e) | 2 | `new OptionsDialog(this)`, Resize(234, 50, 348, prefH(348)), AddDialog(2). |
| ConfirmNormalMode (0x4d56ec) | 3 | If state 6, SetTimedMode(false,true). Else DoDialog(3, CONFIRMNORMALMODE1/2, YES_NO). |
| ConfirmTimedMode (0x4d57cd) | 4 | If state 6, SetTimedMode(true,true). Else DoDialog(4, CONFIRMTIMEDMODE1/2, YES_NO). |
| ShowRegisterDialog (0x4d5272) | 5 / 19 | If mIsRegistered: `d = DoDialog(19, true, "Already Registered", "You have already registered Bejeweled!", "OK", FOOTER)` (literals), then `d->Move(d->mX, d->mY+24)`. Else `new RegisterDialog`, Resize((mWidth-348)/2, 32, 348, prefH), AddDialog(5). port: only the 19 branch. |
| ShowRegistrationInvalidDialog (0x4d5386) | 6 | DoDialog(6, REGISTRATION_INVALID_DIALOG1/2/3, FOOTER); PlaySample(7). |
| ShowRegisterToRecordScoreDialog(lines) (0x4d5486) | 7 | DoDialog(7, GetString("HIGH_SCORE"), lines, "", YES_NO). |
| DoRecordScoreDialog(lines) (0x4d45e3) | 8 | `new RecordScoreDialog(this, lines)`, Resize(250, 32, 316, prefH(316)), AddDialog(8). |
| DoInstructionDialogette (0x4d59cc) | 9 | `NewDialog(9, false, "", INSTRUCTIONDIALOG, "", NONE)`; mContentInsets = Insets(16,24,16,24); Resize(201, 38, 418, prefH(418)); AddDialog(9). Non-modal. |
| OpenURL (0x4d5077) | 10 | `NewDialog(10, true, "", OPENING_BROWSER, "", NONE)`; mContentInsets = (16,26,16,16); Resize(180,140,280,70); AddDialog; return SexyAppBase::OpenURL(url, shutdown). |
| URLOpenFailed (0x4d4e97) | 11 | Base call; KillDialog(10); CopyToClipboard(url); NewDialog(11, true, OPEN_BROWSER, URL_OPEN_ERROR1 + url + URL_OPEN_ERROR2, DIALOG_BUTTON_OK, FOOTER); Resize(100, 50, 440, prefH(440)); AddDialog. |
| URLOpenSucceeded (0x4d4592) | - | Base call; KillDialog(10). |
| DoFullInstructions (0x4d5ac1) | 12 | NewDialog(12, true, FULL_INSTRUCTIONS1, FULL_INSTRUCTIONS2, DONE, FOOTER); mTextAlign = -1 (left); Resize(32, 8, mWidth-64, prefH(mWidth-64)); AddDialog. |
| DoColorDepthExplanation (0x4d5bef) | 12 | DoDialog(12, COLOR_DEPTH_EXPLANATION1/2, DIALOG_BUTTON_OK, FOOTER). |
| DoHighScoreDialog (0x4d48b6) | 13 | `new HighScoreDialog(this)`, Resize(194, 50, 428, 326), AddDialog(13). |
| ShowRegisterNagDialog (0x4d5cdf) | 15 | See below; NewDialog(15, true, REGISTER_NOW, msg, "", YES_NO); Resize(234, 50, 348, prefH(348)); AddDialog. |
| TryConfirmLoad (0x4d550d) | 16 | If `mBoard->HasSavedGame()`: d = DoDialog(16, CONFIRM_LOAD_DIALOG1/2, YES_NO); d->Resize(234, 50, 348, prefH(348)). |
| DoUpdateCheck (0x4d68da) | 18 | UpdateCheckDialog + HTTP check. Not ported. |

**Register nag message.**
```
sprintf(buf, GetString(mIsDiamondMine ? "YOU_HAVE_PLAYED_DIAMOND_MINE_D_TIMES"
                                      : "YOU_HAVE_PLAYED_BEJEWELLED_D_TIMES").c_str(), mTimesPlayed);
msg = buf;
```
Then append:
- if mTimesPlayed >= 40: `YOU_ARE_A_DIAMOND_MINE_AHOLIC` or `YOU_ARE_A_BEJEWELAHOLIC`
- else if >= 30: `LOOKS_LIKE_YOU_ARE_ADDICTED`
- else if >= 25: `ARE_YOU_GETTING_HOOKED`
- always `LOADING_NAG`

**`KillDialog(int id)` (0x4d4a0e).** If `SexyAppBase::KillDialog(id)` succeeds:
- When the dialog map is now empty, `mWidgetManager->SetFocus(mBoard)` (mBoard may be NULL).
- Return true; otherwise return false.

**Pause on modal/focus.**
- `ModalOpen()` and `LostFocus()` (shared body 0x4d46fc): `if (mBoard) mBoard->Pause(mTimedMode)`.
- `ModalClose()` and `GotFocus()` (0x4d4716): `if (mBoard) mBoard->Unpause(mTimedMode)`.
- None of them call the base.

### 4.4 Buttons

**`ButtonPress(int id)` (0x4d48a9):** PlaySample(12 menuclick) for every id. This includes dialog presses, which SexyAppBase forwards as 2000+id / 3000+id.

**`ButtonDepress(int id)` (0x4d6a40).** r = id % 10000.

YES/OK/footer: `2000 <= r < 3000`, d = id - 2000.

| d | Action |
|---|---|
| 10018 | KillDialog(10018); KillDialog(18); OpenUpdateURL() |
| 20018 | KillDialog(20018); KillDialog(18) |
| other > 10018 | KillDialog(d) |
| 0 | `mBoard->RequestNewGame(); KillDialog(0)`. The board shakes 50 ticks, falls, then NewGame. |
| 1 | `Shutdown(); KillDialog(1)` |
| 2 | Options Done, see below |
| 3 | KillDialog(3); SetTimedMode(false, true) |
| 4 | KillDialog(4); SetTimedMode(true, true) |
| 5 | Register, see below |
| 7 | ShowRegisterDialog(); KillDialog(7) |
| 8 | Record score, see below |
| 15 | KillDialog(15); ShowRegisterDialog() |
| 16 | KillDialog(16); `mBoard->LoadGame(); mBoard->DeleteSavedGame()` |
| 17 | KillDialog(17); DoUpdateCheck() |
| default | KillDialog(d) |

- d = 2 (Options Done): `o = (OptionsDialog*)GetDialog(2)`.
  1. `windowed = !o->mFullscreenCheckbox->IsChecked()`; if mIsWindowed != windowed, `SwitchScreenMode(windowed)`.
  2. `EnableCustomCursors(o->mCustomCursorsCheckbox->IsChecked())`.
  3. KillDialog(2).
- d = 5 (Register): name and code come from RegisterDialog +0x158 / +0x15c ->mString.
  - If `!Validate(name, code)`: ShowRegistrationInvalidDialog(), and dialog 5 stays open.
  - Otherwise: mUserName (+0x6c0) = name; mRegCode = code; mIsRegistered = true; KillDialog(5); MarkAllDirty; `if (mHighScoreWidget) mHighScoreWidget->CheckForHighScore()`.
- d = 8 (Record score): `mHighScoreWidget->AddHighScore(dlg8->mNameWidget->mString)` (no NULL check); mLastScoreName = that string; KillDialog(8).

NO/CANCEL: `3000 <= r < 4000`, d = id - 3000.

| d | Action |
|---|---|
| 16 | KillDialog(16); mBoard->DeleteSavedGame() |
| 10018 | KillDialog(10018); KillDialog(18) |
| other | KillDialog(d) |

Then, for every id, a second switch:

| Id | Action |
|---|---|
| 10 | ConfirmNewGame() |
| 11 | `if (!mTimedMode) ConfirmNewGame(); else if (mBoard->mIsNewGame) SetTimedMode(false, true); else ConfirmNormalMode();` |
| 12 | `if (mTimedMode) ConfirmNewGame(); else if (mBoard->mIsNewGame) SetTimedMode(true, true); else ConfirmTimedMode();` |
| 13 | DoOptions() |
| 14 | ConfirmQuit() |
| 15 | mBoard->ShowHint() |

### 4.5 Registry and files

| Item | Detail |
|---|---|
| HKCU\SOFTWARE\PopCap\Bejeweled\LastScoreName | REG_SZ, read and written |
| NoPopLink | DWORD, read only, unused |
| savegame.dat | `<AppData>userdata\savegame.dat` (Board) |
| hiscores.dat | `<AppData>userdata\hiscores.dat` (HighScoreWidget) |
| diff.txt | `data\diff.txt` (read only) |

On pre-Vista systems, `<AppData>` is "" (the current directory). port: the user dir.

---

## 5. TitleScreen (`games/bejeweled/src/TitleScreen.h`)

**Ctor (0x4d41c8).**
1. Widget; mLoadingProgress = mDelayProgress = 0.0; mApp = app; mLoadingComplete = mRegisterClicked = false.
2. tp = app->mTimesPlayed.
   - `mTotalTimerSec = tp<=10 ? 10 : tp<=20 ? tp : tp<=50 ? 2*tp-20 : 80`, then `= GetInteger("TotalTimerSec", that)`.
   - `mDisplayedPlayCount = tp<2 ? 0 : tp<10 ? tp : 2`.
3. `mRegisterLink = new HyperlinkWidget(0, this)`: SetFont(app->mMainFont) (the link stores Duplicate()); mColor = (32,255,32); SetVisible(!app->mIsRegistered).
4. `mPlayLink = new HyperlinkWidget(1, this)`: SetFont(app->mHeaderFont); SetVisible(false); mColor = (32,255,32). mOverColor stays white.
5. A direct call to `TitleScreen::Update()`. It sets mUpdateCnt = 1, the statics, mMustRegister and the labels; mWidth is still 0 at this point.

**`AddedToManager` / `RemovedFromManager`.** WidgetContainer base, then add/remove both links as top-level WidgetManager widgets (after the title screen, so they draw on top).

**`Resize` (0x4d4118).** `Widget::Resize`, then:
```
w = mRegisterLink->mFont->StringWidth(mRegisterLink->mLabel);
mRegisterLink->Resize((mWidth-w)/2, sRegisterLinkY /*324*/, w, 16);
```
It is never re-laid-out after this.

**`Update` (0x4d3c6a).** Statics: sPlayLinkY = GetInteger("PlayLinkY", 386), sMaxPlays = GetInteger("MaxPlays", 15), sMaxExecutions = GetInteger("MaxExecutions", 0).
1. `Widget::Update()`.
2. `mMustRegister = app->mTimesPlayed >= sMaxPlays && !app->mIsRegistered`.
3. `if (sMaxExecutions > 0) mMustRegister = app->mTimesExecuted > sMaxExecutions && !app->mIsRegistered`.
4. Nag mode (mMustRegister):
   - app->mTimedOut = true.
   - At mUpdateCnt == 1000: mPlayLink->mLabel = CLICK_HERE_TO_QUIT; w = its font's StringWidth; mPlayLink->Resize((mWidth-w)/2, 386, w, 24); SetVisible(true).
   - mRegisterLink->mLabel = CLICK_HERE_TO_REGISTER_NOW.
   - The bar and timer do not update.
5. Normal mode:
   1. Play link: mLabel = CLICK_HERE_TO_PLAY; `Resize((mWidth-w)/2, sPlayLinkY, w, 24)`, every frame.
   2. mRegisterLink->mLabel = REGISTER_NOW_TO_REMOVE_THIS_DELAY.
   3. `mDelayProgress = 0.01/(double)mTotalTimerSec + mDelayProgress`, clamped to 1.0.
   4. `n = (int)((double)(tp+1) * mDelayProgress); if (n <= tp && n > mDisplayedPlayCount) { mDisplayedPlayCount = n; MarkDirty; }`
   5. If `!mLoadingComplete && mUpdateCnt % 4 == 0`:
      - Registered: `mLoadingProgress = app->GetLoadingThreadProgress()`.
      - Otherwise: `mLoadingProgress = min(progress, mDelayProgress)`. The original calls GetLoadingThreadProgress a second time for the <= case.
      - `if (mLoadingProgress == 1.0 && app->mLoaded) { mPlayLink->SetVisible(true); mLoadingComplete = true; }`
      - MarkDirty.
6. Always: `mRegisterLink->SetVisible(!app->mIsRegistered)`. Then `if (app->mLoaded && mRegisterClicked) { app->ShowRegisterDialog(); mRegisterClicked = false; }`.

**`Draw` (0x4d370b).** Statics: LoadingBarY 275, RegisterStringY 319, MaxPlays 15, MaxExecutions 0.
1. `DrawImage(mImages[0], 0, 0)`.
2. If `!app->mIsRegistered && !GetBoolean("Cobranded", false)`: `DrawImage(mImages[2], 31, 346)`.
3. `DrawImage(mImages[1], 143, 275, Rect(0, 0, (int)(mLoadingProgress*340.0), mImages[1]->GetHeight()))`.
4. `SetFont(mainfont1); SetColor(255,64,255)`. Status line into a 256-byte buffer:
   - Registered: THANKS_FOR_REGISTERING.
   - Else if mMustRegister: YOU_MUST_REG1 (Diamond Mine) or YOU_MUST_REG2.
   - Else if sMaxExecutions == 0: literal `"YOU HAVE %d PLAYS REMAINING"` with sMaxPlays - mTimesPlayed.
   - Else: literal `"YOU CAN RUN BEJEWELED %d MORE TIME[S] FOR FREE"` (singular when n == 1).
   - `if (!app->mBuildUnlocked) WriteCenteredLine(g, 319, buf)`.
5. `SetColor(128,128,128)`; s = GetString("VERSION") + " " + mProductVersion; `DrawString(s, 537 - StringWidth(s)/2, 221)` (font still mainfont1).

**`ButtonDepress(id)` (0x4d36c1).** No sound.
- id 0: mRegisterClicked = true.
- Otherwise: `mMustRegister ? app->Shutdown() : app->TitleScreenIsFinished()`.

**Steam config (partner.xml NoReg=true, so mIsRegistered = mBuildUnlocked = true).**
- Visible: titlescreen6, the green bar (real progress), "Version 1.87" and the green CLICK HERE TO PLAY! link (white on hover).
- Everything else is dead (status line, trial circle, nag, register link).

---

## 6. HighScoreWidget (`games/bejeweled/src/HighScoreWidget.h`)

**Ctor (0x4d07cd).**
1. mSlideY = 0; mApp = app; mColorCycle = 0.
2. Play-again link: `mPlayAgainLink = new HyperlinkWidget(0, this)`.
   - mLabel = HSWIDGET_CLICK_HERE_TO_PLAY_AGAIN; SetFont(afont1).
   - mColor = (0,0,0); mOverColor = (255,255,255); mUnderlineSize = 0.
3. mBoardImage = NULL.
4. RNG: `r = Rand()%9`; mTip = GetString(r <= 7 ? "HSWIDGET_TIP" + r : "HSWIDGET_TIP_DEFAULT").
5. Defaults:
   - Today: "" / 0 for both modes.
   - Normal: Ace 1000, John 900, Chris 800, Brian 700, Ben 600.
   - Timed: Bradley 1000, Mike 900, Billy 800, Jason 700, Wim 600.
6. `ReadFromFile()`.
7. port(variant): the table file is chosen before the load, from the board then in `mApp->mBoard`: a
   user-seeded game has its own table (12.3).

**`~HighScoreWidget` (0x4d0dad).** Delete mBoardImage and mPlayAgainLink, then `WriteToFile()`. It always re-saves.

**`Resize` (0x4cedf5).**
1. Widget::Resize; mSlideY = -theHeight.
2. If mBoardImage is NULL, take the board snapshot:
   - `new DDImage(app->mDDInterface); Create(mWidth, mHeight);`
   - `Graphics g(img); app->mBoard->Draw(&g); img->GenerateDDSurface()`.

**`AddedToManager` / `RemovedFromManager`.** Base, then add/remove mPlayAgainLink as a top-level widget.

**`Update` (0x4d1187).**
1. Widget::Update.
2. If mSlideY < 0:
   - mSlideY += 8; if it is now >= 0: mSlideY = 0 and `ProcessScoreStuff()`.
   - MarkDirty; `mPlayAgainLink->Resize(265, mSlideY + 389, 286, 23)`.
   - At height 480 the slide takes 60 ticks.
3. `if (mUpdateCnt % 10 == 0) { ++mColorCycle; MarkDirty; }`

**`ButtonDepress` (0x4ceeb7):** `app->NewGame()` (no sound).

**`GetScorePlace(mode, score)` (0x4ced82).** `rank = 0; for (i = 4; i >= 0; i--) if (score > mScores[mode][i]) rank = i+1;`. Ties do not place.

**`GetDayNum()`:** `tm_year*366 + tm_yday` from `_localtime64(_time64())`.

**`GetOrderSuffix(n)`:** `(n%100)/10 == 1` gives "th"; otherwise n%10 of 1/2/3 gives "st"/"nd"/"rd", anything else "th".

**`ProcessScoreStuff()` (0x4d0e4b).** The virtual `CheckForHighScore()` (0x4d11fd) jumps here.
1. `score = app->mBoard->mScore; mode = app->mTimedMode` (app's flag).
2. `best = score > mTodayScores[mode]; rank = GetScorePlace(mode, score); N = "%d"(rank) + GetOrderSuffix(rank)`.
3. Messages (literals). `\n` is a newline; R = `"\n\nWould you like to register so\nyou can record your score?"`.

| Case | Registered | Unregistered |
|---|---|---|
| best && rank > 0 | `"You got the highest score of\nthe day and " + N + " place\nof all time!"` | same + R |
| best && rank == 0 | `"You got the highest score\nof the day!"` | same + R |
| !best && rank > 0 | `"You got " + N + " place of all time!"` | same + R |
| otherwise | no dialog | no dialog |

   - Registered: `app->DoRecordScoreDialog(msg)`. Unregistered: `app->ShowRegisterToRecordScoreDialog(msg)`.
   - The HSWIDGET_*PLACEMENT* property strings are unused.

**`AddHighScore(name)` (0x4d06d8).** Called on dialog 8 OK or Enter.
1. If name is empty, return without saving.
2. `mode = app->mBoard->mTimedMode` (the Board's flag); score = board->mScore; rank = GetScorePlace(mode, score).
3. `if (score > mTodayScores[mode]) { mTodayScores[mode] = score; mTodayNames[mode] = name; }`
4. `if (rank) { for (k = 4; k >= rank; k--) shift [k-1] -> [k]; mNames[mode][rank-1] = name; mScores[mode][rank-1] = score; }`
5. `WriteToFile()`.

**hiscores.dat format.**
- Little-endian, no header. Two tables, Normal then Timed. Each table:
  ```
  int32 day                                  // GetDayNum() at save time
  today:   u16 len, char name[len], int32 score, int32 checksum
  5 x all-time: u16 len, char name[len], int32 score, int32 checksum
  ```
- `checksum = score ^ 0x42BEEF ^ XOR_k((int)(signed char)name[k] * 17)`.
- File size = 8 + 12*10 + total name bytes.

**Load (`ReadFromFile`, 0x4ceec2).** `fopen(appdata+"userdata\\hiscores.dat","rb")`; if missing, keep the defaults. Per table:
1. Read day; today's len (min(len,255)), name, score and checksum.
2. The today record is accepted only if `day == GetDayNum() && checksum ok`; otherwise the defaults stay.
3. For each all-time entry: the name is always taken; score = checksum ok ? score : score % 256 (tamper penalty).
4. fread results are never checked. port: stop at EOF and keep the defaults (see open questions).

**Save (`WriteToFile`, 0x4cf1b4).**
1. `DeleteFileA(path); MkDir(appdata+"userdata"); fopen "wb"`.
2. On failure: `Popup("Failed to create high score file!\nPlease report to support@popcap.com.")` and return.
3. Writes the format above. The day is always today, so stale today records saved earlier as ""/0 are re-stamped.

**`Draw` (0x4cf50a).**

Setup:
- `SetColor(Black); FillRect(0,0,mWidth,mHeight)`.
- `clipH = min(mSlideY + mHeight, 436)`.
- `Graphics g2(*g); g2.ClipRect(0,0,mWidth,clipH); g2.Translate(0,mSlideY)`.

Panel content (g2, panel coordinates; R = right edge):
1. `SetColor(21,0,56); FillRect(0,0,mWidth,mHeight)`.
2. afont1, (255,0,177): HSWIDGET_GAME_OVER at (408 - w/2, 45).
3. mainfont1, (252,0,195), right-aligned labels:

| Label | R | y |
|---|---|---|
| GAME_DURATION | 341 | 88 |
| GEMS_CLEARED | 341 | 105 |
| LONGEST_CASCADE | 546 | 88 |
| BEST_SINGLE_MOVE | 546 | 105 |

4. (255,100,220), right-aligned values:

| Value | Format | R | y |
|---|---|---|---|
| board mGameDuration d | "%2d.%02d" (d/60, d%60) | 382 | 88 |
| mGemsCleared | "%d" | 382 | 105 |
| mLongestCascade | "%d" | 587 | 88 |
| mBestMoveScore | "%d" | 587 | 105 |

5. mainfont1, (250,0,0): mTip at (408 - w/2, 136).
6. (0,218,0): FillRect(191,151,434,23).
7. (0,0,0), afont1: HSWIDGET_HIGH_SCORES at ((434-w)/2 + 191, (ascent+23)/2 + 150).
8. (0,218,0): FillRect(191,389,434,23). This is the play-again bar.
9. For m in {0,1}, cx = 287 + 238*m:
   - mainfont1, (255,193,0): NORMAL/TIMED at (cx - w/2, 197) with an underline FillRect(x, 200, w, 1).
   - (46,216,0): TODAYS_HIGH_SCORE at y 223, underline at 226.
   - Color `HSLToRGB((mColorCycle*20)%256, 255, 180)`. Today's row at y 249: EMPTY centered if the score <= 0; otherwise the name at cx-74 and the score right-aligned at cx+74.
   - (46,216,0): ALL_TIME_HIGH_SCORES at y 279, underline at 282.
   - For i = 0..4, y = 303 + 17*i: color `HSLToRGB(((mColorCycle+2+i)*20)%256, 255, 180)`; the name at cx-74 and the score right-aligned at cx+74.

Overlay:
10. `g2.Translate(0,-mSlideY)` (still clipped to clipH):
    - `DrawImage(mImages[7] alphaback1, 0, 0)`.
    - scorefont1, (173,231,255): "%d" board->mScore at (93 - w/2, 106).
11. `Graphics g3(*g); g3.ClipRect(0, clipH, mWidth, mHeight-clipH); g3.DrawImage(mBoardImage, 0, 0)` (the board snapshot below the panel).

## 7. HighScoreDialog (`games/bejeweled/src/HighScoreDialog.h`)

**Ctor (0x4ce4bc).**
1. `Dialog(mImages[10], mImages[11], 13, true, GetString("HSDLG_HIGH_SCORES"), "", GetString("HSDLG_DONE"), FOOTER)`.
2. `mHighScoreWidget = new HighScoreWidget(app)`. This consumes 1 Rand() and loads hiscores.dat; the widget is never shown.
3. mApp = app; fonts: buttons mainfont1, header afont1, lines mainfont1.

**`GetPreferredHeight`:** 326.

**Dtor.** Deletes the widget, which re-saves hiscores.dat.

**`Draw` (0x4ce60b).** Dialog::Draw, then for m in {0,1}, cx = mWidth/2 + (m ? 90 : -90):
1. mainfont1, (255,255,128): NORMAL/TIMED centered at y 73, underline FillRect(x,76,w,1).
2. TODAYS_HIGH_SCORE at y 99, underline 102.
3. (255,255,0): today's entry at y 125. EMPTY if <= 0; otherwise the name at cx-74 and the score right-aligned at cx+74.
4. (255,255,128): ALL_TIME_HIGH_SCORES at y 155, underline 158.
5. (255,255,0): 5 entries at y = 179 + 17*i.

At the shipped size (194,50,428,326) the columns are at x 124 and 304.

## 8. OptionsDialog (`games/bejeweled/src/OptionsDialog.h`)

**Ctor (0x4d1fda).**
1. `Dialog(mImages[10], mImages[11], 2, true, GetString("OPTIONSDLG_OPTIONS"), "", GetString("OPTIONSDLG_DONE"), FOOTER)`.
2. Checkbox images: `CopyImage(mImages[27], Rect(0,0,31,32))` (checked) and `CopyImage(mImages[27], Rect(31,0,31,32))` (unchecked).
3. Fonts as in the other dialogs.
4. Buttons: `new DialogButton(mImages[11], id, this)` + SetFont(mainfont1) + label, for:
   - Help (id 0, OPTIONSDLG_HELP)
   - Register (id 1, OPTIONSDLG_REGISTER)
   - High Scores (id 2, OPTIONSDLG_HIGH_SCORES)
5. Links: `for (i = 0; i < GetInteger("NumOptionsLinks", 1); i++)`, re-reading the property on each test:
   - DialogButton id 100+i, label `GetString(StrFormat("OptionsLinkText%d", i), "Visit PopCap.com For More Games")`.
   - partner.xml gives 1 link, "Play More Great Games!".
6. mCheckUpdatesButton: id 4, OPTIONSDLG_CHECK_FOR_UPDATES.
7. Sliders (`new Slider(mImages[25], mImages[26], id, this)`):
   - Slider 0: `SetValue(app->GetMusicVolume())`.
   - Slider 1: `SetValue(app->GetSfxVolume())`.
8. Checkboxes (`new Checkbox(unchecked, checked, id, this)`):
   - Checkbox 0: `SetChecked(!app->mIsWindowed, true)`. This notifies the listener during construction.
   - Checkbox 1: `SetChecked(app->mCustomCursorsEnabled, true)`.

**`GetPreferredHeight`:** 256 + 34*links (290 shipped).

**`AddedToManager`.** Adds, in order: help, register, high scores, music slider, sfx slider, cursor checkbox, fullscreen checkbox, check-updates button, then the links. `RemovedFromManager` mirrors it.

**`Resize(x,y,w,h)` (0x4d15ac).** Dialog::Resize, then:
- If `!app->mBuildUnlocked`, three buttons with bw = (mWidth-60)/3:
  - help (mX+24, mY+166, bw, 32)
  - register (mX+30+bw, …)
  - high scores (mX+36+2bw, …)
- Else two buttons with bw = (mWidth-54)/2:
  - help (mX+24, mY+166, bw, 32)
  - high scores (mX+30+bw, …)
  - register is never sized (0x0, invisible). This is the shipped case.
- Link i: `(mX+24, (i-n)*34 + mYesButton->mY, mWidth-48, 32)`, stacked just above Done.
- Sliders: (mX+136, mY+60, mWidth-160, 32) and (mX+136, mY+92, mWidth-160, 32).
- Checkboxes: fullscreen (mX+32, mY+124, 32, 32), cursors (mX+146, mY+124, 32, 32).
- mCheckUpdatesButton is never resized, so it is invisible.

**`Draw`.** Dialog::Draw, then mainfont1 with SetColorizeImages(true) and color (255,255,0):
- "Music" at (32,80); "Sound Fx" at (32,112).
- FULLSCREEN at (fs.mX-mX+34, fs.mY-mY+21); CUSTOM_CURSORS likewise relative to its checkbox.
- Then colorize off.

**`SliderVal(id, v)`:** id 0 calls `app->SetMusicVolume(v)`, id 1 calls `app->SetSfxVolume(v)` (live).

**`CheckboxChecked(id, checked)`.** Only when `id == 0 && !checked && app->mForceFullscreen`: `app->DoColorDepthExplanation(); mFullscreenCheckbox->SetChecked(true, true)`.

**`ButtonPress(id)`:** Dialog::ButtonPress(id), then PlaySample(12). The footer Done therefore plays menuclick twice (here and in WinDMApp::ButtonPress(2002)).

**`ButtonDepress(id)`.** Dialog::ButtonDepress(id) (Done leads to app 2002), then:
- id 100..199: `app->OpenURL(GetString(StrFormat("OptionsLinkURL%d", id-100), "http://www.popcap.com"), false)`. Shipped: steam://publisher/popcap. port: platform open.
- 0: DoFullInstructions. 1: ShowRegisterDialog. 2: DoHighScoreDialog.
- 3: OpenURL("http://www.popcap.com") (no such button). 4: DoUpdateCheck (unreachable).

## 9. RecordScoreDialog (`games/bejeweled/src/RecordScoreDialog.h`)

**Ctor (0x4d29b6).**
1. `Dialog(mImages[10], mImages[11], 8, true, GetString("RECORDSCOREDLG_HIGH_SCORE"), theLines, "", OK_CANCEL)`.
2. Fonts as in the other dialogs.
3. `mNameWidget = new EditWidget(0, this)`:
   - `SetFont(mainfont1, NULL)`
   - `SetColors(gRecordScoreEditColors, 5)`
   - `mMaxPixels = 104`
   - `SetText(app->mLastScoreName, true)`

**Layout.**
- `GetPreferredHeight` = Dialog's + 56.
- `Resize`: Dialog::Resize; `mNameWidget->Resize(mX+48, mY+mHeight-100, mWidth-96, 24)`.

**Manager hooks.** `AddedToManager` adds the edit widget and gives it focus. `RemovedFromManager` removes it.

**`EditWidgetText` (Enter):** `app->ButtonDepress(2000 + mId)` (2008). Dialog::ButtonDepress is not called and no sound plays.

**`Draw`.**
1. Dialog::Draw.
2. `r = mNameWidget->GetRect()` relative to the dialog, `Inflate(4,4)`, then `DrawEditBox(g, r)`.
3. mainfont1, (255,255,0): RECORDSCOREDLG_ENTER_YOUR_NAME at (name.mX-mX, name.mY-mY-8).

**`DrawEditBox(g, r)` (0x4d2857).**
```
DrawImage(editbox, r.mX, r.mY, Rect(0,0,32,32));
DrawImage(editbox, r.mX + r.mWidth - 32, r.mY, Rect(64,0,32,32));
Graphics g2(*g); g2.ClipRect(r.mX+32, r.mY, r.mWidth-32, 32);
for (i = 0; i < (r.mWidth-33)/32; i++) g2.DrawImage(editbox, r.mX+32+32*i, r.mY, Rect(32,0,32,32));
```
The middle tiles are drawn after the right cap and can overdraw it. Keep this order.

On OK (2008): AddHighScore + mLastScoreName, then KillDialog(8). Cancel (3008): KillDialog(8).

## 10. RegisterDialog (not ported; reference)

- Dialog id 5, header REGISTER_BEJEWELED / REGISTER_DIAMOND_MINE, OK_CANCEL, height 320.
- Name and code EditWidgets (ids 0/1, tab ring, colors gRegisterEditColors).
- A yellow mainfont1 HyperlinkWidget CLICK_HERE_TO_REGISTER_ONLINE calls `app->OpenRegisterPage(DefinesMap())` and switches to windowed if fullscreen. The dtor switches back.
- Enter in the name field moves focus to the code field; Enter in the code field leads to ButtonDepress(2005).
- Under NoReg nothing reaches it: ShowRegisterDialog takes the "Already Registered" branch (dialog 19).

---

## 11. Cross-module flows

**Start.**
1. Init → TitleScreen. Start → loading thread.
2. CLICK HERE TO PLAY → TitleScreenIsFinished: buttons, two NewGame() calls (normal mode), then TryConfirmLoad.
3. If savegame.dat exists, dialog 16 is shown:
   - YES: LoadGame (sets the mode buttons from the save), then delete the file.
   - NO: delete the file.

**Playing.** Board ticks at 100 Hz.

| Event | Effect |
|---|---|
| Tick 150 | GO banner, or the instructions dialog + tutorial arrows for the first 5 games |
| Hint button | ShowHint |
| Key press | pause toggle |
| Dialog or focus loss | pause (visible in timed mode) |

**Level complete.** Bar full → state 7 vortex → state 8 wipe → NewBoard (state 0).

**No moves.**
- Normal mode: NO MOVES! → GameOver(200).
- Timed mode: state 9 fall-out → NewBoard.

**Time up.** Bar empty → TIME UP! → GameOver(200).

**Game over.**
1. State 5: shake, then explode/fall.
2. gameover1 → state 6 → ShowHighScoreWidget: the board is removed but kept; the widget slides down over the snapshot.
3. At the end of the slide, ProcessScoreStuff → dialog 8 (registered) or 7 (unregistered).
4. OK → AddHighScore → save.
5. "CLICK HERE TO PLAY AGAIN" → NewGame. This deletes the widget (re-save) and the old board.

**New Game button.**
- In state 6: NewGame immediately.
- Otherwise dialog 0 → YES → RequestNewGame (GameOver(50) + mNewGamePending) → after the fall, NewGame. No high-score screen.

**Mode buttons.**
- Same mode: behaves like New Game.
- Other mode: if the board has no scoring move yet, switch immediately. Otherwise dialog 3/4 → YES → SetTimedMode(x, true).
  - This ends the board without a game-over.
  - The abandoned game is still "in progress", so ~Board writes it to savegame.dat and ++mTimesPlayed.
  - The next launch offers to continue it (dialog 16) unless a later save overwrites the file.
  - New Game (dialog 0) does not do this: the board is in state 5 when it is deleted.

**Quit.** Dialog 1 → YES → Shutdown: save an in-progress game (savegame.dat) and count it as played.

---

## 12. Port guidance (deviations that do not change observable behaviour)

- Initialise the fields the originals leave uninitialised:
  - Board: mMouseX/Y = -1, swap fields 0.
  - Piece: all zero.
  - WinDMApp: diff values 0, mode buttons NULL, mTimedMode false.
- Leaks the port may fix:
  - pieces in ~Board
  - WinDMApp mScoreFont and the shrink arrays
- Leaks the port must NOT fix by deleting: the pieces NewBoard orphans. Park them in a Board-owned list (3.6).
- The Piece byte layout (0x78, padding included) is part of savegame.dat. Keep the 0x78-byte record
  (`Piece::SAVE_SIZE`) exactly as is: the code-mod fields (`mKind`, `mKindFrame`) follow it, and the save
  writes and reads exactly the record.
- Keep every RNG draw site and its order (3.16), including:
  - the double NewGame at startup
  - the tip roll in every HighScoreWidget ctor (also the dialog's hidden one)
  - the conditional `%5` nag draw
- Registration and update checks are not ported:
  - the SexyApp port is always registered and build-unlocked (NoReg)
  - TitleScreen nag, register link, RegisterDialog, UpdateCheckDialog and DoUpdateCheck are dead in that configuration
  - ShowRegisterDialog only needs the dialog-19 branch
- Paths: "userdata\\x" maps to the user dir. `data\\diff.txt` and the property files are read through the VFS (mods can override them).

### 12.1 Variant hooks (code mods)

The code mods (games/bejeweled/docs/CODE_MODS_DESIGN.md) touch the translated functions only with
constants that become members holding the same value and with inert hooks, each line tagged
`// port(variant):`. `games/bejeweled/tools/list_variant_hooks.py` prints those lines grouped by function
and address; its count is tracked per stage.

| Stage | Hook lines | Translated functions | What |
|---|---|---|---|
| 1 | 55 | 22 | `InitVariant()` in the ctor; loop bounds `BOARD_ROWS`/`BOARD_COLS`/8/7 → `mRows`/`mCols`; `BOARD_X`/`BOARD_Y` → `mLayout.mOriginX/Y` in `GetColX`/`GetRowY`; `mHeight` → `mLayout.mScreenBottom` (480) in `UpdateGameOverAnim`/`UpdateBoardDropping`; `sizeof(Piece)` → `Piece::SAVE_SIZE` in the save |
| 2 | 63 | 27 | Set seed (12.3): `ChooseBoardRng()` in `WinDMApp::NewGame`; `UsePendingSeed()` on the board's first tick in `Update` (after the pause checks); `GameLoadedVariant()` at the end of a successful `LoadGame`; `delete mOwnRng` in `~Board`; `HighScoreWidget`: the table file chosen in the ctor and used by `ReadFromFile`/`WriteToFile`, the SEED line in `Draw`. The 9 `Rand()` draws keep their text: `Board::Rand()` hides `Sexy::Rand` by class scope |
| 2, seed rule | 65 | 30 | The pending seed waits for play (12.3, the user's decision of 2026-10-18): `UsePendingSeed()` leaves `Update` for `UpdateSwapping` (after `mIsNewGame = false`) and `GameOver`; `RequestNewGame` clears `mTakesPendingSeed` before its `GameOver(50)` |
| 3a | 94 | 41 | Replays, recording side (12.4). One-line observer hooks `if (mRecorder != NULL) mRecorder->...;` in `GameOver`, `RequestNewGame` (NEW_GAME), `Pause`/`Unpause`, `MouseMove`/`MouseLeave` (HOVER), `MouseDown` (SELECT, DESELECT, TARGET), `MouseDrag` (TARGET), `TryShowHint` (HINT), `SaveGame` (`BeginSave`), `~Board` (`OnBoardDestroyed`, `delete mRecorder`), `Update` (entry `SyncExternalRand`, end `AfterUpdate`); `WinDMApp::NewGame` attaches the recorder. Playback guards `if (mPlaybackOnly) return;` in `IsInProgress` and the input handlers, for stage 3b. `LoadGame`'s file read and `SaveGame`'s write go through `ReadSaveFile`/`WriteSaveFile` (playback bytes; SAVE and the sidecar), `DeleteSavedGame` deletes the sidecar, `GameLoadedVariant(aBuffer)` restores the sidecar's RNG |
| 3b | 104 | 47 | Replays, playback side (12.4). `Pause`/`Unpause`: a playback board takes pauses only from its player (`if (mPlaybackOnly && !mPlaybackApplying) return;`). `WinDMApp`: `mWatch`/`mReplayQuit` set in the ctor, `mWatch` deleted in the dtor; `UpdateFrames` runs `mWatch->BeforeFrame()` first; `TitleScreenIsFinished` starts the watch instead of its two new games; `ButtonDepress` gives New Game and the mode buttons to the watch; `NewGame` and `ShowHighScoreWidget` return at once when the playback board calls them (its game ended) |
| 2b | 106 | 47 | Auto LiveSplit (12.5), merged after stage 3 (on stage 2's code it was 67 lines in 32 functions): `LiveSplitObserver::AfterUpdate` at the end of `WinDMApp::UpdateFrames` (after 3b's `mWatch->BeforeFrame()`, the update and the test hooks, `CodecCheck` included; only while wanted), `LiveSplitObserver::AppDestroyed` at the end of `~WinDMApp` (after 3b's `delete mWatch`). Both functions were already edited by 3b, so the count of functions stays. No Board function changes: the board serial and the resumed mark are set in `InitVariant` and `GameLoadedVariant` |
| 4 | 106 | 47 | The headless verifier (12.6) compiles the translated Board sources as they are: no hook. `TraceBoard`, a port test hook, moved its settle check and block to `BoardTrace.cpp`; the watch's ended-board line stays in `WinDMApp.cpp` |
| 5 | 113 | 47 | Allow invalid moves, the noMoves rule and the v2 save (12.7). `UpdateSwapping`: the swap back only `&& !mVariant.mInvalid`, then `else if (mState == STATE_SWAPPING) KeepInvalidMove();`, and the match branch's `UsePendingSeed()` became `OnMoveMade()` (it also counts the move). `UpdateFalling`: `else if (mVariant.mNoMoves) OnNoMovesVariant();` before the original no-moves branches. `LoadGame`: the version check becomes `mVariant.mAny ? !ReadVariantHeader(aBuffer, aVersion) : (aVersion != SAVEGAME_VERSION)`. `WriteSaveGame`: the version `0x101` and the header after it, the chunks after the last field, on a variant board only. Every function was edited before, so the count of functions stays |
| 5, review | 115 | 47 | Hint on a board without a move (12.7): `ShowHint`: `if (NoHintVariant()) return;` after the playback guard; `Update`: after the idle glow's two lines, `if (NoHintVariant()) mHintGlowTimer = 0;`. Both functions were edited before |

With the original rules every edited line computes what the original did: the board is 8x8, the layout
is the original constants, and no hook reads the variant flags yet. `UpdateClearing`'s refill pass,
`NewBoard`, `SetupLevelData`'s tile loop and `Draw` keep their 8x8 constants: on other boards they will be
replaced by variant paths, not generalised.

### 12.2 Mod numbers (port)

- Tunables (`mod.json` "tunables", `GameConfig`) are read as the native game always read them: a wrong
  type gives the default, numbers are truncated, and a number outside the int range (2^31 or more, or
  below -2^31) gives INT_MIN, the x86 (int) cast's "integer indefinite". The web build used to saturate
  instead (INT_MAX); since stage 1 it reads such a value as the native game does. Example: `board.gemColors`
  3e9 plays with 3 colours on both.
- `Json.cpp` rejects `nan`, `inf` and numbers too large for a double (strtod accepts them, JSON does not
  have them), so such a `mod.json` fails to parse like any other invalid JSON. Before stage 1 the mod
  loaded and the value played differently on native and web.
- The board's own `(int)` casts of doubles (a match's points times the multiplier, `SetupLevelData`'s level
  points, the hint penalty, the score roll) leave the int range with legal values too: `setValues`
  2,000,000,000 in a timed game, a `diff.txt` with large steps. Natively they give `INT_MIN` (x86's
  `cvttsd2si`, also for NaN); emcc's default, wasm's saturating conversion, gave `INT_MAX`, `INT_MIN` or 0, so
  the score and the replay's CHECKs differed between the builds. Since the stage 4 review the web build (and
  the verifier's wasm) compiles the game with `-mno-nontrapping-fptoint`, whose range-checked conversion gives
  `INT_MIN` as x86 does: the timed bot game with that mod scores -2,147,483,648 on both, and each build's
  replay verifies on the other (the oracle's `overflow` cases). The sums that follow wrap on both.

### 12.3 Set seed (code mods, stage 2)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.10 and 6.2, as built:

- **Board RNG.** Every draw of the translated code goes through `Board::Rand()`: the board's own
  `MTRand` (`mOwnRng`) when it has one, else `Sexy::Rand()`, the global stream, draw for draw as the
  original (`MTRand::Next` and `Sexy::Rand` return the same value). Without a seed source nothing else
  changes: no draw moves, nothing new is drawn, logged, saved or shown.
- **Source** (`WinDMApp::ChooseBoardRng` in NewGame, read by `Board::InitVariant`):
  1. a seed the player chose (`GameConfig::mPendingSeed`: `-gameseed=N`, `?seed=N` on the web): the
     board owns `MTRand(N)` and is user-seeded;
  2. else any rule, or `features.seeds.perGame` (also implied by `features.leaderboard`): NewGame draws
     `seed = Sexy::Rand()` from the global stream (so it follows `-seed`; a drawn 0 becomes 4357, the
     seed MTRand plays for 0) and the board owns `MTRand(seed)`. `"seeds": true` turns on `entry` and
     `perGame`; the object form only the members it sets to true, so `{"entry": true}`, the seed field
     alone, keeps every game on the global stream;
  3. else the global stream. Tunables-only mods count as the original rules (`example-relaxed` plays as
     before).
- **The pending seed** (design 4.10, the user's decision of 2026-10-18). NewGame gives it to every board
  it creates (`mTakesPendingSeed`), until a game on it is played: the board uses it up
  (`UsePendingSeed`) with its first swap that makes a match (`UpdateSwapping`, where `mIsNewGame` is
  cleared, the moment that also decides whether quitting saves the game) or at its game over
  (`GameOver`: no moves in normal mode, time up; a timed game can run out without any move). New Game is
  no game over: `RequestNewGame` clears `mTakesPendingSeed` before its `GameOver(50)`. Until then every
  board that replaces the untouched one gets the seed again: the second board of the startup double
  NewGame (4.2), a mode switch (Timed straight after a fresh start plays the seed; with `mIsNewGame` set
  it needs no confirmation), New Game. A swap that makes no match leaves it pending. A board that
  resumes a save never uses it up: the next new game gets it (resume, then a mode switch plays the seed
  in timed mode). A timed fall-out is not a game over and cannot come before the first match (the
  constructor's board always has a move). Seeds are 1..4294967295 (leading zeros allowed); main.cpp
  refuses anything else with exit status 2, the web shell starts without a seed and says so in its status
  line.
- **Shown and kept apart.** The game-over screen of a board with its own RNG shows "SEED n" (mainfont,
  y 120, between the stats at 105 and the tip at 136; string `HSWIDGET_SEED`, default "SEED"). A
  user-seeded game uses its own table, `hiscores-seeded.dat` (`hiscores-<key>-seeded.dat` with gameplay
  mods), also in Options → High Scores during it; per-game seeds keep the normal table. The
  HighScoreWidget remembers its file (`mFileName`): NewGame deletes it only after the next board has
  replaced `mApp->mBoard`.
- **Resume (stage 3).** The v1 save bytes stay as they are. A board with its own RNG leaves a sidecar next
  to its save (`savegame.bjr`, scoped like the save; design 7.3): an `RNGS` record with the seed,
  `userSeeded` and the MT state at the save, and the save's hash (`SVHA`). A successful `LoadGame` whose
  sidecar goes with the loaded save gives the board that RNG (`GameLoadedVariant`): the resumed game keeps
  its stream (the refills after the resume are the ones the uninterrupted game gets, so quit and resume
  cannot reroll), its SEED line and, for a seed the player chose, the `-seeded` table. Without a matching
  sidecar (a game on the global stream, a save from before stage 3) the resumed game goes on on the global
  stream as before. Either way the resume never uses the pending seed up. A board of rules (a variant board)
  keeps its RNG in its v2 save instead since stage 5 (12.7). Oracle: `seed_quit` (a match, quit: the sidecar), `seed_quit.resume` (no
  -gameseed: the next two moves give `seed_highscores.nodialog`'s refills), `seed_quit.bot` (the bot
  plays the resumed game to game over: SEED 4242, `hiscores-seeded.dat` only).
- **Builds and test hooks.** The RNG source is not a code path: `-forcevariant` leaves it alone, and the
  `BEJ_NO_VARIANTS` build keeps it (seeds, per-game seeds, the SEED line and the seeded table work there
  too; only `mVariant.mOwnRng` reads false). `-trace` prints `TRACE   seed n user|pergame` after the rows
  of a board with its own RNG; `-tracehash` adds that RNG's state (seed, draw count, MT words and index)
  and a pending seed, only when present, so original games hash as before.
- **Checks.** `games/bejeweled/tools/check_rng_use.py` (design 6.1 rule 1) and the oracle's stage-2 cases
  (the pending seed: `seed_start`, `seed_resume`, `seed_timed`, `seed_match_timed`, `seed_nomatch_timed`,
  `seed_newgame`); `MTRand` `GetState`/`SetState` round trips in `games/bejeweled/tools/rulestest`.
  `SetState` refuses an index outside 0..625 and the all-zero state (every draw 0: `NewBoard` would never
  end), so a save's `RNGS` reader (stages 3 and 5) can treat `false` as a chunk error.
  `MTRand::Serialize()` keeps no index (a copy made from it restarts at the next 624-word block); it stays
  as it is because the `-tracehash` FULL hash hashes it.

### 12.4 Replays (code mods, stage 3: recording and watching)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.11, 6.2-6.6 and 7.3, as built:

- **Recording.** With `features.replays`, `WinDMApp::NewGame` gives each board a `ReplayRecorder`
  (`games/bejeweled/src/ReplayRecorder.h`), and `Board::InitVariant` records the RNG start: the seed, or
  the global stream's whole state before the constructor's first draw. The hooks only read: no draw, no
  field play reads, no log line. The commands are stamped with the board tick at which they happened and
  are applied before the next update: `SELECT`, `DESELECT`, `TARGET`, `HINT`, `PAUSE`/`UNPAUSE` (not the
  pair `SaveGame` makes), `NEW_GAME`, `HOVER` (with `hover`). The markers are `XRAND` (outside draws, on
  the global stream only), `SAVE`, `LOAD` and `CHECK`, the LOGIC hash at each settle.
- **Session mode.** Without a seed source (`seeds.perGame` off, no rule, no `-gameseed`) a recorded game is
  on the global stream exactly as without the mod. The oracle's `rec_*` cases play the stage-0 scripts and
  the autoplay games with `mods/stage3/replays` and compare them with their runs without mods: TRACE,
  `hash.txt`, logs, saves, high scores and registry are identical (design 9.1 item 5).
- **Files.** Finished replays go to `replays/` in the user dir (natively; `/persist/replays` on the web,
  `-replaydir`) when the game-over screen appears, after New Game, or for a game dropped unsaved. A
  saved game goes on in the sidecar (12.3) and continues as more segments when it is resumed. A declined
  resume, or the first save of another board that replaces it (a mode switch saved one game, the next one
  is saved too), turns the sidecar into a finished replay with END "saved". Games without a move are
  dropped. `-keep` pins replays that qualified for the table, and pruning keeps the newest `keep` others,
  newest by the time each file was written. The hover flag of a replay says every segment has the hover
  channel (a game resumed under the other setting has it off, and the player then checks no save bytes; since
  stage 4 it checks none of a save made in the vortex either, whose libm-set positions and angles differ in the
  last bits between native and wasm, 12.6).
  A recording too long for the web backup's one localStorage item (100,000 events) leaves only its RNG
  sidecar there.
- **StateCodec.** One field table (`games/bejeweled/src/StateCodec.cpp`) drives three encodings:
  - FULL, which is the `-tracehash` FULL column byte for byte;
  - LOGIC, the replay hash;
  - SNAPSHOT, restorable, the base for the v2 save and undo.

  `-codeccheck=<file>` runs the write → restore → write round trip after every update.
- **ENV** (stage 3b): the recorder takes `mShowInstructions` and `mTimesPlayed` at the start of the update
  that runs the tick-150 block, not at the constructor: `NewGame` deletes the board it replaces after the
  new one is made, and that `~Board` counts a game in progress as played (4.2), so the constructor's count
  can be one low.
- **Watching** (stage 3b, `games/bejeweled/src/WatchController.h`, design 6.8 "As built"): `-replay=<file>`
  plays a replay instead of the game the title screen starts (`-replayquit`: quit at its end). Each segment
  gets a playback board that owns an `MTRand` from the segment's RNG start (`Board::InitVariant`), takes no
  input (the handlers' `mPlaybackOnly` guards) and no pause but the replay's (`Pause`/`Unpause`), never
  saves, counts or deletes anything (`IsInProgress`), and loads the bytes the segment before saved (or
  its `LDSV`) with the segment's `RNGS` (`GameLoadedVariant` reads no sidecar for it). The player updates it
  once per frame (the widget manager does not), applies each event at its board tick after its gate,
  compares every `CHECK` and `END`, and swaps the replay's app state (rules, tunables, diff values, ENV) in
  for each step only. A corrupt replay stops with the reason on stdout ("REPLAY end: CORRUPT ..."). Options
  → High Scores while watching shows the player's table (`HighScoreFileName` takes a playback board for no
  board), and after the end the board that stays on screen has no more `-trace` settles (its last `SAVE`
  can drop a swap to `STATE_PLAYING`, which the recorded board, deleted at once, never showed).
- **Untrusted files** (after the stage 3 review). A replay is someone else's file, while the original
  `LoadGame` trusts a save: an `LDSV`'s bytes must pass `Replay::CheckSave` (every value `Draw` or an update
  indexes, loops or divides with in range) before they reach it; `-codeccheck` checks every board state's
  save against the same check. The reader takes at most 16 MB and 4M events, refuses records out of order
  or repeated and an `END` reason over 5; the player refuses more than 200M board ticks (23 days) in all, more
  than 10,000 events at one tick and more than 1,000,000 outside draws. `savegame.dat` is still read as the
  original reads it. After the stage 4 review (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.4, 6.8): the reader
  also takes at most 1,000 segments and refuses more than 10,000 events at one tick as it reads them (a
  segment's events are checked, then allocated once); the player takes `SAVE`s only at their segment's last
  tick, at most 4 a segment, and a `RULE` only if it is the canonical form of rules the mod path makes
  (`GameRules::CheckRecorded`) with `diff.txt` values that give every level at least 1 point (finite, bases
  above 0 with a product of at least 1, steps not below 0): a 0 points base made `Board::Update` divide by
  zero. A sidecar of 1,000 segments is not continued: the resume starts a fresh replay that holds the save.
  A score or level points past the int range (the `overflow` and `highlevel` cases, 12.2) give saves the
  `LDSV` check refuses, so the oracle watches those recordings without `-codeccheck`.
- **Oracle** (stage 3b): the playback cases (`watch_*`) play every recording of the stage-3 cases in a fresh
  user dir: every `CHECK` and `END` match, each segment's TRACE settles are the recording's, StateCodec's
  round trip holds after every update, nothing is written; seeded, per-game and global-stream games, normal
  and timed, resumes as two segments, a tunables `RULE`, outside draws (`XRAND`), and four broken copies the
  player calls corrupt. After the review: recordings without the hover channel and resumed under the other
  setting (`rec_main_nohover`, `rec_load_hover_on`, `rec_load_hover_off`), a quit mid-swap
  (`quit_midswap`), a save that replaces a recorded one (`mode_switch_match`: the timed game kept), and
  Options → High Scores mid-watch (`watch_seed_quit_highscores`). The browser, the speeds and the REPLAY
  END screen are stage 13 (12.18).

### 12.5 Auto LiveSplit (code mods, stage 2b)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15, as built:

- **The block** (`LiveSplitBlock.h`, layout 1, 0xB0 bytes, a `static_assert` per offset) is
  `gLiveSplitBlock` in `LiveSplitObserver.cpp`: constant-initialised, `aligned(64)`, `used`, so its magic
  ("BYOPOP-LIVESPLIT" F0 F1 F2 F3, a brace list of bytes) is in the exe once, in `.data`'s raw data. In
  every Windows build; nothing writes it until `features.livesplit` is on (`active` and `self` stay 0).
  Elsewhere (web) the observer is empty inlines and GameConfig warns "features.livesplit: desktop only,
  ignored"; the feature is parsed everywhere, so `mods-status.json` names it on every build.
- **The observer** (`LiveSplitObserver::AfterUpdate`, after each app update while the feature is on, or
  once more to write `active = 0` after a `gGameConfig.Load()` turned it off) only reads: `mBoard`'s
  fields, the score key, the board's RulesId, `gGameConfig.mFeatures`. Each write is seq odd, release fence,
  the fields from a shadow copy, release fence, seq even. Against the previous update of the same board:
  GO when the tick passes 150 on a board that did not resume a save and is no replay's playback board
  (`mPlaybackOnly`, 12.4); a level clear when the board enters state 7 on an update its tick advanced; a
  game over when it enters state 5, its cause New Game (`mNewGamePending`), else time up (from state 1),
  else no moves (from state 0), else other. A board's
  first update, the activation's update and the update a `LoadGame` ran in (the resumed mark turned on)
  are baselines only: a loaded vortex save enters state 7 on an update whose tick advances (the board
  updates in the same frame as the dialog's button), so the tick test alone would count it.
  `displayScoreMax` follows `mDisplayScore` up to the game-over update, then stays. Flags: 01 timed, 02
  resumed, 04 no score key and no chosen seed, 08 chosen seed, 10 own RNG, 40 game over (per board), 80
  paused (`mPauseCount` or `mUserPauseCount`), 20 replay (a playback board, `mPlaybackOnly`: `-replay`
  watches; the script never starts a run on one).
- **Board bookkeeping:** `InitVariant` sets `mBoardSerial` (1, 2, ... per process) and clears `mResumed`;
  `GameLoadedVariant` sets `mResumed` (a playback board's LOAD too, before its early return). Both outside
  the `BEJ_NO_VARIANTS` parts; neither is hashed (the FULL and LOGIC lists leave them out; StateCodec has
  no row for them, 12.4), saved, recorded or read by play. StateCodec's scratch board sets them to 0 and
  false; it is never `mBoard`, so the observer never sees it.
- **Activation** (the first update with the feature on): `self`, `qpcFrequency`, `active = 1`, the status
  line "livesplit: block at 0x...", and the split templates: every `*.lss` in the `livesplit/` folder of
  the mod that turned the feature on (`GameFeatures::mLiveSplitMod`) is copied to `<user dir>/livesplit/`
  unless a file of that name is there (written as `<name>.tmp` and renamed when complete, so a failed copy
  leaves no truncated template). `~WinDMApp` makes the last write: `active = 0`, `boardSerial = 0`, which
  tells `lsprobe` a clean quit from a crash; the script skips inactive copies and resets on a quit through
  the board-less update after `Shutdown` deleted a game in progress, or its `exit` action.
- **Status lines** (SDL_Log, only with the feature): the address, the templates, each GO, level clear and
  game over with its tick, a resumed board's GO that is not counted, "off". `-livesplitlog=<file>` appends
  "u=<update>" and the block's fields per write, with `self` and the clock masked.
- **Mod signs (Q18):** a mod whose `mod.json` has only descriptive keys (id, name, version, author,
  description, priority) and `"features": {"livesplit": ...}`, and no `images`, `sounds`, `music`, `data`
  or `properties` folder, does not count for `GameConfig::mAnyModEnabled`: no crossbar on GO's G, no fourth
  hint sparkle.
- **The script's timing** (`tools/livesplit/BejeweledDeluxe-byopop.asl`): nothing may run for the first
  time between a decision in `start`, `split` or `reset` and LiveSplit's timer. The C# binder compiles a
  `dynamic` call site on its first run (about a millisecond each), and a first `DateTime.Now` or file
  append costs milliseconds too. In the live test with LiveSplit 1.8.37 the first script lost 20–35 ms to
  that in each session's first start and a few more in its first split, so that run started 15–18 ms late
  and its splits came out about 10 ms short. Now `start`, `split` and `reset` (a poll whose game over
  `reset` holds back splits in the same cycle, after it) read every field, var and setting they use into
  typed locals in every cycle (`Reached` both its fields), the typed `LogEvent` runs once in `startup` and
  only stamps the decision and keeps the line, and `onStart`, `onSplit` and `onReset` write the line after
  LiveSplit took the event's time; `update` drops a line no event took (timed in `asltest`: the first
  start 0.04–0.23 ms instead of 22–40 ms, the first split under 0.02 ms, and the held-back split's cycle
  under 0.02 ms instead of 10–14 ms). ScriptableAutoSplit
  runs `exit` without `old`, `current` and `game` (`DoExit` clears the process, then `RunNoProcessMethod`)
  but the timer events' actions with the last copies (`RunMethod`), so the reset that `exit` makes is
  logged by `onReset` as `reset exit` (`vars.ExitReset` is set around it).
- **Checks:** the oracle's `livesplit` cases (every playtest, stage-0 and autoplay case with the mod
  against an inert `features.livesplit: false` twin, and the block's log against `-tracehash`; since the
  merge with stage 3 also a watch, `watch_main_timed.ls`: the playback board carries flag 0x20 and counts
  no GO), and `games/bejeweled/tools/livesplit/run_tests.sh` (lsprobe, the script compiled as C# 5, the
  decisions).

### 12.6 Headless verifier (code mods, stage 4)

games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9, as built (`games/bejeweled/src/headless`, target `bejeweled_verify`):

- **What it compiles.** The translated `Board.cpp` and `BoardRules.cpp` and the port's `BoardVariant.cpp` and
  `BoardSave.cpp` (the v2 save, 12.7) unchanged, with `BoardTrace`, `GameRules`, `GameConfig.cpp` (the framework-free half), `StateCodec`, `Replay`,
  `Sha256`, `WatchController` (the game's replay player), `MTRand`, `Buffer`, `Color` and `Json`. The include
  path starts with `headless/shim`, so the framework headers whose implementations need SDL resolve to shims:
  `Widget.h` (`mUpdateCnt` counted as `WidgetContainer::Update` counts it, `Resize`, the mouse-handler
  dispatch), `Graphics.h`, `Image.h`, `MemoryImage.h`, `DDImage.h` (no drawing: Draw, the background and the
  banner write only image bits), `Font.h`/`ImageFont.h` (metrics only; the points font's ascent, which moves the
  score popups, from the replay's ENV), `SoundInstance.h`/`SoundManager.h` (no sound instance, as without an
  audio device), `MusicInterface.h`, `WidgetManager.h`, `SexyApp.h` and `<SDL.h>`. The SDL-free framework
  headers (`Common.h`, `Rect.h`, `Color.h`, `Flags.h`, `Buffer.h`, `MTRand.h`, the listeners) are the real
  ones; `HeadlessShims.cpp` replaces `Common.cpp`'s `Rand`, `GetRandCount`, `GetRandMTState`, `vformat` and
  `StrFormat`, `SDL_Log` (stderr) and `GameConfig::ScopedFileName` (no mods: the name unchanged). Built with the
  game's switches: `-msse2 -mfpmath=sse` on i686, `BEJ_NO_VARIANTS` in the novariants build.
- **The app** (`HeadlessApp.h`, `WinDMAppHeadless.cpp`): every `WinDMApp` member a board or the player calls,
  on the real `WinDMApp.h`. `NewGame` and `ShowHighScoreWidget` end the watched replay
  (`WatchController::OnBoardEnd`) or set an end flag; `SetTimedMode` sets the mode; `KillDialog` and
  `DoInstructionDialogette` do nothing (that dialog is not modal); files live in memory; `SafeDeleteWidget`
  defers the delete to the end of the frame as the framework does, so a segment's new board never takes the old
  one's address (the trace's settle rule compares boards). The diff.txt values start as the original's. The
  recorder's methods (`ReplayRecorderHeadless.cpp`) only link: no verifier board records.
- **A replay** is played by the game's own `WatchController` (one implementation of the gates, the CHECKs and
  END, 12.4): `SetBytes` gives it the file from memory, `GetResult`, `GetChecks` and `GetEventsApplied`
  report. Each verifier frame is the game's: `BeforeFrame` (one board update), the `-trace` settle
  check (`BoardTracer`, which the game's `-trace` uses since `TraceBoard` moved to `BoardTrace.cpp`), the
  deferred deletes. So `bejeweled_verify replay.bjr -trace` prints the game's `-replay=replay.bjr -replayquit
  -trace` lines exactly (the REPLAY lines and the TRACE settles; only "TRACE update N", the app's count,
  differs), then one line of JSON: `result` ok, corrupt or unplayable, `message` (the REPLAY end line),
  `checks`, `events`, `updates`, and from the file `segments`, `engineAbi`, `flags`, `rulesId`, `end`,
  `seeds`. Exit status 0 only for ok. `-forcevariant` plays it on the generalised paths.
- **Difftest** (`bejeweled_verify difftest --seeds a..b [--ticks n] [--mode normal|timed|both] [-v]`): per seed and
  mode two boards on `MTRand(seed)`, each with its own app (a board's tick-150 block and its swaps read and
  write the app's `mShowInstructions` and `mTimesPlayed`), one plain and one with the `-forcevariant` flags, each
  played by a `DiffBot` with the seed (`headless/DiffBot.h`: the autoplay bot's moves, clicks, zone clicks and
  drags through the board's own handlers, random swaps, hints, right clicks, selections while falling, key
  and dialog pauses, New Game in one game of 40; timed games mostly slow, so that time runs out). After every
  update the two LOGIC encodings must be equal (compared whole, which is stronger than their hashes), at every
  settle also the move lists and the save a shutdown would write; a game runs to its game-over screen, New
  Game's end, or 60,000 updates. Seeds 1..500, both modes (stage 4): 1,000 games, none differs (normal: 470 game
  overs, 12 New Game, 18 at the cap, levels up to 7; timed: 286 time ups, 14 New Game, 200 at the cap, levels up to
  10; 33 million updates, 167,329 settles, under 4 minutes natively). Since stage 5 the `-forcevariant` board settles
  without a move through `OnNoMovesVariant` at its defaults ("gameOver" in normal mode, "fallOut" in timed mode)
  where the plain board takes the original branches, so the difftest checks that path at every such game over and
  fall-out. The other generalised paths (shape, match, search, draw) arrive from stage 7 on; until then the rest of
  the two boards' code is the same, and for those paths this proves the harness more than the paths. (The figures
  above are stage 4's, from before the noMoves path.)
- **StateCodec's writer** now writes in place (8 KB reserved, one bounds check per value) instead of appending
  byte by byte: the same bytes, about six times faster, for the difftest and `-codeccheck`, which encode after
  every update.
- **Wasm** (`build/web/bejeweled_verify.wasm`, `-sSTANDALONE_WASM --no-entry -sFILESYSTEM=0
  -sALLOW_MEMORY_GROWTH=1`, 1 MB stack, no JS glue): imports only `fd_write`, `fd_read`, `fd_close`, `fd_seek`
  and `emscripten_notify_memory_growth`; exports `_initialize` (call first), `bv_alloc`, `bv_free`,
  `bv_verify(ptr, len)` (the JSON; the REPLAY and TRACE lines go to fd 1, the file named replay.bjr),
  `bv_set_options` (1 -trace, 2 -forcevariant), `bv_rules_id(ptr, len)` (a mod.json's text: the RulesId of its
  tunables and rules over the defaults with the original diff.txt values, with the merge's errors, dropped
  mods and warnings) and `bv_difftest(first, last, ticks, modes)` (+4 for a line per game).
- **The Node runner** (`games/bejeweled/tools/verify/wasmrun.js`, Node's own modules only): `verify`,
  `difftest` and `rulesid` on the wasm verifier, which print what the native exe prints; and `game`, the
  game's shipped web build (`build/web/bejeweled.js`) with the native test switches the web shell never
  passes (`-script`, `-trace`, `-tracehash`, `-autoplay...`, `-codeccheck`, `-replay`, `-gameseed`,
  `-forcevariant`). It copies the working directory into the build's in-memory file system (and switches
  naming absolute host paths, read-only), stubs the page (a canvas whose 2D context keeps nothing, so SDL renders
  in software; no WebGL, no AudioContext, `window.open` a no-op; the browser's frames back to back), puts
  finished replays in `<user dir>/replays` as natively, and when the game quits flushes its files (the web build
  exports `fflush`: a page never deletes the app, whose destructor closes them natively) and writes back what
  it wrote or deleted. On the oracle's scripts its LOGIC column and TRACE are the native game's, and so are its
  saves, its registry and mostly even the FULL column (not in the vortex, whose positions libm's cos and sin
  set). What a page never writes, the native game writes when it deletes the app: the high-score table of a game
  quit on its game-over screen (`~HighScoreWidget` saves it) and `-codeccheck`'s summary line.
- **Checks.** `games/bejeweled/tools/verify/run_tests.sh` (RulesId vectors native and wasm, the costliest
  `bv_rules_id` inputs alike, files no player can start, extreme `diff.txt` values played to END with the same
  LOGIC hashes native and wasm, a short difftest whose per-game lines must be the same native and wasm,
  `bv_difftest`'s argument checks, command lines), and the oracle's stage-4 cases
  (`games/bejeweled/tools/verify/oracle_cases.sh`, sourced at the end of `compare.sh`'s case list; the tests and
  the difftest start in the background as the cases are defined): every watched replay played again by both
  verifiers with the game's REPLAY and TRACE lines and verdict (the drift guard, 9.1 item 6), broken copies no
  player can start, the difftest over 500 seeds, the web build under Node on the playtests, the stage-0 scripts,
  the bot runs, a seeded game, a recording and two watches with the candidate's LOGIC hashes and TRACE, and two
  games whose casts leave the int range (12.2) recorded natively and by the web build, each recording watched
  and verified on both.
- **Found on the way:** a replay whose `SAVE` falls in the vortex (the oracle's `s5_vortex` snapshot) played
  natively but was corrupt in wasm: the save's raw piece bytes hold the positions and angles libm's `cos`, `sin`
  and `atan2` set, whose last bits differ between the two. The player (12.4) no longer compares a vortex save's
  bytes with the replay's hash (nor a sidecar's `SVHA` after one); CHECK and END, whose LOGIC hash never took
  those fields, still decide.
- **After the review** (CODE_MODS_DESIGN.md 6.9 "After the stage 4 review"): the web builds convert doubles to
  int as x86 does (12.2), a replay's `RULE` and its contents are bounded before a board is made (12.4), the
  verifier parses a file once and takes its bytes without a copy, `bv_difftest` and `bv_rules_id` check their
  inputs (seeds, ticks, modes; a mod.json of at most 1 MB, at most 32 `match.shapes`), the fill minimum counts
  each pattern placement once at its last cell (the same numbers, a sixteenth of the work for 16-cell shapes),
  and the Node runner, like the Worker to come, replaces an instance after a trap or once its memory passed 64
  MB. `games/bejeweled/tools/verify/craft_replays.py` makes the broken and extreme files the tests and the oracle
  use.
- Neither verifier nor the web build under Node opens a window, plays a sound or opens a URL: no rundesk.

### 12.7 Allow invalid moves, the noMoves rule and the v2 save (code mods, stage 5)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.1 and 7.2, as built:

- **Kept swaps** (`moves.allowInvalid`). A swap that makes no match stays: at 180 degrees, where the original swaps
  back, `KeepInvalidMove` leaves the two gems in their new cells, takes `moves.invalidPenalty` times the multiplier
  off the score (floored at 0, as a hint's penalty), plays bad2 and counts it (`mInvalidMoves`); with
  `moves.invalidCountsAsMove` (the default) the game has started as after a match: `mIsNewGame` is cleared (a quit
  saves the game, a mode switch asks first), the move counts (`mMoveCount`) and a seed the player chose is used up
  (12.3). Then `STATE_FALLING`: the next update finds every gem at rest and no match, and goes on as after any
  move: `STATE_PLAYING`, or no move is left and the noMoves rule decides. The swap takes its 36 ticks like any swap
  (the timed drain runs in them) and the penalty lowers the bar like a hint's; nothing else costs time. Two gems of
  one colour can be swapped too (no change on the board, a move all the same). Without `invalidCountsAsMove` a kept
  swap is no move: the game stays new (no save at a quit, no question at a mode switch, a chosen seed stays
  pending), only `mInvalidMoves` counts it. A kept swap leaves `mCascadeCount` 0, so the settle after it records no
  cascade.
- **The first-game tutorial.** A swap that goes back leaves the board as it was, so the arrows stay right; a kept swap
  changes the board, and the swap the arrows show may make no match any more. `KeepInvalidMove` tries it (the two
  gems swapped, `HasSet`, swapped back): if it no longer matches, the arrows and the instructions dialog go, as at
  a game over (`mShowTutorial` false, `KillDialog`), and `mShowInstructions` stays, so the next game shows them again
  (no match has been made). A tutorial move that still matches keeps them up until a match, as before.
- **The noMoves rule** (`OnNoMovesVariant`, for a board that settles without a move). "gameOver" and "fallOut" are
  the original branches (NO MOVES! and the game over; the fall-out and a refilled board), in either mode;
  "continue" (only with `allowInvalid`) shows the banner `NO_MATCHES_LEFT` (default "NO MATCHES!") and the game goes
  on in `STATE_PLAYING`: the player swaps on until a kept swap makes a move again. A normal game under "continue"
  ends only by New Game, a quit or (penalties pushing the score a level down) TIME UP; a timed one by time up.
  `-forcevariant` also takes this path, with the default rules. With `gold.enabled` the rule's normal-mode default is
  "fallOut" (`GameRules::ResolveDerived`): before the gold rule itself is played (stage 12) that part of it already
  is, and `mods-status.json` says so for the mod that turned gold on (unless a mod sets `noMoves.normal`).
- **Hint without a move.** The original never plays a board without a move, so its Hint always finds one. A
  "continue" board has none: `TryShowHint` would take the hint penalty and show nothing, and the idle glow (every 1500
  ticks) would invite that. `Board::NoHintVariant` (moves.allowInvalid, `STATE_PLAYING`, `FindMove` finds nothing):
  `ShowHint` then gives no hint (no penalty, no sparkle, no `HINT` recorded) and the glow stays off; the replay player's
  `HINT` gate refuses one there too.
- **The v2 save.** A variant board (any rule beyond the tunables; `VariantFlags::mAny`) writes version `0x101`: a
  header (its RulesId, 8x8, level shape 0, the mask), the v1 fields, then the chunks `RNGS` (the RNG the game goes on
  with) and `MOVE` (the move counts), `END`; 10,463 bytes at 8x8. It takes only a v2 save of its own rules and
  shape, checked whole before anything changes; a v1 save, a save of other rules (a mod edited without a new
  version keeps its file name) or one cut short is refused with "savegame: <file> not loaded: <why>" on the log,
  and deleted as the original deletes a save of another version. A board of the original rules refuses a v2 save
  by the original version check. A variant board leaves no RNG sidecar (its save holds the RNG); a recording's
  sidecar continues across the resume as before. The resume restores what v1 restores plus the RNG and the counts.
- **Replays.** A kept swap is recorded as any swap (`SELECT`, `TARGET`): no new command. The LOGIC hash of a variant
  board includes its move counts; engine ABI 2 (a replay of the original rules from engine 1 still plays). A
  playback board writes and loads v2 like a live one, and its `LOAD` requires the segment's `RNGS` to be the RNG its
  v2 save holds. A resume continues the sidecar of a game of the original rules that engine 1 recorded, as the
  player plays it (`ReplayRecorder::OnLoad`; the merged replay is engine 2's). A sidecar the resume does not continue
  (other rules, or an engine whose replays of those rules this one cannot continue) is kept as a finished replay
  (END saved) before the save and the sidecar go (`Board::DeleteSaveSidecar`), so its moves are not lost; the new
  replay holds the loaded save (`LDSV`).
- **Trace.** A variant board's settle prints `TRACE   variant moves <mMoveCount> invalid <mInvalidMoves>` after the
  rows and the seed line; a "continue" board settles without move lines.
- **Oracle** (`compare.py` "variant", mods/stage5, scripts/stage5): `free_quit` (a match, a kept swap, the quit:
  v2), `free_quit.resume` against `free_through` (the resumed v2 save gives the refills of the game played
  through), `free_quit.edited`, `v1_refused` and `v2_refused` (the refusals; `v2_refused` on every exe),
  `kept_timed` and `strict_timed` (the pending seed after a kept swap that is, or is not, a move), `strict_last`
  (kept swaps that take the last move: the tutorial's move broken first, then the fall-out in normal mode, which
  leaves the game new and the seed pending, and NO MOVES and the game over in timed mode, TimesPlayed 1 and the seed
  used up), `free_hint` (Hint on a "continue" board: no penalty, no glow), `gold_status`, four bot games (kept swaps;
  "continue" in normal and timed mode; `fallOut` in normal and `gameOver` in timed mode, each after a match), and
  their recordings played back (two segments with a v2 `LOAD`, an `LDSV` that is a v2 save, the `strict_last`
  games). `rec_load_engine1`/`rec_load_engine3` resume a sidecar engine 1 wrote (continued) and one it cannot
  continue (kept in replays/). A candidate built with `BEJ_NO_VARIANTS` plays no rule and leaves the stage-5 rule
  cases out; it plays stage 1's and 2's rule cases as before stage 5, against their own facts in expect.txt
  (`<case>@novariants`: the v1 save and the RNG sidecar).

### 12.8 The mod menu's hooks (docs/MOD_MENU_DESIGN.md 8.2)

None of these runs without an enabled mod that has settings, or a value held for the next game; the oracle has
neither.

- **Next-game values.** `main` sets `ModManager::SetNextGameFilter(GameConfig::IsNextGamePath)` before `Refresh`:
  `rules`, `tunables` and `features`, except `features.livesplit` and the whole `features` object (the LiveSplit
  observer outlives a board). `GameConfig::Load(canCommit)` starts with `CommitNextGameValues`.
  `WinDMApp::CanCommitNextGameValues` decides `canCommit`: no replay is watched, the board is not in progress and no
  save exists under the current key. `NewGame` asks it before the old board's deferred delete; `Init` asks it too,
  so a held value applies at startup unless "Continue?" will be offered.
- **Per-board save names.** `Board::GetSaveGameFileName()` returns `mSaveGameFileName`, which the constructor takes
  from `ScopedSaveGameFileName()` (the name under the current score key). The deferred delete of a board in
  progress saves it, and a commit just before can change the key. The sidecar follows the save's name;
  `GetShutdownSaves` asks `mBoard`.
- **The untouched board** (review A9). `WinDMApp::ApplyNextGameValues`, at the end of `UpdateFrames`, commits and
  calls `NewGame` again when values are held and nothing would be stranded. It also needs a board nobody has moved
  on (`mIsNewGame`, not game over), no dialog, no mod menu, no title or game-over screen. It tries once per board
  (`mBoardSerial`), so a commit that cannot happen is not retried every frame. Its `Rand()` draws happen only then.
- **Options.** While an enabled mod has settings, a Mods button (id 6; 5 is kept for Extras) takes the register slot
  of the locked build's three-button row: help at `mX+24`, Mods at `mX+30+bw`, high scores at `mX+36+2bw`, with
  `bw = (mWidth-60)/3`, all at `mY+166`. It opens the menu over the dialog. An installed mod that is off does not
  count: the oracle's examples case keeps its Options screens.
- **Restart.** `HasGameInProgress` is `mBoard->IsInProgress()` (a replay's board never is). `CanRestartNow` is
  false while `DIALOG_RECORD_SCORE` is open. `GetRestartArgs` adds `-replaydir=` when one is set and `-gameseed=`
  while `mPendingSeed` waits.
- **Messages.** `GameConfig::mStatus` keeps the last merge's report. `GetModMessages` gives the menu its errors and
  dropped mods as errors and its warnings as warnings. `mods-status.json` entries of mods with settings gain
  `values`. `IsLiveSplitOnly` reads the effective manifest and treats `settings` as descriptive.
- **Verifier.** `rulesid`/`bv_rules_id` resolve a mod's `$setting` placeholders at their defaults first
  (`ModSettings::Parse`, `Analyze`, `ReadValues`, `Resolve`). Setting errors and keys left out are errors.
  `HeadlessShims.cpp` carries a copy of `VFS::Normalize` for `ModSettings.cpp`.
- **`override`.** WinDMApp.h marks every framework virtual it overrides; the headless shim `SexyApp.h` declares
  them so the verifier still compiles that header.

### 12.9 Undo (code mods, stage 6)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.12 and 6.7, as built. Nothing here runs unless the rules enable undo:
every hook tests `mVariant.mUndo`, a compile-time false in a BEJ_NO_VARIANTS build, and the orb draws nothing and
takes no mouse on any other board.

- **Snapshots.** `SwapPieces` (0x4c9ff8), after its adjacency check, pushes StateCodec's UNDO encoding of the board
  (`TakeUndoSnapshot`, only in `STATE_PLAYING`, where every gem rests in its cell). At most `undo.depth` are kept,
  the oldest dropped. The swap back's last step in `UpdateSwapping` (0x4cd84f) pops the snapshot again, since the board
  is as it was; a kept invalid move keeps it. `DoWhirlpool` (0x4ca58d) empties the stack, so no undo crosses a
  level, and so does a loaded game (`GameLoadedVariant`): saves hold no snapshots.
- **The undo** (`Undo`). The grid comes back as new gems at rest. Score, display score, stats, move score, idle
  ticks, `mIsNewGame`, the move counts and the RNG come back too; the timed drain only with `undo.restoreTime`.
  With `undo.restoreRng` false the RNG goes on. It keeps the board tick, the pauses, the play time and the undos
  taken, and whether the board still holds a chosen seed. It clears the selection, swap pieces, popups, the "+N"
  move score and the hint glow. It takes `undo.penalty` times the multiplier (floored at 0, as a hint), counts the
  undo and shows the `UNDO` banner for 150 ticks (75 in, 75 out). Repeating the undone move with the RNG restored
  refills exactly as before.
- **When** (`CanUndo`): `STATE_PLAYING`, no pause, a snapshot, `undo.max` not reached, and in timed mode only with
  `undo.inTimed`.
- **Input.** `KeyChar` (0x4ca9d3): z, u and Backspace call `RequestUndo` instead of pausing, unless a user pause is
  on (then they unpause, as any key) or the board is timed without `inTimed` (then they pause, as any key). A
  refused undo plays bad2. The red orb left of the timer bar is `UndoOrb` (`BUTTON_UNDO` 16), made with the side-bar
  buttons. It lights its ball while an undo is possible, brighter under the mouse, and draws the undos left in the
  header font, like the level number in the right-hand orb, when `undo.max` is set.
- **Replays and traces.** `RequestUndo` records `UNDO`, and playback applies it after the same gate. The `-trace`
  variant line ends in `undos <n>` on a board with undo. An undo is a settle of its own in `-trace` and
  `-codeccheck`, and `-codeccheck` also round-trips the UNDO encoding in every update of play.

### 12.10 Board sizes and shapes (code mods, stage 7)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.3 and 5, as built (`BoardShape.cpp`, `BoardDrawVariant.cpp`). The
rules `board.cols`, `board.rows`, `board.mask` and `board.levels` (3..16 a side). Nothing here runs on a board of
one full 8x8 shape: `mVariant.mShape` is set when `board.levels` has more than one shape or the first is not the
original. Dirt cells (`d`, `2`..`9`) play as holes until stage 7b.

- **Flags.** `mScaled` when any shape's logical extent passes 406 px (the view then scales by 406/D about the
  board window's corner, 203,16). `mNoTutorial` when the tutorial move cannot apply: it looks for a horizontal
  3-run swap in rows 4..6, so it needs swap moves, runs of 3, no diagonals and a first shape of at least 8x7
  whose rows 4..6 are playable in columns 0..7. `mDraw` (the copy of Draw) with either.
- **Holes** are permanent NULLs in `mBoard[16][16]`. Three places assumed a full board and test for NULL now:
  `UpdateFalling` (0x4cd687), `FindMove`'s target cell (0x4c9dda) and `MouseDrag`'s neighbour (0x4ca8c1).
- **Fill.** `NewBoard` (0x4cb182) calls `NewBoardVariant`: the original's park-and-fill over the playable cells,
  bottom row first, each gem placed above the nearest gem below. After 1000 rerolls it logs and starts again. The
  constructor (0x4cb5bc) with `mNoTutorial` fills until a move exists (at most 100 fills), with no tutorial move.
- **Refill.** `UpdateClearing` (0x4ca9ed) calls `RefillMasked`: the original pass over each column's playable
  rows, so gems fall through holes, and a new gem spawns above the column's top playable cell.
- **Input.** On a scaled board `MouseMove`, `MouseDown` and `MouseDrag` take the window position to the logical
  one (`BoardLayout::ToLogical`) before the original code runs.
- **Level change.** `SetupLevelData` (0x4cae29) draws the next level's background for its shape
  (`DrawVariantChecker`) and sets `mPendingLayout`. `UpdateWhirlpool` (0x4cb931) deletes the old gems with the old
  bounds, then applies it (`ApplyPendingLayout`: layout, size, `mLevelShape` = level % shapes).
- **Background.** The window is first filled with a void tile: a mod's `images\voidtile`, else the level's
  texture coloured (40, 32, 36). Then the cells and a ring of one cell around them get the original checker
  tiles, holes the void tile. Unscaled boards draw whole tiles as the original does (an 8x8 board is pixel for
  pixel the original's); scaled ones draw each into its rounded window rectangle.
- **Drawing.** `DrawVariant`, a copy of Draw (0x4cbc70): the gems, sparkles, cursor and tutorial arrows through
  a scaled `Graphics`, the score popups at their window positions in the unscaled font; the vortex, banner, hole
  mask and pause text centred on the window.
- **Saves.** The v2 header holds the level's shape, size and mask. `ReadVariantHeader` refuses a save whose shape
  is not one of the rules', is not its level's (level % shapes) or has another size or mask, and applies the shape
  before the cells are read. `GameLoadedVariant` then rebuilds the background for the loaded level and shape
  (the original keeps level 0's after a load).
- **StateCodec.** `mLevelShape` is in SNAPSHOT and V2. LOGIC has the level and the board's size already; UNDO
  leaves it out, as no undo crosses a level.
- **Replays.** Engine ABI 3. `Replay::CheckSave` and `CheckPlayable` take the rules' shapes and check a LOAD's or
  LDSV's v2 save against them. `-trace` prints window positions as a move's clicks on a scaled board, and the
  autoplay bot and the verifier's diff bot click there too and leave holes alone.
- **The score roll with a mod's scoring tunables** (a fix found by the stage 7 tests). Update (0x4cdabc) rolls
  the displayed score toward the real one by at most 3 times the multiplier a tick, and its level-up and TIME_UP
  tests read the displayed score. With thousands of points a match it falls levels behind: the next level,
  which starts at the real score, then ends at once in TIME_UP (and a hint penalty of thousands would give free
  levels). On a board whose scoring tunables differ from the original's (`mFastScoreRoll`), a gap past 150
  times the multiplier now closes by an eighth a tick (`RollScoreFast`), and the original roll does the rest.
  Engine 3 refuses earlier engines' replays of such rules (`GameRules::PlaysAlikeInEveryEngine`).

### 12.11 Twilight gravity (code mods, stage 7c)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.16, as built (`BoardShape.cpp`, `BoardVariant.cpp`). The rules
`board.gravity.direction` (`down`, `up`, `alternate`: Bejeweled 2's Twilight) and `board.gravity.fallAccel`, and the
look keys `look.backdrop` and `look.soundPitch`, which change no play and make no variant board.

- **Flags.** `mGravity` when the direction is not down, `mFall` for a shape, a direction or a `fallAccel` off 180
  (the fall loop goes to `FallPiecesVariant`), `mChecker` for a shape or the twilight backdrop (`DrawVariantChecker`
  draws the level's tiles mirrored, at half brightness). `-forcevariant` sets all three with the rules at down.
- **The flip.** `GravityUp()` is up, or alternate at an even count of flips. `OnGravitySettle`, at the end of
  `UpdateFalling`'s no-match branch, counts a flip when the board is now playing, except after a kept invalid move.
  So the opening board rises and its settle turns the gravity down; a swap that goes back never reaches it.
- **Up.** Refill and fill run from the bottom; the fall-out and the game-over fall leave through the top, at the
  mirror of the original limits about the board's middle row line (`GetRowY(0) + GetRowY(rows - 1)`). The gem
  layer is clipped to the board's own rectangle, (194,12)-(428,416) on 8x8 as before, and `GetPieceAt` ignores
  a rising gem still under the clip's bottom edge.
- **State.** `mGravityFlips` and `mGravityNoFlip` are in StateCodec (FULL, LOGIC, SNAP, V2 and UNDO) and in a v2
  save's `GRAV` chunk, only when a gravity field is off its default (`HasGravityRule`); `-trace` adds
  `gravity up|down flips N` to the variant line then. An undo brings the flips back with the board.
- **Help and sounds.** The tutorial text and Help's first paragraph have Twilight and gravity-up versions
  (English defaults, mod strings override them). `look.soundPitch` sets `SoundManager::SetDefaultPitch`, which
  every new sound instance takes; music is apart.

### 12.12 Dirt and diagonal falling (code mods, stage 7b)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.14, as built (`BoardShape.cpp`, `BoardDrawVariant.cpp`). A mask cell `d`
(one layer) or `2`..`9` is dirt: `CELL_DIRT` in the board's own copy of the layout, with its layers in `mDirt`.
`board.dirtPoints` is what a layer scores, times the multiplier.

- **Flags.** `mDirt` when any level's mask has dirt (`GameRules::HasDirtRule`, `mDirtRules` on the board), and
  under `-forcevariant`. A mask with dirt is a shape, so `mShape`, `mFall` and `mDraw` are on already.
- **Clearing.** After each run `FindSets` scores, `ClearDirtNextToRun` takes one layer off each dirt cell
  orthogonally next to the run, once per run. When the last layer goes, the cell is a gem cell for the rest of
  the level. A new level's layout comes whole from the rules.
- **Refill.** `UpdateClearing` calls `RefillDiagonal` in place of `RefillMasked`. It alternates two steps:
  - segment passes: a column splits at its dirt cells, only the segment on the entry side spawns, and the
    other segments' gaps gather under their dirt;
  - one diagonal slide (`SlideDiagonal`): an empty cell under dirt takes a gem from a neighbouring column one
    row nearer the entry side. When both sides have one, `mDiagonalLeftFirst` picks, and it flips after every
    slide.

  Without dirt the passes are `RefillMasked`'s, draw for draw. Under up gravity the steps mirror vertically.
  `FallPiecesVariant` moves a slid gem over to its column once it is no further from its row than from its
  column, so it falls straight and then glides in at 45 degrees.
- **State.** The layers and `mDiagonalLeftFirst` are StateCodec rows (FULL, LOGIC, SNAP, V2 and UNDO) and a v2
  save's `DIRT` chunk, only when the rules have dirt. The save's mask is the current layout, so a cleared cell
  is a gem cell there (`BoardLayout::FitsSaveMask`). `-trace` prints a dirt cell as `-1 - layers` and adds
  `dirt <layers> left|right` to the variant line. Engine ABI 4: an earlier engine's replays of variant rules no
  longer play.
- **Art.** The level's checker tile is colorized brown, with a crack line per layer left (`DrawDirt`, under the
  gems). A cleared cell sends a puff of dust (`DrawDirtDust`, over the gems; cosmetic, not saved). A mod's
  `images/dirttile` replaces the tile.

### 12.13 More gem colours (code mods, stage 8)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.2, as built (`GemSheet.cpp`, `WinDMApp::EnsureGemColors`).
`board.gemColors` plays 3..12 (`GameConfig::GemColors`); the original paths already draw `Rand() % mGemColors`, so
it stays a tunable that every build plays, `BEJ_NO_VARIANTS` included.

- **The sheet.** `images/gemsheet6` gives each colour 104 px: 15 spin frames in a 52 px row, then the glow frames.
  Before a board is made (`NewGame`, and `WatchController::CreateBoard` with the replay's rules), `EnsureGemColors`
  grows the sheet to the rules' colours: a mod's taller sheet needs nothing, the rows it lacks are recoloured
  copies of another row, on the 1x pixels and on a mod's HD pixels alike (`GemSheet::Extend`, then `SetBits` and
  `SetHDBits`, so the texture follows). A watched replay of more colours than loaded grows it then.
- **Looks.** The enabled mods' `imageOps` (`extendRows` on `images/gemsheet6`, 104 px rows) give the generated rows
  their looks in order; the others get the default looks: the white gem tinted rose, teal, violet, onyx, lime. A
  tint keeps the base row's shading as lightness (luminance to the power 1.6, so the white gem's bright body takes
  the colour) under the tint's hue and saturation; `hue`, `saturation` and `lightness` then rotate, scale and shift.
  What an op gets wrong is a warning in mods-status.json (`GemSheet::ModLooks`).
- **Glyphs.** Each generated row has a white glyph with a dark outline in the bottom right of every frame (circle,
  square, triangle, diamond, star for rows 7..11; `"glyph": false` leaves it out), so the extra colours differ by
  more than colour.
- **Shrink images.** `mGemShrinkImages` has room for 12 colours. `Init` makes the original 7, so the loading bar
  counts as before; `EnsureGemColors` makes the others (`mGemShrinkColors`).
- **Replays.** A save's piece colour is checked against the rules' colours (`Replay::CheckSave`,
  `CheckPlayable`'s `theGemColors`). Engine ABI 5: engines before it played more than 7 colours as 7, so their
  replays of such rules no longer play (`GameRules::PlaysAlikeInEveryEngine` wants at most 7).
- **Test hook.** The script command `gemsheet <file.png>` writes the sheet as grown, and its HD pixels next to it.
- **Not built.** `gGemTints` (the design's per-colour average) waits for the first effect that tints by gem.

---

### 12.14 Match rules (code mods, stage 9)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.6 and 4.7, as built (`MatchFinder.cpp`, `BoardMatch.cpp`). The `match.*`
rules play since stage 9: `minRun`, `diagonal` and `diagonalMinRun`, `crossings: "merge"`, `shapes` and `bonus`. A
board whose rules set any of them takes the variant paths: `FindSets` and `HasSet` dispatch at their top on
`mVariant.mMatch`, `FindMove` on `mVariant.mSearch`, and `-forcevariant` takes them at default rules, where the
oracle shows they play exactly as the originals.

`MatchFinder` is framework-free, so tools/rulestest runs it on fixtures and random grids. `FindGroups` gives the
vertical runs column by column and the horizontal runs row by row (the original's two passes, in its order), then
the `\` and `/` diagonals from their top ends, then the shape instances: ids in sorted order, anchors row-major,
orientations as `GameRules::ShapeOrientations` lists them. An instance whose cells all lie in one earlier group of
its colour is skipped, so a shape that repeats a run adds nothing. With `"merge"`, runs of one colour that share a
cell (diagonals too) are one group at the place of its first run: an L where two cross at an end of both, a T at an
end of one, a plus inside both, the largest crossing naming the group.

`ScoreGroup` is the original's scoring block for one group: the set value of the move's n-th group, the length
bonuses for a straight run only (a merged group or a shape gets none), then `match.bonus` for its kind (`diagonal`,
`L`, `T`, `plus` or the shape id) before the multiplier. A negative bonus never takes the score below 0; without one,
a score past the `int` range wraps or turns INT_MIN as the original's does (an early clamp on any negative score
made `-forcevariant` differ there, in the oracle's `overflow` cases). Dirt next to
any of a group's cells loses a layer, once for the group (`ClearDirtNextToCells`, which `ClearDirtNextToRun` now
calls). The sounds are the original's, a group's cells counting as a run's length.

The search keeps `FindMove`'s contract and its order of candidates: a swap counts when the gem it moves into the
cell lands in a group (`MatchFinder::InGroup`, a local scan around the cell), and `horizontal` or `vertical` alone
means a run of that direction, as the tutorial asks. `TryShowHint` needs no variant path for swaps, since it already
sparkles the gem that moves in. The fill rerolls while `AnyGroup` finds a group; an orientation's anchor is the
corner of its box, which for an L can be a hole, so holes anchor instances too. The tutorial is off with runs of
another length or with diagonals, its text being about rows of three.

Engine ABI 6: earlier engines played the match rules as runs of 3. With match rules that are not the original's,
`-trace`'s variant line ends in the groups the board scored by kind (`groups L T plus diagonal shapes`), which the
oracle's `kinds=` reads; its `match=` recomputes every settle's moves by brute force and finds no group left in a
settled grid.

### 12.15 Twists and slides (code mods, stage 10)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.4, 4.5 and 4.7, as built (`MoveSet.cpp`, `BoardMoves.cpp`). `moves.type`
is a set of `swap`, `twist` and `slide`, and a board whose set holds more than swaps takes the variant paths for its
input, its states and its search. `MoveSet` is framework-free (tools/rulestest compares it with a brute force on
random masks and match rules). A move is a type and five numbers: a swap's two cells, a twist's top-left cell and
direction (+1 clockwise, -1 anticlockwise), a slide's axis, line, segment start, segment length and the number of
cells it moves by. The search gives the swaps in the original's order, then the twists (blocks row-major, clockwise
before anticlockwise), then the slides (each row's segments, then each column's, by 1 to length - 1 cells). A twist
or a slide counts once, at the first of its moved gems that lands in a group, in `MovedCells`' order (the corners
from the top left clockwise; a slide's cells from the segment's start). `FindMove`'s contract is unchanged: where
that gem lands and where it comes from. `TryShowHint` therefore sparkles the right gem with no variant path, and the
callers that read a swap out of `FindMove` (the tutorial board, the headless verifier's bots and CascadeHunt) only
ever play swap rules.

Four new states: `STATE_TWISTING` (10), `STATE_TWISTING_BACK`, `STATE_SLIDING` and `STATE_SLIDING_BACK` (13). The
grid holds the board before the move until the forward animation ends; then the move is made in the grid and
`FindSets` runs as after a swap (the tutorial box goes, `mIsNewGame` is cleared). Without a group the move is
undone in the grid at once and only the positions animate back with the bad move's sound, or `KeepInvalidMove`
keeps it. A twist turns 5 degrees a tick for 18 ticks (the swap's pace) by a table of cos and sin times 1024, so the
positions are integers on every platform. A slide goes from the offset it was let go at to its k cells, or to k
cells less the segment's span when that is nearer, in `moves.slide.snapTicks` ticks; the gem that leaves one end is
drawn again at the other, clipped to the segment. A save in the middle of a move writes state 1 and the board
before it and puts the gems back (`EndVariantMove`); `LoadGame` maps the four states to playing.

Input (the design's table): a click at a point where four gems meet twists them clockwise, a right click
anticlockwise when `moves.twist.direction` is `both`. With swaps too, only a click within 13 px of the point twists
and one elsewhere selects. With slides, a drag along a row or a column locks its axis after 16 px and slides the
segment it started on, rounded to whole cells; less than half a cell snaps back with no move, and a press at a twist
point waits for the mouse up, since a drag from there slides. The ring under the mouse is four 26 px quarters of the
selection cursor. The instructions name the types: `INSTRUCTIONDIALOG_<types>` and `FULL_INSTRUCTIONS_<types>` (the
suffix `TWIST`, `SWAP_SLIDE` and so on), with English defaults; the tutorial is off for any set but swaps alone.

Engine ABI 7: the events `TWIST` and `SLIDE`, which the watch controller applies after the board's own checks; the
codec's rows for the move in play exist only when the rules have twists or slides. `-autoplay` plans typed moves on
such boards (a click, a right click or a drag from the trace's points). `-trace` prints a twist as its block,
direction, landing cell, origin and click point, and a slide as its line, segment, k, landing cell, origin and drag;
the oracle's `moves=` recomputes them by brute force and `kept=` checks a kept move is one move of the rules' types.

### 12.16 Flame and lightning gems (code mods, stage 11)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.8, as built (`Specials.cpp`, `BoardSpecials.cpp`). `specials.flame` and
`specials.lightning` are each null (off) or an object; an empty one is Bejeweled 2's: a flame from a run of 4 or more
(`run4`), radius 1, 20 points a gem; lightning from a run of 5 or more (`run5`), 30 points a gem. `createdBy` names
patterns: `runN` is a straight run of N or more, `L`, `T` and `plus` are merged crossings, `diagonal` a diagonal run,
any other name a shape's id. `Specials` is framework-free, so tools/rulestest checks it on fixtures.

`FindSetsVariant` scores the groups and marks their gems dying as before; `ResolveSpecials` then runs in the
same tick. Each group whose pattern a special's `createdBy` names (lightning's checked first) keeps one gem alive as
that special. On the move's own resolve (the first after it) that is the first moved gem in the group, in
`MovedCells`' order (a swap's two gems, a twist's corners, a slide's cells); otherwise, and when no moved gem is in
it, the group's middle cell (`mCells[size / 2]`: a run's middle, a merged group's runs in order, a shape's
orientation's cells), then on round the group from there. A cell already special or taken by another group's
creation is skipped, and keeping a gem wins over the death another group gave it. `createIn: "move"` makes specials
only on the move's own resolve. A created special keeps its colour, falls with the board, and is set off by a later
group that takes it.

The blasts are a queue, first the specials in this pass's groups in group order (each group's cells in order). A
flame takes the cells within its radius (a square), lightning its whole row and column; holes are never in reach,
dirt cells are. A blast destroys the gems in reach that are not dying yet and were not made special in this pass;
with `chain` (the default) a special it destroys joins the queue, without it that special only dies. Each blast
scores its special's points for each gem it destroyed, times the multiplier, with a popup at the special's cell;
the points count toward the score, the gems cleared and the move's score (EXCELLENT) but not the set value's count
(`mMoveMatchCount`), so the next group's points are as if the blast had not been. Under dirt rules each dirt cell in
reach loses a layer, once per blast. `UpdateClearing` shrinks the blasts' gems with the groups', so no state is new.
The first blast of each kind in a pass plays its sound: a mod's `sounds\flame` or `sounds\lightning`, else the
original's explode and firecrackle samples.

Drawing is cosmetic and reads no RNG. A mod's `images\flamegem` and `images\lightninggem` (49 px frames across) are
drawn additively over the gem; without them a flame glows with the sparkle sheet in fire colours, pulsing, and
lightning shows the level wipe's electric strip clipped in the gem. Each blast shows for 36 ticks: a flame's
sparkles over its square, lightning's strip along its row and down its column. These effects are not saved.

Engine ABI 8: a piece's kind is play state (hashed and snapshotted by the codec since stage 5), and a v2 save under
specials rules writes a `KIND` chunk, a byte a cell, row-major (0 normal, 1 flame, 2 lightning; 0 where there is no
gem), read back on resume; a v1 save cannot hold one. An undo brings the kinds back with the board (the codec's undo
rows). `-trace` adds the flames and lightning made and the blasts so far to the variant line, then a line naming each
special on the board. The counts are the game's since it was started or resumed, and an undo does not take them
back, so a resumed board can hold more specials than it made; the oracle's `specials=` and `after=` allow for that.

### 12.17 The gold board (code mods, stage 12)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.9, as built (`BoardGold.cpp`). With `gold.enabled` a level is won by
turning every cell of the goal gold, Jewel Quest's way, and no longer by points. The goal is the board's gem cells,
and its dirt cells too with `gold.countDirt` (the default). `FindSetsVariant` gilds the cells of every group it
scores, before the specials are resolved, and with `gold.gild` at its default (`matchesAndBlasts`) a blast gilds the
cells of the gems it destroys; `matches` leaves a blast's cells as they were. Only a gem cell turns gold, so a dirt
cell does once its last layer is gone and a group takes the gem that fell into it. The gold belongs to the cell: it
stays put when the gems fall, through a fall-out (with gold, `noMoves.normal` defaults to `fallOut`) and through the
cascades, and only the next level clears it.

`Update`'s level block calls `UpdateGoldGoal` in place of the score's level end and TIME_UP, and the timed mode's score
drain does not run. A level may have a time limit, `gold.timeLimit.normal` or `.timed` for the mode (Normal is
untimed by default, Timed `{180, -10, 60}`): `max(min, base + perLevel × level)` seconds, the first level being
level 0, each at least 10. Its ticks run down in the states the drain runs in (playing, swapping, twisting,
sliding) once the GO delay is over. In the playing state with no cell of the goal left, a timed level scores
`timeBonus` (10 by default) for each whole second left, times the multiplier, and the vortex takes the board to the
next level as after a score level. When the time runs out first, TIME_UP shows and the game is over, which a replay
records as END reason 4. The bar shows the time left of a level with a limit, else the gold done, and takes the timed
colours only with a limit. Over it the HUD writes `GOLD done/goal` in the main font (string `GOLD_PROGRESS`, a format
of the two numbers).

Drawing is cosmetic and reads no RNG. Under the gems each gold cell shows a mod's `images\goldtile`, or else the
level's checker tile tinted gold (235, 185, 45), made once a level. A cell that just turned gold shines for 30 ticks
with the sparkle sheet in gold, additively. The shine is not saved.

Engine ABI 9: the gold layer and the level's time are play state (hashed and snapshotted by the codec, the gold's
limit outside the undo rows). A v2 save under gold rules writes a `GOLD` chunk, a byte a cell, row-major (1 gold,
only on a gem cell of the mask), and `GTIM`, the ticks left and the limit as two u32s; without them a resume has no
gold and the level's whole time. A resume on a shape board rebuilds its background through `SetupLevelData`
(`RebuildLoadedBackground`, stage 7), which keeps the loaded gold and time as it keeps the level's other values. An
undo brings the gold back with the board; the time left comes back only with `undo.restoreTime`, so by default the
clock runs on. `-forcevariant` does not turn gold on. `-trace` ends the variant line with `gold D/N`, and `time T/L`
in ticks with a limit, then prints a gold line, a word a row: `1` gold, `0` a cell of the goal not yet gold, `.` a
cell outside it. Since gold plays, the stage 5 warning that only its fall-out default did is gone.

### 12.18 The watch in the game and the Extras dialog (code mods, stage 13)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.10, 4.11 and 8, as built. With a feature mod on (the seed field, per-game
seeds, replays or the leaderboard) Options shows an Extras button in the register slot of its three-button row, where
the Mods button stands otherwise; the Mods button then moves into Extras, which shows it while an enabled mod has
settings. Without feature mods Options is drawn as before. The Extras dialog (`ExtrasDialog.cpp`, 348 wide like
Options) holds the seed field (`features.seeds.entry`: digits only, at most ten, filled in with the seed of a board
that has one) with "Play this seed", the line "This game: SEED n" for such a board, and "Replays..." with the replays
feature. Play this seed takes 1 to 4294967295 and refuses anything else with a red line (`MTRand::SRand(0)` would
play 4357's board). Over a game in progress it asks first, as New Game does; then it sets the pending seed and starts
a new game, from which the pending-seed rule of 4.10 takes over. Enter in the field plays the seed too.

The replay browser (`ReplayBrowserDialog.cpp`, 560 wide, ten rows and a scrollbar) lists the files of the replay
folder whose names `ReplayRecorder::IsReplayName` accepts, newest first by the time in the name, a star in front of a
pinned one. Each row shows the date, the mode and the score from the name, and from the file itself (`Replay::Peek`,
which reads only the records' heads) the first segment's seed and META's title. Watch (or a double click) watches the
selected file; Keep and Unkeep rename it to and from its `-keep` name, refused when a file has that name already;
Delete asks first. Every change calls `Platform::UserDataChanged`, which the web build persists.

A watch starts from the browser, from the high-score screen's WATCH REPLAY link (the replay that game's end just
wrote, `ReplayRecorder::FinishedReplay`), and as before from `-replay=` and the web page's `?replay=`. Over a game in
progress it asks first; the player's board is deleted for the playback board, and `~Board` saves it as on a mode
switch. A transparent overlay (`ReplayOverlay.cpp`) covers the board right of the side bar and holds the focus: a
banner over the board's top frame says "REPLAY" and the speed, "REPLAY PAUSED", "REPLAY NEXT MOVE" or "REPLAY END",
and while paused a strip over the bottom frame lists the keys. Space or a click pauses and plays on; 1 to 4 set 1x,
2x, 4x or 8x, the board's updates in one frame (`WatchController::BeforeFrame`); N plays on at 8x and pauses at the
next move, the first tick at which a TARGET naming a cell, a TWIST, a SLIDE or an UNDO is due, once at least one update
has run. A dialog the player opens holds the watch. At the end the REPLAY END dialog gives the score and level, or
where the replay went out of step: Watch again plays it from its start, and Back (or Esc during the watch, or New
Game and the mode buttons, as in stage 3b) leaves it through `NewGame(); TryConfirmLoad();`, which offers the saved
game again. A file that cannot be played opens a dialog with the reason instead, and gives the player's game back
when a watch was showing. `-replayquit` still quits at the end, and the headless verifier has no overlay or dialogs.

The test hooks (`-trace`, `-codeccheck`, `-tracehash`) run once a frame, so faster than 1x they see a settle only when
a frame ends on it. The controller still matches every CHECK and END itself, which is what the oracle's speed cases
check. The mods status no longer says replays are watched only from the command line, nor that the seed field is to
come: of the features only the leaderboard is "not in this build" (stage 14). The new strings have defaults and can
be renamed by mods: `OPTIONSDLG_EXTRAS`, the `EXTRASDLG_*`, `REPLAYSDLG_*` and `REPLAY_*` ids, `CONFIRM_SEED1/2` and
`HSWIDGET_WATCH_REPLAY`.

### 12.19 Steven Universe gems (code mods, stage 15)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.17, as built. `tools/make_su_gems_mod.py` draws the mod `su-gems`: the
seven gems as gems from Steven Universe in their Bejeweled colours, plus Rose Quartz, Lapis Lazuli, Bismuth and Garnet
for boards of up to 11 colours. It writes a whole `images/gemsheet6@4x.png` rather than a patch, because a patch cannot
make the sheet taller; the game's 1x comes from box downsampling, and boards of 7 colours read only the first seven
rows. Since 2026-10-26 each gem is a static flat drawing in the show's style: a dark rim, wedge facets toned by the
direction they face around a table, no glints. Every spin frame is the same picture and the glow row is the gem
lifted toward white. (Stage 15 ray-cast cel-shaded solids instead; the user found them not recognisably the
show's.) The sheet uses the frame layout of `tools/gemsheet.py`, shared with the Bejeweled 2 gems. The title screen's five gems are covered and redrawn as SU gems, unless
`--no-title`. The output is fan art of the show's designs and stays git-ignored (`mods/.gitignore`). Nothing in the
game changed.

### 12.20 Code-drawn art (code mods, stage 16)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.18, as built. The mod `code-art` turns on `features.codeArt`, and code in
`src/art/` then draws every image the game loads and its five bitmap fonts while the loading screen shows, so the game
runs without its `images` folder and the fonts' `data/*.txt`. The look is the port's own: bevelled panels, procedural
textures, a stroke font and ray-cast gems in three styles (cel, glossy, and stage 15's Steven Universe gems), in five
palettes. Images keep the originals' names and sizes, so no Draw code, layout or hit test changed; the font strips
differ in width but keep each font's height, ascent and space. The density follows the window's render scale (2 to
4; 2 on the web). The platform asks the game's generator after the mod layers and before the game's files (shared
commit 935cc5d), so mod images and image patches still apply over the drawn art. A missing sound file is not fatal
with code art on, and the title screen then says the game plays silent. Nothing changes without the mod: the
generator is not registered, and play never reads what it draws.

### 12.21 Game-over stats (code mods, stage 17)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.19, as built. The mod `game-stats` turns on `features.gameStats`.
The board then owns a `GameStats` (`src/GameStats.*`), which its hooks feed: each scored run, the other points, each
penalty, move, swap back, hint, undo, twist, slide, special and gilded cell, and the settle that ends a move. After
each update it counts the play time and notices level-ups. It is no game state. It draws from no RNG, is not in
StateCodec's LOGIC and does not touch the score key, so the oracle's runs without the mod are unchanged and the bot's
games hash the same with it. The game-over screen gets a STATS link next to the SEED line's row, and the REPLAY END
dialog of a watched replay gets one too; either opens `GameStatsPage` over the board window. Undo snapshots keep
a copy of the counts, and saves keep them in the sidecar's STAT record.

### 12.22 Arrow gems (code mods, stage 18)

games/bejeweled/docs/CODE_MODS_DESIGN.md 4.20, as built. With `arrows.enabled` some gems are arrows (`Piece::mArrow`)
that step one gem the way they point after each move, at the settle that ends it, in a state of their own,
`STATE_ARROW_STEPPING` (14), which no original save or replay holds. A new arrow, every few moves, draws its direction
and its gem from the board's RNG, so the rules give each game their own high-score table and no oracle case without
them changes. The arrows and their counts are game state: StateCodec rows (FULL, LOGIC, SNAPSHOT, UNDO; the steps
not UNDO) and the v2 save's `ARRW` chunk. A save keeps `mState` within the original's 0..9: a save mid-step
writes `STATE_FALLING`. An escape ends the game with the replays' end reason 6, `END_ARROW_ESCAPED`.

Engine ABI 10: the arrows' rules, and with them the four diagonal directions (`upRight`, `downRight`, `downLeft`,
`upLeft`, bits 4..7 of `arrows.directions`, so `Piece::mArrow` is 1..8). Engine 9 played a replay of arrow rules
without arrows. A diagonal arrow starts on a gem with no gem behind it on its diagonal, at least half as far from its
edge as the farthest such gem; the straight directions keep their draws, so the stage 18 games replay the same.

## 13. Naming reconciliation (analyst names → chosen names)

**Board fields.**

| Offset | Chosen | Alternatives |
|---|---|---|
| +0x1a0 | mIsNewGame | mNoMovesMadeYet |
| +0x1a8 | mCursorFrame | mSelectAnimCounter |
| +0x1ac | mShowTutorial | mShowTutorialHint |
| +0x1b0/+0x1b4 | mTutorialBob/Dir | mTutorialArrowOffset/Dir |
| +0x1d8 | mBarWidth | mProgressBarWidth |
| +0x1dc | mPlayStartTick | mTimerStartTick |
| +0x1e0 | mPlaySeconds | mElapsedSeconds |
| +0x1e4 | mGameDuration | mGameSeconds |
| +0x1f0 | mPointMultiplier | mLevelMultiplier |
| +0x200 | mMoveMatchCount | mMatchesThisMove (part 1 was wrong about its use) |
| +0x20c | mGameOverCountdown | mGameOverTimer |
| +0x240 | mNewGamePending | mUnk240 |
| +0x248 | mUserPauseCount | mVisiblePauseCount |
| +0x24c | mScorePopups / ScorePopup | mFloatingTexts / FloatingText |
| +0x258 | mPointsFont | mDigitFont |
| +0x26c | mLevelTargetScore | mLevelEndScore |
| +0x290 | mBarColor | mBarFrame |
| +0x2c0 | mTextColorImage | mTextFxImage |
| +0x2c4 | mShowingText | mShowText |
| +0x2f4 | mTextTimeLeft | mTextTimer |
| +0x2fc | mElectOffset | mWipeHue (refuted by Draw: it is the elect strip x scroll) |
| +0x300 | mBlackHoleFrame | mWhirlpoolFrame |
| +0x304 | mTimeDrainDelay | mTimerDelay |
| +0x308 | mBlackHoleLevel | mWhirlpoolVolume / mWhirlpoolLevel (alpha and volume) |

**Piece fields.**

| Offset | Chosen | Alternatives |
|---|---|---|
| +0x40 | mGlowAlpha | mGlowFade |
| +0x58/+0x5c | mSparkleCount/Frame | mHintTicks/Frame (Draw shows they drive the hint sparkles) |
| +0x60..+0x70 | mHoleAngle / mHoleAngVel / mHoleRadius | mVortex* / mAngle* |

**Functions.**

| Address | Chosen | Alternatives |
|---|---|---|
| 0x4c9bab | RequestNewGame | EndGameForNewGame |
| 0x4c9bbe | IsInProgress | CanSaveGame |
| 0x4c9ff8 | SwapPieces | BeginSwap |
| 0x4ca36e | ColorizeText | RenderTextImage |
| 0x4ca58d | DoWhirlpool | StartVortex |
| 0x4cae29 | SetupLevelData | LevelSetup / BuildLevelBackground |
| 0x4cb3bf | UpdateBoardDropping | UpdateBoardDrop |
| 0x4cb475 | ShowHint | Hint |
| 0x4cb931 | UpdateWhirlpool | - |
| 0x4cca37 | HasSavedGame | SavedGameExists |
| 0x4cd287 | FindSets | ResolveMatches |
| 0x4d0e4b | ProcessScoreStuff | CheckForHighScoreImpl (10_framework.jsonl name, overridden by 20_game.jsonl) |

**Other.**
- SexyApp +0x715 is `mBuildUnlocked` (FRAMEWORK_NOTES), not "mNoReg". +0x6c0 is `mUserName`.

---

## 14. Unresolved questions

1. **x87 precision.** All results above assume 53-bit precision control (the CRT default; DirectDraw-only mode keeps it). If the D3D7 device is created (3D mode) it may switch to 24-bit. That would change every double expression, for example the shrink sizes (43,35,27,21,13,7). Not verified at runtime.
2. **FindMove NULL-check quirk.** Harmless with full boards. Confirm no caller ever passes a board with holes (none found).
3. **TIME UP in normal mode.**
   - The Update time-up test is not gated on timed mode.
   - In normal mode it requires `mDisplayScore - mLevelStartScore <= -mLevelPoints`, reachable only by spamming hints (each costs 50*mult) on level 2+.
   - Derived from code, not observed in game.
4. **Unused fields.**
   - Board +0x218 is only zeroed. +0x264 and +0x260 are saved but never set. The +0x2c8 string is unused. +0x310 is never read.
   - ScorePopup +0x0c is always -1.
   - TitleScreen +0xa4 (mDisplayedPlayCount) has no visible effect.
   - WinDMApp +0x7d8/+0x7f0 (12 pointers) are only zeroed and deleted.
   - Purpose unknown; kept for layout fidelity.
5. **High-score mode source.** HighScoreWidget mode: ProcessScoreStuff uses app->mTimedMode, AddHighScore uses board->mTimedMode. They should always be equal (the board is created from the app flag, and LoadGame sets both); not proven for every path.
6. **Truncated hiscores.dat.** ReadFromFile reuses stale or uninitialised locals on a truncated file. The recommended port behaviour (stop at EOF, keep defaults) is a guess at "most faithful sane" behaviour.
7. **HighScoreWidget overlay.** It relies on alphaback1's alpha to let the panel show through the left HUD area. Visually unverified.
8. **DrawEditBox overdraw.** The visible result depends on editbox.png pixels; reproduce the draw order rather than "fixing" it.
9. **Double menuclick.** OptionsDialog Done plays sample 12 twice in one tick. Whether the port's mixer layers two instances like DirectSound is a framework decision.
10. **TitleScreen version text.** Position (537 - w/2, 221) in mainfont1 and grey; not checked against a screenshot.
11. **OptionsDialog base order.** The analyst places SliderListener at +0x150 and CheckboxListener at +0x154; FRAMEWORK_NOTES lists them in the other order. Layout-only; it does not affect the port.
12. **Dialog 17.** "Check for updates" is handled but never created (dead code); its strings exist only in default.xml.
13. **Pre-Vista AppData.** GetAppDataFolder() returns "" there, so saves go to the working directory. The port always uses its user dir.

---

## 15. Test commands (2026-11-07)

The standard test commands of `docs/TEST_COMMANDS.md`, in `src/testhooks/TestCommands.cpp`, offered first by
`WinDMApp::HandleScriptCommand`; the existing `save <file>`, `gemsheet <file>` and `memreport <file>` stay beside them. They are
reachable only from a test (`-script`, `-commands`, the web page's `byopop.test.command`); a normal run never gives one, so
unmodded play and the oracle are unchanged. Each answers on stdout with one line.

| Command | Answer |
|---|---|
| `state` | `STATE {...}`: `screen` (loading, title, game, gameover, recordscore, quit, help, highscores, options, dialog; Help and High Scores are opened from Options and so are named before it), `windowed` (false in full screen), `mode` (normal, timed), `level` (the 1-based one shown), `score`, `shown` (the rolling score), `start` and `target` (the level's score window), `bar` (the progress or time bar's width, 0 to 340), `drain` (timed: the points drained from the bar), `phase` (the board's state: falling, playing, swapping, clearing, vortex, level wipe, fall out, game over ...), `tick`, `moves`, `over` (the game is over: the board's states 5 and 6), `busy` (the board takes no move now), `paused`, `newgame`, `tutorial` (the first game's hint and arrows are showing), `selected` (`[col,row]`, when a gem is selected) and `board` (8 strings, a gem by its colour's digit, `.` for none) |
| `goto normal` / `timed` / `game` | a new game in that mode (`game`: the current mode) through `SetTimedMode`, as the side bar's buttons start one; from the title it first does the play link's own click (`TitleScreenIsFinished`), and a saved game then asks its continue question, which stays open (`screen` says dialog; answer it with real input) |
| `goto options` / `highscores` / `help` | the dialog the side bar's Options button, the High Scores link or Help opens; `goto menu` closes any of them. `goto title` and `goto credits` answer an error: the game has neither to return to |
| `unlock all` | `OK`: nothing is locked |
| `level <n>` | n from 2: the level's score is reached at once (`win`) with the level set so that the vortex ends at level n, the game's own level-up |
| `win` | the board's score is set to the level's goal, so that its own update starts the vortex and the next level |
| `lose` | the game's own ending: the "no more moves" banner (normal) or "time up" (timed) and `GameOver(200)` |
| `set score <n>` | the score and the shown score; a score past the goal ends the level at once, as play would |
| `set drain <n>` | timed only: the points drained from the bar (there are no seconds in Bejeweled's clock); a big number ends the game |
| `seed <n>` | `SRand(n)`, the same as `-seed=<n>` |
| `move` | `MOVE {"kind":"swap","from":[c,r],"to":[c,r],"input":[{"move":[x,y]},{"click":[x,y]},{"move":[x,y]},{"click":[x,y]},{"wait":4}]}`: a valid swap and the pointer and two clicks that make it, in game pixels (the steps of `docs/TEST_COMMANDS.md`). The choice is one of the valid swaps whose gems take a click (`AutoplayBot::ValidClickableMoves`, the list `-autoplay`'s valid moves choose from), picked by a hash of the board and the score: nothing is drawn from the game's random numbers, nothing changes, and the same board gets the same move. While the board is busy (falling, swapping, clearing, the vortex, a pause or a gem still selected) it answers `{"kind":"wait","input":[{"wait":10}]}`, so a driver that only plays what it is given waits; `MOVE null` when the game is over, no game is on or no move is left. On a board whose rules twist or slide (a code mod) it answers an error: the bot there is `-autoplay`'s |

`games/bejeweled/tools/commands_test.sh` runs `commands_test.txt` as a script under rundesk and checks the answers.

## 16. Fix candidates and the fixes mod (2026-11-07)

`games/bejeweled/mods/fixes` is the game's fixes mod (`docs/MOD_ROADMAP.md`, "The fixes mod"): the feature `features.fixes`
(read through `Platform::FixOn`) with one setting for each fix, all on by default, on the site by default and off when the player turns
the mod off. It shares the high scores and the saved game with the original (no rules change, so no variant board and no v2
save). The port without the mod is the original, and a replay never carries a fix: the verifier and the watch play with the
features of the default configuration. `tools/fixes_test.sh` plays each fix with the mod off and on and checks that only the
fix differs.

**Made**
- `selection`: a gem selected when Timed mode's board falls out and refills (no move left) stays selected in the original
  though it is no longer on the board (`NewBoard` drops the grid and keeps the pointer). The next click targets it:
  `MouseDown` with a stale selection runs `SwapPieces(stale, clicked)`, which plays the "bad" sound and clears the selection when
  the gems are far apart, so the click selects nothing, and when they are next to each other (by the stale gem's old column
  and row) puts the stale gem back into the grid and loses the gem there. With the fix `Board::DropStaleSelection` (called in
  `MouseDown` and `MouseDrag`) drops a selection that is no longer in the grid, records a deselect, and the click selects what
  it hits. To see it: a Timed game whose board has one move left (`tools/origcmp/nomoves.py --one`), select a gem as the board
  falls out, click another after the refill.
- `resumeLook`: `LoadGame` does not call `SetupLevelData` (section 3.14), so a saved game of level 2 or later comes back with the
  first level's background tint and texture until the next level-up. With the fix `Board::ResumeLevelLook` runs `SetupLevelData`
  after the load, keeps the level's values and the drain that the save holds, and draws the loaded level's look. A save taken
  in the middle of the level wipe, and a board with code-mod rules, keep the original's.

**Looked at and not made**
- `FindMove`'s NULL test of the fixed cell (section 14, 2): harmless on a full board.
- Time up in Normal mode through hint penalties (section 14, 3): reachable only by spamming hints, not seen in play.
- The truncated `hiscores.dat` read (section 14, 6) is a memory fault and belongs to the port, not the mod.
- The double `NewGame` at start-up: it keeps the random numbers in the original's order, so it is not touched.

## 17. Real-input drivers (2026-11-08)

Two drivers play the game with real input only, so that the path a hand takes (SDL's queue, `HandleEvent`, the window-to-game
mapping, the widget manager; on the web the page's DOM events and the canvas's scaling) is the one that is tested. Both are thin
loops around the test commands of section 15: ask `state`, ask `move` (the bot's swap), send the move's `input` steps as clicks,
wait for the next answer. The session writes and judges them; it never plays.

- **Native**, `tools/realplay/drive.py`: starts the game under rundesk with `-commands=<file>` and `-realinput=<empty file>`
  (`docs/REAL_INPUT.md`), appends `move game x y`, `click game x y`, `key ...`, `keydown/keyup Left Alt`, `focus lost` and
  `focus gained` lines and waits for each `OK`. Its steps: `launch` (loading screen, title, the play link, the first game's
  hint, Options, Help and both Done buttons), `normal` and `timed` (a whole game each, then the name entry typed with real keys),
  `focus`, `fullscreen` (Alt+Enter, moves, Alt+Enter back, moves). A Timed game needs `--realtime`: Bejeweled's clock is the
  board's update count, a `-fast` run does thousands of updates in the time the channel takes for one move, and the clock then
  runs out in the middle of the first moves. The coordinates are the original's 640x480 pixels, so `click game` works in a
  window of any size and in full screen.
- **Web**, `tools/web/onegame.js`, run by the player-only page (`?player=1&assets=1&fast=1&silent=1&driver=games/bejeweled/tools/web/onegame.js`,
  `&mode=normal|timed`, `&focus=1` to lose and regain the focus after six moves, `&moves=N` to stop after N moves, `&scale=2`
  for a canvas drawn at twice its size): the same route, through the page's `t.click` and `t.move` events, answering the
  "Continue old game?" question of a profile that left a game with a click on No.

What they showed, and what stayed unproven, is in `IDENTICAL_PLAY.md` ("Real input"). The Options dialog's "Play More Great
Games!" link, which a click of these drivers never touched, is taken out of the port (section 18).

## 18. Registration code and the More Games button taken out (2026-11-08)

The user's rules of 2026-11-07 (CLAUDE.md, "Order of the work"; `docs/COORDINATION.md`): a game takes out the registration
code (trial, nag, unlock, register and update-check paths) and every button that leads out of a game, "More Games" first,
before its final merge, with its neighbours moved to close the gap. The port plays as the registered original does, which
stays the reference. The Steam copy's `partner.xml` has `NoReg`, so none of this was reachable in play but the link.

**Taken out**
- The Options dialog's "Play More Great Games!" link (partner.xml's one `OptionsLinkText0`; it opened the PopCap site) and the
  `OpenURL` path behind it: `WinDMApp::OpenURL`, `URLOpenFailed` and `URLOpenSucceeded`, the "opening the browser" and "could
  not open it" dialogs (ids 10 and 11), the unused "visit PopCap" handler, the `NoPopLink` registry read.
- The Options dialog's Register button (id 1) and check-for-updates button (id 4): the original created and added them but
  never laid them out in the Steam build (0x0, invisible). Their handlers, `WinDMApp::ShowRegisterDialog` and its "Already
  Registered" dialog (id 19), and the ids of the registration, update-check and nag dialogs (5 to 7, 15, 17, 18, and the
  update results 10018 and 20018).
- The title screen's register link (hidden while registered), its nag delay timer (`TotalTimerSec`) and the play counter that
  only marked the screen dirty, the register link's label and layout, and the register URL (`mRegisterLink`).
- The images `trialcircle2` and `waitbar` (the trial circle and the update-check dialog's bar): no longer loaded, and their
  code-art drawings go too. `ImageId` is renumbered (34 images, 6 of them before the title screen).

**Visible change.** Only the Options dialog: the link row is gone and the row of Help and High Scores (and Mods or Extras,
when a mod adds one) stands 12 px lower (y 178 from the dialog's top, not 166), in the middle of the space the link left
between it and Done. The dialog keeps the original's 290 px height and its Done button (clicks on both still land where they
did, so the oracle's scripts need no change). Nothing else on any screen changes, no random number is drawn differently, no
save or high score changes. The oracle (`compare.py`) lets the baseline's four shots with the dialog open (`12_options`,
`o01_options`, `o02_high_scores`, `o03_back_to_options`) differ inside the dialog's box, nowhere else; the frozen exes keep
the link.

**Left in place.** The shared framework's `SexyApp` still has the registration members (`mIsRegistered`, `mBuildUnlocked`, the
register link and the update check); they are the coordinator's. The strings and images of the original's own files stay in
the user's game folder. The comments in `Board.cpp` and `BoardRules.cpp` about the nag's random draw say why that draw is
absent; it was an unregistered copy's.

**Loading (the user's rule of 2026-11-07, fast loading is part of every port).** The port's loading has no wait that the
original's code put into it: `WinDMApp::LoadingThreadProc` loads the sounds and images in one go with no sleep or per-frame
slice, the title screen reads its progress every fourth update and shows the play link on the first read after the load, and
the registered original had no minimum time (that was the trial's nag delay, taken out above). The framework logs
"Resource Loading Time" (about 0.6 s on the test machine, the whole load); the title and its play link follow within the
next few updates.

## 19. Performance (2026-11-08)

The original and the port measured back to back on the same machine (a Core i7-12700F, 12 cores, Windows 11, a GTX 1080),
for the user's morning report (`build/slots/news.md`, 2026-11-08). The machine was not quiet: other sessions' games ran
meanwhile (0 to 3 game processes besides the one measured, noted per run below), so the numbers are for comparing the two
here and now; the performance pass re-measures later on a quiet machine. Both games ran windowed, in real time, with no sound
(the port under rundesk, SDL's silent driver; the original under origrun on the host with its silent stand-in), from a fresh
user folder and with no mods.

**What was measured, and how.** One run is one process from its creation to its exit, sampled every 50 ms by
`tools/profile/perfmin.py` (working set, private bytes, processor time, threads), the same way for both. The input is the
same on both sides (`tools/profile/perfscript.py`): the title screen for 15 s, a click on Play, then 40 swaps of neighbouring
gems 1.5 s apart (17 s to 75.5 s), quit at 90 s. The two games deal different boards (the original's seed is its clock's, and
the seed patch of the identical-play runs is not used here), so a swap makes a match in some games and not in others; what
the runs share is the input, the 60 seconds of playing and the drawing of the board. The processor figures are for the
window from 20 s to 80 s of the process's life, which lies inside the playing in both (the Play click comes at 15 s).

| | original (3 runs) | port (3 runs) |
|---|---|---|
| Peak working set | 48.3 MB (48.2 to 48.4) | 53.0 MB (52.8 to 61.3) |
| Peak private bytes | 34.0 MB (33.8 to 34.1) | 52.7 MB (52.4 to 54.5) |
| Processor time in the window | 4.7, 5.5 and 7.8 % of one core | 6.2, 6.5 and 11.0 % of one core |
| Processor time, whole 90 s run | 4.1, 4.2 and 6.6 s | 4.9, 5.0 and 9.0 s |
| Launch to the first response to Play (click probe, 5 and 7 runs) | 1.06 s median (0.95 to 1.64) | 0.88 s median (0.75 to 0.96) |
| Launch to the title's play link (the state command, 5 runs) | not measurable (no such command) | 0.66 s median (0.59 to 0.73) |

The slowest run of each game (7.8 % and 11.0 %, the first round) had up to three other game processes running; the other
two rounds had one or two. The port's second round is its memory outlier (61.3 MB working set, 54.5 MB private bytes); its
other two runs and all three of the original's sit within 0.4 MB of their medians. The counts of other game processes at the
start and end of each run are in `build/perf/*/procs.txt` and `build/perf/rounds.txt` (git-ignored).

**Reading it.** The port holds about 18 MB more private memory than the original. `PROFILE.md` (sections 2 and 6) has the
port's side: every image's pixels kept at 4 bytes a pixel after the GPU has its texture, and the sounds as 48 kHz stereo
float. Why the original holds less (probably images and sounds kept in fewer bytes) was not examined. The original's working
set stands 14 MB above its private bytes (DLL pages and mapped files), the port's is level with them, so the two working sets
differ by only 5 MB. The port's processor time is about 20 % higher than the original's in the two quieter rounds (6.2
against 5.5 % and 6.5 against 4.7 % of one core), and both are a few per cent of one core. Loading is a little faster in the
port by the same probe (section 18: it loads the sounds and images in one go).

**What could not be measured.** (1) The web build's load time: the Browser pane is hidden, its timers are throttled and
`requestAnimationFrame` does not run, and a load in the user's browser is dominated by the network and the wasm compile,
which a hidden pane does not show faithfully. (2) The original's loading to the second: it has no state command, so its
figure is the click probe's (a click on Play every 100 ms from the start; the first one the game answers), which is good to
about 0.15 s and includes building the board; the port's click probe, with the same spacing, is the comparable figure
(0.88 s), and its own state command gives the lower bound (0.66 s). (3) Full screen and the GPU's own load were not measured.

The tools: `tools/profile/perfmin.py` (the sampler; `--attach` for the original, which origrun starts), `perfscript.py` (the
input for both), `loadtime.py` (the port's loading time through the live channel). The original ran from a copy of the
Steam install with the real exe carved out of the wrapper (`tools/extract_game_exe.py`, in `C:\byo-og\bej-perf\game`),
under `build/origrun-bin/origrun.exe` with `--sound stub` and no virtual clock.

### The performance pass (2026-11-09)

**Measured again, in pairs.** `tools/profile/pairs.sh` runs the original and then the port over the same minute of play
as above (`perfscript.py`), each sampled by `perfmin.py`, one game process at a time. The original runs under an origrun
rebuilt after 12eda33b: its stand-in now answers `GetLastInputInfo`, so a SexyAppFramework original no longer stops
drawing once the machine has been idle about ten minutes (`docs/ORIGRUN.md`, "The last-input hook"), which may have made
the figures of 2026-11-08 read low. While a run goes, the machine's game processes are counted once a second. The script
waited 1 h 46 min for the machine to have none, then ran 8 pairs between 04:46 and 05:12. Other sessions' games came back
during the first six pairs, so only pairs 7 and 8 saw no other game process:

| Pair | Other game processes (original, port) | Original: processor in the window, whole run | Port: processor in the window, whole run | Private bytes, original / port | Working set, original / port |
|---|---|---|---|---|---|
| 1 | 2, 2 | 4.70 %, 4.19 s | 3.90 %, 3.42 s | 36.3 / 54.7 MB | 57.9 / 61.6 MB |
| 2 | 2, 2 | 4.03 %, 3.38 s | 4.43 %, 3.45 s | 35.9 / 55.1 MB | 57.5 / 61.9 MB |
| 3 | 1, 2 | 4.70 %, 4.13 s | 5.72 %, 4.67 s | 35.7 / 54.8 MB | 57.3 / 61.8 MB |
| 4 | 1, 1 | 3.48 %, 2.97 s | 4.61 %, 4.08 s | 35.7 / 54.4 MB | 57.2 / 61.4 MB |
| 5 | 1, 0 | 4.12 %, 3.72 s | 2.61 %, 3.45 s | 35.9 / 53.8 MB | 57.6 / 61.7 MB |
| 6 | 0, 1 | 4.88 %, 3.97 s | 3.08 %, 3.19 s | 35.7 / 54.9 MB | 57.1 / 61.8 MB |
| **7** | **0, 0** | **4.95 %, 4.17 s** | **4.10 %, 3.48 s** | **35.9 / 55.4 MB** | **57.6 / 62.2 MB** |
| **8** | **0, 0** | **4.31 %, 3.52 s** | **4.03 %, 3.67 s** | **35.9 / 54.7 MB** | **57.5 / 61.7 MB** |

The processor figures are the percentage of one core in the window from 20 s to 80 s, and the seconds over the whole run.

In the two quiet pairs the port uses a little less processor time than the original, 4.1 and 4.0 % of one core against 5.0
and 4.3 %. The medians of all eight pairs agree: 4.1 % against 4.5 %, and 3.5 s against 3.8 s a run. On 2026-11-08 the
port had seemed about 20 % above the original. That was within the noise of a busy machine, or the original's idle stop.
The original's memory reads higher than on 2026-11-08 (36 MB private, 57.5 MB working set, against 34 and 48). It draws
for the whole minute now, and in pair 8 it chose Direct3D's software renderer (D3D10Warp). The port holds about 19 MB more
private memory, as before.

**Where the port's time goes.** `cpuprof.py` was run on the port with `-fast`, because a test run is where its speed
counts. For an autoplay game to its end on seed 7 (2.2 s, 10,637 updates, 1,064 draws), 85 to 90 % of the cycles are
outside the game: system calls (ntdll, 53 %), the NVIDIA driver (21 %), win32u (10 %), and imagehlp (5 %), which wintrust
loads for the driver's signature check when the device is made. The exe has 5 %, spread thin: the textures' upload
(`DDInterface::PrepareTexture`, 1.3 %), the Ogg decoder (about 1 %), `MemoryImage::NormalBlt` (0.6 %), the GIF decoder and
the crossfade image. A run that loads and quits at the title (0.39 s of it after loading, which takes 351 ms by the game's
own log) is 94 % system and driver, and 6 % the exe's decoders (Vorbis, JPEG, PNG, the module player), none above 1 %.
`cpuprof.py` now lists the modules every 200 samples instead of every 2,000, so a run of under a second still names them.

**What the pass changed: nothing in the game.** Every cost the profile shows is one of two kinds:

- The two shared costs in `docs/COORDINATION.md`, to be fixed once for every game: presenting a frame, and creating the
  window and the device, along with the driver's signature check that comes with it.
- The framework's own work, spread thin: decoding images and sounds, uploading textures.

No function of Bejeweled's own code is above 0.3 % of a run, so there was nothing in the game to speed up. The
frozen baseline and the oracle need no new run, since no source changed. The memory gap is the framework's: every
image's pixels kept after the GPU has its texture (`PROFILE.md` section 5, "Pixels after upload", which the coordinator
left for now).

**Not measured.** As on 2026-11-08: the web build in a browser, since the Browser pane is hidden and throttled; full screen;
and the GPU's own load. The samples and logs are in `build/perf/pass/` (git-ignored).

## 20. The readability pass (2026-11-08)

Bejeweled was the pilot of the readability pass: the rules are `docs/CODE_STANDARDS.md`, written while applying them here,
and `tools/namecheck.py games/bejeweled` checks the ones a tool can check (no findings; the exceptions, each with its reason,
are in `games/bejeweled/namecheck.txt`). Nothing the port does changed: the whole oracle, three ways, is ALL IDENTICAL
(443 cases by default and with `-forcevariant`, 293 in the novariants build), the verifier's tests pass, and the code-drawn
art, which the oracle does not reach, dumps byte for byte as before (densities 2 and 4, five gem styles and palettes).
`TitleScreen::Resize` (0x4d4118), which the coverage pilot found with no counterpart, places only the register link after
`Widget::Resize`, so it has a `none` line in `coverage.txt`.

**Names from the Mac build.** The functions `MAC_COMPARISON.md` lists under "Names that differ" now carry the Mac's names,
with the names built on them: `HasMatch` is `HasSet`, `ProcessMatches` `FindSets`, `StartSwap` `SwapPieces`, `UpdateSwap`
`UpdateSwapping`, `ShowText` `DoBouncyText`, `StartBlackHole` `DoWhirlpool`, `UpdateBlackHole` `UpdateWhirlpool`,
`UpdateGameOver` `UpdateGameOverAnim`, `UpdateLevelWipe` `UpdateLevelTransition`, `UpdateFallOut` `UpdateBoardDropping`,
`DoHint` `TryShowHint`, `SetupLevel` `SetupLevelData`, `FillBoard` `NewBoard`, `GetPanForX` `GetPanPosition`,
`IsGameInProgress` `IsInProgress`, `SaveGameExists` `HasSavedGame`, `DeleteSaveGame` `DeleteSavedGame`; in WinDMApp
`ShowOptionsDialog` `DoOptions`, `ShowHighScoreDialog` `DoHighScoreDialog`, `ShowRecordScoreDialog` `DoRecordScoreDialog`,
`ShowInstructionsDialog` `DoInstructionDialogette`, `ShowFullInstructionsDialog` `DoFullInstructions`,
`ShowColorDepthDialog` `DoColorDepthExplanation` (the Mac spells it "Explination"), `ConfirmLoadGame` `TryConfirmLoad`,
`LoadAppImage` `DoLoadImage`, `MakeButton` `CreateControlButton`; in HighScoreWidget `GetRank` `GetScorePlace`,
`GetDayNumber` `GetDayNum`, `GetOrdinalSuffix` `GetOrderSuffix`, `LoadHighScores` `ReadFromFile`, `SaveHighScores`
`WriteToFile` and `DoCheckForHighScore` `ProcessScoreStuff`. The variant versions follow (`FindSetsVariant`,
`NewBoardVariant`, `NewBoardWithMove`, ...). The oracle's input scripts keep the old names in their comments, so that its
cache of the baselines' runs stays valid; earlier reports (`IDENTICAL_PLAY.md`, `MAC_COMPARISON.md`) keep the names of their
time.

**Fields.** Board's `mUnused218` is `mClearedAtMoveStart` (zeroed by the ctor and a move's first scored run, never read),
`mUnused264` is `mSavedZero` (0 throughout, saved and loaded), `ScorePopup::mUnused` is `mAlwaysMinusOne`, and Piece's padding
is named for the field it aligns (`mPadBeforeX`, `mPadBeforeVelY`, `mPadBeforeGlowAlpha`, `mPadBeforeDyingFrame`). Two
fields nothing depends on are left out: Board's SexyString at +0x2c8, which the original constructs and destroys and never
uses, and WinDMApp's two arrays of six images at +0x7d8 and +0x7f0, zeroed, deleted and never used. The state codec's
field names follow the new names (they label its rows in reports; its bytes are unchanged).

**Numbers.** The score popup's offsets and life (`ScorePopup::OFFSET_X`, `OFFSET_Y`, `START_LIFE`), half a gem
(`Board::GEM_HALF`), the gem sheet's frame pitch and colour rows (`GEMSHEET_PITCH`, `GEMSHEET_COLOR_HEIGHT`), the target
cursor's frame (`CURSOR_FRAME_SIZE`) and the rectangle most dialogs take (`DIALOG_X`, `DIALOG_Y`, `DIALOG_WIDTH`) are named.

**One-letter names.** 135 one-letter variables used far from their declaration got words (`ByteWriter w` is `aWriter`,
`Board* b` `aBoard`, ...); in the code-drawn art, the helpers `B()` and `P()` are `MakeRgb()` and `GetPalette()`.
SHA-256's working variables keep the standard's letters.

**Addresses and comments.** Every function names its address, the original function it is part of (`ReadSaveBool` is part
of `LoadGame`, 0x4cca98; the high-score record helpers of `ReadFromFile` and `WriteToFile`), or a port marker; `main.cpp`,
`ModIndicators.cpp` and `testhooks/MemReport.cpp` carry the marker in their first comment. Comments that told the code-mod
stages as a story (`BoardVariant.cpp`'s header, `VariantFlags`, the build warnings in `GameConfigMods.cpp`) or had gone
stale ("no TWIST or SLIDE yet", "nothing reads them yet in stage 1") describe the code as it is.

**No development stages in comments (2026-11-11).** The user on the standards: "the references to stages don't make sense
to anyone looking at the code" (`docs/CODE_STANDARDS.md`, rule 8). The pass had kept 238 lines that named a stage of the
code mods' development ("stage 7b", "the stage-0 list"). Each comment now names the feature (dirt, the gold board, arrow
gems, the frozen baseline) and its section of `CODE_MODS_DESIGN.md`. What files carry stays: the oracle's mod folders
(`mods/stage4/overflow`), `build/baseline-stage0` and `stage_assets.py`. `tools/readability/destage.py` made the edits,
with no line added or removed. Every unit compiles to the same object code as before (98 objects, their disassembly
and data sections hashed before and after).

## 21. Coverage check (2026-11-09)

The coverage check of `docs/COVERAGE.md`: origrun's `--coverage` recorded which of the original's functions ran, and
`tools/coverage.py report` put them beside the port's sources. Bejeweled's comparisons run in Windows Sandbox, where
`--coverage` does not work yet, so the runs were taken on the host, where origrun posts input and takes no shot. The
scripts come from `tools/coverage/scripts.py`, with every position taken from the port's layout, and `tools/coverage/run.sh`
runs one. The hits are in `build/coverage/bejeweled/hits/`. The copy is the prepared one (the exe carved out of
`WinBej.exe`), on the virtual clock with the silent stand-in.

The runs:

- **play**: a fresh Normal game. The first move's hint, the Hint button three times, 340 random swaps (sets, cascades,
  level-ups, a game over), the focus lost and regained, New Game answered No and then Yes, and the window closed during
  play.
- **dialogs**: the focus lost and regained on the title. Options with both sliders, the custom cursors box twice, Help
  and its Done, High Scores and its Done, and Options' Done. Then Timed, Normal with its question, the Quit button
  answered No, and the window closed with Options open.
- **timed**: a Timed game left until its time ran out. The name entry typed and Enter, play again, and the window closed.
- **nomoves** (three runs), **nomoves_hi** (two) and **fallout** (three): a saved game with one move left
  (`origcmp/nomoves.py --one`), loaded through the question, and the move made. Whether the refill leaves a move is the
  original's RNG, which is not pinned on the host, so each was run more than once. Those runs gave "no more moves", the
  game over, the name entry and the high-score screen, with the window closed there. With a score that places on the
  all-time table, they gave the record dialog's "Nth place" text. In Timed mode they gave the board falling out and in.
- **levelup**: a saved game whose score has reached the level's target. The level-up (the wipe and the black hole),
  then play, and the Quit button answered Yes.
- **declined**: a saved game whose load question is answered No, then Quit and Yes.
- **title_quit**: the window closed on the title.
- **extra**: a key typed on the board during play.
- **timedconfirm**: Timed clicked over a loaded game in progress, with its question answered Yes.
- **realclock**: on the real clock with the silent stand-in, so the sound code runs. The title, play and swaps.

Every run ended with the game exiting normally. The runs took about 11 minutes of machine time, one game process at a
time. The user's saved game (`C:\ProgramData\Steam\Bejeweled\userdata\savegame.dat`) was backed up first and put back
afterwards, byte for byte.

**Two things the runs had to get around.**

- The virtual clock does not see the title's loading thread wait on the disk. In two runs the whole script ran before
  the "click here to play" link existed, and the game only reached its title. So origrun's counter is the int at
  0x53ba60, whose top byte is the app's loaded flag (0x53b570 + 0x4f3, which `TitleScreen::Update` 0x4d3c6a reads). Each
  script's first line is `at 16777216 ...`, which waits for that flag.
- The original checks for "no more moves" only when the board settles after a move, so a saved board with no move just
  sits. The saves need exactly one move left.

One run failed at its start, with the original's "Failed to initialize DirectDraw" (`DDERR_NOEXCLUSIVEMODE`) answered by
the stand-in, and passed when run again. `tools/origrun/bejeweled.cfg` and the seed patch, which the identical-play runs
used, are no longer on this machine: they lived only in a worktree since removed. Coverage needs neither, because run.sh
gives the counter itself. **To rebuild before the next Sandbox comparison:** both, from what `IDENTICAL_PLAY.md`
records (the app static 0x53b570, its update count 0x53b9f4, the input gate's ranges in `UpdateAppStep` 0x44c900 and
`Process` 0x44c340, and the seed patch at 0x44e9a1), placed in the main checkout's `tools/origrun`, still git-ignored, so
the E: backup copies them.

`games/bejeweled/coverage.txt` sets the game code as one range, 0x4c9a94-0x4d82d2, with 152 functions. 24 of them are
left out on purpose:

- RegisterDialog and the other registration code;
- the URL functions;
- the update check;
- `WinMain`;
- three empty listener defaults and `TRect::Inflate`, which the framework's headers have;
- `TitleScreen::Resize`: the original's version is `Widget::Resize` followed by the register
  link's place, and the link is taken out (section 18).

**Reached in the original with no counterpart in the port: none, from the first report on.** The readability pass had
already put an address or port marker above every function (section 20).

**Defined in the port, never reached: 24 in the first report, 2 at the end.** The first set of runs reached the
dialogs, play and the level-up, but not the hint, the game over, the high scores or the record dialog. Of the runs
that reached those, two had lost the race with the loading thread and one was the save with no move. The reruns and four
short runs reached all but two:

- `WinDMApp::GetGameSEHInfo` (0x4d4ad9) runs only in the framework's crash report. It forwards to `SexyApp`'s, as the
  export does.
- `WinDMApp::DoColorDepthExplanation` (0x4d5bef, `ShowColorDepthDialog` in the export) runs only when the display
  forces full screen and the box is unchecked, which a modern desktop never does. The export shows dialog 12, modal,
  with the same header, lines and OK footer as the port.

Both are code that only an error or a rare machine reaches, so they stay. **No candidates for taking out**: every
other port function in the game code ran in the original. **Never reached and not defined in the port: none.**

**No change to the game.** The check changed no source file. It added the tools in `tools/coverage/`; the `none` line
for `TitleScreen::Resize` came in with the readability pass's merge.

**Not covered.** Alt+Enter and the Options dialog's full-screen box were not run. On the host they would change the
display mode of the user's machine. The port's full screen is its own (the host switches the window, a listed
deviation). The realplay driver's `fullscreen` step tests it on the port.

## 22. Code the port does not need (2026-11-09)

The last step before a game is done (`docs/COORDINATION.md`): taking out code the port does not need, beyond the
registration code and the buttons that lead out of the game, which went in section 18. The coverage check (section 21)
found no candidate in the translated game: every port function of the game code ran in the original, except two error
paths that stay. So this step looked for port code that nothing calls. Every definition under `src/` was set against
every use of its name in the game, the framework and the platform. That left only the code-drawn art's library (`art/`,
the `codeArt` mod's own code, which no play of the unmodded game reaches): eleven functions written for it and never
called.

- `Raster`: `Path::QuadTo`, `Path::CubicTo`, `Mask::Inner`, `Mask::Union`, `Mask::Scale`, `Canvas::AlphaMask`,
  `Canvas::MultiplyAlpha`, and `Mask::Distance`, which only `Inner` used.
- `VectorFont`: `HasGlyph` and `CharInk`.
- `GemArt`: `GemModelCount`.
- `CodeArt`: `Disable` (the mod is never switched off while the game runs).

Every source file is built, since the build takes each folder's files by pattern, and no other header declares a function
nothing calls.

**Kept on purpose.** The fields the original sets and never reads stay: Board's `mClearedAtMoveStart` and
`mLevelTextShown`, and ScorePopup's `mAlwaysMinusOne`. So do the ones it saves but never sets, such as `mSavedZero`. The save
file and the state codec carry them, and so does the trace hash the oracle compares against the frozen baselines, so
taking them out would change the oracle's output, not the game. Each is named for what it holds (`docs/CODE_STANDARDS.md`,
rule 3). The colour-depth dialog and `GetGameSEHInfo` stay as error paths (section 21). Piece's paddings stay with the
layout the save keeps.

**Proof.** Nothing the game runs changed. The native and web builds pass, and so do the unit suite (rulestest, mod
settings, real input, commands and the rest) and verify (54 checks). The code-drawn art, the only code touched, dumps
byte for byte as main's exe does at scales 1, 2 and 4 (`-dumpcodeart`, 41, 77 and 77 files). The oracle needs no run: no
code it reaches changed.

## 23. The Collection version (2026-11-11)

The port was made from the Steam copy, so it is checked against the game's version on the PopCap Game Collection disc
(2009; `docs/COORDINATION.md`): the two versions compared, and the port run on the Collection's files.

**Getting the files.** The disc image packs every game in InstallShield 15 cabinets. The user allowed unshield for it
("unshield is fine", 2026-11-10): its 1.5.1 source (github.com/twogood/unshield, 67 KB), built with llvm-mingw in
`E:\retools\unshield-build`. No zlib is on the machine, so a 20-line stand-in `zlib.h` serves unshield's three inflate
calls from `third_party/stb`'s inflate, and no second download was needed. The image was mounted read-only, and only
the group `BejeweledDeluxeComponent` came out: 93 files, 2.7 MB, in the main checkout's
`re/_installers/BejeweledDeluxeCollection` (git-ignored). Two things to know when using it for another game: give the
cabinet's path with forward slashes (`F:/data1.cab`), and create the output folders first and extract with `-d .` from
inside them, since its own folder creation fails on a path with a drive letter.

**The exe is the same build.** The Collection's `Bejeweled.exe` is the game itself, with no launcher around it. Set
beside the game carved out of Steam's `WinBej.exe` (`tools/collection/cmpexe.py`), it has the same link time (13 April
2007) and the same sections. `.text` and `.data` are byte for byte the same. `.rdata` differs in 7 bytes, the digits of
a partner stamp at 0x51d340. The resources differ, and the Collection's exe carries a signature after its last section.
So everything the decompilation, the identical-play pass and the coverage check found holds for the Collection's game
too, and no code was compared further.

**The files** (`tools/collection/cmpfiles.py`). Of the Collection's 93 files, 88 are the same as Steam's, byte for byte:
every image, sound, music and data file it has, and `default.xml`. The differences:

- `properties/partner.xml`: the settings' registry key is `PopCap\Bejeweled`, not `Steam\Bejeweled`; the Options link
  goes to PopCap's site, not Steam's page; the update check is not turned off; there is no partner name or Steam id.
  The port reads the registry key from it, so its settings file gets that section name. The link and the update
  check are taken out of the port (section 18), so the rest changes nothing.
- The readme, and the exe's name.
- Steam's copy has files the Collection lacks: the launcher's own (`drm/`, `Flash.ocx`), an MP3 copy of every sound
  beside its Ogg file, and five images (`_overfont2.png`, `overfont2.gif`, `afont1.png`, `mainfont1.png`,
  `_real-back.png`).

**One visible difference, and it is the files'.** The port ran the oracle's playtest script (a Normal game, the hint,
Options, a Timed game to its end, the name entry, play again, quit; 28 screenshots and the trace) once on each set of
files, staged fresh. The traces are the same line for line, and so are the saved game and the high scores. 24
screenshots are the same, and four differ, all in the big banner text ("GO!", "TIME UP"): its edges are a little heavier
on the Collection's files. The cause is Steam's extra `_overfont2.png`. The image loader takes a font's alpha from
`_overfont2.png` when it is there, and the Collection has only `_overfont2.gif`. The Steam files with that one file
removed give all 28 of the Collection run's screenshots. The original has the same loader, so each version of the
original draws its own files the same way; the port follows the files it is given.

**The web build** on the Collection's files (under Node, the same script) gives the same trace as the native build. The
page finds a game folder by `images/gemsheet6.*`, which the Collection's folder has.

**One change.** `tools/stage_assets.py` asked for `WinBej.exe` in the folder it stages from, and so refused the
Collection's. It now takes a folder with `Bejeweled.exe` too. No game code changed.

**Not done.** The Collection's own exe was not run. Its code is the Steam game's, and a run would only show the
banner's edges again.

**The staged assets were an earlier state of the Steam install (found 2026-11-11).** Until this check, `assets/bejeweled`
held 27 images whose bytes differ from the Steam folder's as it is now. Nothing in the repository re-encoded them. They
were staged on 2026-10-18 from the files Steam had downloaded (dated 2018), and on 2026-10-23 at 17:49, the minute the
launcher `WinBej.exe` first ran and made its cache under `C:\ProgramData\Steam\Bejeweled`, 35 files of the install
were rewritten: those 27 images, six of the launcher's own, the licence and the readme. The rewritten images are the
Collection's, byte for byte. The two sets hold the same pictures. 21 decode to the same pixels, and the other six
(`blackcover`, `dialogbox2`, `holemask`, `slider-thumb`, `slider-track`, `timerbar`) differ only in the colour stored
under fully transparent pixels. The port draws both the same: the oracle's playtest and its level-up script give the
same trace and the same 32 screenshots on either staging. The worktree is restaged from the install as it is now
(`python -I games/bejeweled/tools/stage_assets.py`), and the unit and verify suites pass on it.
