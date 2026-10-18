# Bejeweled Deluxe 1.87: game-code behaviour spec

This is the behavioural specification of the game code (0x4c9a94..0x4d87bc of
`re/bin/BejeweledGame.exe`), consolidated from six module analyses plus spot re-checks against
the disassembly. Translators use it together with the decompilation in `re/game/decomp/` and the
headers in `src/game/`. Framework facts (vtable slots, Widget/Dialog/SexyAppBase layouts, RNG,
float rules) are in `docs/FRAMEWORK_NOTES.md`.

**Symbols.** Function, struct-field and data names are in `re/symbols/20_game.jsonl` (for
`ApplySymbols.java`). Header names match those symbols.

**What was re-verified in the disassembly during synthesis:**
- Board::Update score roll (section 3.7, step 16).
- UpdateClearing grid shifting (3.8.4).
- ProcessMatches run scan, averages and the 300..1200 clamp (3.9).
- ColorizeText pixel formula (3.12).
- UpdateBlackHole x87 expressions (3.8.7).
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

## 1. Resources (`src/game/Res.h`)

**Images.**
- `WinDMApp::mImages` (app+0x790) is a `new Image*[36]` indexed by `ImageId`.
- `LoadAppImage(i)` sets `mImages[i] = GetImage("images\\" + gImageNames[i], true)` (app slot 65), then calls `((DDImage*)mImages[i])->GenerateDDSurface()`.
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

## 2. Piece (`src/game/Piece.h`)

- 0x78-byte POD; the field table is in the header.
- The ctor (0x4c9aec) zeroes mColor, mX, mY, mCol, mRow, mVelX, mVelY, mSwapOffX/Y, mGlowing, mGlowAlpha, mGlowFrame, mDying, mDyingFrame, mSpinFrame, mSparkleCount and mSparkleFrame.
- It leaves the padding (+0x04, +0x24, +0x39..+0x3f, +0x4d..+0x4f) and the vortex fields (+0x60, +0x68, +0x70) uninitialised.
- The struct is written raw to savegame.dat (0x78 bytes per present cell).
- port: zero everything. When loading, accept arbitrary padding bytes.
- port: code-mod fields follow the record at +0x78 (`Piece::SAVE_SIZE`); the save still writes and reads 0x78 bytes per piece (12.1).

---

## 3. Board (`src/game/Board.h`)

### 3.1 Geometry

| Function | Result |
|---|---|
| `GetColX(c)` | `c*51 + 203` |
| `GetRowY(r)` | `r*51 + 16` |
| `GetBoardSize` | w = (49 - ColX(0)) + ColX(7) = 406; h = (49 - RowY(0)) + RowY(7) = 406 |
| `GetBoardCenter` | x = (ColX(7)+49-ColX(0))/2 + ColX(0) = 406; y = (RowY(7)+49-RowY(0))/2 + RowY(0) = 219 |

- Gems are 49x49, drawn at `((int)mX, (int)mY)`.
- `GetPanForX(x)` = `(int)(((double)(x - 203 - 203) / 406.0) * 2.0 * 800.0)`, range about -800..800. It is used as the PlaySample pan.
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
   - mMoveScore, mUnused218, mMoveScoreTimer, mHintGlowTimer, mHintGlowDuration
   - mInputDelay, mMouseDown, mPlayStartTick, mPauseCount, mUserPauseCount
   - mStatsDisabled, mLevelStartScore, mUnused264, mSwapPiece1, mSwapPiece2
6. Fonts and level: mPointsFont = app->mAlphaDigitsFont; mScoreFont = app->mScoreFont; mLevel = 0; mDisplayLevel = 1.
7. Background images:
   - `mBackgroundImage = new DDImage(app->mDDInterface)`, then `SetImageMode(false,false)` (MemoryImage slot 32), then `Create(640,480)`.
   - Same for `mNextBackgroundImage`.
8. `SetupLevel()` (3.5).
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
      FillBoard();                       // RNG, sets mState = 0, mIdleTicks = 0
      for (i = 0; FindMove(m, i, true, false); i++)   // m = {col,row,col2,row2}
        if (m[2] == m[0]+1 && (unsigned)(m[1]-4) <= 2 && m[0] >= 1 && m[0] < 7)
          { mTutorialCol = m[0]; mTutorialRow = m[1]; mTutorialCol2 = m[2]; break; }
    } while (mTutorialCol == -1);
    ```
    - The loop accepts the first horizontal-run move (in FindMove order) that swaps to the right, in rows 4..6, with col 1..6.
    - Rejected boards consume RNG and their pieces leak; FillBoard does not delete them.

Not initialised by the ctor: mState (FillBoard sets it), mSwapAngle, mSwapCenterX/Y, mMouseX/Y,
mMouseDownX/Y, mTextPhase, mTextDuration, mTextTimeLeft, mWipeY, mBlackHoleFrame,
mLevelTextShown. port: zero them, and set mMouseX/Y to -1 (as after MouseLeave). Before the first
mouse event, the original's hover test reads heap garbage.

### 3.3 Destruction: `~Board` (0x4cd1ba)

1. If `IsGameInProgress()`, call `SaveGame()`.
2. Delete (virtual) mBackgroundImage, mNextBackgroundImage, mTextImage and mTextColorImage.
3. `mWhirlpoolSound->Release()` if set.
4. `app->KillDialog(9)`.
5. If `IsGameInProgress()` is still true, `++app->mTimesPlayed`. An abandoned game counts as played.
6. Destroy the string and the list; Widget::~Widget.

Pieces in the grid are not deleted (leak). port: delete them here.

`IsGameInProgress()` (0x4c9bbe) = `!mIsNewGame && mState != 5 && mState != 6`.

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

**`ShowHint()` (0x4cb475).** If `mState == 1 && mPauseCount == 0`, call `DoHint()`. Called from WinDMApp button 15.

**`DoHint()` (0x4cad2f).**
1. `mScore = (int)((double)mScore - mPointMultiplier*50.0)`, clamped to >= 0.
2. If `FindMove(m, 0, true, true)`:
   - q = mBoard[m.row2][m.col2], the gem the move would bring in
   - q->mSparkleCount = 58; q->mSparkleFrame = 0; q->mSpinFrame = 1 (one spin)
3. mIdleTicks = 0.

No sound, no RNG.

### 3.5 `SetupLevel()` (0x4cae29): background and level parameters

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

**`FillBoard()` (0x4cb182).**
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
       do { p->mColor = Rand()%7; } while (HasMatch());    // RNG draws 2..n
     }
   ```
The result has no 3-in-a-row. Pieces start above the screen with mVelY = 0 and fall in state 0.

**port: do not free the orphaned pieces in FillBoard.**
- Original: the pieces from state 9 (and from rejected ctor boards) leak but stay valid. `mSelectedPiece` can still point at one, because a gem can be selected during states 0/2/3/4/9.
- A later click in state 1 then calls `StartSwap(stalePiece, target)`. If adjacent by the stale col/row, the stale piece is swapped back into the grid. This is an original bug.
- To stay faithful and memory-safe, park orphans in a list owned by the Board and free them in ~Board.

**`HasMatch()` (0x4c9cf5).**
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
| DoHint | `FindMove(m, 0, true, true)` |

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
    - If `!app->mShowInstructions || app->mTimesPlayed >= 5`: `ShowText(GetString("GO"), 250)`.
    - If `app->mShowInstructions && app->mTimesPlayed < 5`: `app->ShowInstructionsDialog(); mShowTutorial = true;`
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
    - `if (mDisplayScore - (int)mTimeDrain - mLevelStartScore >= mLevelTargetScore - mLevelStartScore) StartBlackHole();` (sets state 7)
    - Then, if still state 1: `if (mDisplayScore - (int)mTimeDrain - mLevelStartScore <= mLevelStartScore - mLevelTargetScore) { ShowText(GetString("TIME_UP"), 400); GameOver(200); }`
    - The time-up test is not restricted to timed mode. In normal mode it can only fire when hints (DoHint) push mScore far enough below mLevelStartScore (one level's worth); see open question 3.
21. Dispatch on mState:

    | State | Handler |
    |---|---|
    | 0 | UpdateFalling |
    | 2, 3 | UpdateSwap |
    | 4 | UpdateClearing |
    | 5 | UpdateGameOver |
    | 7 | UpdateBlackHole |
    | 8 | UpdateLevelWipe |
    | 9 | UpdateFallOut |
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
  { mLastLandSoundTick = cnt; PlaySample(8 gemongem2, GetPanForX(sumX/landed)); }
if (settled) {
  if (ProcessMatches()) mState = 4;
  else {
    if (!mStatsDisabled) { mLongestCascade = max(mLongestCascade, mCascadeCount);
                           mBestMoveScore  = max(mBestMoveScore,  mMoveScore); }
    if (FindMove(NULL, 0, true, true)) mState = 1;
    else if (mTimedMode) mState = 9;
    else { ShowText(GetString("NO_MOVES"), 400); GameOver(200); }
  }
}
MarkDirty;
```

#### 3.8.2 State 1

Idle. Input (3.10) and Update steps 18 and 20 apply.

#### 3.8.3 States 2/3: swap animation

**`StartSwap(Piece* a, Piece* b)` (0x4c9ff8).**
1. If `abs(a->mCol - b->mCol) + abs(a->mRow - b->mRow) != 1`: PlaySample(7 bad2) and return.
2. Tutorial check: if `mShowTutorial && a->mCol == mTutorialCol && a->mRow == mTutorialRow && b->mCol == mTutorialCol2 && b->mRow == mTutorialRow`:
   - app->mShowInstructions = false; mShowTutorial = false; app->KillDialog(9).
   - Only the exact demonstrated move (first gem dragged right) dismisses it here.
3. mSwapAngle = 0; mSwapPiece1 = a; mSwapPiece2 = b.
4. `mSwapCenterX = (int)((b->mX + a->mX)*0.5); mSwapCenterY = (int)((a->mY + b->mY)*0.5)`.
5. `a->mSwapOffX = (int)(a->mX - mSwapCenterX)`, and likewise for a's Y and for b.
6. mState = 2. There is no NULL check on the pieces.

**`UpdateSwap()` (0x4cd84f).** MarkDirty; mSwapAngle += 5.

If mSwapAngle == 180 (A = mSwapPiece1, B = mSwapPiece2):
1. Exchange the pieces:
   - (c,r) = (A.col, A.row); A.col = B.col; A.row = B.row
   - A.x = ColX(A.col); A.y = RowY(A.row); mBoard[A.row][A.col] = A
   - B.col = c; B.row = r; B snaps likewise; mBoard[r][c] = B
2. mCascadeCount = 1; mMoveMatchCount = 0.
3. If `ProcessMatches()`:
   - mSwapPiece1 = mSwapPiece2 = NULL; mState = 4.
   - If mShowTutorial: mShowTutorial = false; app->KillDialog(9); app->mShowInstructions = false.
   - mIsNewGame = false.
4. Else if mState == 2 (illegal swap):
   - `A.mSwapOffX = -A.mSwapOffX; A.mSwapOffY = -A.mSwapOffY; B.mSwapOffX = -A.mSwapOffX; B.mSwapOffY = -A.mSwapOffY` (B uses A's new values)
   - mSwapAngle = 0; PlaySample(7 bad2); mState = 3.
5. Else (state 3 finished): mSwapPiece1 = mSwapPiece2 = NULL; mState = 1. ProcessMatches already ran on the restored board and found nothing.

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

#### 3.8.5 State 5: `UpdateGameOver()` (0x4cab82)

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

**`StartBlackHole()` (0x4ca58d).** Called from Update step 20.
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

**`UpdateBlackHole()` (0x4cb931).** Verified.
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
   ShowText(StrFormat(GetString("LEVEL").c_str(), mLevel + 2), 400); }   // "LEVEL %d", the next level's number
if (done) {
  ++mLevel; SetupLevel();               // new background into mNextBackgroundImage, new multiplier/points/drain
  delete all 64 pieces, cells = NULL;
  mLevelTargetScore = mLevelPoints + mScore; mLevelStartScore = mScore;
  PlaySample(17 firecrackle); mWipeY = 0; mState = 8;
}
MarkDirty;
```
No RNG.

#### 3.8.8 State 8: `UpdateLevelWipe()` (0x4cb284)

1. `Graphics g(mBackgroundImage); g.ClipRect(0, mWipeY, mWidth, 3); g.DrawImage(mNextBackgroundImage, 0, 0);`
2. Whirlpool fade:
   - `mBlackHoleLevel -= 0.015`.
   - If it is still >= 0: `mWhirlpoolSound->SetVolume(level)` (if set).
   - Else: `mBlackHoleLevel = 0.0`; Release the sound and set it to NULL.
3. `if (cnt % 4 == 0) mElectOffset = Rand() % 128;`
4. MarkDirty; mWipeY += 3.
5. If `mWipeY > mHeight`:
   - mDisplayLevel = mLevel + 1; mTimeDrainDelay = 200; FillBoard().
   - Then `if (!app->mIsRegistered && app->mTimesPlayed >= 15 && Rand()%5 == 0) app->ShowRegisterNagDialog();`
   - The Rand draw happens only when the first two conditions hold, after FillBoard's draws.

#### 3.8.9 State 9: `UpdateFallOut()` (0x4cb3bf)

Timed mode with no moves left:
```
allGone = true; MarkDirty;
for col 0..7 { start = (Rand() % 15 == 0);          // exactly 8 draws per tick, even for empty columns
  for row 7..0 { p = mBoard[row][col]; if (!p) continue;
    if (p->mVelY == 0.0) { if (start) { p->mVelY = 1.0; start = false; } }   // no move this tick
    else { p->mY += p->mVelY; p->mVelY += 0.18; }
    if (p->mY < (double)(mHeight + 32)) allGone = false; } }
if (allGone) { mTimeDrainDelay = 200; FillBoard(); }
```
- Pieces start in UpdateFalling with mVelY = 0, so each column is released bottom-up, one gem per lucky tick.
- The drain keeps running only in states 1 and 2, so it pauses during the fall-out.

### 3.9 `ProcessMatches()` (0x4cd287): scoring (verified)

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
        if (mMoveMatchCount == 1) { mHintGlowAlpha = 0.0; mHintGlowTimer = 0; mMoveScore = 0; mUnused218 = 0; }
        thr = (int)(mPointMultiplier * 120.0);
        if (mMoveScore < thr && mMoveScore + pts >= thr)
          { ShowText(GetString("EXCELLENT"), 200); PlaySample(20 excellent1); }
        mMoveScore += pts;
        if (mMoveScoreTimer != -1)
          mMoveScoreTimer = (int)clamp((double)mMoveScore / mPointMultiplier * 3.0, 300.0, 1200.0);
        maxLen = max(maxLen, count);
      }
      color = p ? p->mColor : -1; count = 1; start = (col,row);
    } } }
if (numMatches > 0) {
  pan = GetPanForX(sumAvgX / numMatches + 24); mIdleTicks = 0;
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
   - Otherwise: mCursorFrame = 0; mSelectedPiece = p; MarkDirty; `PlaySample(11 select, GetPanForX((int)(p->mX + 24.0)))`; return.
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
   if (target && target != sel) StartSwap(sel, target);   // a non-adjacent target plays bad2
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
5. `StartSwap(sel, neighbour)` (no NULL check).
6. In every case past the threshold: mSelectedPiece = NULL; MarkDirty.

**`KeyChar(c)` (0x4ca9d3):** any key toggles the user pause (`Unpause(true)` if mUserPauseCount > 0, else `Pause(true)`).

### 3.11 Banner text

**`ShowText(text, duration)` (0x4ca1c8).**
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

**`SaveGameExists()` (0x4cca37):** `app->FileExists(GetAppDataFolder() + "userdata\\savegame.dat")`.

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
| | i32 x 4 | mUnused264, mLevelStartScore, mLevelTargetScore, mLevelPoints |
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

`SetupLevel()` is NOT called, so the background stays the level-0 look (faithful bug). A state-4 save resumes with dying gems.

**`DeleteSaveGame()` (0x4cd130):** `app->EraseFile(path)`.

Callers:
- WinDMApp ButtonDepress 2016: LoadGame + DeleteSaveGame.
- 3016: DeleteSaveGame.
- Shutdown and ~Board: SaveGame when IsGameInProgress.

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

1. FillBoard: per cell (rows 7..0, cols 0..7): `Rand()%240`, then `Rand()%7` until no match.
2. UpdateClearing refill: `Rand()%7` per gapped column per pass, cols 0..7, passes repeated.
3. UpdateGameOver: every 6th tick and the final tick, per piece row-major: `%9` (x), `%9` (y); on the final tick also `%8` (vy) and `%7` (vx).
4. UpdateLevelWipe: `%128` every 4th tick; at the end, FillBoard, then optionally `%5` (unregistered, 15+ plays).
5. UpdateFallOut: 8 x `%15` per tick.
6. Outside Board: `HighScoreWidget()` ctor: 1 x `%9` (game over and every High Scores dialog open).
7. Startup consumes RNG in two Board ctors (TitleScreenIsFinished calls NewGame twice).

---

## 4. WinDMApp (`src/game/WinDMApp.h`)

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
5. mImages = new Image*[36] (all NULL). For i = 0..6: `if (!LoadAppImage(i)) { Shutdown(); return; }`.
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
3. Images 7..35: `LoadAppImage(i)`; on failure go to end; ++mCompletedLoadingThreadTasks per image.
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
3. Side-bar buttons, `MakeButton(id, Rect, overImg, downImg)`:

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
6. `ConfirmLoadGame()`.

**`MakeButton` (0x4d6e71).**
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
1. If not mShutdown, and mBoard && `mBoard->IsGameInProgress()`:
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
| ConfirmQuit (0x4d58af) | 1 | lines = CONFIRMQUIT1, + CONFIRMQUIT2 if `mBoard && mBoard->IsGameInProgress()`. DoDialog(1, modal, GetString("QUIT"), lines, "", YES_NO). |
| ShowOptionsDialog (0x4d483e) | 2 | `new OptionsDialog(this)`, Resize(234, 50, 348, prefH(348)), AddDialog(2). |
| ConfirmNormalMode (0x4d56ec) | 3 | If state 6, SetTimedMode(false,true). Else DoDialog(3, CONFIRMNORMALMODE1/2, YES_NO). |
| ConfirmTimedMode (0x4d57cd) | 4 | If state 6, SetTimedMode(true,true). Else DoDialog(4, CONFIRMTIMEDMODE1/2, YES_NO). |
| ShowRegisterDialog (0x4d5272) | 5 / 19 | If mIsRegistered: `d = DoDialog(19, true, "Already Registered", "You have already registered Bejeweled!", "OK", FOOTER)` (literals), then `d->Move(d->mX, d->mY+24)`. Else `new RegisterDialog`, Resize((mWidth-348)/2, 32, 348, prefH), AddDialog(5). port: only the 19 branch. |
| ShowRegistrationInvalidDialog (0x4d5386) | 6 | DoDialog(6, REGISTRATION_INVALID_DIALOG1/2/3, FOOTER); PlaySample(7). |
| ShowRegisterToRecordScoreDialog(lines) (0x4d5486) | 7 | DoDialog(7, GetString("HIGH_SCORE"), lines, "", YES_NO). |
| ShowRecordScoreDialog(lines) (0x4d45e3) | 8 | `new RecordScoreDialog(this, lines)`, Resize(250, 32, 316, prefH(316)), AddDialog(8). |
| ShowInstructionsDialog (0x4d59cc) | 9 | `NewDialog(9, false, "", INSTRUCTIONDIALOG, "", NONE)`; mContentInsets = Insets(16,24,16,24); Resize(201, 38, 418, prefH(418)); AddDialog(9). Non-modal. |
| OpenURL (0x4d5077) | 10 | `NewDialog(10, true, "", OPENING_BROWSER, "", NONE)`; mContentInsets = (16,26,16,16); Resize(180,140,280,70); AddDialog; return SexyAppBase::OpenURL(url, shutdown). |
| URLOpenFailed (0x4d4e97) | 11 | Base call; KillDialog(10); CopyToClipboard(url); NewDialog(11, true, OPEN_BROWSER, URL_OPEN_ERROR1 + url + URL_OPEN_ERROR2, DIALOG_BUTTON_OK, FOOTER); Resize(100, 50, 440, prefH(440)); AddDialog. |
| URLOpenSucceeded (0x4d4592) | - | Base call; KillDialog(10). |
| ShowFullInstructionsDialog (0x4d5ac1) | 12 | NewDialog(12, true, FULL_INSTRUCTIONS1, FULL_INSTRUCTIONS2, DONE, FOOTER); mTextAlign = -1 (left); Resize(32, 8, mWidth-64, prefH(mWidth-64)); AddDialog. |
| ShowColorDepthDialog (0x4d5bef) | 12 | DoDialog(12, COLOR_DEPTH_EXPLANATION1/2, DIALOG_BUTTON_OK, FOOTER). |
| ShowHighScoreDialog (0x4d48b6) | 13 | `new HighScoreDialog(this)`, Resize(194, 50, 428, 326), AddDialog(13). |
| ShowRegisterNagDialog (0x4d5cdf) | 15 | See below; NewDialog(15, true, REGISTER_NOW, msg, "", YES_NO); Resize(234, 50, 348, prefH(348)); AddDialog. |
| ConfirmLoadGame (0x4d550d) | 16 | If `mBoard->SaveGameExists()`: d = DoDialog(16, CONFIRM_LOAD_DIALOG1/2, YES_NO); d->Resize(234, 50, 348, prefH(348)). |
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
| 16 | KillDialog(16); `mBoard->LoadGame(); mBoard->DeleteSaveGame()` |
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
| 16 | KillDialog(16); mBoard->DeleteSaveGame() |
| 10018 | KillDialog(10018); KillDialog(18) |
| other | KillDialog(d) |

Then, for every id, a second switch:

| Id | Action |
|---|---|
| 10 | ConfirmNewGame() |
| 11 | `if (!mTimedMode) ConfirmNewGame(); else if (mBoard->mIsNewGame) SetTimedMode(false, true); else ConfirmNormalMode();` |
| 12 | `if (mTimedMode) ConfirmNewGame(); else if (mBoard->mIsNewGame) SetTimedMode(true, true); else ConfirmTimedMode();` |
| 13 | ShowOptionsDialog() |
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

## 5. TitleScreen (`src/game/TitleScreen.h`)

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

## 6. HighScoreWidget (`src/game/HighScoreWidget.h`)

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
6. `LoadHighScores()`.

**`~HighScoreWidget` (0x4d0dad).** Delete mBoardImage and mPlayAgainLink, then `SaveHighScores()`. It always re-saves.

**`Resize` (0x4cedf5).**
1. Widget::Resize; mSlideY = -theHeight.
2. If mBoardImage is NULL, take the board snapshot:
   - `new DDImage(app->mDDInterface); Create(mWidth, mHeight);`
   - `Graphics g(img); app->mBoard->Draw(&g); img->GenerateDDSurface()`.

**`AddedToManager` / `RemovedFromManager`.** Base, then add/remove mPlayAgainLink as a top-level widget.

**`Update` (0x4d1187).**
1. Widget::Update.
2. If mSlideY < 0:
   - mSlideY += 8; if it is now >= 0: mSlideY = 0 and `DoCheckForHighScore()`.
   - MarkDirty; `mPlayAgainLink->Resize(265, mSlideY + 389, 286, 23)`.
   - At height 480 the slide takes 60 ticks.
3. `if (mUpdateCnt % 10 == 0) { ++mColorCycle; MarkDirty; }`

**`ButtonDepress` (0x4ceeb7):** `app->NewGame()` (no sound).

**`GetRank(mode, score)` (0x4ced82).** `rank = 0; for (i = 4; i >= 0; i--) if (score > mScores[mode][i]) rank = i+1;`. Ties do not place.

**`GetDayNumber()`:** `tm_year*366 + tm_yday` from `_localtime64(_time64())`.

**`GetOrdinalSuffix(n)`:** `(n%100)/10 == 1` gives "th"; otherwise n%10 of 1/2/3 gives "st"/"nd"/"rd", anything else "th".

**`DoCheckForHighScore()` (0x4d0e4b).** The virtual `CheckForHighScore()` (0x4d11fd) jumps here.
1. `score = app->mBoard->mScore; mode = app->mTimedMode` (app's flag).
2. `best = score > mTodayScores[mode]; rank = GetRank(mode, score); N = "%d"(rank) + GetOrdinalSuffix(rank)`.
3. Messages (literals). `\n` is a newline; R = `"\n\nWould you like to register so\nyou can record your score?"`.

| Case | Registered | Unregistered |
|---|---|---|
| best && rank > 0 | `"You got the highest score of\nthe day and " + N + " place\nof all time!"` | same + R |
| best && rank == 0 | `"You got the highest score\nof the day!"` | same + R |
| !best && rank > 0 | `"You got " + N + " place of all time!"` | same + R |
| otherwise | no dialog | no dialog |

   - Registered: `app->ShowRecordScoreDialog(msg)`. Unregistered: `app->ShowRegisterToRecordScoreDialog(msg)`.
   - The HSWIDGET_*PLACEMENT* property strings are unused.

**`AddHighScore(name)` (0x4d06d8).** Called on dialog 8 OK or Enter.
1. If name is empty, return without saving.
2. `mode = app->mBoard->mTimedMode` (the Board's flag); score = board->mScore; rank = GetRank(mode, score).
3. `if (score > mTodayScores[mode]) { mTodayScores[mode] = score; mTodayNames[mode] = name; }`
4. `if (rank) { for (k = 4; k >= rank; k--) shift [k-1] -> [k]; mNames[mode][rank-1] = name; mScores[mode][rank-1] = score; }`
5. `SaveHighScores()`.

**hiscores.dat format.**
- Little-endian, no header. Two tables, Normal then Timed. Each table:
  ```
  int32 day                                  // GetDayNumber() at save time
  today:   u16 len, char name[len], int32 score, int32 checksum
  5 x all-time: u16 len, char name[len], int32 score, int32 checksum
  ```
- `checksum = score ^ 0x42BEEF ^ XOR_k((int)(signed char)name[k] * 17)`.
- File size = 8 + 12*10 + total name bytes.

**Load (`LoadHighScores`, 0x4ceec2).** `fopen(appdata+"userdata\\hiscores.dat","rb")`; if missing, keep the defaults. Per table:
1. Read day; today's len (min(len,255)), name, score and checksum.
2. The today record is accepted only if `day == GetDayNumber() && checksum ok`; otherwise the defaults stay.
3. For each all-time entry: the name is always taken; score = checksum ok ? score : score % 256 (tamper penalty).
4. fread results are never checked. port: stop at EOF and keep the defaults (see open questions).

**Save (`SaveHighScores`, 0x4cf1b4).**
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

## 7. HighScoreDialog (`src/game/HighScoreDialog.h`)

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

## 8. OptionsDialog (`src/game/OptionsDialog.h`)

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

**`CheckboxChecked(id, checked)`.** Only when `id == 0 && !checked && app->mForceFullscreen`: `app->ShowColorDepthDialog(); mFullscreenCheckbox->SetChecked(true, true)`.

**`ButtonPress(id)`:** Dialog::ButtonPress(id), then PlaySample(12). The footer Done therefore plays menuclick twice (here and in WinDMApp::ButtonPress(2002)).

**`ButtonDepress(id)`.** Dialog::ButtonDepress(id) (Done leads to app 2002), then:
- id 100..199: `app->OpenURL(GetString(StrFormat("OptionsLinkURL%d", id-100), "http://www.popcap.com"), false)`. Shipped: steam://publisher/popcap. port: platform open.
- 0: ShowFullInstructionsDialog. 1: ShowRegisterDialog. 2: ShowHighScoreDialog.
- 3: OpenURL("http://www.popcap.com") (no such button). 4: DoUpdateCheck (unreachable).

## 9. RecordScoreDialog (`src/game/RecordScoreDialog.h`)

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
2. CLICK HERE TO PLAY → TitleScreenIsFinished: buttons, two NewGame() calls (normal mode), then ConfirmLoadGame.
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

**Level complete.** Bar full → state 7 vortex → state 8 wipe → FillBoard (state 0).

**No moves.**
- Normal mode: NO MOVES! → GameOver(200).
- Timed mode: state 9 fall-out → FillBoard.

**Time up.** Bar empty → TIME UP! → GameOver(200).

**Game over.**
1. State 5: shake, then explode/fall.
2. gameover1 → state 6 → ShowHighScoreWidget: the board is removed but kept; the widget slides down over the snapshot.
3. At the end of the slide, DoCheckForHighScore → dialog 8 (registered) or 7 (unregistered).
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
- Leaks the port must NOT fix by deleting: the pieces FillBoard orphans. Park them in a Board-owned list (3.6).
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

The code mods (docs/CODE_MODS_DESIGN.md) touch the translated functions only with constants that become
members holding the same value and with inert hooks, each line tagged `// port(variant):`.
`tools/list_variant_hooks.py` prints those lines grouped by function and address; its count is tracked
per stage.

| Stage | Hook lines | Translated functions | What |
|---|---|---|---|
| 1 | 55 | 22 | `InitVariant()` in the ctor; loop bounds `BOARD_ROWS`/`BOARD_COLS`/8/7 → `mRows`/`mCols`; `BOARD_X`/`BOARD_Y` → `mLayout.mOriginX/Y` in `GetColX`/`GetRowY`; `mHeight` → `mLayout.mScreenBottom` (480) in `UpdateGameOver`/`UpdateFallOut`; `sizeof(Piece)` → `Piece::SAVE_SIZE` in the save |

With the original rules every edited line computes what the original did: the board is 8x8, the layout
is the original constants, and no hook reads the variant flags yet. `UpdateClearing`'s refill pass,
`FillBoard`, `SetupLevel`'s tile loop and `Draw` keep their 8x8 constants: on other boards they will be
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

---

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
| 0x4c9bbe | IsGameInProgress | CanSaveGame |
| 0x4c9ff8 | StartSwap | BeginSwap |
| 0x4ca36e | ColorizeText | RenderTextImage |
| 0x4ca58d | StartBlackHole | StartVortex |
| 0x4cae29 | SetupLevel | LevelSetup / BuildLevelBackground |
| 0x4cb3bf | UpdateFallOut | UpdateBoardDrop |
| 0x4cb475 | ShowHint | Hint |
| 0x4cb931 | UpdateBlackHole | - |
| 0x4cca37 | SaveGameExists | SavedGameExists |
| 0x4cd287 | ProcessMatches | ResolveMatches |
| 0x4d0e4b | DoCheckForHighScore | CheckForHighScoreImpl (10_framework.jsonl name, overridden by 20_game.jsonl) |

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
5. **High-score mode source.** HighScoreWidget mode: DoCheckForHighScore uses app->mTimedMode, AddHighScore uses board->mTimedMode. They should always be equal (the board is created from the app flag, and LoadGame sets both); not proven for every path.
6. **Truncated hiscores.dat.** LoadHighScores reuses stale or uninitialised locals on a truncated file. The recommended port behaviour (stop at EOF, keep defaults) is a guess at "most faithful sane" behaviour.
7. **HighScoreWidget overlay.** It relies on alphaback1's alpha to let the panel show through the left HUD area. Visually unverified.
8. **DrawEditBox overdraw.** The visible result depends on editbox.png pixels; reproduce the draw order rather than "fixing" it.
9. **Double menuclick.** OptionsDialog Done plays sample 12 twice in one tick. Whether the port's mixer layers two instances like DirectSound is a framework decision.
10. **TitleScreen version text.** Position (537 - w/2, 221) in mainfont1 and grey; not checked against a screenshot.
11. **OptionsDialog base order.** The analyst places SliderListener at +0x150 and CheckboxListener at +0x154; FRAMEWORK_NOTES lists them in the other order. Layout-only; it does not affect the port.
12. **Dialog 17.** "Check for updates" is handled but never created (dead code); its strings exist only in default.xml.
13. **Pre-Vista AppData.** GetAppDataFolder() returns "" there, so saves go to the working directory. The port always uses its user dir.
