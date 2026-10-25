#ifndef __BIGMONEY_BOARD_H__
#define __BIGMONEY_BOARD_H__

// Board.cpp: the 14x15 coin field of Big Money Deluxe - rules, per-tick update, input, scoring, the
// money meter, level flow and savegame.dat. The drawing half (Draw, DrawPiece, DrawStatsPanel,
// GetBagSlotPos, ShowContinueButton and their helpers) is in BoardDraw.cpp (board_draw).
// Original: class Sexy::Board : public Widget, public ButtonListener, sizeof 0x810 (WinBMApp::NewGame
// 0x41bfb0 does new(0x810)), vftables 0x4f54e4 (Widget, 54 slots) and 0x4f54c4 (ButtonListener at
// +0x80). Board data starts at +0x84; offsets in the comments are absolute (the port's base classes
// have other sizes, nothing depends on them).
// Behaviour spec: re/bigmoney/analysis/board_logic.md (logic) and board_draw.md (drawing).

#include "Widget.h"
#include "ButtonListener.h"
#include "Color.h"
#include "Point.h"
#include "MTRand.h"
#include "ModRules.h"
#include "Piece.h"
#include "Res.h"
#include "PuzzleSolver.h"
#include <list>
#include <string>
#include <vector>

namespace Sexy
{

class WinBMApp;
class Graphics;
class MemoryImage;
class SoundInstance;
class DialogButton;
class HighScoreWidget;
class Points;
class WidgetManager;
class Buffer;
class ReplayRecorder;
class ReplayPlayer;

// Board::mState (+0xa0)
enum BoardState
{
	STATE_PLAYING = 0,          // input accepted; minting, danger, bag / bomb drops
	STATE_LEVEL_DONE = 1,       // board empty, stats panel, "CLICK HERE TO PLAY NEXT LEVEL!" button
	STATE_LEVEL_COMPLETE = 2,   // fanfare, bills, level-up sign; the next row keeps rising; 160 ticks
	STATE_PUZZLE_SWEEP = 3,     // Puzzle: one row cleared every 16 ticks for 3 points a coin
	STATE_LEVEL_CLEAR_FALL = 4, // every piece falls off; the sign settles at y 16, then state 1
	STATE_GAME_OVER = 5,        // coins flung (after mFlingDelay); waits for mLoosePieces to empty
	STATE_GAME_OVER_SCORES = 6, // HighScoreWidget slides in, "CLICK HERE TO PLAY AGAIN"
	STATE_BOMB = 7,             // dollar bomb: a coin of mBombColor explodes every 8 ticks; physics frozen
	STATE_FROZEN = 8            // the GameSelector is over a running game: Update returns at once
};

// Board::mHintId (+0x7f0): the instructor arrow/text shown by ShowHint / ShowHintAt
enum HintId
{
	HINT_METER = 1,             // ShowHintAt (610, 0), follows the meter top
	HINT_DANGER = 2,            // ShowHintAt (380, 8)
	HINT_BOMB = 3,
	HINT_BAG = 5,               // on the coin under a landed bag (sets mHintAlt)
	HINT_CLICK_GROUP = 6,
	HINT_CLICK_GROUP_PUZZLE = 7,
	HINT_PUZZLE_BAG = 8
};

// Coin colours (Piece::mColor) as r, g, b. In the original these are Color[5] built by the dynamic
// initialisers 0x4f3220 / 0x4f32a0 (alpha 255).
inline constexpr int gCoinColors[Piece::NUM_COIN_COLORS][3] =      // @0x52d8c8, dollar-bomb tint
{
	{255, 0, 0}, {0, 148, 255}, {255, 255, 0}, {0, 255, 0}, {255, 0, 255}
};
inline constexpr int gCoinLightColors[Piece::NUM_COIN_COLORS][3] = // @0x52d878, selection glow tint
{
	{255, 128, 128}, {128, 128, 255}, {240, 240, 64}, {87, 255, 87}, {240, 64, 240}
};

// Rank thresholds (@0x4f53c0): rank = highest i with gRankThresholds[i] <= score (names gRankNames)
enum { NUM_RANKS = 27 };
inline constexpr int gRankThresholds[NUM_RANKS] =
{
	0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 11500, 13000, 15000, 20000,
	30000, 40000, 50000, 60000, 70000, 80000, 100000, 125000, 150000, 200000, 300000, 500000
};

// OnClick base points of a group of n coins, n = 2..15 (a stack table in 0x40b120); n >= 16 scores
// (n-2)*25 - 145
inline constexpr int gGroupBaseScores[14] = { 5, 10, 15, 20, 30, 40, 50, 65, 80, 95, 115, 135, 155, 180 };

// @0x52d864: GetStringVector("gRankNames"), refilled by every Board::Board (27 rank names)
extern std::vector<std::string> gRankNames;

// mParticles element (std::list node 0x40; offsets relative to the payload)
struct Particle
{
	int		mFrameDelay;     // +0x00 mFrame advances when mUpdateCnt % mFrameDelay == 0
	// +0x04 (padding)
	double	mX;              // +0x08
	double	mY;              // +0x10
	double	mVX;             // +0x18
	double	mVY;             // +0x20
	double	mGravity;        // +0x28 added to mVY every tick
	int		mFrame;          // +0x30 0..13; erased when it reaches 14
	bool	mBright;         // +0x34 1: flight / burst sparkles, 0: bag / drop trails
};

// mBills element (node 0x28)
struct Bill
{
	double	mX;              // +0x00
	double	mY;              // +0x08
	double	mVY;             // +0x10
	int		mFrame;          // +0x18 0..15, +1 every 4 ticks
	int		mType;           // +0x1c 0..4 (Draw: bill scale)
};

// mTubeCoins element (node 0x20)
struct TubeCoin
{
	double	mY;              // +0x00 32 at launch, +15 per tick
	double	mDist;           // +0x08 distance to the stack top, -15 per tick; lands when < 0
	int		mFrame;          // +0x10 (int)(mDist*9*(1/450))
};

class Board : public Widget, public ButtonListener
{
public:
	enum
	{
		BOARD_COLS = 14,
		BOARD_ROWS = 15,
		COIN_SIZE = 29,             // cell pitch and hit box
		BOARD_X = 177,              // ColX(0)
		BOARD_Y = 3,                // RowY(0)
		METER_UNITS = 64,           // a full money meter
		METER_TARGET_X = 611,       // where combo coins fly
		METER_TARGET_Y = 25,
		SAVEGAME_VERSION = 0
	};

	// +0x080 ButtonListener vfptr
	WinBMApp*		mApp;                   // +0x084 ctor arg
	MTRand			mRand;                  // +0x088 port: an MTRand* (new(0x9c4)) in the original; the board's own RNG
	unsigned long	mCreateTick;            // +0x08c GetTickCount() in the ctor; never read
	unsigned long	mTimerTick;             // +0x090 start of the current timed stretch (ctor, unpause, next level)
	int				mStatsSeconds;          // +0x094 game seconds snapshot for the stats panel (ShowContinueButton)
	int				mPlaySeconds;           // +0x098 game seconds banked at pause and at level end
	int				mMinGroup;              // +0x09c smallest clickable group: 3, Puzzle 2
	int				mState;                 // +0x0a0 BoardState
	bool			mPlayStarted;           // +0x0a4 a valid click was made (or a game was loaded): CanSaveGame
	Piece*			mGrid[BOARD_ROWS][BOARD_COLS]; // +0x0a8 [row][col], row 0 = top, NULL = empty
	Piece*			mNextRow[BOARD_COLS];   // +0x3f0 coins being minted below row 14
	int				mRiseOffset;            // +0x428 UpdatePhysics floor of the bottom row is RowY(14) + this: +29 per push, -1 per tick, so the pieces (which keep their pixel y in PushRow) rise smoothly. Draw never reads it
	bool			mHasMoves;              // +0x42c a group >= mMinGroup exists (FindGroups)
	int				mCols;                  // +0x430 14
	int				mRows;                  // +0x434 15
	int				mMintPixels;            // +0x438 (int)((1-mMint)*29): px of the next row still hidden
	double			mMint;                  // +0x440 next-row progress; > 1 pushes a row
	double			mMintTarget;            // +0x448 mMint eases up to it at 0.015/tick; 0.25 at level start
	double			mMintSpeed;             // +0x450 Action mint rate
	double			mMintAccel;             // +0x458 Action: added to mMintSpeed each mint step
	double			mMintPerClick;          // +0x460 Strategy: added to mMintTarget per click
	Color			mMovesColor;            // +0x468 Strategy moves-left lamp colour (eased to red/yellow/green)
	int				mLastPieceHitTick;      // +0x478 throttles SOUND_PIECEHIT
	int				mLastBagTick;           // +0x47c Speed Bagger timing
	int				mLastColor;             // +0x480 colour combo
	int				mColorStreak;           // +0x484
	int				mLastGroupSize;         // +0x488 previous click's group size
	int				mPrevGroupSize;         // +0x48c the one before
	int				mScore;                 // +0x490
	int				mDisplayScore;          // +0x494 rolls toward mScore
	double			mPointsMult;            // +0x498 score multiplier of the level
	std::list<Points*> mPointsWidgets;      // +0x4a0 live floating texts (VC7 list: head +0x4a4, size +0x4a8)
	Points*			mLastBonusText;         // +0x4ac stacking of bonus texts
	int				mMouseX;                // +0x4b0
	int				mMouseY;                // +0x4b4
	int				mLastWarningTick;       // +0x4b8 SOUND_WARNING throttle
	int				mMeterFlash;            // +0x4bc set to 16 by AddPoints, never read
	double			mMeterJustFull;         // +0x4c0 set to 1.0 at a bag drop, never read
	double			mMeterFraction;         // +0x4c8 sum of points/mMeterCapacity since the last bag (tone pitch)
	double			mMeterTarget;           // +0x4d0 earned meter units (64 per meter)
	int				mMeterCoins;            // +0x4d8 tube coins landed
	double			mMeterDrained;          // +0x4e0 units consumed (drain, bag drops, level end)
	int				mMeterPending;          // +0x4e8 tube coins still to launch
	double			mMeterLaunchTimer;      // +0x4f0 launch spacing
	double			mMeterEmptying;         // +0x4f8 units still to drain after a bag drop
	double			mMeterCapacity;         // +0x500 points per full meter
	double			mMeterDrainRate;        // +0x508 Action drain (x64 units per 2 ticks)
	std::list<Piece*> mLoosePieces;         // +0x510 pieces outside the grid (head +0x514, size +0x518)
	std::list<Particle> mParticles;         // +0x51c sparkles (head +0x520, size +0x524)
	std::list<Bill>	mBills;                 // +0x528 falling bills (head +0x52c, size +0x530)
	std::list<TubeCoin> mTubeCoins;         // +0x534 coins dropping into the meter tube (head +0x538, size +0x53c)
	int				mBillsToSpawn;          // +0x540 one bill per tick while > 0; never set > 0 (dead)
	bool			mDangerFlash;           // +0x544 danger flash phase
	int				mMeterLevel;            // +0x548 Action: (int)min(6*fill, 5)
	int				mBagsReleased;          // +0x54c bags that left the board (holder slot index)
	int				mBagsBanked;            // +0x550 bags arrived in the holder
	int				mMeterFlashFrame;       // +0x554 0..3
	int				mMeterFlashTimer;       // +0x558 160 at a bag drop
	int				mBagsNeeded;            // +0x55c
	bool			mLevelUpActive;         // +0x560 "level up" sign shown
	double			mLevelUpY;              // +0x568 sign y
	double			mLevelUpAngle;          // +0x570 atan2(rightY - leftY, levelup width)
	double			mLevelUpUnused;         // +0x578 0.03, never read
	double			mLevelUpStretch;        // +0x580 189 -> 150 in state 4 (Draw)
	bool			mLevelUpBouncing;       // +0x588
	double			mLevelUpLeftY;          // +0x590
	double			mLevelUpLeftVY;         // +0x598
	double			mLevelUpRightY;         // +0x5a0
	double			mLevelUpRightVY;        // +0x5a8
	int				mMintDelay;             // +0x5b0 Action minting paused while > 0
	bool			mPiecesMoving;          // +0x5b4 a grid piece is still falling (state 4: still on screen)
	int				mLevel;                 // +0x5b8 0-based level
	int				mLevelEndDelay;         // +0x5bc state 2 duration (160)
	int				mPauseCount;            // +0x5c0 nested pauses
	double			mPauseFade;             // +0x5c8 pause overlay 0..1
	int				mPauseAnim;             // +0x5d0 counter while the pause overlay shows
	bool			mPauseClickable;        // +0x5d4 no dialog is open ("click here to continue")
	int				mNoMovesY;              // +0x5d8 state 6: +5/tick to 500 (Draw: IMAGE_NOMOREMOVES)
	int				mGoTimer;               // +0x5dc ticks to "Go!" (140 first level, 80 later)
	bool			mBannerVisible;         // +0x5e0 "collect N money bags" banner blink phase
	int				mBannerTimer;           // +0x5e4 banner ticks left (480 level 0, else 384)
	int				mColWarning[BOARD_COLS]; // +0x5e8 per-column danger arrow frame 0..3 or -1
	int				mOverflowTicks;         // +0x620 Action overflow countdown (300 = game over)
	int				mFlingDelay;            // +0x624 game over: ticks until FlingAllCoins
	bool			mCountdownActive;       // +0x628
	double			mCountdownValue;        // +0x630 0..2.99 (Draw: digit and zoom)
	int				mGameOverRiseDelay;     // +0x638 100
	DialogButton*	mContinueButton;        // +0x63c "click here to play next level / again"
	int				mButtonBlink;           // +0x640 0..299
	int				mLevelCoinsCleared;     // +0x644
	int				mGameCoinsCleared;      // +0x648
	int				mLevelBiggestBonus;     // +0x64c biggest single AddPoints
	int				mGameBiggestBonus;      // +0x650
	int				mLevelBiggestCombo;     // +0x654 biggest group
	int				mGameBiggestCombo;      // +0x658
	double			mColTowerHeight[BOARD_COLS]; // +0x660 smoothed tower heights (Tower Toppler)
	double			mSpeedBonus;            // +0x6d0 +0.16 per Action click, -0.0025 per tick
	double			mMintSpeedBase;         // +0x6d8 0.0025   (tuning constants: SetupLevelParams uses L*k + c)
	double			mMintSpeedPerLevel;     // +0x6e0 0.001
	double			mMintAccelBase;         // +0x6e8 1e-6
	double			mMintAccelPerLevel;     // +0x6f0 5e-8
	double			mMeterCapBase;          // +0x6f8 300
	double			mMeterCapPerLevel;      // +0x700 20
	double			mDrainBase;             // +0x708 0
	double			mDrainPerLevel;         // +0x710 7e-5
	double			mPointsMultBase;        // +0x718 0.5
	double			mPointsMultPerLevel;    // +0x720 0.1
	double			mBagsBase;              // +0x728 2.6
	double			mBagsPerLevel;          // +0x730 0.26
	double			mPuzzleBagsBase;        // +0x738 2.6
	double			mPuzzleBagsPerLevel;    // +0x740 0.26
	double			mPerClickBase;          // +0x748 0.24501
	double			mPerClickPerLevel;      // +0x750 0.011
	// +0x758..+0x767 never touched
	int				mBombColor;             // +0x768 colour being detonated
	int				mBombIdle;              // +0x76c state 7 linger counter
	bool			mHoverBag;              // +0x770 the mouse is over a money bag
	int				mRepaintTimer;          // +0x774 counts down, repaints at 0; only ever set to 0 (dead)
	// +0x778 std::list (8-byte nodes; head +0x77c, size +0x780): never used, omitted
	int				mNumColors;             // +0x784 coin colours this level (not saved)
	int				mMovesLeft;             // +0x788 Strategy: clicks until the next push
	bool			mMoveDigitActive;       // +0x78c Strategy "3/2/1" digit
	int				mMoveDigit;             // +0x790 3 - mMovesLeft (0..2)
	double			mMoveDigitFade;         // +0x798 1.0 -> 0 (-0.01 per tick)
	int				mClipExtra;             // +0x7a0 0 (Draw only)
	int				mUnused7a4;             // +0x7a4 ctor 0
	SoundInstance*	mExplodeSound;          // +0x7a8 GetSoundInstance(SOUND_EXPLODE)
	bool			mGlintActive;           // +0x7ac random glint: written by Update only, Draw never reads it (dead)
	int				mGlintFrame;            // +0x7b0 (dead, as mGlintActive)
	int				mGlintTick;             // +0x7b4 (dead, as mGlintActive)
	int				mSweepTone;             // +0x7b8 Puzzle sweep tone index
	// The three Draw caches are NULL from the ctor, made by Draw, and deleted only by ShowContinueButton,
	// LoadGame and ~Board
	MemoryImage*	mStatsImage;            // +0x7bc Draw cache (396x263)
	MemoryImage*	mStatsFadeImage;        // +0x7c0 Draw cache
	bool			mStatsDirty;            // +0x7c4 Draw rebuilds mStatsFadeImage; set with every mStatsFade change
	int				mHighScoreDelay;        // +0x7c8 state 6: 180 ticks before the widget slides
	int				mHighScoreSlide;        // +0x7cc 0 -> -146 (-2 per tick)
	double			mStatsFade;             // +0x7d0 0..1 (+0.015 per tick)
	MemoryImage*	mLevelUpImage;          // +0x7d8 Draw cache (see mStatsImage)
	HighScoreWidget* mHighScoreWidget;      // +0x7dc game-over high scores (state 6)
	double			mBagGlow;               // +0x7e0 0..1 phase, +0.02 per tick while a bag is in the grid
	bool			mHintVisible;           // +0x7e8
	bool			mHintAlt;               // +0x7e9 set with HINT_BAG
	int				mHintTimer;             // +0x7ec ticks left (0 = never times out)
	int				mHintId;                // +0x7f0 HintId
	int				mHintX;                 // +0x7f4
	int				mHintY;                 // +0x7f8
	Piece*			mHintPiece;             // +0x7fc piece the hint follows
	bool			mHintFollowMeter;       // +0x800 HINT_METER tracks the meter top
	int				mBagFlashCount;         // +0x804 3 pulses per banked bag
	double			mBagFlash;              // +0x808 0..1 pulse phase (sizeof 0x810)

	// port(mods): not in the original (ModRules.h)
	BoardTunables	mTunables;              // the mods' tunables that are not original members
	uint32_t		mGameSeed;              // the seed the player chose for this game, 0 for none
	bool			mUserSeeded;            // mGameSeed != 0: the board plays on it, with its own high-score tables
	MTRand			mEffectsRand;           // a seeded game's effects (EffectsRand)
	// port(mods): replays (Replay.h)
	uint32_t		mRngSeed;               // the seed mRand started from (a replay's start)
	ReplayRecorder*	mRecorder;              // this game is being recorded (the "replays" feature), else NULL
	ReplayPlayer*	mPlayer;                // a playback board: it plays this replay and ignores the player's input
	std::string		mReplayPath;            // the replay this game was written to at its game over ("" if none)
	// port(mods): LiveSplit (LiveSplitObserver.h)
	uint32_t		mBoardSerial;           // 1, 2, ... for each board the app makes
	bool			mResumed;               // LoadGame restored a saved game into this board
	// port(mods): the Puzzle solver (PuzzleSolver.h; the "puzzleSolver" feature)
	enum
	{
		SOLVER_SHOW_ALWAYS = 0,         // the status line says what the solver found
		SOLVER_SHOW_LOST = 1,           // only that the level can't be won
		SOLVER_SHOW_NEVER = 2,
		SOLVER_PRICE_FREE = 0,
		SOLVER_PRICE_PERCENT = 1,       // mSolverHintValue percent of the score, rounded up
		SOLVER_PRICE_AMOUNT = 2,        // mSolverHintValue dollars
		SOLVER_MESSAGE_TICKS = 300,
		SOLVER_WARN_OFF = 0,
		SOLVER_WARN_SHOW = 1,           // the hovered group is red when its click loses the level
		SOLVER_WARN_BLOCK = 2,          // and that click does nothing (while another click can still win)
		SOLVER_LINE_STATUS = 0,         // the status lines (GetSolverStatusRect)
		SOLVER_LINE_WARNING = 1,
		SOLVER_LINE_HINT = 2,           // how to get a hint, or the answer to the last hint or undo asked for
		SOLVER_LINE_UNDO = 3
	};
	struct SolverLine
	{
		SexyString	mText;
		int			mKind;                  // SOLVER_LINE_*
	};
	// The game just before a click, for the solver's undo: the savegame's bytes and what it leaves out
	struct SolverUndo
	{
		std::vector<unsigned char> mSave;   // WriteSaveGame
		MTRand		mRand;                  // whole (the savegame keeps the draw index out)
		int			mLastColor;
		int			mColorStreak;
		int			mLastGroupSize;
		int			mPrevGroupSize;
		double		mColTowerHeight[BOARD_COLS];
		int			mScore;
		int			mSpent;                 // mSolverSpent then
		uint64_t	mBoard;                 // the board's PuzzleModel::Hash, 0 if it was moving
		int			mTick;
		int			mPickpockets;
	};
	PuzzleSolver*	mSolver;                // a Puzzle board with the feature on, else NULL
	int				mSolverShow;            // SOLVER_SHOW_*
	bool			mSolverEndWhenLost;     // the game is over as soon as the level can't be won
	bool			mSolverHints;           // H (or a click on the status line) shows a winning click
	int				mSolverHintPrice;       // SOLVER_PRICE_*
	int				mSolverHintValue;
	int				mSolverBudget;          // positions the solver looks at per board tick
	bool			mSolverHintWanted;      // a hint was asked for; given once the board is still and solved
	uint64_t		mSolverHintBoard;       // the board (PuzzleModel::Hash) the hint shown is for, 0 for none
	bool			mSolverHint[BOARD_ROWS][BOARD_COLS]; // the hinted group
	int				mSolverLogged;          // the status last logged
	int				mSolverLostLevel;       // the level the solver found lost (a test: it must not be won)
	SexyString		mSolverMessage;         // an answer to a hint or undo request on the status line
	int				mSolverMessageTicks;
	int				mSolverFreeHints;       // hints free in each level
	bool			mSolverUndo;            // U (or a click on the undo line) takes back the last click
	int				mSolverUndoPrice;       // SOLVER_PRICE_*
	int				mSolverUndoValue;
	int				mSolverUndoLimit;       // undos in each level
	int				mSolverWarn;            // SOLVER_WARN_*
	int				mSolverLevel;           // the level the counts below are for
	int				mSolverHintsTaken;      // hints given in this level
	int				mSolverUndosTaken;      // undos in this level
	int				mSolverSpent;           // dollars paid for hints and undos in this game
	std::vector<SolverUndo> mSolverUndos;   // this level's last clicks, the latest last
	PuzzleClickCheck* mSolverCheck;         // mSolverWarn: checks the hovered group's click, else NULL
	uint64_t		mSolverVerdictBoard;    // the board mSolverVerdict is for
	int8_t			mSolverVerdict[BOARD_ROWS * BOARD_COLS]; // a group's (by its first cell) PuzzleClickCheck::Verdict
	int				mSolverCheckGroup;      // the group mSolverCheck is checking, -1 for none
	bool			mSolverWarnCell[BOARD_ROWS][BOARD_COLS]; // the hovered group, when its click loses
	bool			mSolverWarning;         // mSolverWarnCell has a group
	int				mSolverExpect;          // a test: the verdict of the last click's group (VERDICT_NONE: none)
	uint64_t		mSolverExpectBoard;     // the board it was clicked on
	// port(mods): Puzzle boards built to be winnable (PuzzleBuilder.h; the "winnablePuzzles" feature)
	bool			mWinnablePuzzles;
	std::vector<PuzzleSolver::Move> mBuildPlan; // the clicks that win this level's board as built (until the solver has them)
	uint64_t		mBuildPlanBoard;        // that board, bags and all (PuzzleModel::Hash)
	// port(mods): Pickpockets (the "pickpocket" feature): each lifts a group too small to click, for no points
	bool			mPickpocket;            // the feature is on
	int				mPickpockets;           // in hand (saved in the savegame's trailer)
	int				mPickpocketsPerLevel;   // more at each new level
	int				mPickpocketsMax;        // the most in hand
	bool			mPickpocketArmed;       // P (or a click on the counter): the next click on a small group uses one
	int				mPickpocketsUsed;       // in this board's game (the log)

public:
	Board(WinBMApp* theApp);                                        // 0x40e760
	virtual ~Board();                                               // 0x403f10 (scalar deleting dtor 0x4045a0, slot 0)

	// Widget
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override; // 0x403aa0 slot 8
	virtual void	Update() override;                                       // 0x40c9f0 slot 9
	virtual void	Draw(Graphics* g) override;                              // 0x405480 slot 13 (BoardDraw.cpp)
	virtual void	OrderInManagerChanged() override;                        // 0x401460 slot 15
	virtual void	KeyChar(SexyChar theChar) override;                      // 0x403050 slot 31
	virtual void	MouseLeave() override;                                   // 0x401fa0 slot 35
	virtual void	MouseMove(int x, int y) override;                        // 0x401fb0 slot 36
	virtual void	MouseDown(int x, int y, int theClickCount) override;     // 0x40c5f0 slot 38

	// ButtonListener (+0x80)
	virtual void	ButtonPress(int theId) override;                         // 0x401c70 slot 1
	virtual void	ButtonDepress(int theId) override;                       // 0x40e580 slot 2

	// Geometry and grid
	static int		ColX(int theCol);                               // 0x4015e0 29*c + 177
	static int		RowY(int theRow);                               // 0x4015f0 29*r + 3
	void			ResetMarks();                                   // 0x401600
	void			SetDangerFlash(bool on);                        // 0x401660
	void			FloodFill(int theCol, int theRow, int theMark, bool sameColor, int* theCount, int* theSum); // 0x401720
	void			FindGroups();                                   // 0x401840
	Piece*			PieceAt(int x, int y);                          // 0x401960
	int				CountCoinColumns();                             // 0x401a90
	int				GetTopCoinRow();                                // 0x401af0
	double			GetAverageCoinRow();                            // 0x401b40
	void			UpdateMovesLeft();                              // 0x401bd0 (Strategy)
	void			UpdateHover();                                  // 0x401e90

	// Instructor hints
	void			ShowHintAt(int theId, const Point& thePos, int theDuration); // 0x401a20 (skill 1 only)
	void			HideHint();                                     // 0x401a80
	void			ShowHint(int theId, Piece* thePiece, int theDuration); // 0x401fe0 (app->mShowInstructor only)

	// Level and game flow
	void			SetupLevelParams();                             // 0x401490
	void			GenerateBoard();                                // 0x40a110
	void			NextLevel();                                    // 0x40ae20
	void			StartNextLevel();                               // 0x40e4b0
	void			LevelComplete();                                // 0x4083d0
	void			ClearBoardForLevelEnd();                        // 0x408650
	void			GameOver();                                     // 0x401d30
	void			FlingAllCoins();                                // 0x408220
	void			Pause(bool pause);                              // 0x402930
	void			RemovePointsWidgets();                          // 0x403a20
	void			Freeze();                                       // 0x403b80 (state 8, behind the GameSelector)
	bool			CanSaveGame();                                  // 0x4010d0

	// Per-tick parts
	void			UpdateLoosePieces();                            // 0x409520
	void			UpdateMeter();                                  // 0x40c640
	void			UpdatePhysics();                                // 0x408a60
	void			Compact();                                      // 0x402b70
	void			PushRow();                                      // 0x4088a0
	void			DropSpecial(int theType);                       // 0x40aea0
	bool			TowerToppler();                                 // 0x409e00

	// Input and scoring
	void			ClickAt(int x, int y);                          // 0x40c5c0
	void			OnClick(Piece* thePiece);                       // 0x40b120
	void			AddPoints(int thePoints, int x, int y, bool toMeter); // 0x407ea0
	void			ShowBonusText(const SexyString& theText);       // 0x408110

	// Drawing (BoardDraw.cpp, board_draw)
	void			DrawPiece(Graphics* g, Piece* thePiece, int theYOffset); // 0x402060 (3 args: 0, or mMintPixels for mNextRow)
	void			DrawStatsPanel(Graphics* g);                    // 0x404950 (into mStatsImage)
	void			GetBagSlotPos(Point* thePos, int theIndex);     // 0x401e30 (also UpdatePhysics)
	void			ShowContinueButton();                           // 0x4045e0 (LoadGame, UpdateLoosePieces, Update)

	// savegame.dat
	bool			SaveGame();                                     // 0x403300 (the only writer; from ~WinBMApp)
	bool			LoadGame();                                     // 0x4075d0
	// port (web persistence): the bytes SaveGame would write now, without touching the game
	void			SaveGameSnapshot(Buffer* theBuffer);
	void			WriteSaveGame(Buffer* theBuffer);               // port: the body of 0x403300
	bool			ReadSaveGame(Buffer* theBuffer, bool theTrailer); // port: the body of 0x4075d0

	// port(mods): tunables and the set seed (ModRules.h)
	void			LoadTunables();
	unsigned long	EffectsRand();
	int				BagColumnRand();
	void			UsePendingSeed();
	// port(mods): replays (Replay.h): input to record, and the game-over screen
	void			RecordInput(int theOp, int theX = 0, int theY = 0);
	void			ReplayGameOver();
	// port(mods): the Puzzle solver (PuzzleSolver.h)
	void			UpdateSolver();
	void			RequestSolverHint();
	void			GiveSolverHint();
	void			ClearSolverHint();
	int				SolverPrice(int thePrice, int theValue, int theScore) const;
	int				SolverHintCost() const;
	int				SolverUndoCost() const;
	bool			IsSolverHintPiece(Piece* thePiece) const;
	bool			IsSolverWarnPiece(Piece* thePiece) const;
	void			StartSolverLevel();
	void			GetSolverModel(PuzzleModel* theModel);
	int				SolverRowOffset() const { return BOARD_ROWS - mRows; } // the board's row 0 in the solver's model
	int				SolverGroupKey(int theCol, int theRow) const;
	void			UpdateSolverCheck();
	bool			CheckSolverClick(Piece* thePiece);
	void			SaveSolverUndo();
	void			RequestSolverUndo();
	bool			GetSolverStatusRect(Rect* theRect, std::vector<SolverLine>* theLines);
	void			BuildWinnablePuzzle();
	void			CheckWinnablePuzzle();
	void			DrawSolverStatus(Graphics* g);
	// port(mods): Pickpockets
	void			GetPickpocketGroup(Piece* thePiece, std::vector<Piece*>* theGroup);
	bool			IsPickpocketTarget(Piece* thePiece);
	bool			CanPickpocket();
	void			ArmPickpocket(bool theArmed);
	void			UsePickpocket(int x, int y);
	bool			GetPickpocketRect(Rect* theRect, SexyString* theText);
	void			DrawPickpockets(Graphics* g);
	static std::string GetSaveGameFileName();                      // port: "userdata\\savegame.dat"

	using Widget::MouseDown;
	using ButtonListener::ButtonPress;
};

// Free functions of Board.cpp (WinBMApp calls them)
bool	SaveGameExists();                                           // 0x4010f0 fopen("savegame.dat", "rb")
void	DeleteSaveGame();                                           // 0x401440 DeleteFileA("savegame.dat")

}

#endif
