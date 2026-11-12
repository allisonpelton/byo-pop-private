#ifndef __PLATFORM_AUTOSPLITBLOCK_H__
#define __PLATFORM_AUTOSPLITBLOCK_H__

// The autosplit block every port publishes for speedrun timers (docs/WEB_PLAYER.md, "The splits timer", "One design for
// every port"): a tagged, read-only block in the game's .data that a LiveSplit script finds and reads from outside the
// process, and that the site's Splits window reads from the game's memory (web/autosplit.js). Bejeweled, Big Money and
// BookWorm wrote layout 1 of it before this header; layout 2 keeps every byte of layout 1 and appends 0x20.
//
// Little-endian, naturally aligned, 0xD0 bytes:
//   0x00 u8  magic[20]      "BYOPOP-LIVESPLIT" F0 F1 F2 F3
//   0x14 u32 gameId         four ASCII letters as they lie in memory ("BMDX" is 0x58444D42)
//   0x18 u16 layoutVersion  2;   0x1A u16 size 0xD0
//   0x1C u32 self           the block's own address once active, else 0
//   0x20 u32 seq            odd while the game writes
//   0x24 u32 active         1 while it is written (asked for);   0x28 u32 updates (heartbeat)
//   0x2C u32 flags          AUTOSPLIT_FLAG_*
//   0x30 u32 boardSerial    the current board, 0 = none
//   0x34 u32 goCount, goSerial;  0x3C levelClears, levelClearSerial;  0x44 gameOvers, gameOverSerial
//   0x4C u32 deaths[4]      the game's four kinds of game over, which add up to gameOvers (the game's table names them)
//   0x5C i32 tick, state, level, displayLevel, displayScore, score, boardLevelClears, goTick (-1 before GO)
//   0x7C i32 displayScoreMax  the board's highest displayScore up to its game over, then frozen
//   0x80 u8  gameOwn[32]    the game's own (layout 1's: Bejeweled's rulesId; Big Money's and BookWorm's mode, skill,
//                           score key and 16 zero bytes)
//   0xA0 u64 qpc (the clock at this write);  0xA8 u64 qpcFrequency
//   layout 2:
//   0xB0 u32 wins, winSerial  the game was won (its last level, an escape, the final chapter), and on which board
//   0xB8 i32 mode           the mode played, numbered as the game's rules table names it; -1 off a board
//   0xBC i32 skill          the difficulty, numbered as the table names it; 0 for a game without one
//   0xC0 u32 inGame         1 from the board's GO until its game over or win (a pause keeps it 1), else 0
//   0xC4 i32 progress       a figure of the game's own that a category ends on; -1 when unused
//   0xC8 u8  reserved[8]    zero
//
// Append-only: fields are never moved, resized or given a new meaning. An append bumps layoutVersion and size; a reader
// for layout N accepts layoutVersion >= N and size >= its size and reads only its bytes. An incompatible change gets a
// new magic.
//
// Writing (the game, AutosplitWriter::Publish): seq++ (odd), release fence, the fields, release fence, seq++ (even).
// Reading: seq -> s1, the bytes, seq -> s2; the copy counts when s1 == s2 and s1 is even.
//
// Inert: nothing is written until something asks for the block (the web page through the game's byopop_livesplit_block
// export, natively the livesplit feature), and the writer only reads what the game hands it.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

// The magic as a brace list of bytes: the block's definition is the only place in a game that spells it, so it occurs
// once in the exe. Not a string literal: that would put a second copy in .rdata.
#define AUTOSPLIT_MAGIC_BYTES { 0x42, 0x59, 0x4F, 0x50, 0x4F, 0x50, 0x2D, 0x4C, 0x49, 0x56, 0x45, 0x53, 0x50, 0x4C, 0x49, 0x54, \
	0xF0, 0xF1, 0xF2, 0xF3 }
// A game id from its four letters, as they lie in memory
#define AUTOSPLIT_GAME_ID(a, b, c, d) ((uint32_t) (a) | ((uint32_t) (b) << 8) | ((uint32_t) (c) << 16) | ((uint32_t) (d) << 24))

enum : uint32_t
{
	AUTOSPLIT_MAGIC_SIZE = 20,
	AUTOSPLIT_LAYOUT_VERSION = 2,
	AUTOSPLIT_SIZE_V1 = 0xB0,
	AUTOSPLIT_SIZE_V2 = 0xD0,

	AUTOSPLIT_FLAG_TIMED = 0x01,        // Bejeweled only: a Timetrial board
	AUTOSPLIT_FLAG_RESUMED = 0x02,      // the board resumed a saved game (its GO is not counted)
	AUTOSPLIT_FLAG_VANILLA = 0x04,      // no gameplay mod (no score key) and no seed the player chose
	AUTOSPLIT_FLAG_CHOSEN_SEED = 0x08,  // a seed the player chose
	AUTOSPLIT_FLAG_OWN_RNG = 0x10,      // Bejeweled only: the board owns its generator
	AUTOSPLIT_FLAG_REPLAY = 0x20,       // a replay is playing (its GO is not counted)
	AUTOSPLIT_FLAG_GAME_OVER = 0x40,    // the board had its game over (latched per board)
	AUTOSPLIT_FLAG_PAUSED = 0x80,       // the board is paused (a dialog, the focus, the player)
	AUTOSPLIT_FLAG_WON = 0x100          // layout 2: the board was won (latched per board)
};

struct AutosplitBlock
{
	uint8_t		magic[AUTOSPLIT_MAGIC_SIZE];
	uint32_t	gameId;
	uint16_t	layoutVersion;
	uint16_t	size;
	uint32_t	self;
	uint32_t	seq;
	uint32_t	active;
	uint32_t	updates;
	uint32_t	flags;
	uint32_t	boardSerial;
	uint32_t	goCount;
	uint32_t	goSerial;
	uint32_t	levelClears;
	uint32_t	levelClearSerial;
	uint32_t	gameOvers;
	uint32_t	gameOverSerial;
	uint32_t	deaths[4];
	int32_t		tick;
	int32_t		state;
	int32_t		level;
	int32_t		displayLevel;
	int32_t		displayScore;
	int32_t		score;
	int32_t		boardLevelClears;
	int32_t		goTick;
	int32_t		displayScoreMax;
	uint8_t		gameOwn[32];
	uint64_t	qpc;
	uint64_t	qpcFrequency;
	// layout 2
	uint32_t	wins;
	uint32_t	winSerial;
	int32_t		mode;
	int32_t		skill;
	uint32_t	inGame;
	int32_t		progress;
	uint8_t		reserved[8];
};

static_assert(offsetof(AutosplitBlock, magic) == 0x00, "autosplit layout");
static_assert(offsetof(AutosplitBlock, gameId) == 0x14, "autosplit layout");
static_assert(offsetof(AutosplitBlock, layoutVersion) == 0x18, "autosplit layout");
static_assert(offsetof(AutosplitBlock, size) == 0x1A, "autosplit layout");
static_assert(offsetof(AutosplitBlock, self) == 0x1C, "autosplit layout");
static_assert(offsetof(AutosplitBlock, seq) == 0x20, "autosplit layout");
static_assert(offsetof(AutosplitBlock, active) == 0x24, "autosplit layout");
static_assert(offsetof(AutosplitBlock, updates) == 0x28, "autosplit layout");
static_assert(offsetof(AutosplitBlock, flags) == 0x2C, "autosplit layout");
static_assert(offsetof(AutosplitBlock, boardSerial) == 0x30, "autosplit layout");
static_assert(offsetof(AutosplitBlock, goCount) == 0x34, "autosplit layout");
static_assert(offsetof(AutosplitBlock, goSerial) == 0x38, "autosplit layout");
static_assert(offsetof(AutosplitBlock, levelClears) == 0x3C, "autosplit layout");
static_assert(offsetof(AutosplitBlock, levelClearSerial) == 0x40, "autosplit layout");
static_assert(offsetof(AutosplitBlock, gameOvers) == 0x44, "autosplit layout");
static_assert(offsetof(AutosplitBlock, gameOverSerial) == 0x48, "autosplit layout");
static_assert(offsetof(AutosplitBlock, deaths) == 0x4C, "autosplit layout");
static_assert(offsetof(AutosplitBlock, tick) == 0x5C, "autosplit layout");
static_assert(offsetof(AutosplitBlock, state) == 0x60, "autosplit layout");
static_assert(offsetof(AutosplitBlock, level) == 0x64, "autosplit layout");
static_assert(offsetof(AutosplitBlock, displayLevel) == 0x68, "autosplit layout");
static_assert(offsetof(AutosplitBlock, displayScore) == 0x6C, "autosplit layout");
static_assert(offsetof(AutosplitBlock, score) == 0x70, "autosplit layout");
static_assert(offsetof(AutosplitBlock, boardLevelClears) == 0x74, "autosplit layout");
static_assert(offsetof(AutosplitBlock, goTick) == 0x78, "autosplit layout");
static_assert(offsetof(AutosplitBlock, displayScoreMax) == 0x7C, "autosplit layout");
static_assert(offsetof(AutosplitBlock, gameOwn) == 0x80, "autosplit layout");
static_assert(offsetof(AutosplitBlock, qpc) == 0xA0, "autosplit layout");
static_assert(offsetof(AutosplitBlock, qpcFrequency) == 0xA8, "autosplit layout");
static_assert(offsetof(AutosplitBlock, wins) == 0xB0, "autosplit layout 2");
static_assert(offsetof(AutosplitBlock, winSerial) == 0xB4, "autosplit layout 2");
static_assert(offsetof(AutosplitBlock, mode) == 0xB8, "autosplit layout 2");
static_assert(offsetof(AutosplitBlock, skill) == 0xBC, "autosplit layout 2");
static_assert(offsetof(AutosplitBlock, inGame) == 0xC0, "autosplit layout 2");
static_assert(offsetof(AutosplitBlock, progress) == 0xC4, "autosplit layout 2");
static_assert(offsetof(AutosplitBlock, reserved) == 0xC8, "autosplit layout 2");
static_assert(sizeof(AutosplitBlock) == AUTOSPLIT_SIZE_V2, "autosplit layout 2");

// A game's block, as it defines it once (constant-initialised, so the magic is only in .data):
//   extern "C" __attribute__((used, aligned(64))) volatile AutosplitBlock gLiveSplitBlock = AUTOSPLIT_BLOCK_INIT(id);
#define AUTOSPLIT_BLOCK_INIT(theGameId) { AUTOSPLIT_MAGIC_BYTES, (theGameId), AUTOSPLIT_LAYOUT_VERSION, AUTOSPLIT_SIZE_V2 }

// The writer: the game hands it the events and its fields after each update, and it keeps the counters and serials and
// does the seq writes. It holds no game state and reads nothing of the game's by itself.
class AutosplitWriter
{
public:
	explicit AutosplitWriter(volatile AutosplitBlock* theBlock) : mBlock(theBlock), mActive(false), mWon(false), mOver(false)
	{
		memset(&mNext, 0, sizeof(mNext));
		ClearBoard();
	}

	bool			IsActive() const { return mActive; }
	// What the next Publish writes: the game sets the per-update fields here (tick, state, level, displayLevel,
	// displayScore, score, mode, skill, progress, gameOwn, the flags it owns)
	AutosplitBlock&	Next() { return mNext; }

	// The first write: the block's address and the clock's frequency
	void Activate(uint64_t theQpcFrequency)
	{
		mActive = true;
		mNext.self = (uint32_t) (uintptr_t) mBlock;
		mNext.active = 1;
		mNext.qpcFrequency = theQpcFrequency;
	}
	// The last write (the app going away): inactive, no board
	void Deactivate()
	{
		mActive = false;
		mNext.active = 0;
		ClearBoard();
	}

	// The board changed (0: none). A board's first update is a baseline: it raises no event.
	void NewBoard(uint32_t theSerial)
	{
		ClearBoard();
		mNext.boardSerial = theSerial;
	}
	void ClearBoard()
	{
		mNext.boardSerial = 0;
		mNext.boardLevelClears = 0;
		mNext.goTick = -1;
		mNext.displayScoreMax = 0;
		mNext.flags = 0;
		mNext.tick = 0;
		mNext.state = -1;
		mNext.level = 0;
		mNext.displayLevel = 0;
		mNext.displayScore = 0;
		mNext.score = 0;
		mNext.mode = -1;
		mNext.skill = 0;
		mNext.inGame = 0;
		mNext.progress = -1;
		mWon = false;
		mOver = false;
	}

	// The events, on the current board
	void Go(int32_t theTick)
	{
		mNext.goCount++;
		mNext.goSerial = mNext.boardSerial;
		mNext.goTick = theTick;
		mNext.inGame = 1;
	}
	void LevelClear()
	{
		mNext.levelClears++;
		mNext.levelClearSerial = mNext.boardSerial;
		mNext.boardLevelClears++;
	}
	// theKind 0 to 3: which of the game's four kinds of game over
	void GameOver(int theKind)
	{
		mNext.deaths[(theKind >= 0 && theKind < 4) ? theKind : 3]++;
		mNext.gameOvers++;
		mNext.gameOverSerial = mNext.boardSerial;
		mNext.inGame = 0;
		mOver = true;
	}
	void Win()
	{
		mNext.wins++;
		mNext.winSerial = mNext.boardSerial;
		mNext.inGame = 0;
		mWon = true;
	}
	// A board first seen already over (its game over came before the block was asked for): no event, but the
	// GAME_OVER flag and a frozen high-water mark, as after GameOver
	void SeenOver() { mOver = true; }
	bool HadGameOver() const { return mOver; }
	bool HadWin() const { return mWon; }

	// The score on screen, which is also the one its high-water mark follows
	void SetDisplayScore(int32_t theScore)
	{
		mNext.displayScore = theScore;
		RaiseScoreMax(theScore);
	}
	// The board's high-water mark (displayScoreMax), frozen at its game over or win. A game whose table follows
	// another score than the one on screen (BookWorm: its stats' score) sets displayScore itself and calls this.
	void RaiseScoreMax(int32_t theScore)
	{
		if (!mOver && !mWon && theScore > mNext.displayScoreMax)
			mNext.displayScoreMax = theScore;
	}

	// One write: seq odd, everything after the header, seq even. theQpc: the clock now
	void Publish(uint64_t theQpc)
	{
		mNext.updates++;
		mNext.qpc = theQpc;
		if (mWon)
			mNext.flags |= AUTOSPLIT_FLAG_WON;
		if (mOver)
			mNext.flags |= AUTOSPLIT_FLAG_GAME_OVER;
		volatile AutosplitBlock& b = *mBlock;
		uint32_t aSeq = b.seq + 1;
		b.seq = aSeq;
		std::atomic_thread_fence(std::memory_order_release);
		// every byte from self on but seq, as 32-bit words (the block is 4-byte aligned throughout)
		const uint32_t* aFrom = (const uint32_t*) &mNext;
		volatile uint32_t* aTo = (volatile uint32_t*) mBlock;
		for (unsigned i = offsetof(AutosplitBlock, self) / 4; i < AUTOSPLIT_SIZE_V2 / 4; i++)
			if (i != offsetof(AutosplitBlock, seq) / 4)
				aTo[i] = aFrom[i];
		std::atomic_thread_fence(std::memory_order_release);
		b.seq = aSeq + 1;
		mNext.seq = aSeq + 1;
	}

private:
	volatile AutosplitBlock*	mBlock;
	AutosplitBlock				mNext;
	bool						mActive;
	bool						mWon;
	bool						mOver;
};

// One block as a line of "key=value" fields in block order, for a game's -livesplitlog (the header is checked by
// whoever reads the block and not repeated; gameOwn is printed as hex). maskVolatile prints "*" for self, qpc and
// qpcFrequency, so a game's log is deterministic.
inline int FormatAutosplitBlock(const AutosplitBlock& theBlock, bool maskVolatile, char* theBuffer, size_t theSize)
{
	char anOwn[65];
	for (int i = 0; i < 32; i++)
		snprintf(anOwn + 2 * i, 3, "%02x", (unsigned) theBlock.gameOwn[i]);
	char aSelf[16], aQpc[24], aQpcFrequency[24];
	if (maskVolatile)
	{
		snprintf(aSelf, sizeof(aSelf), "*");
		snprintf(aQpc, sizeof(aQpc), "*");
		snprintf(aQpcFrequency, sizeof(aQpcFrequency), "*");
	}
	else
	{
		snprintf(aSelf, sizeof(aSelf), "%08x", (unsigned) theBlock.self);
		snprintf(aQpc, sizeof(aQpc), "%llu", (unsigned long long) theBlock.qpc);
		snprintf(aQpcFrequency, sizeof(aQpcFrequency), "%llu", (unsigned long long) theBlock.qpcFrequency);
	}
	return snprintf(theBuffer, theSize,
		"seq=%u active=%u updates=%u flags=%02x board=%u go=%u/%u lc=%u/%u over=%u/%u deaths=%u,%u,%u,%u "
		"tick=%d state=%d level=%d dlevel=%d dscore=%d score=%d blc=%d gotick=%d dsmax=%d own=%s "
		"wins=%u/%u mode=%d skill=%d ingame=%u progress=%d self=%s qpc=%s qpcf=%s",
		(unsigned) theBlock.seq, (unsigned) theBlock.active, (unsigned) theBlock.updates, (unsigned) theBlock.flags,
		(unsigned) theBlock.boardSerial, (unsigned) theBlock.goCount, (unsigned) theBlock.goSerial, (unsigned) theBlock.levelClears,
		(unsigned) theBlock.levelClearSerial, (unsigned) theBlock.gameOvers, (unsigned) theBlock.gameOverSerial,
		(unsigned) theBlock.deaths[0], (unsigned) theBlock.deaths[1], (unsigned) theBlock.deaths[2], (unsigned) theBlock.deaths[3],
		(int) theBlock.tick, (int) theBlock.state, (int) theBlock.level, (int) theBlock.displayLevel, (int) theBlock.displayScore,
		(int) theBlock.score, (int) theBlock.boardLevelClears, (int) theBlock.goTick, (int) theBlock.displayScoreMax, anOwn,
		(unsigned) theBlock.wins, (unsigned) theBlock.winSerial, (int) theBlock.mode, (int) theBlock.skill, (unsigned) theBlock.inGame,
		(int) theBlock.progress, aSelf, aQpc, aQpcFrequency);
}

#endif
