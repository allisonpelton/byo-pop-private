#ifndef __TIPTOP_BOARD_H__
#define __TIPTOP_BOARD_H__

// The board of TipTop (TipTop.exe 0x401000-0x410d35, without the glyph blits 0x40a792-0x40aaf9 that are in Gui.cpp): the
// game itself. Four classes, all with the original's layout (32-bit):
//   Piece (0x5c bytes): a block, a special block or the preview of one
//   Field (0x96c bytes, vtable 0x46c320): the grid of pieces and the rules: motion, collisions, matches, score,
//         the choice of the next block, the clearing animation. The board owns one (board + 0x34c).
//   BoardState (0x12b4 bytes): the level in play, the goal and the meter, and the board's own copy of the configuration
//         (the level's values are written into it). The field keeps a pointer to it (field + 0x8f0).
//   Board (0x1658 bytes, vtable 0x46c350): the picture of the game: the four preview blocks, the background glow,
//         the screen layout, the level changes and the bonus rounds, and the save file.
// The grid is indexed by row * columns + column; row 0 is the bottom (pieces fall toward it) and rows - 1 the top
// (where the next block enters). A piece's y is 24.8 fixed point (y >> 8 is the pixel row from the bottom).

#include "Config.h"
#include "Lowos.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace tiptop
{

// Piece kinds (Piece::mKind): 0-9 colors; the others are the special blocks (and the tap rows of each in the
// configuration: gConfig.m*TapRows); 0x11 none
enum
{
	KIND_ROCKET = 10,
	KIND_EXPLODER = 11,
	KIND_HAMMER = 12,
	KIND_MAGNET = 13,
	KIND_RAINBOW = 14,
	KIND_COLORBOMB = 15,
	KIND_COLUMNNUKE = 16,
	KIND_NONE = 0x11,
};

// Positions on the field (Piece::mY, mTargetY, mShakeX and mShakeY, and the steps of a motion) are fixed point with 8 bits
// of fraction: FIXED_ONE is one pixel (the row of a block is y >> FIXED_SHIFT over the block's height).
constexpr int FIXED_SHIFT = 8;
constexpr int FIXED_ONE = 1 << FIXED_SHIFT;

// The ring of the chain counters (Field::mChains), and the group number of a piece that is in no chain
constexpr int CHAIN_COUNT = 0x400;
constexpr int NO_CHAIN = CHAIN_COUNT;

// How long a block flashes once before a column nuke's or a color bomb's effect removes it (ms)
constexpr int FLASH_TIME = 0xfa;

// Piece flags (Piece::mFlags)
enum
{
	PF_MOVING = 0x0001,     // moves along its column (mSpeed: negative toward row 0, positive up)
	PF_TARGET = 0x0002,     // moves to mTargetY (a pushed or secondary move)
	PF_PREVIEW = 0x0004,    // the preview of a block that is not on the field yet
	PF_REMOVING = 0x0008,   // dissolving (StartRemoval)
	PF_CHAIN = 0x0010,      // mGroup is the chain counter of the group it fell with
	PF_NOFALL = 0x0020,     // the blocks above do not fall when it goes
	PF_NOFLOATS = 0x0040,   // it creates no floaters when it goes (also: it was a floater)
	PF_ENTERING = 0x0080,   // flies in from the top (the start of a level with a fixed layout)
	PF_LEAVING = 0x0100,    // falls off the screen (the end of a level, 0x408ceb)
	PF_MATCHED = 0x0200,    // part of a group being removed (it shakes)
	PF_GONE = 0x0400,       // dissolved: the field removes it
};

// A block (0x5c bytes; 0x401000, copied by 0x401099 and 0x4011c4)
struct Piece
{
	int32_t mKind;           // +0x00
	uint8_t mMarked;         // +0x04 visited by the group search
	float mSpeed;            // +0x08 y speed: positive for the player's block (a "faller" in the configuration, it rises), negative for the blocks that enter from the top ("floaters")
	uint32_t mMoveTime;      // +0x0c GameTime of its last move step
	int32_t mTargetY;        // +0x10 where it goes (PF_TARGET), 24.8
	int32_t mX;              // +0x14 pixels from the left of the field
	int32_t mY;              // +0x18 24.8 from the bottom
	uint32_t mFrame;         // +0x1c the animation frame to draw
	uint32_t mSeed;          // +0x20 random phase of the idle animation; StartRemoval stores the time it started
	uint32_t mDelay;         // +0x24 ms it waits before it moves
	uint16_t mFlags;         // +0x28
	uint32_t mPower;         // +0x2c rows a special still passes through (0: it acts where it lands)
	int32_t mGroup;          // +0x30 index of the chain counter (Field::mChains)
	uint32_t mFlyTime;       // +0x34 PF_ENTERING / PF_LEAVING: when it starts
	uint32_t mFlyDelay;      // +0x38 and its delay
	int32_t mShakeX;         // +0x3c 24.8
	int32_t mShakeY;         // +0x40
	int32_t mShakeTargetX;   // +0x44 in pixels
	int32_t mShakeTargetY;   // +0x48
	uint32_t mShakeTime;     // +0x4c
	int32_t mEffectKind;     // +0x50 a removal effect still to play (KIND_COLORBOMB, KIND_COLUMNNUKE) or KIND_NONE
	uint32_t mEffectDelay;   // +0x54 ms before it starts
	uint8_t mCrossed;        // +0x58 a magnet reported that it crossed the field's middle (0x406974)

	Piece();                              // 0x401000
	Piece(const Piece& theOther);         // 0x401099
	~Piece();                             // 0x4011bb (and 0x403bbb, its deleting form, is delete)
	Piece& operator=(const Piece& theOther);  // 0x4011c4
	// 0x402c5d: starts the removal of a piece (a no-op for 0 or one already being removed)
	static void StartRemoval(Piece* thePiece, uint16_t theFlags, uint32_t theEffectKind, uint32_t theEffectDelay);
};

// The level in play and the board's copy of the configuration (board + 0x35c). The field reads all its rules from
// here: Config.h names the keys; the fields below are the ones it does not (the level's values, 0x40b82c).
struct BoardState
{
	uint32_t mLevel;         // +0x00 the level in play (0-based; past the end of the list the values ramp up)
	int32_t mBlocksLeft;     // +0x04 the goal: blocks to clear to finish the level (not in the timed game)
	float mMeter;            // +0x08 the timed game's time bar; also counts the blocks cleared
	uint32_t mMeterTime;     // +0x0c GameTime of the meter's last update
	Config mConfig;          // +0x10
};

// Config offsets that Config.h does not name. The first group is the level's record (Level, 0x70 bytes), copied
// into the board's configuration by Board::NextLevel; in gConfig itself these are zero.
template <typename T> inline T& ConfigAt(Config& theConfig, size_t theOffset)
{
	return *reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(&theConfig) + theOffset);
}
inline int32_t& CfgStopRow(Config& c)          { return ConfigAt<int32_t>(c, 0x04); }   // Level mStopRow: the row the next block stops at (-1 the middle, -2/-3 specials)
inline int32_t& CfgStopRowAdd(Config& c)       { return ConfigAt<int32_t>(c, 0x08); }   // Level mStopRowAdd
inline int32_t& CfgStartRows(Config& c)        { return ConfigAt<int32_t>(c, 0x110); }  // Level mStartRows: rows of blocks at the start
inline int32_t& CfgStartRowsAdd(Config& c)     { return ConfigAt<int32_t>(c, 0x114); }  // Level mStartRowsAdd
inline int32_t& CfgGoal(Config& c)             { return ConfigAt<int32_t>(c, 0x118); }  // Level mGoal: blocks to clear / the bar's goal (Config.h names it mBonusTime: in gConfig it is the bonus round's seconds)
inline int32_t& CfgSpawnThreshold(Config& c)   { return ConfigAt<int32_t>(c, 0x120); }  // Level mSpawnThreshold
inline float& CfgFloatsPerRemoval(Config& c)   { return ConfigAt<float>(c, 0x130); }    // Level mFloatsPerRemoval
inline float& CfgFloatsPerTap(Config& c)       { return ConfigAt<float>(c, 0x134); }    // Level mFloatsPerTap
inline int32_t& CfgRange(Config& c, int i)     { return ConfigAt<int32_t>(c, 0x138 + i * 4); }  // Level mRange[i]: min and max of the interval between two blocks of each special (8 pairs)
inline int32_t& CfgNumColors(Config& c)        { return ConfigAt<int32_t>(c, 0x18c); }  // Level mNumColors
inline int32_t& CfgMeterStart(Config& c)         { return ConfigAt<int32_t>(c, 0x468); }  // Level mMeterStart (the third list: the meter's start)
inline float& CfgMeterLoss(Config& c)           { return ConfigAt<float>(c, 0x46c); }    // Level mMeterLoss
inline int32_t& CfgMeterDoublingTime(Config& c)         { return ConfigAt<int32_t>(c, 0x470); }  // Level mMeterDoublingTime * 1000

// The grid and the rules (0x96c bytes; 0x4012e6)
struct Field
{
	uint32_t mScore = 0;                   // +0x004
	uint32_t mLevelStartScore = 0;         // +0x008 mScore when the level began (0x4082f7 gives the difference)
	uint32_t mLockFall = 0;                // +0x00c bits: 1 until all stop (mLockFallers), 2 set by the board, 4 turn based: the player may not drop
	uint32_t mLockSpawn = 0;               // +0x010 the same for the next block entering the field
	uint32_t mColumns = 0;                 // +0x014
	uint32_t mRows = 0;                    // +0x018
	uint32_t mCount = 0;                   // +0x01c columns * rows
	Piece mNext;                           // +0x020 the block that enters next (its preview is in the board's four)
	uint32_t mTotalTime = 0;               // +0x07c ms of play
	uint32_t mLevelTime = 0;               // +0x080 of the level
	uint32_t mBestChain = 0;               // +0x084 the longest chain of matches
	uint32_t mLevelBestChain = 0;          // +0x088
	uint32_t mBestMatch = 0;               // +0x08c the biggest group removed at once
	uint32_t mLevelBestMatch = 0;          // +0x090
	uint8_t mQueue[0x40] = {};             // +0x094 the columns the next floaters come in (floater determination 2)
	uint32_t mQueueHead = 0;               // +0x0d4
	uint32_t mQueueCount = 0;              // +0x0d8
	int16_t mChains[CHAIN_COUNT] = {};           // +0x0dc matches of each chain (a ring: mNextChain); use Chain()
	int32_t mLowRow = -1;                  // +0x8dc the lowest row that has a block at rest (-1 none)
	int32_t mHighRow = -1;                 // +0x8e0 the highest
	int32_t mBlockWidth = 0;               // +0x8e4
	int32_t mBlockHeight = 0;              // +0x8e8
	Piece** mPieces = nullptr;             // +0x8ec mCount pointers, 0 for an empty cell
	BoardState* mState = nullptr;          // +0x8f0
	uint32_t mPendingSpawns = 0;           // +0x8f4 blocks still to enter
	float mFloaterCredit = 0;              // +0x8f8 floaters earned (a fraction each tap) before they are queued
	uint32_t mPendingFloaters = 0;         // +0x8fc floaters to queue
	int32_t mSpawnDelay = 0;               // +0x900 ms until the next block enters
	uint8_t mIdle = 0;                     // +0x904 nothing moves or dissolves
	uint32_t mCountdown[8] = {};           // +0x908 blocks until the next of each special (0x40428c)
	uint32_t mNextChain = 0;               // +0x928
	int32_t mRowDelay = 0;                 // +0x92c
	uint32_t* mColumnWeights = nullptr;    // +0x930 one per column (the chance the next floater is there)
	uint32_t mClearTime = 0;               // +0x934 the clearing animation's clock
	uint32_t mClock = 0;                   // +0x938 GameTime of the last update (0 none)
	uint8_t mClearStart = 0;               // +0x93c the clearing animation starts over
	uint32_t mPlayTime = 0;                // +0x940 ms played with nothing locked
	uint32_t mPlayTime2 = 0;               // +0x944 the same, used up by UpdateSpawnTimer
	uint32_t mColumnCounter = 0;           // +0x948 floater determination 1: the next column
	float mFloaterCredit2 = 0;             // +0x94c floaters earned by removals
	int32_t mClearRowA = 0;                // +0x950 the clearing animation: the row it has reached from the bottom
	int32_t mClearRowB = 0;                // +0x954 and from the top
	int32_t mClearIndexA = 0;              // +0x958 the cell it is at
	int32_t mClearIndexB = 0;              // +0x95c
	uint8_t mClearActiveA = 0;             // +0x960
	uint8_t mClearActiveB = 0;             // +0x961
	int32_t mClearInterval = 0;            // +0x964 ms between its steps
	uint8_t mClearPhaseB = 0;              // +0x968 the top sweep's turn
	uint8_t mClearRight = 0;               // +0x969 the sweep goes to the right
	uint8_t mClearReady = 0;               // +0x96a

	Field(int theColumns, int theRows, const int32_t* theBlockSize);  // 0x4012e6
	virtual ~Field();                      // 0x401455 (the deleting destructor), body 0x40147e

	// The chain counter theIndex (mChains[theIndex]; NO_CHAIN means "no chain" in CheckGroup and is never read)
	int16_t& Chain(int32_t theIndex);
	Piece* PieceAt(uint32_t theIndex);                 // 0x4014e1
	BoardState* SetState(BoardState* theState);        // 0x40150a (0 clears it and returns 0, else returns the old one)
	BoardState* GetState();                            // 0x401547
	int32_t LoadLayout();                              // 0x401559
	int32_t Reset();                                   // 0x4016ae
	uint32_t CheckGroup(uint32_t theIndex, char theOnlyOne);  // 0x401cb4
	int32_t FloodFill(uint32_t theIndex, int32_t* theChain, uint8_t* theFlags);  // 0x4029c6
	void Land(Piece* thePiece);                        // 0x402cc0
	void DeletePiece(uint32_t theIndex);               // 0x403b50
	void StartSecondaryFall(uint32_t theIndex);        // 0x403be4
	int32_t RandomKind(int32_t theMode, int32_t theNotA, int32_t theNotB);  // 0x40428c
	uint32_t IndexOf(Piece* thePiece);                 // 0x40476d
	void PrepareNext();                                // 0x4047c7
	uint32_t Spawn();                                  // 0x404ce5
	void QueueSpawn();                                 // 0x40500e
	uint32_t Place(Piece* thePiece);                   // 0x405058
	uint32_t CanPlace(int theColumn);                  // 0x40533e
	uint32_t CanSpawn(int theColumn);                  // 0x4053a4
	void UpdateLowRow();                               // 0x40542a
	void UpdateHighRow();                              // 0x4055b0
	void UpdateRows();                                 // 0x405778
	uint32_t Neighbor(uint32_t theIndex, char theUp);  // 0x405791
	uint32_t EdgeRow(int theColumn, char theTop);      // 0x4057f7
	uint32_t Within(uint32_t theIndex, int theDistance);  // 0x405963
	void Collide(int theA, uint32_t theB);             // 0x405a19
	void ClearShake();                                 // 0x4065b2
	void ChooseShake(Piece* thePiece);                 // 0x406634
	void UpdateShake(Piece* thePiece);                 // 0x4067fb
	uint32_t Move(uint32_t theIndex);                  // 0x406974
	void Update();                                     // 0x407cc3
	uint32_t GetBlocksLeft();                          // 0x4082d3
	uint32_t GetScore();                               // 0x4082e8
	int32_t GetLevelScore();                           // 0x4082f7
	uint32_t ScorePoints(int theCount, uint32_t theChain);   // 0x40830c
	uint32_t MeterPoints(int theCount, uint32_t theChain);   // 0x40836c
	uint32_t ClearStep();                              // 0x4083cc
	uint32_t BlowAway();                               // 0x408ceb
	uint32_t HasFlags(uint16_t theFlags);              // 0x408e41
	void FreePieces();                                 // 0x408ea1
	void ClearLockBit();                               // 0x408f49
	void SetLockBit();                                 // 0x408f70
	uint8_t IsIdle();                                  // 0x408f97
	uint32_t HasTapRows(int theKind, char theFaller);  // 0x40947b
	uint32_t ColumnClear(int theColumn);               // 0x40959b
	void ResetClocks();                                // 0x40962d
	void UpdateSpawnTimer();                           // 0x40965f
	uint32_t Save(FILE* theFile, uint32_t theNow);     // 0x4096ce
	uint32_t Load(FILE* theFile, uint32_t theNow);     // 0x409f18
	int32_t GetLowRow();                               // 0x40e742
	int32_t GetHighRow();                              // 0x40e754
	int32_t GetSize();                                 // 0x4103c7
	float GetMeter();                                  // 0x4103dd
};

// The board (0x1658 bytes; 0x40ad4f)
struct Board
{
	uint32_t mWidth = 0;                   // +0x004 pixels
	uint32_t mHeight = 0;                  // +0x008
	uint32_t mColumns = 0;                 // +0x00c
	uint32_t mRows = 0;                    // +0x010
	int32_t mX = 0;                        // +0x014 the field's place on the screen
	int32_t mY = 0;                        // +0x018
	int32_t mRight = 0;                    // +0x01c mX + mWidth; nothing reads it
	int32_t mBottom = 0;                   // +0x020 mY + mHeight; nothing reads it
	int32_t mBlockWidth = 0;               // +0x024
	int32_t mBlockHeight = 0;              // +0x028
	lowos::Surface* mBlocks = nullptr;     // +0x02c image 0x24: the blocks, one row of frames per kind
	lowos::Surface* mGlowTop = nullptr;    // +0x030 image 0x25: the top of the lit area (the glow's edge)
	lowos::Surface* mArrows = nullptr;     // +0x034 image 0x23: the arrow over the column to drop in (four frames)
	lowos::Surface* mTopCover = nullptr;   // +0x038 image 0x42
	lowos::Surface* mBottomCover = nullptr;  // +0x03c image 0x43
	Piece mNextBlocks[4];                  // +0x040 the blocks about to enter (the first is dropped by a tap)
	int32_t mLastColumn = -1;              // +0x1b0 the column of the arrow
	int32_t mArrowY[100] = {};             // +0x1b4 each column's arrow height
	int32_t mQueueShifting = 0;            // +0x344 the preview blocks are moving up
	int32_t mQueueIndex = 0;               // +0x348 the next of the bonus round's queue
	Field* mField = nullptr;               // +0x34c
	int32_t mGameOver = 0;                 // +0x350
	int32_t mLevelComplete = 0;            // +0x354
	int32_t mEnding = 0;                   // +0x358 the game is over: the field only clears
	BoardState mState;                     // +0x35c
	uint32_t mGlowPitch = 0;               // +0x1610 the pitch of the three glow buffers (the App makes them)
	uint8_t* mGlowBuffer = nullptr;        // +0x1614
	uint32_t mGlowBufferUnused = 0;              // +0x1618 cleared with mGlowBuffer; nothing reads it
	uint8_t* mGlowBufferA = nullptr;       // +0x161c the glow as drawn for the column mGlowColumn
	uint8_t mGlowValidA = 0;               // +0x1620
	uint8_t* mGlowBufferB = nullptr;       // +0x1624 and for the second sweep (the timed game)
	uint8_t mGlowValidB = 0;               // +0x1628
	int32_t mGlowColumn = 0;               // +0x162c
	int32_t mGlowLength = 0;               // +0x1630
	int32_t mGlowColumnB = 0;              // +0x1634
	int32_t mGlowLengthB = 0;              // +0x1638
	int32_t mGlowLowRow = 0;               // +0x163c
	int32_t mGlowHighRow = 0;              // +0x1640
	int32_t mGlowBrightness = 0;           // +0x1644
	uint32_t mGlowTime = 0;                // +0x1648
	uint8_t* mGlowTable = nullptr;         // +0x164c the glow's profile (0x40af30)
	uint8_t* mGlowAlpha = nullptr;         // +0x1650 the App makes it
	uint32_t mGlowAlphaPitch = 0;          // +0x1654

	Board(int theX, int theY, int theColumns, int theRows);  // 0x40ad4f
	virtual ~Board();                      // 0x40adba (the deleting destructor), body 0x40ade3

	int32_t Init(int theX, int theY, int theColumns, int theRows);  // 0x40af30
	int32_t SetConfig(const Config* theConfig);        // 0x40b296
	void Start();                                      // 0x40b2bc
	void Ramp(uint32_t* theValue, float theRamp);      // 0x40b3ad
	bool CheckBonusRound();                            // 0x40b41c
	void NextLevel();                                  // 0x40b82c
	void UpdateSpawnTimer();                           // 0x40c6ec
	void FillQueueBonus();                             // 0x40c703
	void FillQueue();                                  // 0x40ca07
	void GlowColumn(int theX, int theY, uint32_t theLength, char theBytesPerPixel, int theBrightness);  // 0x40cd63
	void DrawGlow(lowos::Surface* theScreen);          // 0x40cea5
	void Draw(lowos::Surface* theScreen);              // 0x40e766 (the frame: update, background, blocks)
	void StartLevel();                                 // 0x4103f2
	void Tap(uint32_t theX);                           // 0x410427
	uint32_t GetScore();                               // 0x410651
	uint32_t GetBlocksLeft();                          // 0x410668
	void EndGame();                                    // 0x41067f
	uint32_t IsEnding();                               // 0x410695
	uint32_t ClearStep();                              // 0x4106a7
	void ResetClocks();                                // 0x4106be
	void ResumeQueue();                                // 0x4106d5
	uint32_t Save(FILE* theFile, uint32_t theNow);     // 0x41070c
	uint32_t Load(FILE* theFile, uint32_t theNow);     // 0x410d35
};

#if UINTPTR_MAX == 0xffffffffu
static_assert(sizeof(Piece) == 0x5c, "Piece");
static_assert(sizeof(BoardState) == 0x12b4, "BoardState");
static_assert(sizeof(Field) == 0x96c, "Field");
static_assert(sizeof(Board) == 0x1658, "Board");
#endif

} // namespace tiptop

#endif
