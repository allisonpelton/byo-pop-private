#ifndef __BOOKWORM_LIVESPLITBLOCK_H__
#define __BOOKWORM_LIVESPLITBLOCK_H__

// port(mod): BookWorm's LiveSplit block, layout 1. A tagged, read-only block in the game's .data that the
// auto splitter (games/bookworm/tools/livesplit/BookWormDeluxe-byopop.asl) finds and reads from outside the
// process; LiveSplitObserver.cpp fills it. It follows Big Money's block (games/bigmoney/src/LiveSplitBlock.h)
// and Bejeweled's: the same magic and write protocol, and the same offsets for the fields the games share,
// with game id "BWDX". BookWorm's own fields are the three kinds of death (a fire tile burning through in
// Classic, the countdown running out in Action) and scoreMax, which is the game's score, not the HUD's.
//
// Little-endian, naturally aligned, 0xB0 bytes:
//   0x00 u8  magic[20]      "BYOPOP-LIVESPLIT" F0 F1 F2 F3
//   0x14 u32 gameId         0x58445742: the bytes "BWDX" in memory
//   0x18 u16 layoutVersion  1;   0x1A u16 size 0xB0
//   0x1C u32 self           the block's own address once active, else 0
//   0x20 u32 seq            odd while the game writes
//   0x24 u32 active         1 while features.livesplit is on;   0x28 u32 updates (heartbeat)
//   0x2C u32 flags          LIVESPLIT_FLAG_*
//   0x30 u32 boardSerial    the current board, 0 = none
//   0x34 u32 goCount, goSerial;  0x3C levelClears, levelClearSerial;  0x44 gameOvers, gameOverSerial
//   0x4C u32 deathsFire, deathsCountdown, deathsNewGame, deathsOther   (they add up to gameOvers)
//   0x5C i32 tick, state, level, displayLevel, displayScore, score, boardLevelClears, goTick (-1 before GO)
//   0x7C i32 scoreMax       the board's highest score up to its game over, then frozen
//   0x80 i32 mode (0 Classic, 1 Action; -1 no board);  0x84 i32 skill (always 0: BookWorm has none)
//   0x88 u8  scoreKey[8]    ModManager::GetScoreKey() as 8 ASCII hex digits, zeros without gameplay mods
//   0x90 u8  reserved[16]   zero
//   0xA0 u64 qpc (SDL_GetPerformanceCounter at this write);  0xA8 u64 qpcFrequency
//
// Events (LiveSplitObserver.cpp): GO is the first update of a fresh board (not a resumed save) that is playing,
// not paused, at level 1 with no word or scramble yet, once per board (with the help screen on, that is when
// the player closes it); a level clear is the board entering BOARD_LEVEL_COMPLETE; a game over is the board
// entering BOARD_GAMEOVER_PENDING (a fire tile burned through the bottom: deathsFire in Classic,
// deathsCountdown in Action), or BOARD_INACTIVE (LEAVE GAME: the board is saved and abandoned) before its
// game over: deathsNewGame, which also stands for a board replaced by another.
// state is Board::mState (BoardState in Board.h); level is Board::mLevel (0-based), displayLevel the level
// number the player sees; displayScore is the HUD's rolling score (Board::mShownScore), score the game's
// (mGameStats->mScore). scoreMax follows score, not displayScore: in Classic the game over comes with the
// word that causes it, while the HUD is still rolling up to that word's points.
//
// Not used by BookWorm: skill (0), flags CHOSEN_SEED and REPLAY (never set); the fields stay so the layout is
// the other games'.
//
// Append-only: fields are never moved, resized or given a new meaning; an append bumps layoutVersion and size.
// Writing (the game): seq++ (odd), release fence, the fields, release fence, seq++ (even). Reading (the
// script, tools/livesplit/lsprobe.py): seq -> s1, the 0xB0 bytes, seq -> s2; the copy counts when s1 == s2,
// s1 is even and the copy's own seq is s1.

#include <cstddef>
#include <cstdint>
#include <cstdio>

// The magic as a brace list of bytes: the block's definition is the only place that spells it, so it is in
// the exe once (tools/livesplit/check_asl_offsets.py checks it). Not a string literal, which would add a
// second copy in .rdata.
#define LIVESPLIT_MAGIC_BYTES { 0x42, 0x59, 0x4F, 0x50, 0x4F, 0x50, 0x2D, 0x4C, 0x49, 0x56, 0x45, 0x53, 0x50, 0x4C, 0x49, 0x54, \
	0xF0, 0xF1, 0xF2, 0xF3 }

enum : uint32_t
{
	LIVESPLIT_MAGIC_SIZE = 20,
	LIVESPLIT_GAME_ID = 0x58445742u,    // "BWDX" in memory
	LIVESPLIT_LAYOUT_VERSION = 1,
	LIVESPLIT_SIZE_V1 = 0xB0,

	LIVESPLIT_FLAG_RESUMED = 0x02,      // the board resumed a saved game (its GO is not counted)
	LIVESPLIT_FLAG_VANILLA = 0x04,      // no score key (no gameplay mod)
	LIVESPLIT_FLAG_CHOSEN_SEED = 0x08,  // (the other games' chosen seed: never set here)
	LIVESPLIT_FLAG_REPLAY = 0x20,       // a replay is watched (mod "replays", Replay.h; its GO is not counted)
	LIVESPLIT_FLAG_GAME_OVER = 0x40,    // the board had its game over (latched per board)
	LIVESPLIT_FLAG_PAUSED = 0x80        // the board is paused (the help screen, a dialog, the window out of focus)
};

struct LiveSplitBlockV1
{
	uint8_t		magic[LIVESPLIT_MAGIC_SIZE];
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
	uint32_t	deathsFire;
	uint32_t	deathsCountdown;
	uint32_t	deathsNewGame;
	uint32_t	deathsOther;
	int32_t		tick;
	int32_t		state;
	int32_t		level;
	int32_t		displayLevel;
	int32_t		displayScore;
	int32_t		score;
	int32_t		boardLevelClears;
	int32_t		goTick;
	int32_t		scoreMax;
	int32_t		mode;
	int32_t		skill;
	uint8_t		scoreKey[8];
	uint8_t		reserved[16];
	uint64_t	qpc;
	uint64_t	qpcFrequency;
};

static_assert(offsetof(LiveSplitBlockV1, gameId) == 0x14, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, self) == 0x1C, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, seq) == 0x20, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, flags) == 0x2C, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, boardSerial) == 0x30, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, goCount) == 0x34, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, levelClears) == 0x3C, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, gameOvers) == 0x44, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, deathsFire) == 0x4C, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, deathsOther) == 0x58, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, tick) == 0x5C, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, goTick) == 0x78, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, scoreMax) == 0x7C, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, mode) == 0x80, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, skill) == 0x84, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, scoreKey) == 0x88, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, qpc) == 0xA0, "LiveSplit layout 1");
static_assert(offsetof(LiveSplitBlockV1, qpcFrequency) == 0xA8, "LiveSplit layout 1");
static_assert(sizeof(LiveSplitBlockV1) == LIVESPLIT_SIZE_V1, "LiveSplit layout 1");

// One block as "key=value" fields in block order, for -livesplitlog and lsprobe.py's log
// (tools/livesplit/lsprobe.py --check compares them). maskVolatile prints "*" for self and the clock, so the
// game's log is deterministic.
inline int FormatLiveSplitBlock(const LiveSplitBlockV1& theBlock, bool maskVolatile, char* theBuffer, size_t theSize)
{
	char aKey[9];
	for (int i = 0; i < 8; i++)
		aKey[i] = (theBlock.scoreKey[i] >= 0x20 && theBlock.scoreKey[i] < 0x7F) ? (char) theBlock.scoreKey[i] : '-';
	aKey[8] = 0;
	char aSelf[16];
	char aQpc[24];
	char aQpcFrequency[24];
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
		"tick=%d state=%d level=%d dlevel=%d dscore=%d score=%d blc=%d gotick=%d smax=%d mode=%d skill=%d key=%s "
		"self=%s qpc=%s qpcf=%s",
		(unsigned) theBlock.seq, (unsigned) theBlock.active, (unsigned) theBlock.updates, (unsigned) theBlock.flags, (unsigned) theBlock.boardSerial,
		(unsigned) theBlock.goCount, (unsigned) theBlock.goSerial, (unsigned) theBlock.levelClears, (unsigned) theBlock.levelClearSerial,
		(unsigned) theBlock.gameOvers, (unsigned) theBlock.gameOverSerial, (unsigned) theBlock.deathsFire, (unsigned) theBlock.deathsCountdown,
		(unsigned) theBlock.deathsNewGame, (unsigned) theBlock.deathsOther, (int) theBlock.tick, (int) theBlock.state, (int) theBlock.level, (int) theBlock.displayLevel,
		(int) theBlock.displayScore, (int) theBlock.score, (int) theBlock.boardLevelClears, (int) theBlock.goTick, (int) theBlock.scoreMax, (int) theBlock.mode,
		(int) theBlock.skill, aKey, aSelf, aQpc, aQpcFrequency);
}

#endif
