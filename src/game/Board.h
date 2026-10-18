#ifndef __BOARD_H__
#define __BOARD_H__

// Board.cpp: the 8x8 playfield, rules, scoring, animations and savegame.
// Original: class Sexy::Board : public Widget, sizeof 0x318 (WinDMApp::NewGame does new(0x318)),
// vftable 0x5065cc. Offsets in the comments are absolute from the object start (Widget is 0x88
// bytes, so Board data starts at +0x88; Ghidra's Board_data.offset_N is absolute N+0x88).
// Behaviour spec: docs/GAME_NOTES.md (Board section).

#include "Widget.h"
#include "Piece.h"
#include <list>
#include <vector>

namespace Sexy
{

class WinDMApp;
class Buffer;
class Graphics;
class DDImage;
class MemoryImage;
class ImageFont;
class SoundInstance;

// Board::mState (+0x194)
enum BoardState
{
	STATE_FALLING = 0,          // gems drop into place; when settled: ProcessMatches / no-moves check (UpdateFalling)
	STATE_PLAYING = 1,          // waiting for input
	STATE_SWAPPING = 2,         // swap animation (UpdateSwap)
	STATE_SWAPPING_BACK = 3,    // illegal swap returning (UpdateSwap)
	STATE_CLEARING = 4,         // matched gems shrink, then the columns refill (UpdateClearing)
	STATE_GAME_OVER = 5,        // shake for mGameOverCountdown ticks, then explode and fall (UpdateGameOver)
	STATE_GAME_OVER_DONE = 6,   // HighScoreWidget shown; nothing updates
	STATE_BLACK_HOLE = 7,       // level complete: gems spiral into the vortex (UpdateBlackHole)
	STATE_LEVEL_WIPE = 8,       // new background wiped in 3 px/tick, then FillBoard (UpdateLevelWipe)
	STATE_FALL_OUT = 9          // timed mode, no moves: the board falls out and is refilled (UpdateFallOut)
};

// Element of Board::mScorePopups (0x14 bytes; std::list node 0x1c).
struct ScorePopup
{
	int		mX;              // +0x00 center x = average run x + 24
	int		mY;              // +0x04 y = average run y + 16; -1 every 3 ticks while > ascent/2 + 2
	int		mLife;           // +0x08 110 at creation, -1/tick, erased at <= 0; alpha = min(255, mLife*255/18)
	int		mUnused;         // +0x0c always -1, never read (probably a color)
	int		mPoints;         // +0x10 drawn with "%d"
};

// Background tile tints, indexed [level % 4][tile A/B][r,g,b]  (@0x5064f0; both tiles are identical)
inline constexpr int gBoardTileColors[4][2][3] =
{
	{{102, 82, 70}, {102, 82, 70}},
	{{130, 72, 72}, {130, 72, 72}},
	{{72, 130, 72}, {72, 130, 72}},
	{{72, 72, 130}, {72, 72, 130}}
};

// Base points of the n-th run (1-based) scored since the current swap completed (switch at
// 0x4cd3b7: 1..8 index this table, anything above 8 uses the last entry).
inline constexpr int gMatchBaseScores[9] = {10, 20, 30, 40, 60, 80, 110, 160, 210};

class Board : public Widget
{
public:
	enum
	{
		BOARD_COLS = 8,
		BOARD_ROWS = 8,
		NUM_GEM_COLORS = 7,
		GEM_SIZE = 49,              // gem hit box / sprite size (double 49.0 @0x506560)
		CELL_SIZE = 51,             // grid pitch
		BOARD_X = 203,              // GetColX(0)
		BOARD_Y = 16,               // GetRowY(0)
		BOARD_CENTER_X = 406,       // GetBoardCenter
		BOARD_CENTER_Y = 219,
		BAR_WIDTH = 340,            // progress / timer bar width in px
		NUM_BAR_COLORS = 24,        // timerbar hues 0..23; index 24 = white flash
		BAR_COLOR_NORMAL = 23,
		BAR_COLOR_FLASH = 24,
		TUTORIAL_DIALOG_ID = 9,     // WinDMApp DIALOG_INSTRUCTIONS
		SAVEGAME_VERSION = 1
	};

	WinDMApp*		mApp;                   // +0x088 owning app (ctor arg)
	int				mLevel;                 // +0x08c 0-based level; ++ when the vortex finishes; drives level%4 tint, level%5 texture and the diff.txt formulas
	int				mDisplayLevel;          // +0x090 1-based level shown in the HUD; ctor 1, = mLevel+1 at the end of the wipe
	Piece*			mBoard[8][8];           // +0x094 grid [row][col] (original index row*8+col), NULL = empty; ends at +0x193
	int				mState;                 // +0x194 BoardState
	bool			mTimedMode;             // +0x198 time-trial board (ctor arg = app->mTimedMode)
	SoundInstance*	mWhirlpoolSound;        // +0x19c looping whirlpool1 instance (StartBlackHole); released at the end of the wipe / in ~Board
	bool			mIsNewGame;             // +0x1a0 ctor true; cleared by the first swap that makes a match. While set: no save on exit, mode switch without confirmation
	Piece*			mSelectedPiece;         // +0x1a4 selected gem (target cursor + spin) or NULL
	int				mCursorFrame;           // +0x1a8 target1 cursor frame counter (++ every 8 ticks while selected; frame = %6)
	bool			mShowTutorial;          // +0x1ac first-move tutorial arrows shown (set at tick 150 with the instructions dialog)
	int				mTutorialBob;           // +0x1b0 hand (swapper2) y bob offset, += mTutorialBobDir every 4 ticks
	int				mTutorialBobDir;        // +0x1b4 +1/-1, flips when abs(mTutorialBob) >= 3; ctor 1
	int				mTutorialCol;           // +0x1b8 left gem column of the tutorial move (1..6); ctor -1 until found
	int				mTutorialRow;           // +0x1bc tutorial move row (4..6)
	int				mTutorialCol2;          // +0x1c0 right gem column (= mTutorialCol+1)
	Piece*			mSwapPiece1;            // +0x1c4 swap gem A (the selected one)
	Piece*			mSwapPiece2;            // +0x1c8 swap gem B (the target)
	int				mSwapAngle;             // +0x1cc swap angle in degrees, +5/tick, completes at 180
	int				mSwapCenterX;           // +0x1d0 (int)((B.x + A.x)*0.5)
	int				mSwapCenterY;           // +0x1d4 (int)((A.y + B.y)*0.5)
	int				mBarWidth;              // +0x1d8 displayed bar width 0..340, moves 1 px toward the target up to twice per tick
	int				mPlayStartTick;         // +0x1dc mUpdateCnt when play last (re)started (pause count back to 0)
	int				mPlaySeconds;           // +0x1e0 accumulated play seconds (committed on each first pause)
	int				mGameDuration;          // +0x1e4 total play seconds, snapshotted by GameOver (HighScoreWidget GAME DURATION)
	int				mIdleTicks;             // +0x1e8 ticks since the last scoring/hint/fill; > 1500 in state 1 starts the hint-button glow
	int				mScore;                 // +0x1ec real score
	double			mPointMultiplier;       // +0x1f0 per-level multiplier (normal 1.0+0.5L, timed 2.0+0.5L)
	int				mDisplayScore;          // +0x1f8 rolling score shown in the HUD; drives the bar and level/time-up checks
	int				mGemsCleared;           // +0x1fc += run length per scored run (HighScoreWidget GEMS CLEARED)
	int				mMoveMatchCount;        // +0x200 runs scored since the current swap completed (indexes gMatchBaseScores)
	int				mCascadeCount;          // +0x204 1 at swap completion, ++ per refill; selects the combo sound; longest-cascade stat
	int				mLastLandSoundTick;     // +0x208 mUpdateCnt of the last gemongem2 (played only if abs(diff) >= 10)
	int				mGameOverCountdown;     // +0x20c shake ticks left in state 5 (GameOver arg)
	int				mLongestCascade;        // +0x210 max mCascadeCount at settle (HighScoreWidget LONGEST CASCADE)
	int				mBestMoveScore;         // +0x214 max mMoveScore at settle (HighScoreWidget BEST SINGLE MOVE)
	int				mUnused218;             // +0x218 only ever zeroed (ctor, first run of a move)
	int				mMoveScore;             // +0x21c points of the current move incl. cascades, drawn as "+%d"
	int				mMoveScoreTimer;        // +0x220 "+N" display countdown = (int)clamp(mMoveScore/mult*3, 300, 1200)
	// +0x224 (padding)
	double			mMoveScoreAlpha;        // +0x228 alpha 0..1 of the "+N" text
	double			mHintGlowAlpha;         // +0x230 alpha of the over1-hint1 glow on the hint button
	int				mHintGlowTimer;         // +0x238 hint glow countdown (400 after 1500 idle ticks)
	int				mHintGlowDuration;      // +0x23c total glow duration (400); pulse period = duration/3
	bool			mNewGamePending;        // +0x240 set by RequestNewGame: after the game-over fall, NewGame() instead of the high-score screen
	int				mPauseCount;            // +0x244 pause nesting (any source); Update returns while > 0
	int				mUserPauseCount;        // +0x248 visible pause nesting; GAME_PAUSED screen while != 0, a click unpauses
	std::list<ScorePopup> mScorePopups;     // +0x24c floating point values (VC8 list: pad, _Myhead +0x250, _Mysize +0x254)
	ImageFont*		mPointsFont;            // +0x258 = app->mAlphaDigitsFont (alphadigits4), measures the popups
	ImageFont*		mScoreFont;             // +0x25c = app->mScoreFont (scorefont1), measures the score
	bool			mStatsDisabled;         // +0x260 skips longest-cascade/best-move tracking; saved, never set to true
	int				mUnused264;             // +0x264 ctor 0, saved/loaded, otherwise unused
	int				mLevelStartScore;       // +0x268 score at the start of the level (bar lower bound)
	int				mLevelTargetScore;      // +0x26c score that completes the level (= mLevelStartScore + mLevelPoints)
	int				mLevelPoints;           // +0x270 points needed for this level
	// +0x274 (unused)
	double			mTimeDrain;             // +0x278 timed mode: points drained from the bar (subtracted as (int))
	double			mTimeDrainRate;         // +0x280 drain per second (per tick: /100)
	double			mTimeDrainAccel;        // +0x288 drain-rate increase per second (per tick: /100)
	int				mBarColor;              // +0x290 index into app->mTimerBarImages (0..23 hues, 24 white); ctor 23
	int				mLastWarningTick;       // +0x294 mUpdateCnt of the last warning1
	int				mMouseX;                // +0x298 last mouse x (-1 after MouseLeave); uninitialised by the original ctor
	int				mMouseY;                // +0x29c last mouse y
	int				mMouseDownX;            // +0x2a0 drag origin x
	int				mMouseDownY;            // +0x2a4 drag origin y
	bool			mMouseDown;             // +0x2a8 button held (MouseDown .. MouseUp)
	int				mInputDelay;            // +0x2ac Unpause sets 10; MouseDown ignored while > 0
	DDImage*		mBackgroundImage;       // +0x2b0 640x480 displayed background
	DDImage*		mNextBackgroundImage;   // +0x2b4 640x480 next level's background (SetupLevel)
	bool			mBackgroundChanged;     // +0x2b8 written only (dead)
	MemoryImage*	mTextImage;             // +0x2bc banner text rendered white with overfont2
	MemoryImage*	mTextColorImage;        // +0x2c0 rainbow sine-waved copy (height + 48), rebuilt by ColorizeText each Draw
	bool			mShowingText;           // +0x2c4 banner active
	SexyString		mUnusedString;          // +0x2c8 constructed/destroyed only
	// +0x2e4 (padding)
	double			mTextPhase;             // +0x2e8 banner sine phase, +0.07/tick
	int				mTextDuration;          // +0x2f0 banner total ticks
	int				mTextTimeLeft;          // +0x2f4 banner ticks remaining
	int				mWipeY;                 // +0x2f8 level-wipe scan line, +3/tick
	int				mElectOffset;           // +0x2fc x scroll of the elect strip = Rand()%128 every 4 ticks (state 8)
	int				mBlackHoleFrame;        // +0x300 blackhole frame 0..8 (decrements, wraps to 8)
	int				mTimeDrainDelay;        // +0x304 ticks before the timed drain runs (ctor 300, 200 after a refill)
	double			mBlackHoleLevel;        // +0x308 0..1: vortex image alpha and whirlpool volume
	bool			mLevelTextShown;        // +0x310 set when the LEVEL banner is shown; saved, never read
	// +0x311..+0x317 (padding, sizeof 0x318)

	// port: pieces dropped from the grid without being deleted. FillBoard abandons the old board and a
	// swap with a stale selection overwrites a grid cell; the original leaks those pieces, which must
	// stay valid because mSelectedPiece can still point at one. Owned here and freed in ~Board.
	std::vector<Piece*> mOrphanedPieces;

public:
	static int		GetColX(int theCol);                            // 0x4c9b25 theCol*51 + 203 (thiscall in the binary, this unused)
	static int		GetRowY(int theRow);                            // 0x4c9b37 theRow*51 + 16
	void			GameOver(int theShakeTicks);                    // 0x4c9b47
	void			RequestNewGame();                               // 0x4c9bab GameOver(50) + mNewGamePending
	bool			IsGameInProgress();                             // 0x4c9bbe !mIsNewGame && state != 5 && state != 6
	void			Pause(bool theUserPause);                       // 0x4c9bda
	void			Unpause(bool theUserPause);                     // 0x4c9c2d
	Piece*			GetPieceAt(int theX, int theY);                 // 0x4c9c7c
	bool			HasMatch();                                     // 0x4c9cf5
	static int		GetPanForX(int theX);                           // 0x4c9d94
	bool			FindMove(int* theMove, int theMoveNum, bool horizontal, bool vertical); // 0x4c9dda theMove = {col,row,col2,row2} or NULL
	virtual void	MouseMove(int x, int y);                        // 0x4c9fd0 Widget slot 52
	virtual void	MouseLeave();                                   // 0x4c9fe9 Widget slot 51
	void			StartSwap(Piece* thePiece1, Piece* thePiece2);  // 0x4c9ff8
	static void		GetBoardSize(int* theWidth, int* theHeight);    // 0x4ca13e (406, 406)
	static void		GetBoardCenter(int* theX, int* theY);           // 0x4ca179 (406, 219)
	void			ShowText(const SexyString& theText, int theDuration); // 0x4ca1c8
	void			ColorizeText();                                 // 0x4ca36e
	void			StartBlackHole();                               // 0x4ca58d
	virtual void	MouseDown(int x, int y, int theClickCount);     // 0x4ca6b0 Widget slot 54
	virtual void	MouseUp(int x, int y, int theClickCount);       // 0x4ca8b7 Widget slot 56
	virtual void	MouseDrag(int x, int y);                        // 0x4ca8c1 Widget slot 58
	virtual void	KeyChar(SexyChar theChar);                      // 0x4ca9d3 Widget slot 47
	void			UpdateClearing();                               // 0x4ca9ed state 4
	void			UpdateGameOver();                               // 0x4cab82 state 5
	void			DoHint();                                       // 0x4cad2f
	void			SetupLevel();                                   // 0x4cae29 background + diff.txt level parameters
	void			FillBoard();                                    // 0x4cb182
	void			UpdateLevelWipe();                              // 0x4cb284 state 8
	void			UpdateFallOut();                                // 0x4cb3bf state 9
	void			ShowHint();                                     // 0x4cb475 button 15

	Board(WinDMApp* theApp, bool timedMode);                        // 0x4cb5bc
	void			UpdateBlackHole();                              // 0x4cb931 state 7
	virtual void	Draw(Graphics* g);                              // 0x4cbc70 Widget slot 26
	bool			SaveGameExists();                               // 0x4cca37
	bool			LoadGame();                                     // 0x4cca98
	bool			SaveGame();                                     // 0x4ccdfe
	void			SaveGameSnapshot(Buffer* theBuffer);            // port: what SaveGame() would write, live game untouched
	void			WriteSaveGame(Buffer* theBuffer, int thePlaySeconds, int theState); // port: the body of 0x4ccdfe
	static std::string GetSaveGameFileName();                       // port: "userdata\\savegame.dat", scoped for gameplay mods
	bool			DeleteSaveGame();                               // 0x4cd130
	virtual ~Board();                                               // 0x4cd1ba (scalar deleting dtor 0x4ce430)
	bool			ProcessMatches();                               // 0x4cd287
	void			UpdateFalling();                                // 0x4cd687 state 0
	void			UpdateSwap();                                   // 0x4cd84f states 2, 3
	virtual void	Update();                                       // 0x4cdabc Widget slot 22

	using Widget::MouseDown;
	using Widget::MouseUp;
};

}

#endif
