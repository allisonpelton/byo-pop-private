#ifndef __BOARD_H__
#define __BOARD_H__

// Board.cpp: the 8x8 playfield, rules, scoring, animations and savegame.
// Original: class Sexy::Board : public Widget, sizeof 0x318 (WinDMApp::NewGame does new(0x318)),
// vftable 0x5065cc. Offsets in the comments are absolute from the object start (Widget is 0x88
// bytes, so Board data starts at +0x88; Ghidra's Board_data.offset_N is absolute N+0x88).
// Behaviour spec: games/bejeweled/docs/GAME_NOTES.md (Board section).

#include "GameStats.h"
#include "Widget.h"
#include "MTRand.h"
#include "Piece.h"
#include "GameRules.h"
#include "BoardLayout.h"
#include "Buffer.h"
#include "Replay.h"
#include "MatchFinder.h"
#include "MoveSet.h"
#include "Specials.h"
#include <list>
#include <vector>

namespace Sexy
{

class WinDMApp;
class Graphics;
class DDImage;
class Image;
class MemoryImage;
class ImageFont;
class SoundInstance;
class ReplayRecorder;

// Board::mState (+0x194)
enum BoardState
{
	STATE_FALLING = 0,          // gems drop into place; when settled: FindSets / no-moves check (UpdateFalling)
	STATE_PLAYING = 1,          // waiting for input
	STATE_SWAPPING = 2,         // swap animation (UpdateSwapping)
	STATE_SWAPPING_BACK = 3,    // illegal swap returning (UpdateSwapping)
	STATE_CLEARING = 4,         // matched gems shrink, then the columns refill (UpdateClearing)
	STATE_GAME_OVER = 5,        // shake for mGameOverCountdown ticks, then explode and fall (UpdateGameOverAnim)
	STATE_GAME_OVER_DONE = 6,   // HighScoreWidget shown; nothing updates
	STATE_BLACK_HOLE = 7,       // level complete: gems spiral into the vortex (UpdateWhirlpool)
	STATE_LEVEL_WIPE = 8,       // new background wiped in 3 px/tick, then NewBoard (UpdateLevelTransition)
	STATE_FALL_OUT = 9,         // timed mode, no moves: the board falls out and is refilled (UpdateBoardDropping)
	// port(variant): the move rules' twists and slides (stage 10, BoardMoves.cpp), never under the original rules
	STATE_TWISTING = 10,        // a 2x2 block turns (UpdateTwist)
	STATE_TWISTING_BACK = 11,   // a twist that made no group turns back
	STATE_SLIDING = 12,         // a line's gems slide along it (UpdateSlide)
	STATE_SLIDING_BACK = 13,    // a slide that made no group slides back
	// port(variant): the arrow gems (stage 18, BoardArrows.cpp), never under the original rules
	STATE_ARROW_STEPPING = 14   // at a move's settle the arrows step a cell and trade places (UpdateArrowSteps)
};

// Element of Board::mScorePopups (0x14 bytes; std::list node 0x1c).
struct ScorePopup
{
	int		mX;              // +0x00 center x = average run x + 24
	int		mY;              // +0x04 y = average run y + 16; -1 every 3 ticks while > ascent/2 + 2
	int		mLife;           // +0x08 110 at creation, -1/tick, erased at <= 0; alpha = min(255, mLife*255/18)
	int		mAlwaysMinusOne; // +0x0c -1 at creation, never read
	int		mPoints;         // +0x10 drawn with "%d"

	enum
	{
		OFFSET_X = 24,       // from the run's average gem corner to the popup's centre
		OFFSET_Y = 16,
		START_LIFE = 110     // mLife at creation, in ticks
	};
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

// port(variant): which generalised code paths a board takes (games/bejeweled/docs/CODE_MODS_DESIGN.md 2.2).
// Computed once by Board::InitVariant from the rules snapshot; every flag is false for the original rules
// and for tunables-only mods. Each flag's comment says what reads it.
struct VariantFlags
{
	bool mAny = false;          // any rule differs from default: v2 save, variant trace lines
	bool mOwnRng = false;       // board owns an MTRand (seeded / rules / seeds feature / ranked game); see Board::mOwnRng
	bool mShape = false;        // not a full 8x8: size, mask or per-level shapes
	bool mScaled = false;       // logical extent D > 406, so the view scales (5.1)
	bool mDraw = false;         // DrawVariant: mShape || specials || gold || non-swap moves || mNoTutorial
	bool mMatch = false;        // FindSetsVariant / HasSetVariant: patterns, minRun != 3, specials, gold
	bool mSearch = false;       // FindMoveVariant / TryShowHintVariant: patterns, minRun != 3, non-swap move types
	bool mMoves = false;        // twist / slide in the move set: the variant input layer
	bool mInvalid = false;      // moves.allowInvalid: UpdateSwapping keeps a swap that made no set
	bool mNoMoves = false;      // a noMoves rule other than the original (normal: game over, timed: fall out): UpdateFalling's no-moves branch
	bool mNoTutorial = false;   // the tutorial move (rows 4..6, cols 1..6, horizontal 3-run) cannot apply
	bool mSpecials = false;
	bool mGold = false;
	bool mUndo = false;         // the undo snapshots, keys and orb
	bool mGravity = false;      // board.gravity.direction not "down": the flip, the GetPieceAt guard (with GravityUp)
	bool mFall = false;         // mShape || mGravity || fallAccel != 180: FallPiecesVariant, RefillMasked, NewBoardVariant, the end falls
	bool mChecker = false;      // mShape || look.backdrop "twilight": DrawVariantChecker for SetupLevelData's tiles
	bool mDirt = false;         // a level's mask has dirt (4.14): RefillDiagonal, layers off next to scored runs, the dirt art
	bool mArrows = false;       // arrows.enabled (4.20): new arrows and their steps at a move's settle, the arrow art
};

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
		SAVEGAME_VERSION = 1,
		// port(variant): code mods (games/bejeweled/docs/CODE_MODS_DESIGN.md 2.4)
		MAX_COLS = BoardLayout::MAX_COLS,
		MAX_ROWS = BoardLayout::MAX_ROWS,
		MAX_GEM_COLORS = GameRules::MAX_GEM_COLORS,
		SAVEGAME_VERSION_VARIANT = 0x101
	};

	WinDMApp*		mApp;                   // +0x088 owning app (ctor arg)
	int				mLevel;                 // +0x08c 0-based level; ++ when the vortex finishes; drives level%4 tint, level%5 texture and the diff.txt formulas
	int				mDisplayLevel;          // +0x090 1-based level shown in the HUD; ctor 1, = mLevel+1 at the end of the wipe
	// port(variant): enlarged from [8][8] to [MAX_ROWS][MAX_COLS]; the original grid is [0..7][0..7]
	Piece*			mBoard[MAX_ROWS][MAX_COLS]; // +0x094 grid [row][col] (original index row*8+col), NULL = empty; ends at +0x193
	int				mState;                 // +0x194 BoardState
	bool			mTimedMode;             // +0x198 time-trial board (ctor arg = app->mTimedMode)
	SoundInstance*	mWhirlpoolSound;        // +0x19c looping whirlpool1 instance (DoWhirlpool); released at the end of the wipe / in ~Board
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
	int				mClearedAtMoveStart;    // +0x218 zeroed by the ctor and by a move's first scored run; never read
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
	int				mSavedZero;             // +0x264 0 from the ctor on; saved and loaded, never otherwise read
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
	DDImage*		mNextBackgroundImage;   // +0x2b4 640x480 next level's background (SetupLevelData)
	bool			mBackgroundChanged;     // +0x2b8 written only (dead)
	MemoryImage*	mTextImage;             // +0x2bc banner text rendered white with overfont2
	MemoryImage*	mTextColorImage;        // +0x2c0 rainbow sine-waved copy (height + 48), rebuilt by ColorizeText each Draw
	bool			mShowingText;           // +0x2c4 banner active
	// +0x2c8 a SexyString the original constructs and destroys and never uses: left out
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

	// port: pieces dropped from the grid without being deleted. NewBoard abandons the old board and a
	// swap with a stale selection overwrites a grid cell; the original leaks those pieces, which must
	// stay valid because mSelectedPiece can still point at one. Owned here and freed in ~Board.
	std::vector<Piece*> mOrphanedPieces;

	// ---- port(variant): code mods (games/bejeweled/docs/CODE_MODS_DESIGN.md 2.4) ----
	// All inert for the original rules.
	GameRules		mRules;                 // snapshot of gGameConfig.mRules taken in InitVariant
#ifdef BEJ_NO_VARIANTS
	static constexpr VariantFlags mVariant = {}; // every flag false: the compiler drops every variant hook
#else
	VariantFlags	mVariant;
#endif
	MatchFinder::Spec mMatchSpec;           // the match rules as MatchFinder reads them (InitVariant)
	int				mGroupCounts[MatchFinder::GROUP_SHAPE + 1] = {}; // the groups this board scored, by kind (-trace's variant line)
	int				mSpecialsMade[3] = {};  // stage 11: the flame (1) and lightning (2) gems this board made (-trace's variant line)
	int				mBlastCount = 0;        // ... and the blasts that went off
	struct BlastEffect
	{
		int			mKind;                  // Specials::KIND_FLAME or KIND_LIGHTNING
		int			mCol;
		int			mRow;
		int			mTick;                  // ticks since it went off
	};
	std::vector<BlastEffect> mBlastEffects;  // cosmetic, not saved: the blasts of late (DrawSpecials)
	// The gold board (stage 12, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.9; BoardGold.cpp), on a board with gold rules
	// (mVariant.mGold): the layer is per cell and stays when the gems move; the level ends when every cell that counts
	// is gold (GoldLeft), or the game when the level's time runs out. Hashed, saved (GOLD, GTIM) and undone
	uint8_t			mGold[MAX_ROWS][MAX_COLS] = {}; // 1: the cell is gold
	int				mGoldTicksLeft = 0;     // the level's time left (ticks), when it has a limit
	int				mGoldTickLimit = 0;     // the level's time limit (ticks), 0 for none (gold.timeLimit of the mode)
	struct GoldShine
	{
		int			mCol;
		int			mRow;
		int			mTick;                  // ticks since the cell turned gold
	};
	std::vector<GoldShine> mGoldShines;     // cosmetic, not saved: the cells gilded lately (DrawGold)
	Image*			mGoldTiles[2] = {};     // the level's checker tiles A and B tinted gold (DrawGold)
	int				mGoldTilesLevel = -1;   // the level mGoldTiles were made for, -1 for none
	BoardLayout		mLayout;                // current level's layout (8x8: origin 203/16, D 406, bottom 480)
	int				mCols;                  // == mLayout.mCols; 8
	int				mRows;                  // == mLayout.mRows; 8
	// Stage 7 (games/bejeweled/docs/CODE_MODS_DESIGN.md 5.3): the layout of the level SetupLevelData set up, which
	// UpdateWhirlpool applies after its deletion loop (ApplyPendingLayout); equal to mLayout between ticks
	BoardLayout		mPendingLayout;
	int				mLevelShape;            // index into the rules' board.levels: mLevel % their count (0 without levels)
	// A mod's scoring tunables (GameRules::ScoringTunablesChanged): the displayed score closes a large gap fast
	// (RollScoreFast). A fix of the tunables rather than a variant path, so BEJ_NO_VARIANTS keeps it
	bool			mFastScoreRoll;
	// The board's RNG (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.2), chosen by WinDMApp::NewGame
	// (GameConfig::mNextBoardSeed). Not a code path, so BEJ_NO_VARIANTS keeps it (mVariant.mOwnRng then
	// reads false)
	MTRand*			mOwnRng;                // NULL: Sexy::Rand, the original global stream; owned, freed in ~Board
	uint32_t		mGameSeed;              // mOwnRng's seed; 0 without one
	bool			mUserSeeded;            // a seed the player chose (-gameseed, ?seed=): the "-seeded" high-score table
	bool			mRanked;                // a ranked online game (stage 14); always false for now
	uint32_t		mBoardDraws;            // Board::Rand() calls (for spotting outside draws, stage 3)
	bool			mTakesPendingSeed;      // seeded with GameConfig::mPendingSeed, unplayed: a matching swap or a game over uses it up (4.10)
	// Replays (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.3-6.5): observers of play, so like the RNG source
	// they are no code path and BEJ_NO_VARIANTS keeps them
	GameStats*		mStats;                 // port(variant): the Stats page's counts (features.gameStats, GameStats.h); owned
	std::vector<GameStats> mStatsUndo;      // port(variant): the counts with each of mUndoStack's snapshots, newest last
	ReplayRecorder*	mRecorder;              // records this board's game (features.replays); NULL unless recording; owned, deleted in ~Board
	bool			mPlaybackOnly;          // driven by a replay player (WatchController) or the verifier: takes no input, never saves, counts or deletes
	bool			mPlaybackApplying;      // the player is applying an event: only then do Pause and Unpause reach a playback board
	const Buffer*	mPlaybackLoad;          // playback LOAD: LoadGame reads these bytes instead of the file (NULL: the file)
	Buffer			mPlaybackSave;          // playback SAVE: SaveGame leaves its bytes here instead of writing the file
	Replay::RngStart mRngStart;             // what Board::Rand started from: the seed, or the global stream's state at the constructor (when recording)
	// LiveSplit observer bookkeeping (4.15; LiveSplitObserver.cpp), not game state: unhashed, unsaved. Set
	// outside the BEJ_NO_VARIANTS parts, so that build counts GO like the others
	uint32_t		mBoardSerial;           // 1, 2, ... per board made in this process (InitVariant)
	bool			mResumed;               // the board resumed a saved game (GameLoadedVariant): its GO is not a run start
	// port(mods): the save's name under the score key at the constructor (ScopedSaveGameFileName), not game state
	std::string		mSaveGameFileName;
	// Moves (stage 5, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.1): counted on every board, saved (the v2 save's
	// MOVE chunk), hashed (LOGIC) and traced only on a variant board
	int				mMoveCount;             // moves that stood: swaps that made a match, kept invalid moves that count as moves (OnMoveMade)
	int				mInvalidMoves;          // kept invalid moves (moves.allowInvalid, KeepInvalidMove)
	int				mUndosUsed;             // undos taken (Undo, stage 6)
	// Undo (stage 6, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.12, 6.7): StateCodec's UNDO encodings of the board
	// before each move, newest last, at most undo.depth of them; not saved, so a resumed game has none
	std::vector<std::string> mUndoStack;
	// Gravity (stage 7c, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.16): saved (GRAV), hashed and traced only when
	// the rules set a gravity rule (GameRules::HasGravityRule)
	int				mGravityFlips;          // settles that flipped board.gravity.direction "alternate" (OnGravitySettle)
	bool			mGravityNoFlip;         // a kept invalid move: its settle does not flip
	// Dirt (stage 7b, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.14): the layers are mLayout's (a dirt cell becomes a
	// gem cell when its last layer goes); saved (DIRT), hashed and traced only when the rules have dirt (mDirtRules)
	bool			mDirtRules;             // GameRules::HasDirtRule (false in a BEJ_NO_VARIANTS build, which plays 8x8)
	bool			mDiagonalLeftFirst;     // RefillDiagonal: the side a slide takes when both have a gem; flips at each slide
	struct DirtDust
	{
		int			mCol;
		int			mRow;
		int			mStartTick;             // mUpdateCnt when the cell's last layer went
	};
	std::vector<DirtDust> mDirtDust;        // cosmetic, not saved: the dust of the cells cleared lately
	Image*			mDirtTiles[2];          // the level's checker tiles A and B darkened and tinted brown (DrawDirt)
	int				mDirtTilesLevel;        // the level mDirtTiles were made for, -1 for none
	// Twists and slides (stage 10, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.4 and 4.5; BoardMoves.cpp): the move of
	// states 10..13, hashed and snapshotted only on a board with those moves (mVariant.mMoves). The grid holds the
	// board before the move in all four states (a move that goes back is undone in the grid when it turns back), so a
	// save during one is that board in STATE_PLAYING
	MoveSet::Move	mMoveInPlay;            // the twist or the slide
	int				mMoveTick;              // its animation's ticks so far: TWIST_STEPS, or moves.slide.snapTicks, in all
	int				mSlideFrom;             // a slide's offset along its axis at the release (px; cosmetic, from the SLIDE event)
	int				mSlideTo;               // ... where its animation ends: k cells on, the way round nearest mSlideFrom
	// The variant input layer (MouseDownVariant ..): not game state, neither hashed nor saved
	int				mPressCol;              // the cell under the press (-1: none, or not one that can slide)
	int				mPressRow;
	bool			mPressTwist;            // the press twists when it ends without a slide (MouseUpVariant)
	int				mDragAxis;              // -1 until the drag passes 16 px, then the axis of the slide it shows
	int				mDragOffset;            // the drag along that axis (px, logical): drawn only
	int				mDragStart;             // the dragged segment's first cell and length
	int				mDragLength;
	// Arrow gems (stage 18, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.20; BoardArrows.cpp), on a board with arrow rules
	// (mVariant.mArrows): the arrows are the pieces' mArrow. Saved (ARRW), hashed, traced and undone with these counts
	struct ArrowStep
	{
		int			mFromCol;               // the arrow's cell before the step
		int			mFromRow;
		int			mToCol;                 // ... and after it, where the traded gem was; off the board for the arrow that escapes
		int			mToRow;
	};
	int				mArrowMoves;            // moves since the last new arrow (arrows.every)
	int				mArrowsCleared;         // the arrows cleared this game: the bonus's factor
	bool			mArrowStepDue;          // a move stood: the settle that ends it steps the arrows
	std::vector<ArrowStep> mArrowSteps;     // STATE_ARROW_STEPPING: this settle's steps, in the order they were taken
	int				mArrowTick;             // ... the ticks of their animation so far

public:
	int				GetColX(int theCol);                            // 0x4c9b25 theCol*51 + 203 (thiscall in the binary, this unused; port(variant): a member, + mLayout.mOriginX)
	int				GetRowY(int theRow);                            // 0x4c9b37 theRow*51 + 16 (port(variant): + mLayout.mOriginY)
	void			GameOver(int theShakeTicks);                    // 0x4c9b47
	void			RequestNewGame();                               // 0x4c9bab GameOver(50) + mNewGamePending
	bool			IsInProgress();                             // 0x4c9bbe !mIsNewGame && state != 5 && state != 6
	void			Pause(bool theUserPause);                       // 0x4c9bda
	void			Unpause(bool theUserPause);                     // 0x4c9c2d
	Piece*			GetPieceAt(int theX, int theY);                 // 0x4c9c7c
	bool			HasSet();                                     // 0x4c9cf5
	int				GetPanPosition(int theX);                           // 0x4c9d94 (port(variant): a member, for the board width)
	bool			FindMove(int* theMove, int theMoveNum, bool horizontal, bool vertical); // 0x4c9dda theMove = {col,row,col2,row2} or NULL
	virtual void	MouseMove(int x, int y);                        // 0x4c9fd0 Widget slot 52
	virtual void	MouseLeave();                                   // 0x4c9fe9 Widget slot 51
	void			SwapPieces(Piece* thePiece1, Piece* thePiece2);  // 0x4c9ff8
	void			GetBoardSize(int* theWidth, int* theHeight);    // 0x4ca13e (406, 406) (port(variant): a member)
	void			GetBoardCenter(int* theX, int* theY);           // 0x4ca179 (406, 219) (port(variant): a member)
	void			DoBouncyText(const SexyString& theText, int theDuration); // 0x4ca1c8
	void			ColorizeText();                                 // 0x4ca36e
	void			DoWhirlpool();                               // 0x4ca58d
	virtual void	MouseDown(int x, int y, int theClickCount);     // 0x4ca6b0 Widget slot 54
	virtual void	MouseUp(int x, int y, int theClickCount);       // 0x4ca8b7 Widget slot 56
	virtual void	MouseDrag(int x, int y);                        // 0x4ca8c1 Widget slot 58
	virtual void	KeyChar(SexyChar theChar);                      // 0x4ca9d3 Widget slot 47
	void			UpdateClearing();                               // 0x4ca9ed state 4
	void			UpdateGameOverAnim();                               // 0x4cab82 state 5
	void			TryShowHint();                                       // 0x4cad2f
	void			SetupLevelData();                                   // 0x4cae29 background + diff.txt level parameters
	void			NewBoard();                                    // 0x4cb182
	void			UpdateLevelTransition();                              // 0x4cb284 state 8
	void			UpdateBoardDropping();                                // 0x4cb3bf state 9
	void			ShowHint();                                     // 0x4cb475 button 15

	Board(WinDMApp* theApp, bool timedMode);                        // 0x4cb5bc
	enum ScratchBoardTag { SCRATCH_BOARD };
	Board(WinDMApp* theApp, ScratchBoardTag);                       // port(variant): a target for StateCodec's round trip: nothing drawn, loaded, saved or shown
	void			UpdateWhirlpool();                              // 0x4cb931 state 7
	virtual void	Draw(Graphics* g);                              // 0x4cbc70 Widget slot 26
	bool			HasSavedGame();                               // 0x4cca37
	bool			LoadGame();                                     // 0x4cca98
	bool			SaveGame();                                     // 0x4ccdfe
	void			SaveGameSnapshot(Buffer* theBuffer);            // port: what SaveGame() would write, live game untouched
	void			WriteSaveGame(Buffer* theBuffer, int thePlaySeconds, int theState); // port: the body of 0x4ccdfe
	std::string		GetSaveGameFileName() const { return mSaveGameFileName; } // port: "userdata\\savegame.dat", scoped for gameplay mods (port(mods): the board's own)
	static std::string ScopedSaveGameFileName();                    // port(mods): that name under the current score key
	bool			DeleteSavedGame();                               // 0x4cd130
	virtual ~Board();                                               // 0x4cd1ba (scalar deleting dtor 0x4ce430)
	bool			FindSets();                               // 0x4cd287
	void			UpdateFalling();                                // 0x4cd687 state 0
	void			UpdateSwapping();                                   // 0x4cd84f states 2, 3
	virtual void	Update();                                       // 0x4cdabc Widget slot 22

	// port(variant): code mods (BoardVariant.cpp)
	void			InitVariant(WinDMApp* theApp);                  // first in the ctor (before mApp is set): rules, layout, flags, RNG, sound pitch
	static VariantFlags ComputeVariantFlags(const GameRules& theRules, const GameFeatures& theFeatures);
	void			UsePendingSeed();                               // a matching swap or a game over of a board that holds the pending seed
	void			OnMoveMade();                                   // a move stood (UpdateSwapping's match, a kept invalid move): it counts, the pending seed is used up
	void			KeepInvalidMove();                              // UpdateSwapping: a swap that made no match stays (moves.allowInvalid, 4.1)
	void			OnNoMovesVariant();                             // UpdateFalling: settled without a move, the noMoves rule decides (4.1)
	bool			NoHintVariant();                                // a board playing without a move (noMoves "continue"): no hint, no hint glow
	SexyString		InstructionsText();                             // DoInstructionDialogette's text: the rules' own (2.6; gravity, 4.16)
	SexyString		FullInstructionsText(const SexyString& theText); // DoFullInstructions: Help's text under the rules (gravity, 4.16)
	// Undo (stage 6, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.12, 6.7; BoardVariant.cpp)
	void			TakeUndoSnapshot();                             // SwapPieces: the board before the move (in STATE_PLAYING)
	void			PopUndoSnapshot();                              // UpdateSwapping: the swap went back, so does its snapshot
	bool			CanUndo() const;                                // the UNDO gate, which playback checks again (WatchController)
	int				UndosLeft() const;                              // undo.max minus the undos taken; -1 without a limit
	static bool		IsUndoKey(SexyChar theChar);                    // z, u, Backspace
	bool			UndoKeysActive() const;                         // the undo keys undo (KeyChar) rather than pause
	bool			RequestUndo();                                  // the keys and the red orb: UNDO recorded, then Undo; refused: bad2
	// The fixes mod (features.fixes; docs/MOD_ROADMAP.md "The fixes mod", GAME_NOTES.md "Fix candidates"; BoardVariant.cpp)
	bool			DropStaleSelection();                           // fixes.selection: a selected gem no longer on the board is deselected (true: it was)
	void			ResumeLevelLook();                              // fixes.resumeLook: a loaded game's own level background and tint
	void			Undo();                                         // the newest snapshot back (playback applies UNDO with it)
	// The v2 save of a variant board (games/bejeweled/docs/CODE_MODS_DESIGN.md 7.2; BoardSave.cpp)
	void			GetVariantSave(Replay::VariantSave* theSave);  // the header and chunk values of the board as it is
	void			WriteVariantHeader(Buffer* theBuffer);          // WriteSaveGame, after the version: rules, shape, mask
	void			WriteVariantChunks(Buffer* theBuffer);          // WriteSaveGame, after the v1 fields: RNGS, MOVE, GRAV (gravity rules), END
	bool			ReadVariantHeader(const Buffer& theBuffer, int theVersion); // LoadGame, after the version: the whole save checked first
	void			ReadVariantChunks(const Buffer& theBuffer);    // GameLoadedVariant: the RNG, the MOVE counts and the GRAV state the save holds
	void			GameLoadedVariant(const Buffer& theSave);       // the end of a LoadGame that succeeded: the RNG from the sidecar, the LOAD event
	bool			ReadSaveFile(Buffer* theBuffer);               // LoadGame's read: the save file, or in playback mPlaybackLoad
	bool			WriteSaveFile(const Buffer& theBuffer);        // SaveGame's write: SAVE, the sidecar and the save file, or in playback mPlaybackSave
	// The sidecar of the save (games/bejeweled/docs/CODE_MODS_DESIGN.md 7.3): savegame.bjr next to savegame.dat
	std::string		GetSidecarFileName() const;
	void			WriteSaveSidecar(const Buffer& theSave);       // SaveGame: the SAVE event and the sidecar, or no sidecar
	void			DeleteSaveSidecar();                            // DeleteSavedGame: the sidecar goes with the save
	void			KeepSidecarGame();                              // the recorded game of the sidecar there is, kept as a finished replay
	std::string		SidecarSnapshot(const std::string& theSave);   // the sidecar a shutdown would leave with theSave ("" for none)
	void			AddStatsToSidecar(std::string* theSidecar, const std::string& theSave); // port(variant): the STAT record (GameStats.h)
	bool			ReadSaveSidecar(const std::string& theSave, Replay::File* theSidecar); // the sidecar, if it goes with theSave
	void			GetRngState(Replay::RngState* theRng);         // the RNG Board::Rand draws from, as RNGS stores it
	bool			SetPlaybackRng(const Replay::RngState& theRng); // a playback LOAD: the RNG the recorded board went on with (RNGS)
	Replay::EndInfo	GetEndInfo(int theReason, const MTRand* theRng = NULL); // the END record of the game as it is now
	// Shapes (stage 7, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.3, section 5; BoardShape.cpp)
	void			NewBoardVariant();                             // NewBoard on a shape board: holes skipped, rerolls capped
	void			RefillMasked();                                 // UpdateClearing's refill over each column's cells
	void			NewBoardWithMove();                            // the constructor's fill when the tutorial move cannot apply
	void			DrawVariantChecker(Graphics* g, Image* theTileA, Image* theTileB); // SetupLevelData's tiles for its level's shape
	void			ApplyPendingLayout();                           // UpdateWhirlpool, after the deletion loop: the new level's shape
	void			SetLoadedLayout(const BoardLayout& theLayout, int theLevelShape); // ReadVariantHeader: the save's shape
	void			RebuildLoadedBackground();                      // GameLoadedVariant: the background of the loaded level and shape
	void			DrawVariant(Graphics* g);                       // Draw for mDraw boards (BoardDrawVariant.cpp)
	// Gravity (stage 7c, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.16; BoardShape.cpp)
	bool			GravityUp() const;                              // gems rise now: "up", or "alternate" after an even number of flips
	void			FallPiecesVariant(int* theLandedCount, int* theLandedXSum, bool* isSettled); // UpdateFalling's loop: direction, fallAccel
	void			OnGravitySettle();                              // UpdateFalling: a settle without a match; "alternate" flips if it reached PLAYING
	void			UpdateBoardDroppingVariant();                         // UpdateBoardDropping's fall in the gravity direction
	bool			GameOverFallVariant();                          // UpdateGameOverAnim's fall in the gravity direction: true when every gem is gone
	// Dirt (stage 7b, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.14; BoardShape.cpp, BoardDrawVariant.cpp)
	void			RefillDiagonal();                               // UpdateClearing's refill with dirt: segments, then diagonal slides
	bool			RefillSegmentsPass();                           // RefillDiagonal: one pass over every column's segments; true if a gem moved or spawned
	bool			SlideDiagonal();                                // RefillDiagonal: one gem into an unreachable cell; false when none can go
	void			ClearDirtNextToRun(int theStartCol, int theStartRow, int theEndCol, int theEndRow, int theColor); // FindSets: one layer off each dirt cell next to the run
	void			ClearDirtNextToCells(const std::vector<std::pair<int, int> >& theCells); // ... next to any group's cells
	void			ClearDirtCells(const std::vector<std::pair<int, int> >& theCells); // stage 11: ... in a blast's reach
	void			ClearDirtLayers(const bool (*theCells)[BoardLayout::MAX_COLS]); // ... a layer off each marked cell (row-major)
	// Match rules (stage 9, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.6 and 4.7; BoardMatch.cpp)
	void			FillColorGrid(MatchFinder::ColorGrid* theGrid) const; // each cell's colour, NO_GEM for none
	bool			HasSetVariant();                              // HasSet under the match rules
	bool			FindMoveVariant(int* theMove, int theMoveNum, bool horizontal, bool vertical); // FindMove under the match and move rules: {to col, to row, from col, from row}
	bool			FindSetsVariant();                        // FindSets under the match rules
	void			ScoreGroup(const MatchFinder::Group& theGroup, int* theAvgX); // FindSetsVariant: one group's points
	void			AddMoveScore(int thePoints);                     // ScoreGroup and ScoreBlast: the move's score, EXCELLENT, the score timer
	// Flame and lightning gems (stage 11, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.8; BoardSpecials.cpp)
	void			ResolveSpecials(const std::vector<MatchFinder::Group>& theGroups); // FindSetsVariant: the specials made, the blasts
	void			MovedCells(std::vector<MatchFinder::Cell>* theCells); // the cells the move in its own resolve put gems into
	void			ScoreBlast(const Specials::Blast& theBlast);     // ResolveSpecials: one blast's gems, points, dirt and effect
	void			PlaySpecialSound(int theKind, int thePan);      // sounds/flame or sounds/lightning, else the original's explode or firecrackle
	void			UpdateSpecials();                               // Update: the overlays' frames and the blast effects (cosmetic)
	void			DrawSpecials(Graphics* g, bool isOver);         // DrawVariant: the specials' overlays (under), the blasts (over)
	// The gold board (stage 12, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.9; BoardGold.cpp)
	bool			CountsForGold(int theCol, int theRow) const;    // a cell of the goal: a gem cell, or dirt with gold.countDirt
	int				GoldGoal() const;                               // the cells of the goal
	int				GoldLeft() const;                               // ... not gold yet
	void			GildCells(const std::vector<MatchFinder::Cell>& theCells); // FindSetsVariant and ScoreBlast: gem cells turn gold
	int				GoldLimitTicks() const;                         // the level's time limit for the mode (0: none)
	void			SetupLevelDataGold();                               // SetupLevelData: no gold, the level's time
	void			UpdateGoldGoal();                               // Update's level block on a gold board: the time, the level's end, TIME_UP
	int				GoldBarTarget() const;                          // Update: the bar's width, the gold so far or the time left
	bool			GoldTimed() const { return mGoldTickLimit > 0; }
	void			UpdateGold();                                   // Update: the shines (cosmetic)
	void			DrawGold(Graphics* g, bool isOver);             // DrawVariant: the gold cells (under the gems), the shines (over)
	void			DrawGoldHud(Graphics* g);                       // DrawVariant: GOLD n/m over the bar
	// Arrow gems (stage 18, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.20; BoardArrows.cpp)
	void			OnArrowMove();                                  // OnMoveMade, KeepInvalidMove: a move stood
	bool			ArrowSettle();                                  // UpdateFalling's settle: steps the arrows (true: they step now), a new arrow
	bool			NextArrowCell(int theCol, int theRow, int theDir, int* theNextCol, int* theNextRow) const; // the next gem cell the way theDir points
	int				ArrowDistance(int theCol, int theRow, int theDir) const; // the gem cells between an arrow and its edge
	void			UpdateArrowSteps();                             // STATE_ARROW_STEPPING
	bool			IsArrowEscaping() const;                        // STATE_ARROW_STEPPING: the step is an arrow's escape
	void			AddNewArrow();                                  // the settle after every arrows.every moves: a gem on an edge turns into an arrow
	void			ClearArrow(Piece* thePiece);                    // a matched or blasted arrow: the bonus, the count
	static int		TurnArrow(int theArrow, int theQuarters);       // arrows.turn: a Piece::mArrow a quarter turn on (1 clockwise)
	int				ArrowExitEdges(const ArrowStep& theStep) const; // the ARROW_EDGE_* bits a step leaves the board by
	void			ClearMatchedArrows();                           // FindSetsVariant found groups: each dying arrow
	bool			DrawArrowGem(Graphics* g, Piece* thePiece, int theX, int theY); // an arrow's picture instead of its gem (false: not an arrow)
	void			DrawArrowWarnings(Graphics* g);                 // DrawVariant: an arrow one step from its edge pulses, the edge glows
	bool			ApplyDirtLayers(const std::vector<uint8_t>& theLayers, std::string* theWhy); // a save's or snapshot's layers onto the level's mask
	void			DrawDirt(Graphics* g);                          // DrawVariant: the dirt cells, under the gems
	void			DrawDirtDust(Graphics* g);                      // DrawVariant: the dust of cleared cells, over the gems
	void			DrawStretched(Graphics* g, Image* theImage, const Rect& theDest, const Rect& theSrc); // on the board layer, through its scale
	void			RollScoreFast();                                // Update, before the score roll: a large gap closes fast (mFastScoreRoll)
	// Twists and slides (stage 10, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.4, 4.5 and 4.7; BoardMoves.cpp)
	MoveSet::Options MoveOptions(bool horizontal, bool vertical) const; // the rules' move types for MoveSet's search
	bool			FindVariantMove(MoveSet::Found* theFound, int theMoveNum); // the theMoveNum-th move of every type (MoveSet's order)
	void			FindVariantMoves(std::vector<MoveSet::Found>* theMoves); // ... all of them
	static bool		IsVariantMoveState(int theState) { return (theState >= STATE_TWISTING) && (theState <= STATE_SLIDING_BACK); }
	bool			CanTwist(int theCol, int theRow, int theDir);    // TWIST's gate: the board in play, the block's four gems, the direction allowed
	bool			CanSlide(int theAxis, int theLine, int theStart, int theK, int theOffset); // SLIDE's gate: ... a whole segment of gems, 1 <= k < its length
	void			StartTwist(int theCol, int theRow, int theDir); // a twist of the block at (theCol, theRow) after CanTwist (input or a TWIST event)
	void			StartSlide(int theAxis, int theLine, int theStart, int theK, int theOffset); // a slide after CanSlide (input or a SLIDE event)
	void			UpdateTwist();                                  // states 10 and 11
	void			UpdateSlide();                                  // states 12 and 13
	void			EndVariantMove();                               // SaveGame in states 10..13: the gems back in their cells, STATE_PLAYING
	bool			MouseDownVariant(int x, int y, int theClickCount); // MouseDown on a board with twists or slides: true when it took the press
	bool			MouseDragVariant(int x, int y);                 // MouseDrag ...: true when it took the drag
	void			MouseUpVariant(int x, int y);                   // MouseUp ...: a slide, or a twist the press left pending
	bool			TwistAt(int x, int y, bool isNearOnly, int* theCol, int* theRow); // the block a click at (x, y) twists (logical)
	void			DrawMovesUnder(Graphics* g);                    // DrawVariant, with the gems: a slide's line clipped to its segment
	bool			DrawsSlideCell(int theCol, int theRow);         // ... whose gems the gem loop leaves to it
	void			DrawMovesOver(Graphics* g);                     // DrawVariant, with the cursor: the twist ring

	// port(variant): every draw of the translated code. Class scope finds it before Sexy::Rand, so the 9
	// Rand() expressions keep their text; without an own MTRand it is the original global stream, draw for
	// draw (MTRand::Next and Sexy::Rand return the same value)
	int				Rand() { mBoardDraws++; return (mOwnRng != NULL) ? (int) mOwnRng->Next() : Sexy::Rand(); }

	using Widget::MouseDown;
	using Widget::MouseUp;
};

}

#endif
